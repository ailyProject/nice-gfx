import fs from 'node:fs/promises';
import path from 'node:path';

const root=path.resolve(import.meta.dirname,'..');
const templates=new Map(await Promise.all(['Hardware.h','ExampleDrawing.h'].map(async name=>
  [name,await fs.readFile(path.join(root,'examples/_shared',name),'utf8')])));
const check=process.argv.includes('--check');
const normalize=text=>text.replaceAll('\r\n','\n');
let count=0;
async function walk(dir){
  for(const item of await fs.readdir(dir,{withFileTypes:true})){
    const file=path.join(dir,item.name);
    if(item.isDirectory()){
      if(item.name!=='_shared')await walk(file);
      continue;
    }
    if(!templates.has(item.name))continue;
    const text=await fs.readFile(file,'utf8'),template=templates.get(item.name);
    if(!text.includes(`Generated from examples/_shared/${item.name}.`))continue;
    if(normalize(text)!==normalize(template)){
      if(check)throw Error(`Example header differs from its template: ${path.relative(root,file)}. Edit examples/_shared/${item.name} and run npm run examples:sync.`);
      await fs.writeFile(file,template);
    }
    count++;
  }
}
await walk(path.join(root,'examples'));
console.log(`${check?'Checked':'Synchronized'} ${count} generated sketch headers.`);
