import argparse,pathlib,subprocess,json,hashlib,os
p=argparse.ArgumentParser();p.add_argument('--app',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--fixtures',type=pathlib.Path);a=p.parse_args()
a.app=a.app.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
fixtures=a.fixtures or pathlib.Path(__file__).resolve().parent/'fixtures'
executable=a.app/('Coneic.exe' if os.name=='nt' else 'coneic')
checks=[]
def run(name,arguments,expected=None,artifact=None,codec=None):
 log=a.output/(name+'.log')
 log.unlink(missing_ok=True)
 if artifact:artifact.unlink(missing_ok=True)
 cmd=[str(executable),'--ignore-config','--no-media-library','--no-one-instance','-I','dummy','--play-and-exit','--avcodec-hw=none','--vout=dummy','--aout=dummy','--file-logging','--logfile='+str(log),'--verbose=2']+arguments
 try:
  r=subprocess.run(cmd,cwd=a.app,timeout=65,creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
  text=log.read_text(encoding='utf-8',errors='replace') if log.exists() else ''
  ok=r.returncode==0 and bool(text) and (expected in text if expected else 'EOF reached' in text)
  video=None
  if artifact:ok=ok and artifact.exists() and artifact.stat().st_size>1000
  if artifact and ok and os.name!='nt':
   probe=subprocess.run(['ffprobe','-v','error','-count_frames','-select_streams','v:0','-show_entries','stream=codec_name,width,height,nb_read_frames','-of','json',str(artifact)],capture_output=True,text=True,timeout=30)
   streams=json.loads(probe.stdout).get('streams',[]) if probe.returncode==0 else []
   video=streams[0] if streams else {}
   ok=ok and video.get('codec_name')==codec and int(video.get('nb_read_frames','0'))>0 and video.get('width',0)>0 and video.get('height',0)>0
  check={'name':name,'passed':ok,'exit_code':r.returncode,'expected_log':expected or 'EOF reached','artifact_bytes':artifact.stat().st_size if artifact and artifact.exists() else None,'video':video}
 except subprocess.TimeoutExpired:check={'name':name,'passed':False,'error':'timeout'}
 checks.append(check);print(json.dumps(check),flush=True)
run('native-mjpeg-decode',[str(fixtures/'Coneic motion.avi'),'--stop-time=2'])
run('native-wav-decode',[str(fixtures/'A quiet moment.wav'),'--stop-time=2'])
for fourcc,label,encoder,mux,suffix in [('h264','h264','x264{tune=zerolatency}' if os.name!='nt' else 'x264','mp4','mp4'),('VP80','vp8','vpx{quality-mode=1}','avformat{mux=webm}','webm'),('VP90','vp9','vpx{quality-mode=1}','avformat{mux=webm}','webm')]:
 dest=a.output/('native-cli-'+label+'.'+suffix)
 run('native-'+label+'-encode',[str(fixtures/'Coneic motion.avi'),'--stop-time='+('8' if label=='vp9' else '2'),'--no-sub-autodetect-file','--sout=#transcode{vcodec='+fourcc+',venc='+encoder+',vb=900}:std{access=file,mux='+mux+',dst='+str(dest)+'}'],'using encoder module "'+('vpx' if label!='h264' else 'x264')+'"',dest,label)
 if dest.exists():run('native-'+label+'-decode',[str(dest),'--codec='+('vpx' if label!='h264' else 'avcodec')],'using video decoder module "'+('vpx' if label!='h264' else 'avcodec')+'"')
result={'passed':all(x['passed'] for x in checks),'checks':checks,'scope':'Actual native Coneic executable: CPU decode and H264/VP8/VP9 encode/decode, clean process exit. Dummy output; devices, DRM, GPU and external network behavior not certified.'+(' Linux H264 short-clip validation uses x264 tune=zerolatency.' if os.name!='nt' else '')}
(a.output/'native-cli-results.json').write_text(json.dumps(result,indent=2),encoding='utf-8');raise SystemExit(0 if result['passed'] else 1)
