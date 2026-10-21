import fs from 'node:fs/promises';
import path from 'node:path';
import {spawnSync} from 'node:child_process';
import {root,findCompiler,isZig} from './host-tools.mjs';

// Compile the real View, tile canvas, refresh scheduler and graphics renderer.
// The host shims replace only ESP-IDF clocks/heap, and no compiler is installed.
const compiler = findCompiler();
const work = path.join(root, 'tests/.build/motion-pivot');
await fs.mkdir(path.join(work, 'freertos'), {recursive:true});
await fs.writeFile(path.join(work, 'esp_timer.h'), '#pragma once\n#include <cstdint>\ninline int64_t esp_timer_get_time(){return 0;}\n');
await fs.writeFile(path.join(work, 'esp_heap_caps.h'), '#pragma once\n#include <cstdlib>\n#define MALLOC_CAP_SPIRAM 1\n#define MALLOC_CAP_8BIT 2\ninline size_t heap_caps_get_total_size(unsigned){return 0;}\ninline void *heap_caps_calloc(size_t n,size_t size,unsigned){return std::calloc(n,size);}\n');
await fs.writeFile(path.join(work, 'freertos/FreeRTOS.h'), '#pragma once\n#include <cstdint>\nusing TickType_t=uint32_t;\n#define pdMS_TO_TICKS(ms) (ms)\n');
await fs.writeFile(path.join(work, 'freertos/task.h'), '#pragma once\n#include "FreeRTOS.h"\ninline void vTaskDelay(TickType_t){}\n');
const exe = path.join(work, process.platform === 'win32' ? 'motion-pivot.exe' : 'motion-pivot');
const args = [
  ...(isZig(compiler) ? ['c++'] : []), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra',
  ...(process.argv.includes('--sanitize') ? ['-fsanitize=undefined', '-fno-sanitize-recover=all'] : []),
  '-Wno-misleading-indentation', '-Wno-missing-field-initializers', '-DNICE_GFX_NATIVE_IDF=1',
  '-I'+work, '-I'+path.join(root, 'src'),
  path.join(root, 'tests/motion-pivot.cpp'),
  path.join(root, 'src/UI/View.cpp'),
  path.join(root, 'src/UI/Image.cpp'),
  path.join(root, 'src/Graphics/Gfx.cpp'), '-o', exe,
];
const env = {...process.env, ZIG_GLOBAL_CACHE_DIR:path.join(work, '../zig-global-cache'),
  ZIG_LOCAL_CACHE_DIR:path.join(work, '../zig-local-cache')};
for (const standard of ['c++11','c++17']) {
  const check = spawnSync(compiler, [...(isZig(compiler) ? ['c++'] : []), '-std='+standard,
    '-I'+path.join(root, 'src'), '-c', path.join(root, 'tests/motion-pivot-initializers.cpp'),
    '-o', path.join(work, 'pivot-initializers-'+standard+'.o')], {cwd:root, env, stdio:'inherit'});
  if (check.error) throw check.error;
  if (check.status !== 0) process.exit(check.status || 1);
}
console.log(`Compiling native motion pivot regression with ${compiler}`);
const build = spawnSync(compiler, args, {cwd:root, env, stdio:'inherit'});
if (build.error) throw build.error;
if (build.status !== 0) process.exit(build.status || 1);
const run = spawnSync(exe, [], {cwd:root, env, stdio:'inherit'});
if (run.error) throw run.error;
process.exit(run.status === 0 ? 0 : (run.status ?? 1));
