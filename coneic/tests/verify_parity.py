#!/usr/bin/env python3
"""Read-only VLC fork source parity audit. No compilation or runtime claims."""
import argparse
import collections
import datetime
import hashlib
import io
import json
import pathlib
import re
import subprocess
import tarfile

BASELINE = '6de05adcbaf2e8b85fe86aad4169393098628119'
DEFAULT_ALLOW = {
    'modules/gui/qt/qt.cpp', 'modules/gui/qt/main_interface.cpp',
    'modules/gui/qt/Makefile.am', 'modules/gui/qt/vlc.qrc',
    'modules/gui/qt/coneic_design.hpp', 'modules/gui/qt/pixmaps/coneic.svg',
    'share/icons/vlc.ico', 'src/win32/dirs.c', 'bin/winvlc.c',
    'bin/vlc_win32_rc.rc.in', 'share/Makefile.am',
}
# One bounded engine correction, reviewed with a real block-buffer regression.
# Any other change to this file still raises a protected-scope review flag.
REVIEWED_ENGINE_FIXES = {
    'modules/packetizer/hxxx_nal.c': {
        'baseline_sha256': '227871d260f0bca99fcdfdf0f2e3b284dce5b621b6d0af6d0d7a3e5febee696f',
        'fixed_sha256': '3b857fad7d8499c526d6d0bab1a4bcf5fdf9b09be6f8604793b7db1c93b471fa',
        'reason': 'Copy block timing/flags when Annex-B conversion allocates a replacement; docs/HXXX-METADATA-FIX.md',
    },
}

SOURCE_SUFFIXES = {'.c', '.cpp', '.h', '.hpp', '.m', '.mm', '.S', '.s', '.ui'}

def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])

def digest(b):
    return hashlib.sha256(b).hexdigest()

def protected(p):
    return (p.startswith(('src/', 'lib/', 'bin/', 'include/', 'modules/', 'share/lua/'))
            or p in {'configure.ac', 'Makefile.am'}
            or p.startswith(('extras/package/', 'contrib/')))

