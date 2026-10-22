import fs from 'node:fs/promises';
import path from 'node:path';
import {spawnSync} from 'node:child_process';
const root=path.resolve(import.meta.dirname,'..');
for(const script of ['tools/sync-metadata.mjs','tools/sync-example-hardware.mjs','tools/check-examples.mjs']){const r=spawnSync(process.execPath,[path.join(root,script),'--check'],{stdio:'inherit',windowsHide:true});if(r.status!==0)process.exit(r.status||1);}
async function walk(dir){const result=[];for(const e of await fs.readdir(dir,{withFileTypes:true})){const p=path.join(dir,e.name);if(e.isDirectory())result.push(...await walk(p));else result.push(p);}return result;}
const exists=async p=>{try{await fs.access(p);return true;}catch{return false;}};
const errors=[],src=path.join(root,'src');
const brandedName=/m5stack|seeed|waveshare|adafruit|lilygo|sparkfun|osptek|wioterminal|(?:^|_)rpi(?:_|$)/i;
for(const file of await walk(src)){
  if(!/\.(h|cpp)$/.test(file))continue;
  const text=await fs.readFile(file,'utf8');
  if(/^(?:Drivers[\\/]|Display[\\/]Presets[\\/])/.test(path.relative(src,file))){
    const names=[path.basename(file,path.extname(file))];
    // Board-facing classes/aliases may name a product; chip programs stay generic.
    const boardEntry=/^Display\/Presets\/(?:BoardPresets|DisplayAliases)\.h$/.test(path.relative(src,file).replaceAll('\\','/'));
    // SDK board macros and source/license credits keep their original spelling.
    const code=text.replace(/\/\*[\s\S]*?\*\/|\/\/[^\n]*/g,'');
    for(const match of code.matchAll(/\b(?:class|struct|using)\s+([A-Za-z_]\w*)|\b((?:init|rotate|profile)_\w+)\s*\(/g))if(!boardEntry||!match[1])names.push(match[1]||match[2]);
    for(const name of names)if(brandedName.test(name))errors.push(`${path.relative(root,file)}: ${name} must use controller, interface, dimensions or scan direction naming`);
  }
  for(const m of text.matchAll(/^\s*#include\s+"(\.[^"\n]+)"/gm))if(!await exists(path.resolve(path.dirname(file),m[1])))errors.push(`${path.relative(root,file)}: missing include ${m[1]}`);
}
const docs=[path.join(root,'README.md'),...await walk(path.join(root,'docs')),...await walk(path.join(root,'examples')),...(await walk(src)).filter(f=>f.endsWith('.md')),...await walk(path.join(root,'LICENSES'))].filter(f=>f.endsWith('.md'));
for(const file of docs){const text=await fs.readFile(file,'utf8');for(const m of text.matchAll(/\]\((?:<([^>]+)>|([^\s)]+))\)/g)){let target=m[1]||m[2];if(/^(?:[a-z]+:|#)/i.test(target))continue;target=decodeURIComponent(target.split('#')[0]);if(!target)continue;const p=path.resolve(path.dirname(file),target);if(!p.startsWith(root+path.sep))continue;if(!await exists(p))errors.push(`${path.relative(root,file)}: missing link ${target}`);}}
for(const file of ['TouchTypes.h','TouchController.h','TouchBus.h'])if((await fs.readFile(path.join(src,'Drivers/Touch',file),'utf8')).includes('Display/'))errors.push(`${file}: touch contract depends on display implementation`);
if(await exists(path.join(src,'Fonts/previews')))errors.push('Preview assets must stay outside src/Fonts');
if((await fs.readFile(path.join(src,'UI/View.cpp'),'utf8')).match(/heap_caps_|#include <driver\//))errors.push('View.cpp embeds platform allocation or driver calls');
if((await fs.readFile(path.join(src,'Platform/Common/BufferedDmaBus.h'),'utf8')).includes('../Arduino/'))errors.push('Common DMA template depends on Arduino');
if(errors.length)throw Error(errors.join('\n'));
console.log('PASS library metadata, generated example configuration, relative includes, documentation links and layer boundaries.');
