// Development-time source metadata only; production never executes these texts.
import fs from 'node:fs';import path from 'node:path';import{createRequire}from'node:module';import{execFileSync}from'node:child_process';import{fileURLToPath}from'node:url';
const args=process.argv.slice(2),check=args.includes('--check'),upstream=path.resolve(args.find(a=>a!=='--check')??'D:/project/DX/GitHub/three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw Error('Unexpected upstream checkout');
const require=createRequire(path.join(upstream,'package.json')),{parse}=require('acorn'),root=fileURLToPath(new URL('../',import.meta.url));
function read(name){const text=fs.readFileSync(path.join(upstream,'src',name),'utf8');return {text,tree:parse(text,{ecmaVersion:'latest',sourceType:'module'})};}
function emit(rel,lines){const file=path.join(root,rel),text=lines.join('\n')+'\n';if(check){if(fs.readFileSync(file,'utf8').replaceAll('\r\n','\n')!==text)throw Error('Stale '+file);}else{fs.mkdirSync(path.dirname(file),{recursive:true});fs.writeFileSync(file,text);}console.log(`${check?'checked':'generated'} ${rel}`);}
function arity(params){const i=params.findIndex(p=>p.type==='AssignmentPattern'||p.type==='RestElement');return i<0?params.length:i;}
{
 const{text,tree}=read('math/Sphere.js'),cls=tree.body.find(n=>n.type==='ClassDeclaration');
 emit('src/math/SphereSource.inc',['// Exact source metadata from pinned three.js; see THREEJS-LICENSE.','namespace SphereSource {',`constexpr const char* classSource=${JSON.stringify(text.slice(cls.start,cls.end))};`,'struct Method { const char* name; unsigned arity; const char* source; };','constexpr Method methods[]={',...cls.body.body.map(n=>`{${JSON.stringify(n.key.name)},${arity(n.value.params)},${JSON.stringify(text.slice(n.start,n.end))}},`),'};','}']);
}
{
 const entries=[];for(const[count,names]of[[3,['lengthSq','addScaledVector','setLength','cross','negate']],[2,['distanceTo','distanceToSquared']]]){const{text,tree}=read(`math/Vector${count}.js`),cls=tree.body.find(n=>n.type==='ClassDeclaration');for(const name of names){const n=cls.body.body.find(n=>n.key?.name===name);if(!n)throw Error('Missing Vector method '+name);entries.push(`{${count},${JSON.stringify(name)},${arity(n.value.params)},${JSON.stringify(text.slice(n.start,n.end))}},`);}}
 emit('src/math/SphereVectorSource.inc',['// Exact metadata for Sphere/Frustum/Plane Vector dependencies; see THREEJS-LICENSE.','namespace SphereVectorSource {','struct Method { unsigned dimensions; const char* name; unsigned arity; const char* source; };','constexpr Method methods[]={',...entries,'};','}']);
}
