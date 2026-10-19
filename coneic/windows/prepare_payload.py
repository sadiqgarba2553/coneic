"""Stage the complete Windows app and correct launcher VERSIONINFO, without editing input.

Uses Windows resource APIs; no external Python dependencies. Engine DLL versions stay
at their upstream versions. Resource editing must precede any Authenticode signing.
"""
import argparse
import ctypes as C
from ctypes import wintypes as W
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def block(key, value=b'', children=(), text=False):
    data = bytearray(b'\0' * 6 + (key + '\0').encode('utf-16le'))
    data.extend(b'\0' * (-len(data) % 4))
    data.extend(value)
    for child in children:
        data.extend(b'\0' * (-len(data) % 4))
        data.extend(child)
    struct.pack_into('<HHH', data, 0, len(data), len(value) // 2 if text else len(value), int(text))
    return bytes(data)


def version_resource(product, original_name):
    v = tuple(int(x) for x in product['fileVersion'].split('.'))
    if len(v) != 4 or any(x < 0 or x > 65535 for x in v):
        raise ValueError('fileVersion must contain four unsigned 16-bit numbers')
    fixed = struct.pack('<13I', 0xFEEF04BD, 0x10000,
                        (v[0] << 16) | v[1], (v[2] << 16) | v[3],
                        (v[0] << 16) | v[1], (v[2] << 16) | v[3],
                        0x3F, 2, 0x40004, 1, 0, 0, 0)  # prerelease application
    strings = {
        'CompanyName': product['company'], 'ProductName': product['displayName'],
        'ProductVersion': product['displayVersion'], 'FileVersion': product['fileVersion'],
        'InternalName': 'Coneic', 'OriginalFilename': original_name,
        'FileDescription': 'Coneic media player (VLC source fork)',
        'LegalCopyright': 'Coneic contributors; underlying VLC copyright VideoLAN and VLC Authors',
        'LegalTrademarks': 'VLC media player, VideoLAN and x264 are registered trademarks from VideoLAN',
        'Comments': 'Made by BLUECLOUD - https://bluecloudai.online; VLC engine 3.0.24',
    }
    table = block('040904E4', children=[block(k, (v + '\0').encode('utf-16le'), text=True)
                                          for k, v in strings.items()], text=True)
    return block('VS_VERSION_INFO', fixed, [block('StringFileInfo', children=[table], text=True),
                 block('VarFileInfo', children=[block('Translation', struct.pack('<HH', 0x409, 1252))], text=True)])


def update_version(path, product):
    kernel = C.WinDLL('kernel32', use_last_error=True)
    begin = kernel.BeginUpdateResourceW
    begin.argtypes = [W.LPCWSTR, W.BOOL]; begin.restype = W.HANDLE
    update = kernel.UpdateResourceW
    update.argtypes = [W.HANDLE, C.c_void_p, C.c_void_p, W.WORD, C.c_void_p, W.DWORD]
    update.restype = W.BOOL
    end = kernel.EndUpdateResourceW
    end.argtypes = [W.HANDLE, W.BOOL]; end.restype = W.BOOL
    handle = begin(str(path.resolve()), False)
    if not handle:
        raise C.WinError(C.get_last_error())
    committed = False
    try:
        data = version_resource(product, path.name)
        buf = C.create_string_buffer(data)
        if not update(handle, C.c_void_p(16), C.c_void_p(1), 0x409, buf, len(data)):
            raise C.WinError(C.get_last_error())
        committed = True
    finally:
        if not end(handle, not committed):
            raise C.WinError(C.get_last_error())


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--app', type=Path, required=True)
    p.add_argument('--stage', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--qt-plugin', type=Path, help='Explicit rebuilt Coneic presentation DLL; engine files stay unchanged')
    a = p.parse_args()
    source, stage = a.app.resolve(), a.stage.resolve()
    if stage.exists() or source == stage or source in stage.parents or stage in source.parents:
        raise SystemExit('Stage must be new and separate from the input app')
    required = ['Coneic.exe', 'libvlc.dll', 'libvlccore.dll', 'plugins/gui/libqt_plugin.dll',
                'COPYING.txt', 'COPYING.LIB.txt', 'COPYING.Coneic-design.md', 'COPYING.Inter']
    for name in required:
        if not (source / name).is_file():
            raise SystemExit('Incomplete application: missing ' + name)
    files = sorted(x for x in source.rglob('*') if x.is_file())
    if any(x.is_symlink() or x.is_junction() for x in source.rglob('*')):
        raise SystemExit('Application payload must not contain links or junctions')
    if len(list((source / 'plugins').rglob('*.dll'))) < 360:
        raise SystemExit('Incomplete Coneic plugin payload (expected at least 360 DLLs)')
    before = {x.relative_to(source).as_posix(): sha(x) for x in files}
    shutil.copytree(source, stage)
    product = json.loads(Path(__file__).with_name('product.json').read_text(encoding='utf-8'))
    changed = []
    for name in ['Coneic.exe', 'vlc.exe']:
        if (stage / name).is_file():
            update_version(stage / name, product)
            changed.append(name)
    if a.qt_plugin:
        if not a.qt_plugin.is_file() or a.qt_plugin.suffix.lower() != '.dll':
            raise SystemExit('The rebuilt Qt plugin must be a DLL file')
        shutil.copyfile(a.qt_plugin, stage / 'plugins/gui/libqt_plugin.dll')
        changed.append('plugins/gui/libqt_plugin.dll')
        subprocess.run([str(stage / 'vlc-cache-gen.exe'), str(stage / 'plugins')],
                       cwd=stage, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
        changed.append('plugins/plugins.dat')
    subprocess.run([sys.executable, str(Path(__file__).with_name('stage-shell.py')),
                    '--app', str(stage)], check=True)
    manifest = []
    for rel, original in before.items():
        current = sha(stage / rel)
        shell_file = (rel.startswith('Coneic-shell/') or rel in
                      ['Set Coneic as default.cmd', 'Remove Coneic file associations.cmd'])
        if rel not in changed and not shell_file and current != original:
            raise SystemExit('Unexpected payload mutation: ' + rel)
        if sha(source / rel) != original:
            raise SystemExit('Input changed during packaging: ' + rel)
        manifest.append({'path': rel, 'inputSHA256': original, 'sha256': current})
    additions = sorted(x.relative_to(stage).as_posix() for x in stage.rglob('*')
                       if x.is_file() and x.relative_to(stage).as_posix() not in before)
    result = {'passed': True, 'product': product, 'files': len(manifest),
              'shellIntegrationAdded': additions,
              'pluginDLLs': len(list((stage / 'plugins').rglob('*.dll'))),
              'inputUnchanged': True, 'metadataUpdated': [x for x in changed if x.endswith('.exe')],
              'presentationUpdated': [x for x in changed if not x.endswith('.exe')], 'manifest': manifest}
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.report.write_bytes((json.dumps(result, indent=2) + '\n').encode('utf-8'))
    print(json.dumps({k: v for k, v in result.items() if k != 'manifest'}))


if __name__ == '__main__':
    main()