def tokens(b):
    s = b.decode('utf-8', errors='replace')
    # Strip comments before matching, preserving quoted strings and literal slashes.
    s = re.sub(r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')|/\*.*?\*/|//[^\n]*',
               lambda m: m.group(1) or '', s, flags=re.S)
    return {
        'options': sorted(set(':'.join(x) for x in re.findall(
            r'\b(add_[A-Za-z0-9_]+)\s*\(\s*"([^"\n]+)"', s))),
        'capabilities': sorted(set(re.findall(r'\bset_capability\s*\(\s*"([^"\n]+)"', s))),
        'module_registrations': len(re.findall(r'\bvlc_module_begin\s*\(', s)),
    }

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo', required=True, type=pathlib.Path)
    ap.add_argument('--baseline', default=BASELINE)
    ap.add_argument('--allow', action='append', default=[], help='Exact additional reviewed presentation path')
    ap.add_argument('--json', required=True, type=pathlib.Path)
    ap.add_argument('--markdown', required=True, type=pathlib.Path)
    args = ap.parse_args()
    repo = args.repo.resolve()
    expected = git(repo, 'rev-parse', args.baseline + '^{commit}').decode().strip()
    tag_commit = git(repo, 'rev-parse', '3.0.24^{commit}').decode().strip()
    if expected != BASELINE or tag_commit != BASELINE:
        raise SystemExit('Refusing unrecognized baseline; exact upstream 3.0.24 commit required')
    allow = DEFAULT_ALLOW | set(args.allow)
    reviewed_manifest = repo / 'coneic/presentation-paths.json'
    if reviewed_manifest.is_file():
        reviewed = json.loads(reviewed_manifest.read_text(encoding='utf-8'))['paths']
        for path in reviewed:
            if not isinstance(path, str) or not path.startswith('modules/gui/qt/') or '..' in pathlib.PurePosixPath(path).parts or '\\' in path:
                raise SystemExit('Invalid presentation allowance: exact Qt-relative paths only')
        allow |= set(reviewed)
    # A streamed git archive reads the exact baseline blobs in one process.
    proc = subprocess.Popen(['git', '-C', str(repo), 'archive', '--format=tar', expected], stdout=subprocess.PIPE)
    baseline = {}
    with tarfile.open(fileobj=proc.stdout, mode='r|') as tf:
        for member in tf:
            if member.isfile():
                f = tf.extractfile(member)
                baseline[member.name] = f.read()
    # tarfile stops at the first end marker; drain git's padding before waiting
    # so a small Windows pipe cannot block the archive producer at EOF.
    proc.stdout.read()
    if proc.wait() != 0:
        raise SystemExit('Baseline archive failed')
    current_paths = set(git(repo, 'ls-files', '-z', '--cached', '--others', '--exclude-standard').decode().split('\0')) - {''}
    changed, removed, added, violations = [], [], [], []
    semantic_losses, counts = [], collections.Counter()
    workflow_sources = []
    for p, old in baseline.items():
        target = repo / pathlib.PurePosixPath(p)
        if not target.is_file():
            removed.append(p)
            if protected(p): violations.append({'path': p, 'reason': 'Protected upstream file removed'})
            continue
        new = target.read_bytes()
        fix = REVIEWED_ENGINE_FIXES.get(p, {})
        engine_fix = bool(fix and digest(old) == fix['baseline_sha256'] and digest(new) == fix['fixed_sha256'])
        if old != new:
            entry = {'path': p, 'baseline_sha256': digest(old), 'current_sha256': digest(new),
                     'allowed_presentation_review': p in allow,
                     'reviewed_engine_fix': fix['reason'] if engine_fix else None}
            changed.append(entry)
            if protected(p) and p not in allow and not engine_fix:
                violations.append({'path': p, 'reason': 'Protected upstream file changed outside explicit presentation allowlist'})
        if p == 'modules/gui/qt/Makefile.am':
            refs = lambda value: set(re.findall(rb'[A-Za-z0-9_./-]+\.(?:cpp|hpp|c|h|ui)\b', value))
            missing_refs = sorted(x.decode() for x in refs(old) - refs(new))
            if missing_refs:
                semantic_losses.append({'path': p, 'kind': 'qt_build_source_references', 'removed': missing_refs})
        if p in {'modules/gui/qt/qt.cpp', 'modules/gui/qt/main_interface.cpp'}:
            signals = lambda value: set(re.findall(rb'\b(?:SIGNAL|SLOT)\s*\([^\n;]*?\)\s*\)', value))
            missing_signals = sorted(x.decode(errors='replace') for x in signals(old) - signals(new))
            if missing_signals:
                semantic_losses.append({'path': p, 'kind': 'qt_signal_slot_expressions', 'removed': missing_signals})
        if pathlib.PurePosixPath(p).suffix in SOURCE_SUFFIXES:
            a, b = tokens(old), tokens(new)
            for key in ('options', 'capabilities'):
                missing = sorted(set(a[key]) - set(b[key]))
                if missing: semantic_losses.append({'path': p, 'kind': key, 'removed': missing})
            if b['module_registrations'] < a['module_registrations']:
                semantic_losses.append({'path': p, 'kind': 'module_registrations', 'before': a['module_registrations'], 'after': b['module_registrations']})
            if p.startswith('modules/'):
                counts['baseline_module_registrations'] += a['module_registrations']
                counts['current_module_registrations'] += b['module_registrations']
                counts['baseline_module_source_files'] += 1
                counts['current_module_source_files'] += 1
            if p == 'src/libvlc-module.c':
                counts['baseline_core_option_declarations'] = len(a['options'])
                counts['current_core_option_declarations'] = len(b['options'])
        if p.startswith(('modules/gui/qt/dialogs/', 'modules/gui/qt/components/', 'modules/gui/qt/ui/')) and pathlib.PurePosixPath(p).suffix in SOURCE_SUFFIXES:
            workflow_sources.append({'path': p, 'unchanged': old == new})
    for p in sorted(current_paths - baseline.keys()):
        if not (repo / pathlib.PurePosixPath(p)).is_file(): continue
        added.append(p)
        if p.startswith('modules/') and pathlib.PurePosixPath(p).suffix in SOURCE_SUFFIXES:
            counts['current_module_source_files'] += 1
            counts['current_module_registrations'] += tokens((repo / pathlib.PurePosixPath(p)).read_bytes())['module_registrations']
        if protected(p) and p not in allow:
            violations.append({'path': p, 'reason': 'New protected-scope source/resource outside explicit presentation allowlist; review required'})
    critical = ['modules/gui/qt/menus.cpp', 'modules/gui/qt/menus.hpp',
                'modules/gui/qt/dialogs_provider.cpp', 'modules/gui/qt/dialogs_provider.hpp',
                'modules/gui/qt/actions_manager.cpp', 'modules/gui/qt/actions_manager.hpp',
                'lib/libvlc.sym', 'include/vlc/libvlc.h', 'src/libvlc-module.c',
                'modules/control/hotkeys.c']
    critical_result = []
    for p in critical:
        if p not in baseline: continue
        target = repo / pathlib.PurePosixPath(p)
        identical = target.is_file() and target.read_bytes() == baseline[p]
        critical_result.append({'path': p, 'identical': identical, 'baseline_sha256': digest(baseline[p])})
    # Every baseline .sym API declaration must remain available verbatim.
    api_losses = []
    for p, old in baseline.items():
        if p.endswith('.sym'):
            target = repo / pathlib.PurePosixPath(p)
            new = target.read_bytes() if target.is_file() else b''
            old_api = set(re.findall(rb'^([A-Za-z_][A-Za-z0-9_]*)\s*$', old, re.M))
            new_api = set(re.findall(rb'^([A-Za-z_][A-Za-z0-9_]*)\s*$', new, re.M))
            missing = sorted(x.decode() for x in old_api - new_api)
            if missing: api_losses.append({'path': p, 'removed_symbols': missing})
            counts['baseline_export_symbol_declarations'] += len(old_api)
            counts['current_export_symbol_declarations'] += len(new_api)
    counts['baseline_tracked_files'] = len(baseline)
    counts['current_tracked_and_unignored_files'] = len(current_paths)
    counts['qt_workflow_source_files'] = len(workflow_sources)
    counts['unchanged_qt_workflow_source_files'] = sum(x['unchanged'] for x in workflow_sources)
    counts['reviewed_engine_fix_files'] = sum(bool(x.get('reviewed_engine_fix')) for x in changed)
    status = 'SOURCE CHECKS PASS; PRESENTATION CHANGES REQUIRE REVIEW' if not (violations or semantic_losses or api_losses or removed) else 'REVIEW REQUIRED'
    result = {'schema': 'coneic-vlc-source-parity-v1', 'created_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'repo': str(repo), 'baseline_commit': expected, 'baseline_tag': '3.0.24',
              'status': status, 'counts': dict(counts), 'presentation_allowlist': sorted(allow),
              'changed': changed, 'removed': removed, 'added': added,
              'protected_scope_review_flags': violations, 'semantic_losses': semantic_losses,
              'api_losses': api_losses, 'critical_files': critical_result, 'qt_workflow_sources': workflow_sources,
              'limits': ['Static source comparison only; no build or runtime verification.',
                         'Presentation allowlist permits edits but does not prove their behavioral safety.',
                         'Token extraction is a bounded syntax heuristic, not a C/C++ parser or complete option inventory.',
                         'Source preservation does not establish configure options, linked dependencies or packaged modules.',
                         'Untracked ignored files are excluded; no unrelated filesystem locations are read.']}
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(result, indent=2), encoding='utf-8')
    lines = ['# Coneic VLC source parity', '', f'Generated: {result["created_utc"]}', '',
             f'Baseline: VLC **3.0.24**, commit `{expected}`.', '', f'**{status}**', '',
             'This report compares source files. Build configuration, packaged plugins and runtime behavior remain unverified.', '',
             '| Measure | Count |', '| --- | ---: |']
    lines += [f'| {k.replace("_", " ")} | {v} |' for k,v in sorted(counts.items())]
    lines += ['', '## Critical entry points', '', '| File | Byte-identical to upstream |', '| --- | --- |']
    lines += [f'| `{x["path"]}` | {"yes" if x["identical"] else "NO"} |' for x in critical_result]
    lines += ['', '## Changes and review flags', '', f'{len(changed)} changed baseline files; {len(added)} added files; {len(removed)} removed files.', '']
    lines += [f'- `{x["path"]}`: {"explicit presentation allowance; review required" if x["allowed_presentation_review"] else ("bounded engine correction: " + x["reviewed_engine_fix"] if x.get("reviewed_engine_fix") else "outside presentation allowance")}' for x in changed]
    lines += [f'- Review flag: `{x["path"]}` — {x["reason"]}' for x in violations]
    if not changed and not added and not removed: lines += ['The initial checkout is byte-identical to the upstream baseline.']
    lines += ['', f'Semantic loss candidates: **{len(semantic_losses)}**. API loss candidates: **{len(api_losses)}**.', '',
              'Full per-file hashes, workflow inventory, explicit allowances and potential losses are in the companion JSON.', '', '## Limits', '']
    lines += ['- ' + x for x in result['limits']]
    lines += ['', '## Reproduce', '',
              'Run the workspace script using Python 3; it reads the repository and writes only the two selected reports:', '',
              '```text',
              'python work/vlc-fork/verify_parity.py --repo "' + str(repo) + '" --json outputs/Coneic-VLC-parity.json --markdown outputs/Coneic-VLC-parity.md',
              '```', '',
              'Exit status 0 means the bounded static checks found no protected-source changes or loss candidates; allowed presentation changes still require review. Exit status 1 means review flags, removals or loss candidates were found.']
    args.markdown.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(json.dumps({'status': status, 'counts': dict(counts), 'changed': len(changed), 'added': len(added),
                      'removed': len(removed), 'review_flags': len(violations), 'semantic_losses': len(semantic_losses),
                      'api_losses': len(api_losses), 'json': str(args.json), 'markdown': str(args.markdown)}, indent=2))
    return 1 if (violations or semantic_losses or api_losses or removed) else 0

if __name__ == '__main__':
    raise SystemExit(main())
