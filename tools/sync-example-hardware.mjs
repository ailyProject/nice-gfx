import fs from 'node:fs/promises';
import path from 'node:path';
const root=path.resolve(import.meta.dirname,'..'),template=await fs.readFile(path.join(root,'examples/_shared/Hardware.h'),'utf8');
const check=process.argv.includes('--check');let count=0;
async function walk(dir){for(const item of await fs.readdir(dir,{withFileTypes:true})){const file=path.join(dir,item.name);if(item.isDirectory()){if(item.name!=='_shared')await walk(file);}else if(item.name==='Hardware.h'){const text=await fs.readFile(file,'utf8');if(!text.includes('Generated from examples/_shared/Hardware.h.'))continue;if(check&&text.replaceAll('\r\n','\n')!==template.replaceAll('\r\n','\n'))throw Error(`Example hardware differs from its template: ${path.relative(root,file)}. Edit examples/_shared/Hardware.h and run npm run examples:sync.`);if(!check)await fs.writeFile(file,template);count++;}}}
await walk(path.join(root,'examples'));console.log(`${check?'Checked':'Updated'} ${count} generated sketch hardware headers.`);
