// Development metadata only. These source strings are never executed.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
const args=process.argv.slice(2), check=args.includes('--check'), upstream=path.resolve(args.find(a=>a!=='--check')??'../three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw Error('Unexpected upstream');
const require=createRequire(path.join(upstream,'package.json')), acorn=require('acorn');
const lines=['// Source metadata from pinned three.js; see THREEJS-LICENSE.'];
for(const file of ['accessors/Tangent','accessors/Bitangent','accessors/AccessorsUtils','utils/Packing','display/NormalMapNode','display/BumpMapNode']){
 const name=path.basename(file),text=fs.readFileSync(path.join(upstream,'src/nodes',file+'.js'),'utf8'),ast=acorn.parse(text,{ecmaVersion:'latest',sourceType:'module'}),nodes=[];
 const walk=(n,parent)=>{if(!n||typeof n!=='object')return;if(n.type)nodes.push({n,parent});for(const [k,v]of Object.entries(n))if(k!=='parent'){if(Array.isArray(v))v.forEach(x=>walk(x,n));else if(v&&typeof v==='object')walk(v,n);}};walk(ast);
 const emitted=new Set(),emit=(key,source)=>{if(emitted.has(key))throw Error('Duplicate '+key);emitted.add(key);lines.push(`constexpr const char* ${key}=${JSON.stringify(source)};`);};
 lines.push(`namespace ${name}Source {`);
 for(const {n}of nodes){if(n.type==='ClassDeclaration')emit('classSource',text.slice(n.start,n.end));if(n.type==='MethodDefinition'&&n.kind!=='constructor')emit(n.key.name+'Source',text.slice(n.start,n.end).replace(/^static\s+/,''));if(n.type==='VariableDeclarator'&&n.init?.type==='ArrowFunctionExpression')emit(n.id.name+'Source',text.slice(n.init.start,n.init.end));}
 for(const {n}of nodes.filter(x=>x.n.type==='ArrowFunctionExpression')){
  const source=text.slice(n.start,n.end);
  let key;
  if(name==='Tangent')key='tangentViewCallbackSource';
  else if(name==='Bitangent')key=source.includes('crossNormalTangent,')?'getBitangentCallbackSource':'bitangentViewCallbackSource';
  else if(name==='AccessorsUtils'&&n.params.length===0)key='bentNormalViewCallbackSource';
  else if(name==='BumpMapNode'){
   if(n.params[0]?.type==='ObjectPattern')key='dHdxyCallbackSource';
   else if(n.params[0]?.name==='inputs')key='perturbNormalCallbackSource';
   else if(n.params[0]?.name==='texNode')key='getUVSource';
   else if(n.params[0]?.name==='uvNode')key=n.body.type==='Identifier'?'uvIdentitySource':source.includes('dFdx')?'uvDxSource':'uvDySource';
  }
  if(key&&!emitted.has(key))emit(key,source);
 }
 lines.push('}');
}
const output=new URL('../src/nodes/accessors/TangentNormalBumpSources.inc',import.meta.url),result=lines.join('\n')+'\n';
if(check){if(fs.readFileSync(output,'utf8').replaceAll('\r\n','\n')!==result)throw Error('Stale tangent/normal/bump sources');}else fs.writeFileSync(output,result);
console.log('Tangent/normal/bump source metadata verified');
