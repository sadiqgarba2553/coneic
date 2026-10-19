"""Exercise the packaged native interface in a separate, retained test profile."""
import argparse
import json
import os
from pathlib import Path
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--app', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--material', choices=['system', 'glass', 'solid'], default='glass')
p.add_argument('--playback', type=Path, help='Play a video twice through the GUI, then check clean shutdown')
a = p.parse_args()
app, output = a.app.resolve(), a.output.resolve()
profile = output / 'profile'
if output.exists():
    raise SystemExit('Use a new output folder to retain previous evidence.')
profile.mkdir(parents=True)
(profile / 'interface.ini').write_text('[Coneic]\nmaterial-mode=%d\n' % ['system', 'glass', 'solid'].index(a.material))
env = os.environ.copy()
env['CONEIC_TEST_OUTPUT'] = str(output)
env['CONEIC_TEST_MEDIA'] = str(Path(__file__).resolve().with_name('fixtures'))
command = [str(app / 'Coneic.exe'), '--no-one-instance', '--ignore-config',
           '--config', str(output / 'vlcrc'), '--no-media-library', '--no-qt-privacy-ask',
           '--no-qt-updates-notif', '--avcodec-hw=none', '--aout=dummy', '--file-logging',
           '--logfile=' + str(output / 'player.log'), '--verbose=1']
if a.playback:
    env['CONEIC_TEST_PLAYBACK_ONLY'] = '1'
    command.extend(['--play-and-exit', '--input-repeat=1', str(a.playback.resolve())])
run = subprocess.run(command, cwd=app, env=env, timeout=150,
                     creationflags=subprocess.CREATE_NO_WINDOW)
if a.playback:
    print(json.dumps({'exit': run.returncode, 'playback': str(a.playback), 'repeats': 2}))
    raise SystemExit(run.returncode)
result = json.loads((output / 'native-ui-results.json').read_text())
print(json.dumps({'exit': run.returncode, 'passed': result['passed'],
                  'checks': len(result['checks']), 'evidence': str(output)}))
raise SystemExit(0 if run.returncode == 0 and result['passed'] else 1)
