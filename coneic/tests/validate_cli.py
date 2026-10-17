import argparse,pathlib,subprocess,json,hashlib
p=argparse.ArgumentParser();p.add_argument('--app',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--fixtures',type=pathlib.Path);a=p.parse_args()
a.app=a.app.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
fixtures=a.fixtures or pathlib.Path(__file__).resolve().parents[2]/'outputs'/'coneic-validation'/'fixtures'
checks=[]
def run(name,arguments,expected=None,artifact=None):
 log=a.output/(name+'.log')
 log.unlink(missing_ok=True)
 if artifact:artifact.unlink(missing_ok=True)
 cmd=[str(a.app/'Coneic.exe'),'--ignore-config','--no-media-library','--no-one-instance','-I','dummy','--play-and-exit','--avcodec-hw=none','--vout=dummy','--aout=dummy','--file-logging','--logfile='+str(log),'--verbose=2']+arguments
 try:
  r=subprocess.run(cmd,cwd=a.app,timeout=65,creationflags=subprocess.CREATE_NO_WINDOW)
  text=log.read_text(encoding='utf-8',errors='replace') if log.exists() else ''
  ok=r.returncode==0 and bool(text) and (expected in text if expected else 'EOF reached' in text)
  if artifact:ok=ok and artifact.exists() and artifact.stat().st_size>1000
  check={'name':name,'passed':ok,'exit_code':r.returncode,'expected_log':expected or 'EOF reached','artifact_bytes':artifact.stat().st_size if artifact and artifact.exists() else None}
 except subprocess.TimeoutExpired:check={'name':name,'passed':False,'error':'timeout'}
 checks.append(check);print(json.dumps(check),flush=True)
run('native-mjpeg-decode',[str(fixtures/'Coneic motion.avi'),'--stop-time=2'])
run('native-wav-decode',[str(fixtures/'A quiet moment.wav'),'--stop-time=2'])
for fourcc,label,encoder,mux,suffix in [('h264','h264','x264','mp4','mp4'),('VP80','vp8','vpx{quality-mode=1}','avformat{mux=webm}','webm'),('VP90','vp9','vpx{quality-mode=1}','avformat{mux=webm}','webm')]:
 dest=a.output/('native-cli-'+label+'.'+suffix)
 run('native-'+label+'-encode',[str(fixtures/'Coneic motion.avi'),'--stop-time='+('8' if label=='vp9' else '2'),'--no-sub-autodetect-file','--sout=#transcode{vcodec='+fourcc+',venc='+encoder+',vb=900}:std{access=file,mux='+mux+',dst='+str(dest)+'}'],'using encoder module "'+('vpx' if label!='h264' else 'x264')+'"',dest)
 if dest.exists():run('native-'+label+'-decode',[str(dest),'--codec='+('vpx' if label!='h264' else 'avcodec')],'using video decoder module "'+('vpx' if label!='h264' else 'avcodec')+'"')
result={'passed':all(x['passed'] for x in checks),'checks':checks,'scope':'Actual native Coneic executable: CPU decode and H264/VP8/VP9 encode/decode, clean process exit. Dummy output; devices, DRM, GPU and external network behavior not certified.'}
(a.output/'native-cli-results.json').write_text(json.dumps(result,indent=2),encoding='utf-8');raise SystemExit(0 if result['passed'] else 1)
