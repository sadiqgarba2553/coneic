"""Verify that a MakeAppx archive contains every staged payload byte."""
import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile
from urllib.parse import unquote

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--package', type=Path, required=True)
p.add_argument('--layout', type=Path, required=True)
p.add_argument('--mode', choices=['Store', 'Test'], required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
ns = {'p': 'http://schemas.microsoft.com/appx/manifest/foundation/windows10'}
with zipfile.ZipFile(a.package) as z:
    if z.testzip() is not None:
        raise SystemExit('Archive CRC failure')
    # OPC ZIP part names are URI-escaped (e.g. ca@valencia becomes ca%40valencia).
    parts = {unquote(name): name for name in z.namelist()}
    names = list(parts)
    if len(parts) != len(z.namelist()):
        raise SystemExit('URI-decoded archive path collision')
    if len({x.casefold() for x in names}) != len(names):
        raise SystemExit('Duplicate or case-colliding archive entries')
    expected = {x.relative_to(a.layout).as_posix(): x for x in a.layout.rglob('*') if x.is_file()}
    verified = []
    for rel, path in sorted(expected.items()):
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if hashlib.sha256(z.read(parts[rel])).hexdigest() != digest:
            raise SystemExit('Packaged content differs: ' + rel)
        verified.append({'path': rel, 'sha256': digest})
    extras = sorted(set(names) - expected.keys())
    if set(extras) != {'AppxBlockMap.xml', '[Content_Types].xml'}:
        raise SystemExit('Unexpected package content: ' + repr(extras))
    identity = ET.fromstring(z.read('AppxManifest.xml')).find('p:Identity', ns).attrib
    if a.mode == 'Store' and 'LocalValidation' in identity['Name']:
        raise SystemExit('Test identity cannot be submitted to the Store')
    blockmap = ET.fromstring(z.read('AppxBlockMap.xml'))
    if blockmap.attrib.get('HashMethod') != 'http://www.w3.org/2001/04/xmlenc#sha256':
        raise SystemExit('Package does not use SHA256 block hashes')
result = {'passed': True, 'mode': a.mode, 'identity': identity, 'verifiedFiles': len(verified),
          'packageSHA256': hashlib.sha256(a.package.read_bytes()).hexdigest(),
          'signature': 'Unsigned, for Microsoft Store signing; not public sideload distribution',
          'limits': 'Archive/schema validation does not establish Store certification or runtime parity.',
          'manifest': verified}
a.output.write_bytes((json.dumps(result, indent=2) + '\n').encode('utf-8'))
print(json.dumps({k: v for k, v in result.items() if k != 'manifest'}))
