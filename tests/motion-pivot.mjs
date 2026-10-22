import fs from 'node:fs/promises';
import path from 'node:path';
import {root,findCompiler,compilerArgs,buildEnvironment,run,runCppTest} from './host-tools.mjs';

// Compile the real View, tile canvas, refresh scheduler and graphics renderer.
// The host shims replace only ESP-IDF clocks/heap, and no compiler is installed.
const compiler = findCompiler();
const work = path.join(root, 'tests/.build/motion-pivot');
await fs.mkdir(work, {recursive:true});
const env=buildEnvironment(work);
for (const standard of ['c++11','c++17']) {
  run(compiler, [...compilerArgs(compiler), '-std='+standard,
    '-I'+path.join(root, 'src'), '-c', path.join(root, 'tests/motion-pivot-initializers.cpp'),
    '-o', path.join(work, 'pivot-initializers-'+standard+'.o')], {env});
}
console.log(`Compiling native motion pivot regression with ${compiler}`);
await runCppTest('motion-pivot',{runtime:true});
