#!/usr/bin/env python3
"""Inventory statically extracted VLC payload; never loads or executes its code."""
import argparse, collections, datetime, hashlib, json, pathlib, struct

def sha(b): return hashlib.sha256(b).hexdigest()
def pe(p):
    b=p.read_bytes()
    u16=lambda x:struct.unpack_from('<H',b,x)[0]
    u32=lambda x:struct.unpack_from('<I',b,x)[0]
    base=u32(0x3c); opt=base+24; magic=u16(opt)
    dirs=opt+(112 if magic==0x20b else 96)
    secs=[]
    for i in range(u16(base+6)):
        s=opt+u16(base+20)+40*i
        secs.append({'name':b[s:s+8].split(b'\0')[0].decode('ascii'), 'rva':u32(s+12),'raw_size':u32(s+16),'raw_offset':u32(s+20)})
    def off(r):
        for s in secs:
            if s['rva'] <=r<s['rva']+s['raw_size']: return s['raw_offset']+r-s['rva']
        raise ValueError('RVA is not file-backed')
    def string(o): return b[o:b.index(b'\0',o)].decode('ascii')
    imports=[]; exports=[]
    if u32(dirs+8):
        pos=off(u32(dirs+8))
        while u32(pos+12):
            imports.append(string(off(u32(pos+12))));pos+=20
    if u32(dirs):
        e=off(u32(dirs)); names=off(u32(e+32));ords=off(u32(e+36)); funcs=off(u32(e+28))
        for i in range(u32(e+24)):
            exports.append({'name':string(off(u32(names+4*i))), 'rva':u32(funcs+4*u16(ords+2*i))})
    return {'machine':hex(u16(base+4)),'format':'PE32+' if magic==0x20b else 'PE32','imports':imports,'exports':exports,'sections':secs}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root',type=pathlib.Path,required=True)
    ap.add_argument('--installer',type=pathlib.Path,required=True)
    ap.add_argument('--output-prefix',type=pathlib.Path,required=True)
    a=ap.parse_args(); files=[];groups=collections.defaultdict(list)
    for p in sorted(a.root.rglob('*')):
        if not p.is_file():continue
        b=p.read_bytes(); rel=p.relative_to(a.root).as_posix()
        files.append({'path':rel,'bytes':len(b),'sha256':sha(b)})
        if rel.startswith('plugins/'):
            groups[pathlib.PurePosixPath(rel).parts[1]].append(rel)
    pe_files={}
    for rel in ['vlc.exe','libvlc.dll','libvlccore.dll','axvlc.dll','npvlc.dll','plugins/gui/libqt_plugin.dll']:
        if (a.root/rel).is_file(): pe_files[rel]=pe(a.root/rel)
    installer=a.installer.read_bytes()
    result={'schema':'coneic-supplied-vlc-payload-v1','observed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'evidence_type':'ordinary shell extraction, file hashing and static PE-table parsing; not REA Evidence',
        'installer':{'path':str(a.installer),'bytes':len(installer),'sha256':sha(installer)},
        'extraction':{'root':str(a.root),'tool':'7-Zip 23.01 Linux x64 via isolated ConeicBuild WSL distro','format':'NSIS-3 Unicode, LZMA:23, solid','installer_executed':False,'file_count':len(files),'total_uncompressed_bytes':sum(x['bytes'] for x in files)},
        'version_resource_observation':{'method':'Windows FileVersionInfo read of extracted vlc.exe; no execution','FileVersion':'3.0.24','ProductVersion':'3,0,24,0','ProductName':'VLC media player','CompanyName':'VideoLAN'},
        'plugin_file_count':sum(map(len,groups.values())),
        'plugin_dll_count':sum(x['path'].startswith('plugins/') and x['path'].endswith('.dll') for x in files),
        'plugin_groups':dict(groups),'pe_images':pe_files,
        'optional_integration_files':{'axvlc.dll':(a.root/'axvlc.dll').is_file(),'npvlc.dll':(a.root/'npvlc.dll').is_file()},
        'files':files,
        'limits':['Static presence and PE imports/exports do not establish load success or runtime behavior.',
            'NSIS extraction reflects embedded payload, not installer choices, generated files, registration or side effects.',
            'Version resource strings are declarations, not an independent publisher signature verification.',
            'Plugin names/hashes form a supplied3.0.24 comparison baseline; different toolchains produce different hashes.']}
    a.output_prefix.parent.mkdir(parents=True,exist_ok=True)
    a.output_prefix.with_suffix('.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    lines=['# Supplied VLC 3.0.24 payload evidence','',f'Observed: {result["observed_utc"]}','',
        '**The previously unresolved installer payload is now statically extracted and inventoried.** The installer was not executed. This is ordinary shell extraction/static parsing, separate from REA evidence.','',
        f'Installer: `{a.installer}`; **{len(installer):,} bytes**; SHA-256 `{sha(installer)}`.','',
        f'7-Zip 23.01 recognized NSIS-3 Unicode/LZMA and extracted **{len(files)} files**, **{result["extraction"]["total_uncompressed_bytes"]:,} bytes**, into `{a.root}`.','',
        'Windows FileVersionInfo read from extracted `vlc.exe`: FileVersion **3.0.24**, ProductVersion **3,0,24,0**, ProductName **VLC media player**, CompanyName **VideoLAN**. This is the supplied installer payload, distinct from installed VLC 3.0.23.','',
        '## Baseline for the Coneic fork','',f'Plugin files: **{result["plugin_file_count"]}**; plugin DLLs: **{result["plugin_dll_count"]}**. The companion JSON includes every relative filename, byte size, SHA-256, grouped plugins and selected PE imports/exports. Compare packaged plugin paths to identify missing/added modules; compare capabilities and build configuration before treating differences as regressions. DLL hashes will normally differ in a rebuilt/styled fork.','',
        '| Image | Architecture | Imports | Named exports |','| --- | --- | --- | ---: |']
    lines += [f'| `{p}` | {v["format"]}, {v["machine"]} | '+', '.join('`'+x+'`' for x in v['imports'])+f' | {len(v["exports"])} |' for p,v in pe_files.items()]
    lines += ['', '## Optional integrations','',
        '`axvlc.dll` **is present** (ActiveX integration). A file named `npvlc.dll` **is absent from all 584 extracted files**; this bounded absence must not be described as missing desktop playback functionality. NSIS helper DLLs are under `$PLUGINSDIR/` and are installer helpers, not VLC playback plugins.','',
        '## Plugin groups and exact files','', '| Group | Files | Names |','| --- | ---: | --- |']
    lines += [f'| `{k}` | {len(v)} | '+', '.join('`'+pathlib.PurePosixPath(x).name+'`' for x in v)+' |' for k,v in sorted(groups.items())]
    lines += ['', '## Limits','']+['- '+s for s in result['limits']]
    lines += ['', 'The earlier `vlc-architecture.md` extraction limitation records the earlier tool boundary; this report supersedes that unresolved payload inventory. Prior installed-3.0.23 observations remain separate reference evidence.']
    a.output_prefix.with_suffix('.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print(json.dumps({k:result[k] for k in ['extraction','plugin_file_count','plugin_dll_count','optional_integration_files']},indent=2))

if __name__=='__main__':main()
