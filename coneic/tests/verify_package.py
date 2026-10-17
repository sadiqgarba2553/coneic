#!/usr/bin/env python3
"""Read-only native Coneic package audit against statically extracted VLC payload."""
import argparse, collections, datetime, hashlib, json, pathlib, struct

SYSTEM_DLLS = set('''advapi32.dll avicap32.dll avifil32.dll avrt.dll bcrypt.dll bcryptprimitives.dll
 cabinet.dll cfgmgr32.dll combase.dll comctl32.dll comdlg32.dll crypt32.dll cryptbase.dll cryptnet.dll
 cryptsp.dll d2d1.dll d3d9.dll d3d11.dll d3d12.dll d3dcompiler_43.dll d3dcompiler_47.dll dcomp.dll
 dbghelp.dll ddraw.dll dhcpcsvc.dll dnsapi.dll dsound.dll dwmapi.dll dwrite.dll dxgi.dll dxva2.dll
 faultrep.dll fwpuclnt.dll gdi32.dll gdiplus.dll glu32.dll hid.dll imm32.dll iphlpapi.dll kernel32.dll
 kernelbase.dll ksuser.dll mf.dll mfplat.dll mfreadwrite.dll mfuuid.dll mpr.dll msacm32.dll msasn1.dll
 msvcrt.dll msimg32.dll ncrypt.dll netapi32.dll normaliz.dll ntdll.dll ole32.dll oleacc.dll oleaut32.dll opengl32.dll
 powrprof.dll propsys.dll psapi.dll rasapi32.dll rpcrt4.dll secur32.dll setupapi.dll shell32.dll shlwapi.dll
 strmiids.dll ucrtbase.dll user32.dll userenv.dll usp10.dll uxtheme.dll version.dll winhttp.dll wininet.dll
 winmm.dll winscard.dll winspool.drv wintrust.dll wldap32.dll ws2_32.dll wsock32.dll wtsapi32.dll xmllite.dll'''.split())

def sha(data): return hashlib.sha256(data).hexdigest()

