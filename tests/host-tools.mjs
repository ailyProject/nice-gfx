import {spawnSync} from 'node:child_process';
import path from 'node:path';
export const root=path.resolve(import.meta.dirname,'..');
export const isZig=compiler=>/(?:^|[\\/])zig(?:\.exe)?$/i.test(compiler);
export function findCompiler() {
  const explicit=process.argv.find(a=>a.startsWith('--compiler='))?.slice(11)||process.env.CXX;
  const compiler=(explicit?[explicit]:['c++','clang++','g++','zig']).find(file=>
    spawnSync(file,[isZig(file)?'version':'--version'],{windowsHide:true,encoding:'utf8'}).status===0);
  if(!compiler)throw Error('No host C++ compiler found. Set CXX to a compiler executable or add it to PATH.');
  return compiler;
}
export const compilerArgs=compiler=>isZig(compiler)?['c++']:[];
export const buildEnvironment=work=>({...process.env,ZIG_GLOBAL_CACHE_DIR:path.join(work,'cache'),ZIG_LOCAL_CACHE_DIR:path.join(work,'local-cache')});
export function run(command,args,options={}) {
  const result=spawnSync(command,args,{cwd:root,windowsHide:true,stdio:'inherit',timeout:180000,...options});
  if(result.error)throw result.error;
  if(result.status!==0)throw Error(`${command} exited with ${result.status}\n${result.stdout||''}${result.stderr||''}`);
}
