import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';

const upstream=path.resolve(process.argv[2]);
const binary=process.argv[3]&&process.argv[3]!=='--oracle'?path.resolve(process.argv[3]):null;
const directory=process.argv[4]&&path.resolve(process.argv[4]);
const relative=process.argv.includes('--relative-node-ids');
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=name=>import(pathToFileURL(path.join(upstream,'src',name+'.js')));
// Keep the actual NodeBuilder import graph, including its pending absolute IDs.
await load('nodes/core/NodeBuilder');
const [{default:Node},{default:StackNode},tsl,{default:threshold}]=await Promise.all([
 load('nodes/core/Node'),load('nodes/core/StackNode'),load('nodes/tsl/TSLCore'),load('nodes/functions/material/getAlphaHashThreshold')
]);

function canonical(root,baseline){
 const seen=new Map(),objects=[];
 function encode(v){
  if(v===undefined)return ['undefined'];if(typeof v==='function')return ['function'];if(typeof v==='symbol')return ['symbol'];
  if(typeof v==='number'){if(Number.isNaN(v))return ['number','NaN'];if(!Number.isFinite(v))return ['number',v>0?'Infinity':'-Infinity'];if(Object.is(v,-0))return ['number','-0'];}
  if(v===null||typeof v!=='object')return v;
  if(seen.has(v))return ['ref',seen.get(v)];
  const index=objects.length;seen.set(v,index);const record={kind:Array.isArray(v)?'array':'object',properties:[]};objects.push(record);
  if(Array.isArray(v))record.length=v.length;
  if(v.isNode===true)record.nodeType=v.type;
  for(const key of Object.keys(v)){
   let value;
   // Relative mode changes only the explicitly identified Node.id domain.
   // Newly allocated IDs retain allocation order; existing nodes retain graph identity.
   if(relative&&key==='id'&&v.isNode===true)value=v.id>baseline?['localId',v.id-baseline]:['existingNode',index];
   else value=encode(v[key]);
   record.properties.push([key,value]);
  }
  return ['ref',index];
 }
 return {root:encode(root),objects};
}

const privateShaders={};
function expand(position,stack){
 const result=threshold.shaderNode.jsFunc([position]),seen=new Set(),queue=[],calls=[];
 function walk(v){
  if(v===null||typeof v!=='object'||seen.has(v))return;seen.add(v);
  if(v.isShaderCallNodeInternal===true)queue.push(v);
  for(const key of Object.keys(v))walk(v[key]);
 }
 walk(result);walk(stack.nodes);
 for(let i=0;i<queue.length;++i){
  assert.ok(i<16,'Unexpected recursive private shader graph');
  const call=queue[i],shader=call.shaderNode,type=shader.layout?.inputs?.[0]?.type;
  assert.ok(type==='vec2'||type==='vec3','Unexpected private shader input type');
  const name=type==='vec2'?'hash2D':'hash3D';privateShaders[name]=shader;
  const output=shader.jsFunc(call.rawInputs);
  calls.push({name,shader,inputs:call.rawInputs,output});walk(output);walk(stack.nodes);
 }
 assert.equal(calls.filter(c=>c.name==='hash3D').length,2);
 assert.equal(calls.filter(c=>c.name==='hash2D').length,4);
 return {position,result,calls,stack:stack.nodes};
}

function fixture(s,type){
 const value=new Node(type);
 for(const key of ['x','y','z','xy'])Object.defineProperty(value,key,{configurable:true,get(){
  s.events.push(['get','value.'+key]);if(s.r.throwAt==='value.'+key)throw new TypeError('fixture');
  return key==='xy'?tsl.vec2(.25,-.5):tsl.float(key==='x'?.25:key==='y'?-.5:2);
 }});
 return value;
}
function inputs(s,value){
 if(s.r.iterator==='array')return [value];
 const mode=s.r.iterator;
 const iterator={get next(){s.events.push(['get','iterator.next']);return function(){
  s.events.push(['next']);if(mode==='nextThrow')throw new TypeError('next');
  return {get done(){s.events.push(['get','result.done']);if(mode==='doneThrow')throw new TypeError('done');return mode==='empty';},get value(){s.events.push(['get','result.value']);if(mode==='valueThrow')throw new TypeError('value');return value;}};
 };},get return(){
  s.events.push(['get','iterator.return']);if(mode==='returnGetterThrow')throw new TypeError('return getter');if(mode==='returnMissing')return undefined;
  return function(){s.events.push(['return']);if(mode==='returnThrow')throw new TypeError('return');return mode==='returnPrimitive'?1:{};};
 }};
 return {[Symbol.iterator]:function(){s.events.push(['iterator']);if(mode==='iteratorCallThrow')throw new TypeError('iterator');return iterator;}};
}

function evaluate(r){
 const previous=tsl.getCurrentStack(),stack=new StackNode(),s={r,events:[]};tsl.setCurrentStack(stack);
 const before=new Node();let out;
 try{
  if(r.kind==='expand')out=expand(tsl.vec3(...r.values),stack);
  else{
   const shader=privateShaders[r.name];assert.ok(shader,'Private body was not discovered from the actual graph');
   const value=r.fixture?fixture(s,r.name==='hash2D'?'vec2':'vec3'):tsl[r.name==='hash2D'?'vec2':'vec3'](...r.values);
   const result=shader.jsFunc(inputs(s,value));out={shader,value,result,stack:stack.nodes};
  }
 }catch(e){out={error:'#'+e.name,stack:stack.nodes};}
 finally{tsl.setCurrentStack(previous);}
 const after=new Node();out.events=s.events;out.allocations=after.id-before.id-1;out.stackRestored=tsl.getCurrentStack()===previous;
 return canonical(out,before.id);
}

const cases=[];
for(const values of [[0],[1],[.1,.2,.3],[-1,0,2]])cases.push({kind:'expand',values});
for(const name of ['hash2D','hash3D']){
 for(const values of [[0],[1],[.1,.2,.3],[-1,0,2]])cases.push({kind:'body',name,values,iterator:'array'});
 for(const iterator of ['array','custom','empty','nextThrow','doneThrow','valueThrow','returnGetterThrow','returnThrow','returnPrimitive','returnMissing','iteratorCallThrow'])cases.push({kind:'body',name,fixture:true,iterator});
 for(const throwAt of ['value.x','value.y','value.z','value.xy'])cases.push({kind:'body',name,fixture:true,iterator:'custom',throwAt});
}
if(relative)for(const r of cases)r.relativeNodeIds=true;
const expected=cases.map(evaluate);
if(directory){fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'input.json'),JSON.stringify(cases));fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2));}
if(!binary){console.log(`Prepared ${cases.length} real private alpha-hash body scenarios (${relative?'relative Node.id':'absolute Node.id'})`);process.exit(0);}
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:256*1024*1024}));
if(directory)fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));
assert.equal(actual.length,expected.length);const differences=[];
for(let i=0;i<cases.length;++i)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({index:i,input:cases[i],expected:expected[i],actual:actual[i]});}
if(directory)fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(differences,null,2));
assert.equal(differences.length,0,JSON.stringify(differences.slice(0,1),null,2));
console.log(`Private alpha-hash bodies: ${cases.length} pinned-upstream scenarios passed (${relative?'relative Node.id':'absolute Node.id'})`);
