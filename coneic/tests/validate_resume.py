"""Verify the packaged player's checkpoint survives an abrupt process exit."""
import argparse
import json
import os
from pathlib import Path
import re
import socket
import subprocess
import time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--app',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
app=a.app.resolve();output=a.output.resolve()
output.mkdir(parents=True,exist_ok=False)
media=Path(__file__).resolve().parent/'fixtures/Coneic motion.avi'
env={**os.environ,'CONEIC_TEST_OUTPUT':str(output),'CONEIC_TEST_PLAYBACK_ONLY':'1'}
env.pop('CONEIC_TEST_MEDIA',None)
process=None
def start(first=False):
    with socket.socket() as probe:
        probe.bind(('127.0.0.1',0));port=probe.getsockname()[1]
    command=[str(app/'Coneic.exe'),'--ignore-config','--config',str(output/'vlcrc'),'--no-one-instance','--no-media-library','--no-qt-privacy-ask','--no-qt-updates-notif','--avcodec-hw=none','--aout=dummy','--qt-continue=2','--extraintf=oldrc','--rc-host=127.0.0.1:'+str(port),'--rc-quiet']
    if first:command.append('--start-time=7')
    process=subprocess.Popen(command+[str(media)],cwd=app,env=env,creationflags=subprocess.CREATE_NO_WINDOW,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    deadline=time.monotonic()+12
    while time.monotonic()<deadline:
        if process.poll() is not None:raise RuntimeError('Player exited before the control interface opened')
        try:
            connection=socket.create_connection(('127.0.0.1',port),timeout=.3);connection.settimeout(.3);return process,connection
        except OSError:time.sleep(.1)
    process.kill();process.wait();raise RuntimeError('Local control interface did not open')
def playback_time(connection):
    connection.sendall(b'get_time\n');text=b'';deadline=time.monotonic()+1
    while time.monotonic()<deadline:
        try:text+=connection.recv(4096)
        except socket.timeout:break
    values=re.findall(rb'(?:^|[\r\n])\s*(\d+)\s*(?=[\r\n])',text)
    return int(values[-1]) if values else -1
try:
    process,connection=start(True);deadline=time.monotonic()+10;saved=-1
    while time.monotonic()<deadline:
        settings=output/'profile/interface.ini'
        if settings.exists():
            match=re.search(r'^times=(\d+)',settings.read_text(encoding='utf-8-sig'),re.M)
            if match and int(match[1])>=7000:saved=int(match[1]);break
        time.sleep(.2)
    if saved<7000:raise RuntimeError('Playback position was not checkpointed while the player was running')
    # Only this disposable test process is terminated; no real user profile is used.
    process.kill();process.wait();connection.close()
    process,connection=start();deadline=time.monotonic()+8;resumed=-1
    while time.monotonic()<deadline:
        resumed=playback_time(connection)
        if resumed>=saved//1000-1:break
        time.sleep(.1)
    if resumed<saved//1000-1:raise RuntimeError('The player did not resume its saved position')
    connection.sendall(b'quit\n');connection.close();process.wait(timeout=15)
    report={'passed':process.returncode==0,'savedMilliseconds':saved,'resumedSeconds':resumed,'isolatedProfile':str(output/'profile')}
    (output/'resume-results.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report));raise SystemExit(0 if report['passed'] else 1)
finally:
    if process and process.poll() is None:process.kill();process.wait()
