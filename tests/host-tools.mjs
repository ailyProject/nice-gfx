import {spawnSync} from 'node:child_process';
import fs from 'node:fs/promises';
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

// Keep every host View regression linked to the same implementation units.
// In particular, Image owns out-of-line constructors/destructors independently
// of View and must not disappear from a hand-maintained compiler command.
export const runtimeSources=['UI/View.cpp','UI/Image.cpp','Graphics/Gfx.cpp']
  .map(file=>path.join(root,'src',file));

export async function prepareNativeHost(work) {
  const files={
    'esp_timer.h':'#pragma once\n#include <cstdint>\ninline int64_t esp_timer_get_time(){return 0;}\n',
    'esp_heap_caps.h':'#pragma once\n#include <cstdlib>\n#define MALLOC_CAP_SPIRAM 1\n#define MALLOC_CAP_8BIT 2\ninline size_t heap_caps_get_total_size(unsigned){return 0;}\ninline void *heap_caps_calloc(size_t n,size_t size,unsigned){return std::calloc(n,size);}\n',
    'freertos/FreeRTOS.h':'#pragma once\n#include <cstdint>\nusing TickType_t=uint32_t;\n#define pdMS_TO_TICKS(ms) (ms)\n',
    'freertos/task.h':'#pragma once\n#include "FreeRTOS.h"\ninline void vTaskDelay(TickType_t){}\n',
  };
  for(const [name,text] of Object.entries(files)){
    const file=path.join(work,name);await fs.mkdir(path.dirname(file),{recursive:true});await fs.writeFile(file,text);
  }
}

export async function runCppTest(name,{
  sources=[`tests/${name}.cpp`],runtime=false,native=runtime,fonts=[],
  includes=[],defines=[],flags=[],sanitize=true,
}={}) {
  const work=path.join(root,'tests/.build',name);await fs.mkdir(work,{recursive:true});
  if(native)await prepareNativeHost(work);
  const compiler=findCompiler(),env=buildEnvironment(work);
  const exe=path.join(work,process.platform==='win32'?'test.exe':'test');
  const units=[...sources.map(file=>path.resolve(root,file)),...(runtime?runtimeSources:[]),
    ...fonts.map(font=>path.join(root,'src/Fonts/Mono',`GFX_Font_${font}.cpp`))];
  run(compiler,[...compilerArgs(compiler),'-std=c++17','-O1','-g','-Wall','-Wextra',
    '-Wno-misleading-indentation','-Wno-missing-field-initializers',
    ...(sanitize?['-fsanitize=undefined','-fno-sanitize-recover=all']:[]),
    ...(native?['-DNICE_GFX_NATIVE_IDF=1','-I'+work]:[]),
    ...defines.map(value=>'-D'+value),'-I'+path.join(root,'src'),
    ...includes.map(file=>'-I'+path.resolve(root,file)),...flags,'-UNDEBUG',...new Set(units),'-o',exe],{env});
  run(exe,[],{env});
}
