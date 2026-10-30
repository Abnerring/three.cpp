// Development-time source metadata only; production never executes these texts.
import fs from 'node:fs';import path from 'node:path';import{createRequire}from'node:module';import{execFileSync}from'node:child_process';import{fileURLToPath}from'node:url';
const args=process.argv.slice(2),check=args.includes('--check'),upstream=path.resolve(args.find(a=>a!=='--check')??'D:/project/DX/GitHub/three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw Error('Unexpected upstream checkout');
const require=createRequire(path.join(upstream,'package.json')),{parse}=require('acorn'),root=fileURLToPath(new URL('../',import.meta.url));
function read(name){const text=fs.readFileSync(path.join(upstream,'src',name),'utf8');return {text,tree:parse(text,{ecmaVersion:'latest',sourceType:'module'})};}
function emit(rel,lines){const file=path.join(root,rel),text=lines.join('\n')+'\n';if(check){if(fs.readFileSync(file,'utf8').replaceAll('\r\n','\n')!==text)throw Error('Stale '+file);}else{fs.mkdirSync(path.dirname(file),{recursive:true});fs.writeFileSync(file,text);}console.log(`${check?'checked':'generated'} ${rel}`);}
function arity(params){const i=params.findIndex(p=>p.type==='AssignmentPattern'||p.type==='RestElement');return i<0?params.length:i;}
{
 const{text,tree}=read('math/Box3.js'),cls=tree.body.find(n=>n.type==='ClassDeclaration'),sat=tree.body.find(n=>n.type==='FunctionDeclaration'&&n.id.name==='satForAxes');
 emit('src/math/Box3Source.inc',['// Source metadata from pinned three.js; see THREEJS-LICENSE.','namespace Box3Source {',`constexpr const char* classSource=${JSON.stringify(text.slice(cls.start,cls.end))};`,`constexpr const char* satSource=${JSON.stringify(text.slice(sat.start,sat.end))};`,'struct Method { const char* name; unsigned arity; const char* source; };','constexpr Method methods[]={',...cls.body.body.map(n=>`{${JSON.stringify(n.key.name)},${arity(n.value.params)},${JSON.stringify(text.slice(n.start,n.end))}},`),'};','}']);
}
{
 const{text,tree}=read('math/Vector3.js'),cls=tree.body.find(n=>n.type==='ClassDeclaration'),names=['add','addScalar','addVectors','sub','subVectors','min','max','clamp','distanceTo','distanceToSquared','dot','crossVectors'];
 const entries=names.map(name=>cls.body.body.find(n=>n.key?.name===name));if(entries.some(n=>!n))throw Error('Missing Vector3 method');
 emit('src/math/Box3VectorSource.inc',['// Exact source metadata for the added Vector3 methods; see THREEJS-LICENSE.','namespace Box3VectorSource {','struct Method { const char* name; unsigned arity; const char* source; };','constexpr Method methods[]={',...entries.map(n=>`{${JSON.stringify(n.key.name)},${arity(n.value.params)},${JSON.stringify(text.slice(n.start,n.end))}},`),'};','}']);
}
{
 const{text,tree}=read('math/MathUtils.js'),fn=tree.body.find(n=>n.type==='FunctionDeclaration'&&n.id.name==='clamp');
 emit('src/math/ClampSource.inc',['// Source metadata from pinned three.js; see THREEJS-LICENSE.','namespace MathUtilsClampSource {',`constexpr const char* functionSource=${JSON.stringify(text.slice(fn.start,fn.end))};`,'}']);
}
