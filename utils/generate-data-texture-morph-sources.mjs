// Development-time source metadata only; production never executes these texts.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
const args=process.argv.slice(2),check=args.includes('--check');
const upstream=path.resolve(args.find(x=>x!=='--check')??'../three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw new Error('Unexpected upstream checkout');
const require=createRequire(path.join(upstream,'package.json')),acorn=require('acorn');
function emit(file,lines){const text=lines.join('\n')+'\n';if(check){if(fs.readFileSync(file,'utf8').replaceAll('\r\n','\n')!==text)throw new Error('Stale '+file);}else fs.writeFileSync(file,text);}
for(const name of ['DataTexture','DataArrayTexture']){
 const text=fs.readFileSync(path.join(upstream,`src/textures/${name}.js`),'utf8');
 const tree=acorn.parse(text,{ecmaVersion:'latest',sourceType:'module'}),cls=tree.body.find(n=>n.type==='ClassDeclaration');
 const lines=['// Source metadata from pinned three.js; see THREEJS-LICENSE.',`namespace ${name}Source {`,`constexpr const char* classSource=${JSON.stringify(text.slice(cls.start,cls.end))};`];
 for(const m of cls.body.body.filter(n=>n.kind!=='constructor'))lines.push(`constexpr const char* ${m.key.name}Source=${JSON.stringify(text.slice(m.start,m.end))};`);
 lines.push('}');emit(new URL(`../src/textures/${name}Source.inc`,import.meta.url),lines);
}
const text=fs.readFileSync(path.join(upstream,'src/nodes/accessors/Morph.js'),'utf8'),tree=acorn.parse(text,{ecmaVersion:'latest',sourceType:'module'}),functions=[];
function walk(n){if(!n||typeof n!=='object')return;if(['ArrowFunctionExpression','FunctionDeclaration'].includes(n.type))functions.push(n);for(const [k,v]of Object.entries(n)){if(k==='start'||k==='end')continue;if(Array.isArray(v))v.forEach(walk);else if(v&&typeof v==='object')walk(v);}}
walk(tree);functions.sort((a,b)=>a.start-b.start);
const names=['getMorphSource','getEntrySource','disposeTextureSource','morphReferenceSource','loopSource','ifSource','updateSource','reduceSource'];
if(functions.length!==names.length)throw new Error('Morph source function count');
emit(new URL('../src/nodes/accessors/MorphSources.inc',import.meta.url),['// Source metadata from pinned three.js; see THREEJS-LICENSE.','namespace MorphSource {',...functions.map((n,i)=>`constexpr const char* ${names[i]}=${JSON.stringify(text.slice(n.start,n.end))};`),'}']);
console.log('DataTexture/DataArrayTexture/Morph source metadata verified');
