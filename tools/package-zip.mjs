import {readFile,readdir,mkdir,stat} from 'node:fs/promises';
import {spawnSync} from 'node:child_process';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const config=JSON.parse(await readFile(path.join(root,'release.config.json'),'utf8'));
const input=path.resolve(root,config.unpackedDirectory),output=path.join(root,'.release/packages',config.package.name);
if(config.package.type!=='zip'||output.startsWith(input+path.sep))throw Error('ZIP staging must be separate from the app payload');
await mkdir(path.dirname(output),{recursive:true});
try{await stat(output);throw Error('Package already exists; retain immutable build outputs');}catch(error){if(error.code!=='ENOENT')throw error;}
const names=await readdir(input);
const result=spawnSync('tar.exe',['-a','-cf',output,'-C',input,...names],{windowsHide:true,encoding:'utf8'});
if(result.status)throw Error(result.stderr||'Windows ZIP creation failed');
console.log(`Prepared ${config.package.name}: ${(await stat(output)).size} bytes`);