def inspect_pe(path):
    b=path.read_bytes()
    def u16(o): return struct.unpack_from('<H',b,o)[0]
    def u32(o): return struct.unpack_from('<I',b,o)[0]
    if b[:2]!=b'MZ': raise ValueError('Missing MZ signature')
    p=u32(0x3c)
    if b[p:p+4]!=b'PE\0\0': raise ValueError('Missing PE signature')
    o=p+24; magic=u16(o)
    if magic not in (0x10b,0x20b): raise ValueError('Unsupported optional header')
    directory=o+(112 if magic==0x20b else 96)
    imagebase=struct.unpack_from('<Q',b,o+24)[0] if magic==0x20b else u32(o+28)
    sections=[]
    for i in range(u16(p+6)):
        s=o+u16(p+20)+40*i
        sections.append((u32(s+12),u32(s+16),u32(s+20)))
    def off(r):
        for rva,size,raw in sections:
            if rva<=r<rva+size:
                result=raw+r-rva
                if result>=len(b): break
                return result
        raise ValueError('Unmapped PE RVA '+hex(r))
    def string(pos):
        end=b.find(b'\0',pos,min(pos+4096,len(b)))
        if end<0: raise ValueError('Unterminated PE string')
        return b[pos:end].decode('ascii')
    imports=[]; delays=[]; exports=[]
    if u32(directory+8):
        start=off(u32(directory+8))
        for i in range(4096):
            s=start+20*i; rva=u32(s+12)
            if not rva: break
            imports.append(string(off(rva)))
        else: raise ValueError('Import descriptor budget exhausted')
    if u32(directory+13*8):
        start=off(u32(directory+13*8))
        for i in range(4096):
            s=start+32*i; attr=u32(s); name=u32(s+4)
            if not name: break
            delays.append(string(off(name if attr&1 else name-imagebase)))
        else: raise ValueError('Delay descriptor budget exhausted')
    if u32(directory):
        e=off(u32(directory));count=u32(e+24)
        if count>100000: raise ValueError('Export name budget exhausted')
        names=off(u32(e+32)) if count else 0
        exports=[string(off(u32(names+4*i))) for i in range(count)]
    return {'machine':hex(u16(p+4)),'imports':sorted(set(imports)), 'delay_imports':sorted(set(delays)), 'exports':sorted(exports)}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--app',required=True,type=pathlib.Path)
    ap.add_argument('--baseline',required=True,type=pathlib.Path)
    ap.add_argument('--output-prefix',required=True,type=pathlib.Path)
    ap.add_argument('--entrypoint',default=None,help='Relative app EXE name; default finds vlc.exe or Coneic.exe')
    args=ap.parse_args(); root=args.app.resolve()
    if not root.is_dir(): raise SystemExit('Package directory does not exist: '+str(root))
    baseline=json.loads(args.baseline.read_text(encoding='utf-8'))
    if baseline.get('installer',{}).get('sha256')!='d711e1e1fe52052748c39080c7dce63f6b7e4c315efedf1774a3f1957b782ff3':
        raise SystemExit('Unrecognized supplied-installer baseline identity')
    files={p.relative_to(root).as_posix().lower():p for p in root.rglob('*') if p.is_file()}
    old={x['path'].lower():x for x in baseline['files']}
    baseline_dlls={p for p in old if p.startswith('plugins/') and p.endswith('.dll')}
    baseline_jars={p for p in old if p.startswith('plugins/') and p.endswith('.jar')}
    current_dlls={p for p in files if p.startswith('plugins/') and p.endswith('.dll')}
    data={p for p in old if p.startswith(('lua/','locale/','skins/','hrtfs/'))}
    notices={p for p in old if p in {'copying.txt','readme.txt','authors.txt','thanks.txt','news.txt'}}
    required_core={'libvlc.dll','libvlccore.dll','vlc-cache-gen.exe'}
    entry=args.entrypoint.lower() if args.entrypoint else next((p for p in ('coneic.exe','vlc.exe') if p in files),'vlc.exe')
    required_core.add(entry)
    missing_plugins=sorted(baseline_dlls-files.keys()); missing_jars=sorted(baseline_jars-files.keys())
    missing_data=sorted(data-files.keys());missing_notices=sorted(notices-files.keys())
    missing_core=sorted(required_core-files.keys())
    images={};errors=[];wrong_machine=[];missing_deps=[]
    for rel,path in sorted(files.items()):
        if rel.startswith('$pluginsdir/'): continue  # NSIS installer helpers are intentionally x86.
        if not rel.endswith(('.dll','.exe')):continue
        try: info=inspect_pe(path)
        except (ValueError,struct.error,IndexError,UnicodeDecodeError) as error:
            errors.append({'path':rel,'error':str(error)});continue
        images[rel]=info
        if info['machine']!='0x8664': wrong_machine.append({'path':rel,'machine':info['machine']})
        for kind in ('imports','delay_imports'):
            for name in info[kind]:
                normalized=name.lower()
                if normalized in SYSTEM_DLLS or normalized.startswith(('api-ms-win-','ext-ms-win-')): continue
                local=(pathlib.PurePosixPath(rel).parent/normalized).as_posix()
                if normalized not in files and local not in files:
                    missing_deps.append({'image':rel,'kind':kind,'dependency':name,
                        'resolution_scope':'same image directory and App root; no development PATH assumed'})
    export_checks=[]
    for rel,reference in baseline.get('pe_images',{}).items():
        rel=rel.lower()
        if rel not in {'libvlc.dll','libvlccore.dll','plugins/gui/libqt_plugin.dll'}:continue
        before={x['name'] for x in reference.get('exports',[])}
        actual=set(images.get(rel,{}).get('exports',[]))
        export_checks.append({'image':rel,'baseline_count':len(before),'current_count':len(actual),
            'missing':sorted(before-actual),'added':sorted(actual-before)})
    inventory=[{'path':rel,'bytes':p.stat().st_size,'sha256':sha(p.read_bytes())} for rel,p in sorted(files.items())]
    integration={'baseline_activex_present':'axvlc.dll' in old,'package_activex_present':'axvlc.dll' in files,
        'baseline_npapi_present':'npvlc.dll' in old,'package_npapi_present':'npvlc.dll' in files,
        'classification':'Optional browser/ActiveX integration; separate from desktop-player parity'}
    failures=bool(missing_plugins or missing_jars or missing_data or missing_notices or missing_core or errors or wrong_machine or missing_deps or any(x['missing'] for x in export_checks))
    result={'schema':'coneic-vlc-native-package-parity-v1','created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'app':str(root),'baseline_installer_sha256':baseline['installer']['sha256'],
        'status':'REVIEW REQUIRED' if failures else 'STATIC PACKAGE CHECKS PASS',
        'entrypoint':entry,'counts':{'baseline_plugin_dlls':len(baseline_dlls),'package_plugin_dlls':len(current_dlls),
            'baseline_plugin_jars':len(baseline_jars),'baseline_data_files':len(data),'package_files':len(files),'pe_images_checked':len(images)},
        'missing_plugin_dlls':missing_plugins,'added_plugin_dlls':sorted(current_dlls-baseline_dlls),
        'missing_plugin_jars':missing_jars,'missing_data_files':missing_data,'missing_notice_files':missing_notices,
        'missing_core_files':missing_core,'unresolved_imports':missing_deps,'wrong_machine':wrong_machine,
        'pe_parse_errors':errors,'export_checks':export_checks,'optional_integrations':integration,
        'system_dll_allowlist':sorted(SYSTEM_DLLS),'pe_images':images,'files':inventory,
        'limits':['Static package audit only; no binary was loaded or executed.',
            'Known Windows DLL/API-set names are allowlisted, not checked for presence on a particular Windows release.',
            'Both ordinary and delay-load imports are inspected; runtime-computed LoadLibrary/GetProcAddress dependencies remain unobserved.',
            'DLL resolution only considers the package root and importing image directory; custom runtime loader directories are unobserved.',
            'Identical exported names do not establish ABI/semantic compatibility.',
            'Different hashes are expected after rebuilding; input and package hashes identify artifacts, not functional equivalence.',
            'ActiveX and NPAPI differences are reported separately and do not fail desktop-player checks.']}
    args.output_prefix.parent.mkdir(parents=True,exist_ok=True)
    args.output_prefix.with_suffix('.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    lines=['# Coneic native package parity','',f'Generated: {result["created_utc"]}','',f'**{result["status"]}**','',
        f'Package: `{root}`. Baseline: supplied VLC 3.0.24 installer, SHA-256 `{baseline["installer"]["sha256"]}`.','',
        'This checks files, exported names, PE architecture and static imports. It does not verify playback or runtime feature parity.','',
        '| Measure | Count |','| --- | ---: |']
    lines += [f'| {k.replace("_"," ")} | {v} |' for k,v in result['counts'].items()]
    for label,key in [('Missing plugin DLLs','missing_plugin_dlls'),('Additional plugin DLLs','added_plugin_dlls'),
        ('Missing plugin JARs','missing_plugin_jars'),('Missing data files','missing_data_files'),
        ('Missing notice files','missing_notice_files'),('Missing core files','missing_core_files')]:
        lines += ['', '## '+label,'']
        lines += ['- `'+p+'`' for p in result[key]] or ['None.']
    lines += ['', '## Core exported names','', '| Image | Baseline | Current | Missing |','| --- | ---: | ---: | ---: |']
    lines += [f'| `{x["image"]}` | {x["baseline_count"]} | {x["current_count"]} | {len(x["missing"])} |' for x in export_checks]
    lines += ['', '## Static dependency issues','']
    lines += [f'- `{x["image"]}`: unresolved {x["kind"]} dependency `{x["dependency"]}`.' for x in missing_deps] or ['None within the declared resolution scope.']
    lines += [f'- Invalid PE `{x["path"]}`: {x["error"]}' for x in errors]
    lines += [f'- Architecture mismatch `{x["path"]}`: {x["machine"]}' for x in wrong_machine]
    lines += ['', '## Optional integrations','',
        f'ActiveX: baseline={integration["baseline_activex_present"]}, package={integration["package_activex_present"]}. NPAPI: baseline={integration["baseline_npapi_present"]}, package={integration["package_npapi_present"]}. These differences are separate from desktop-player checks.','',
        '## Limits','']+['- '+x for x in result['limits']]
    args.output_prefix.with_suffix('.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print(json.dumps({'status':result['status'],'counts':result['counts'],'missing_plugins':len(missing_plugins),
        'missing_jars':len(missing_jars),'missing_data':len(missing_data),'unresolved_imports':len(missing_deps),
        'optional_integrations':integration,'output_prefix':str(args.output_prefix)},indent=2))
    return 1 if failures else 0

if __name__=='__main__':raise SystemExit(main())
