import fs from 'node:fs/promises';
import path from 'node:path';

const root=path.resolve(import.meta.dirname,'..'),examples=path.join(root,'examples');
const categories=['01_Basics','02_Graphics','03_Text','04_Images','05_Sprites',
  '06_Touch','07_UI','08_Applications','09_Diagnostics','10_Performance'];
const errors=[],sketches=[];
const exists=async p=>fs.access(p).then(()=>true,()=>false);
async function walk(dir){
  if(/^(?:Boards|\d+\s*[x×]\s*\d+)$/i.test(path.basename(dir)))errors.push(`${path.relative(root,dir)}: group by function; keep board and dimensions in hardware configuration`);
  const entries=await fs.readdir(dir,{withFileTypes:true});
  const inos=entries.filter(e=>e.isFile()&&e.name.endsWith('.ino'));
  const main=path.basename(dir)+'.ino';
  if(inos.length){
    if(!inos.some(e=>e.name===main))errors.push(`${path.relative(root,dir)}: sketch must contain ${main}`);
    else sketches.push(path.relative(examples,path.join(dir,main)).split(path.sep).join('/'));
  }
  for(const entry of entries){
    const file=path.join(dir,entry.name);
    if(entry.isDirectory()){
      if(!['data','Data'].includes(entry.name))await walk(file);
    }else if(/\.(h|cpp|ino)$/.test(entry.name)){
      const source=await fs.readFile(file,'utf8');
      for(const match of source.matchAll(/^\s*#include\s+"([^"\n]+)"/gm)){
        const header=match[1];
        // Bare external library headers remain Arduino's responsibility.
        if(header.startsWith('.')||['Hardware.h','ExampleDrawing.h'].includes(header)){
          if(!await exists(path.resolve(dir,header)))errors.push(`${path.relative(root,file)}: missing include ${header}`);
        }
      }
    }
  }
}
for(const entry of await fs.readdir(examples,{withFileTypes:true}))if(entry.isDirectory()){
  if(entry.name==='_shared')continue;
  if(!categories.includes(entry.name))errors.push(`examples/${entry.name}: use a functional example category`);
  await walk(path.join(examples,entry.name));
}
for(const category of categories)if(!sketches.some(s=>s.startsWith(category+'/')))errors.push(`Missing examples in ${category}`);
sketches.sort();
const catalog='# 示例程序索引\n\n按学习主题查找示例。接线、依赖和布局限制见 [使用说明](README.md)。\n'+
  '\n此文件由 `node tools/check-examples.mjs --write` 生成。\n'+
  categories.map(category=>'\n## '+category+'\n\n'+sketches.filter(s=>s.startsWith(category+'/')).map(s=>`- [${path.posix.dirname(s).slice(category.length+1)}](${s})`).join('\n')+'\n').join('');
const file=path.join(examples,'CATALOG.md');
if(process.argv.includes('--write'))await fs.writeFile(file,catalog);
else if(!await exists(file)||(await fs.readFile(file,'utf8')).replaceAll('\r\n','\n')!==catalog)errors.push('Stale examples/CATALOG.md. Run node tools/check-examples.mjs --write.');
if(errors.length)throw Error(errors.join('\n'));
console.log(`PASS ${sketches.length} function-grouped Arduino sketches, matching sketch names, local includes and catalog.`);
