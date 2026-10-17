#!/usr/bin/env python3
"""Include design and font notices alongside the native portable player."""
import argparse,pathlib,shutil
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--app',required=True,type=pathlib.Path)
a=p.parse_args();root=pathlib.Path(__file__).resolve().parents[1];app=a.app.resolve()
if not (app/'libvlc.dll').is_file():raise SystemExit('Expected a staged Windows VLC/Coneic application directory')
shutil.copy2(root/'modules/gui/qt/fonts/coneic/LICENSE.txt',app/'COPYING.Inter')
shutil.copy2(root/'modules/gui/qt/pixmaps/coneic-reference/ATTRIBUTION.md',app/'COPYING.Coneic-design.md')
shutil.copy2(root/'modules/gui/qt/pixmaps/coneic-supplement/ATTRIBUTION.md',app/'COPYING.Coneic-supplement.md')
print('Staged Inter OFL and design attribution notices')
