import fs from 'node:fs/promises';
import path from 'node:path';
import {root,findCompiler,compilerArgs,buildEnvironment,run} from './host-tools.mjs';
const work=path.join(root,'tests/.build/image');await fs.mkdir(work,{recursive:true});
const compiler=findCompiler(),env=buildEnvironment(work),exe=path.join(work,process.platform==='win32'?'test.exe':'test');
run(compiler,[...compilerArgs(compiler),'-std=c++17','-fsanitize=undefined','-fno-sanitize-recover=all','-I'+path.join(root,'src'),path.join(root,'tests/image.cpp'),path.join(root,'src/UI/Image.cpp'),'-o',exe],{env});
run(exe,[],{env});console.log('PASS standalone image copy, stride, byte order, failure preservation and shared ownership.');
