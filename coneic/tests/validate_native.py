"""Exercise the packaged native fork through its actual libVLC API."""
import argparse, ctypes as C, json, os, pathlib, subprocess, time

p=argparse.ArgumentParser();p.add_argument('--app',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--controls-only',action='store_true');p.add_argument('--fixtures',type=pathlib.Path);a=p.parse_args()
a.app=a.app.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
fixtures=a.fixtures or pathlib.Path(__file__).resolve().parents[2]/'outputs'/'coneic-validation'/'fixtures'
os.add_dll_directory(str(a.app));os.environ['VLC_PLUGIN_PATH']=str(a.app/'plugins')
v=C.CDLL(str(a.app/'libvlc.dll'))
def api(name,rest,args):
 f=getattr(v,'libvlc_'+name);f.restype=rest;f.argtypes=args;return f
P=C.c_void_p; I=C.c_int; S=C.c_char_p; L=C.c_int64
new=api('new',P,[I,C.POINTER(S)]); release=api('release',None,[P]); version=api('get_version',S,[])
media_new=api('media_new_path',P,[P,S]);media_release=api('media_release',None,[P]);option=api('media_add_option',None,[P,S])
player_new=api('media_player_new_from_media',P,[P]);play=api('media_player_play',I,[P]);state=api('media_player_get_state',I,[P]);stop=api('media_player_stop',None,[P]);player_release=api('media_player_release',None,[P])
length=api('media_player_get_length',L,[P]);gettime=api('media_player_get_time',L,[P]);settime=api('media_player_set_time',None,[P,L]);pause=api('media_player_set_pause',None,[P,I]);rate=api('media_player_set_rate',I,[P,C.c_float]);getrate=api('media_player_get_rate',C.c_float,[P]);audio_count=api('audio_get_track_count',I,[P]);spu_count=api('video_get_spu_count',I,[P])
eq_new=api('audio_equalizer_new',P,[]);eq_apply=api('media_player_set_equalizer',I,[P,P]);eq_release=api('audio_equalizer_release',None,[P])
class Stats(C.Structure):
 _fields_=[('read_bytes',I),('input_bitrate',C.c_float),('demux_read_bytes',I),('demux_bitrate',C.c_float),('corrupted',I),('discontinuity',I),('decoded_video',I),('decoded_audio',I),('displayed_pictures',I),('lost_pictures',I),('played_abuffers',I),('lost_abuffers',I),('sent_packets',I),('sent_bytes',I),('send_bitrate',C.c_float)]
stats_get=api('media_get_stats',I,[P,C.POINTER(Stats)])
checks=[]
def check(name,passed,**detail):
 checks.append(dict(name=name,passed=bool(passed),**detail));print(json.dumps(checks[-1]),flush=True)
 (a.output/'native-engine-progress.json').write_text(json.dumps({'completed':False,'checks':checks},indent=2),encoding='utf-8')
def wait(test,seconds=10):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  if test(): return True
  time.sleep(.1)
 return False
args=[b'--ignore-config',b'--no-media-library',b'--aout=dummy',b'--vout=dummy',b'--avcodec-hw=none',b'--file-logging',('--logfile='+str(a.output/'native-engine.log')).encode(),b'--verbose=2']
inst=new(len(args),(S*len(args))(*args));check('Native engine initializes',inst,version=version().decode())
if not inst: raise SystemExit(1)
def exercise(path,kind,controls=False,subtitle=False):
 m=media_new(inst,str(path).encode());
 if subtitle: option(m,(':sub-file='+str(fixtures/'Coneic motion.srt')).encode())
 pl=player_new(m);check(kind+' starts',play(pl)==0 and wait(lambda:state(pl)==3),state=state(pl))
 wait(lambda:gettime(pl)>500)
 if controls:
  pause(pl,1);check('Pause',wait(lambda:state(pl)==4));pause(pl,0);check('Resume',wait(lambda:state(pl)==3))
  settime(pl,3000);check('Seek',wait(lambda:gettime(pl)>=2800),observed_ms=gettime(pl))
  check('Playback rate',rate(pl,1.5)==0 and abs(getrate(pl)-1.5)<.05)
  eq=eq_new();check('Equalizer applies',eq and eq_apply(pl,eq)==0);eq_release(eq)
 if subtitle: check('External subtitles loaded',wait(lambda:spu_count(pl)>1),track_count=spu_count(pl))
 if kind=='WAV audio':check('Audio track detected',audio_count(pl)>0,track_count=audio_count(pl))
 ended=wait(lambda:state(pl) in (6,7),25);st=Stats();available=stats_get(m,C.byref(st))
 check(kind+' decodes to end',ended and state(pl)==6 and available and (st.decoded_video>0 if kind!='WAV audio' else st.decoded_audio>0),state=state(pl),stats={n:getattr(st,n) for n,_ in st._fields_})
 print(kind+' stopping player',flush=True);stop(pl)
 print(kind+' releasing player',flush=True);player_release(pl)
 print(kind+' releasing media',flush=True);media_release(m)
exercise(fixtures/'Coneic motion.avi','MJPEG video',True,True)
exercise(fixtures/'A quiet moment.wav','WAV audio')
if a.controls_only:
 release(inst);check('Native engine releases cleanly',True)
 result={'passed':all(x['passed'] for x in checks),'checks':checks,'scope':'libVLC controls with MJPEG video and WAV audio only. H264 embedding scenario is covered by a separate known-failure comparison.'}
 (a.output/'native-engine-results.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
 raise SystemExit(0 if result['passed'] else 1)
dest=a.output/'native-converted.mp4'
executable=a.app/('Coneic.exe' if (a.app/'Coneic.exe').exists() else 'vlc.exe')
cmd=[str(executable),'--ignore-config','--no-media-library','-I','dummy','--play-and-exit','--file-logging','--logfile='+str(a.output/'native-convert.log'),'--verbose=2',str(fixtures/'Coneic motion.avi'),'--sout=#transcode{vcodec=h264,vb=1200,acodec=mp4a,ab=128}:std{access=file,mux=mp4,dst='+str(dest)+'}']
try:
 r=subprocess.run(cmd,cwd=a.app,timeout=65,creationflags=subprocess.CREATE_NO_WINDOW)
 ok=r.returncode==0 and dest.exists() and dest.stat().st_size>1000 and b'ftyp' in dest.read_bytes()[:32]
 check('Native H264 MP4 conversion',ok,exit_code=r.returncode,bytes=dest.stat().st_size if dest.exists() else 0)
 if ok:exercise(dest,'Converted H264 video')
except subprocess.TimeoutExpired:check('Native H264 MP4 conversion',False,error='timeout')
print('Releasing native engine',flush=True)
release(inst)
check('Native engine releases cleanly',True)
result={'passed':all(x['passed'] for x in checks),'checks':checks,'scope':'Packaged native engine. Dummy audio/video outputs; physical devices, optical discs, casting and external network services not exercised.'}
(a.output/'native-engine-results.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
raise SystemExit(0 if result['passed'] else 1)
