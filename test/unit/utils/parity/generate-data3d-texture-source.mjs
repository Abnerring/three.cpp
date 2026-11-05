// Metadata only: executable constructor/copy logic lives in Data3DTexture.cpp.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {execFileSync} from 'node:child_process';
if(!process.argv[2])throw new Error('Usage: generate-data3d-texture-source.mjs pinned-threejs [output]');
const root=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(fileURLToPath(new URL('../../../../upstream.json',import.meta.url)),'utf8'));
if(execFileSync('git',['-C',root,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!==lock.commit)
    throw new Error('Unexpected upstream commit');
const output=path.resolve(process.argv[3]??'src/textures/Data3DTextureSource.inc');
const source=fs.readFileSync(path.join(root,'src/textures/Data3DTexture.js'),'utf8');
const start=source.indexOf('class Data3DTexture extends Texture {');
const end=source.lastIndexOf('\n}');
const copyStart=source.indexOf('copy( source ) {',start);
const copyEnd=source.indexOf('\n\t}',copyStart)+3;
if(start<0||end<start||copyStart<0||copyEnd<copyStart)throw new Error('Pinned Data3DTexture source shape changed');
fs.writeFileSync(output,'// Source metadata from pinned three.js; see THREEJS-LICENSE.\nnamespace Data3DTextureSource {\n'+
    'constexpr const char* classSource='+JSON.stringify(source.slice(start,end+2))+';\n'+
    'constexpr const char* copySource='+JSON.stringify(source.slice(copyStart,copyEnd))+';\n}\n');
