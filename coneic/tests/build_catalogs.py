#!/usr/bin/env python3
"""Compile supplied VLC-baseline locale paths from exact fork .po sources."""
import argparse, datetime, hashlib, json, pathlib, subprocess

def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source',required=True,type=pathlib.Path)
    ap.add_argument('--app',required=True,type=pathlib.Path)
    ap.add_argument('--baseline',required=True,type=pathlib.Path)
    ap.add_argument('--report',required=True,type=pathlib.Path)
    a=ap.parse_args(); baseline=json.loads(a.baseline.read_text(encoding='utf-8'))
    paths=sorted(x['path'] for x in baseline['files'] if x['path'].startswith('locale/') and x['path'].endswith('/LC_MESSAGES/vlc.mo'))
    results=[]
    for rel in paths:
        lang=pathlib.PurePosixPath(rel).parts[1]
        source=a.source/'po'/(lang+'.po');dest=a.app/pathlib.PurePosixPath(rel)
        if not source.is_file():raise SystemExit('Missing source catalogue: '+str(source))
        dest.parent.mkdir(parents=True,exist_ok=True)
        run=subprocess.run(['msgfmt','--check','-o',str(dest),str(source)],capture_output=True,text=True)
        if run.returncode:raise SystemExit('Catalogue compilation failed: '+str(source)+'\n'+run.stderr)
        results.append({'language':lang,'source':str(source),'source_sha256':digest(source),
            'package_path':rel,'catalogue_sha256':digest(dest),'bytes':dest.stat().st_size,'warnings':run.stderr.strip()})
    report={'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'tool':subprocess.check_output(['msgfmt','--version'],text=True).splitlines()[0],
        'baseline_installer_sha256':baseline['installer']['sha256'],'catalogue_count':len(results),
        'source':str(a.source),'app':str(a.app),'catalogues':results,
        'limits':['Compilation and file presence only; ENABLE_NLS configuration and runtime lookup must be verified independently.',
            'Catalogues are compiled from existing upstream .po sources; no translation strings were rewritten.']}
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({'catalogues_compiled':len(results),'report':str(a.report),'warnings':sum(bool(x['warnings']) for x in results)}))

if __name__=='__main__':main()
