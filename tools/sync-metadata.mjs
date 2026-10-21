import fs from 'node:fs/promises';
import path from 'node:path';
const root=path.resolve(import.meta.dirname,'..'),read=file=>fs.readFile(path.join(root,file),'utf8');
const version=(await read('VERSION.txt')).trim(),abi=Number(/ResourceVersion\s*=\s*(\d+)/.exec(await read('src/UI/Compiled.h'))[1]);
const check=process.argv.includes('--check');
async function sync(file,text){const previous=(await read(file)).replaceAll('\r\n','\n');if(previous===text)return;if(check)throw Error(`Stale generated metadata: ${file}. Run npm run metadata:sync.`);await fs.writeFile(path.join(root,file),text);}
await sync('src/Core/Version.h',`#pragma once\n// Generated from VERSION.txt by tools/sync-metadata.mjs.\n#define NICE_GFX_VERSION "${version}"\n`);
const manifest=JSON.parse(await read('library.json'));manifest.version=version;manifest.description=manifest.description.replace(/ABI \d+/g,`ABI ${abi}`);await sync('library.json',JSON.stringify(manifest,null,2)+'\n');
await sync('library.properties',(await read('library.properties')).replaceAll('\r\n','\n').replace(/^version=.*$/m,`version=${version}`).replace(/ABI \d+/g,`ABI ${abi}`));
const fontCount=(await fs.readdir(path.join(root,'src/Fonts/Mono'))).filter(file=>/^GFX_Font_.*\.cpp$/.test(file)).length;
await sync('README.md',(await read('README.md')).replaceAll('\r\n','\n').replace(/^# NICE_GFX .*$/m,`# NICE_GFX ${version}`).replace(/\d+ 个独立字模编译单元/g,`${fontCount} 个独立字模编译单元`));
console.log(`Library ${version}, resource ABI ${abi}: metadata ${check?'checked':'updated'}.`);
