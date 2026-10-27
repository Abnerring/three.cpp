import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';

const upstream=path.resolve(process.argv[2]),binary=process.argv[3]!=='--oracle'&&process.argv[3]?path.resolve(process.argv[3]):null,directory=process.argv[4];
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=name=>import(pathToFileURL(path.join(upstream,'src/'+name+'.js')));
// Load real dependencies, including the unported part of the upstream graph.
const {default:NodeBuilder}=await load('nodes/core/NodeBuilder');
const {default:Node}=await load('nodes/core/Node');
const {default:StackNode}=await load('nodes/core/StackNode');
const tsl=await load('nodes/tsl/TSLCore');
const colors=await load('nodes/display/ColorSpaceFunctions');
const alpha=await load('nodes/display/PremultiplyAlphaFunctions');
const {default:threshold}=await load('nodes/functions/material/getAlphaHashThreshold');
const functions={...colors,...alpha,getAlphaHashThreshold:threshold};
const structuralNodeIds=process.argv.includes('--structural-node-ids');
function normal(v,seen=[],ids=new Map()){
 if(v===undefined)return '#undefined';if(typeof v==='function')return '#function';if(typeof v==='symbol')return '#symbol';
 if(typeof v==='number'){if(Number.isNaN(v))return '#NaN';if(!Number.isFinite(v))return v>0?'#Infinity':'#-Infinity';if(Object.is(v,-0))return '#-0';}
 if(v===null||typeof v!=='object')return v;if(v.fixtureTag)return '@'+v.fixtureTag;const prior=seen.indexOf(v);if(prior!==-1)return ['#ref',prior];
 seen.push(v);if(Array.isArray(v))return Array.from(v,x=>normal(x,seen,ids));
 return Object.fromEntries(Object.keys(v).map(k=>{const value=v[k];if(k==='id'&&v.isNode===true&&typeof value==='number'&&structuralNodeIds){if(!ids.has(value))ids.set(value,ids.size);return [k,['#nodeId',ids.get(value)]];}return [k,normal(value,seen,ids)];}));
}
function fixture(s,tag){
 const n=new Node('vec4');n.fixtureTag=tag;
 for(const key of ['x','y','z','a','rgb','xyz','xy'])Object.defineProperty(n,key,{configurable:true,get(){s.events.push(['get',tag+'.'+key]);if(s.r.throwAt===tag+'.'+key)throw new TypeError('fixture');return fixture(s,tag+'.'+key);}});
 for(const key of ['mul','add','sub','pow','lessThanEqual','equal','select','div'])Object.defineProperty(n,key,{configurable:true,get(){s.events.push(['get',tag+'.'+key]);if(s.r.throwAt===tag+'.'+key)throw new TypeError('fixture');return function(...args){s.events.push(['call',tag+'.'+key,...args.map(v=>normal(v))]);if(s.r.throwAt===tag+'.'+key+'()')throw new TypeError('fixture');return fixture(s,tag+'.'+key+'()');};}});
 return n;
}
function inputs(s,value){
 if(s.r.iterator==='array')return [value];
 const state={index:0};
 const iterator={get next(){s.events.push(['get','iterator.next']);return function(){s.events.push(['next',++state.index]);if(s.r.iterator==='nextThrow')throw new TypeError('next');return {get done(){s.events.push(['get','result.done']);return s.r.iterator==='empty';},get value(){s.events.push(['get','result.value']);if(s.r.iterator==='valueThrow')throw new TypeError('value');return value;}};};},get return(){s.events.push(['get','iterator.return']);if(s.r.iterator==='returnGetterThrow')throw new TypeError('return getter');return function(){s.events.push(['return']);if(s.r.iterator==='returnThrow')throw new TypeError('return');if(s.r.iterator==='returnPrimitive')return 1;return {};};}};
 return {[Symbol.iterator](){s.events.push(['iterator']);return iterator;}};
}
function metadata(){return Object.entries(functions).map(([key,f])=>({key,name:normal(f.name),length:normal(f.length),own:Object.getOwnPropertyNames(f),isFn:f.isFn,shaderType:f.shaderNode.type,nodeType:f.shaderNode.nodeType,layout:normal(f.shaderNode.layout)}));}
function evaluate(r){
 if(r.kind==='metadata')return metadata();
 const s={r,events:[]},previous=tsl.getCurrentStack(),stack=new StackNode();tsl.setCurrentStack(stack);const out={};
 try{
  const value=r.fixture?fixture(s,'color'):tsl[r.type](...r.values);
  const f=functions[r.name];const result=r.kind==='call'?f(value):f.shaderNode.jsFunc(inputs(s,value));
  out.result=normal(result);out.stack=normal(stack.nodes);
 }catch(e){out.error='#'+e.name;out.stack=normal(stack.nodes);}finally{tsl.setCurrentStack(previous);}
 out.events=s.events;return out;
}
const cases=[{kind:'metadata'}];
for(const name of Object.keys(functions))for(const values of [[0],[1],[.1,.2,.3,.4],[-1,0,2,0],[.0031308,.04045,.5,1]])for(const kind of ['body','call'])cases.push({kind,name,type:name.includes('Alpha')?'vec4':'vec3',values,iterator:'array'});
for(const name of Object.keys(functions))for(const iterator of ['array','custom','empty','nextThrow','valueThrow','returnGetterThrow','returnThrow','returnPrimitive'])cases.push({kind:'body',name,fixture:true,iterator});
for(const name of Object.keys(functions))for(const throwAt of ['color.rgb','color.a','color.xyz','color.mul','color.mul()','color.mul().add','color.pow','color.pow()','color.a.equal','color.a.equal().select','color.rgb.div','color.rgb.div()'])cases.push({kind:'body',name,fixture:true,iterator:'custom',throwAt});
if(structuralNodeIds)for(const r of cases)r.structuralNodeIds=true;
const expected=cases.map(evaluate);
if(directory){fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'input.json'),JSON.stringify(cases));fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2));}
if(!binary){console.log(`Prepared ${cases.length} real TSL function scenarios`);process.exit(0);}
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:256*1024*1024}));
if(directory)fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));
assert.equal(actual.length,expected.length);const differences=[];
for(let i=0;i<cases.length;i++)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({index:i,input:cases[i],expected:expected[i],actual:actual[i]});}
if(directory)fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(differences,null,2));
assert.equal(differences.length,0,JSON.stringify(differences.slice(0,2),null,2));
console.log(`TSL display/material functions: ${cases.length} real pinned-upstream scenarios passed`);
