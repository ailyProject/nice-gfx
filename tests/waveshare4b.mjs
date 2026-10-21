import fs from 'node:fs/promises';
import path from 'node:path';
import {root,findCompiler,compilerArgs,buildEnvironment,run} from './host-tools.mjs';
const work=path.join(root,'tests/.build/waveshare4b');
await fs.mkdir(work,{recursive:true});
const compiler=findCompiler(),exe=path.join(work,process.platform==='win32'?'test.exe':'test'),env=buildEnvironment(work);
run(compiler,[...compilerArgs(compiler),'-std=c++17','-O2','-fsanitize=undefined','-I'+path.join(root,'src'),path.join(root,'tests/waveshare4b.cpp'),'-o',exe],{env});
run(exe,[],{env});
