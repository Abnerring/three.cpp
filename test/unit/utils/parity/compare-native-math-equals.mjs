import assert from 'node:assert/strict';
import {execFileSync,spawnSync} from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),probe=path.resolve(process.argv[3]),output=path.resolve(process.argv[4]);fs.mkdirSync(output,{recursive:true});
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const classes=[];
for(const n of ['Vector2','Vector3','Vector4','Color','Matrix3','Matrix4'])classes.push((await import(pathToFileURL(path.join(upstream,`src/math/${n}.js`))))[n]);
function scenario(kind,mode){
 const fn=classes[kind].prototype.equals,fields=kind===0?'xy':kind===2?'xyzw':kind===3?'rgb':'xyz';
 const count=kind<4?fields.length:kind===4?9:16;
 const trace=[],self={},other={},av=Array.from({length:count},(_,i)=>i+.25),bv=[...av];
 if(mode===1)bv[0]=9;if(mode===2)bv[count-1]=99;if(mode===3)av[0]=bv[0]=NaN;if(mode===4){av[0]=-0;bv[0]=0;}if(mode===5)bv[0]='0.25';
 const firstOther=kind<4?`other.${fields[0]}`:'other[0]';
 function getter(object,key,label,data,index,element){Object.defineProperty(object,key,{enumerable:true,configurable:true,get(){
  trace.push(label);if(mode===9&&element)throw new RangeError('observable getter failure');
  if(mode===8&&element){const replacement={};for(const object of [self,other])Object.defineProperty(object,'elements',{value:replacement,writable:true,enumerable:true,configurable:true});}
  if(mode===10&&label===firstOther)bv[count-1]=99;
  return data[index];
 }});}
 if(kind<4){for(let i=0;i<count;i++){getter(self,fields[i],`self.${fields[i]}`,av,i,true);getter(other,fields[i],`other.${fields[i]}`,bv,i,true);}}
 else {
  const ae={},be={};for(let i=0;i<count;i++){getter(ae,i,`self[${i}]`,av,i,true);getter(be,i,`other[${i}]`,bv,i,true);}
  getter(self,'elements','self.elements',{value:ae},'value',false);getter(other,'elements','other.elements',{value:be},'value',false);
 }
 let result;try{result=fn.call(mode===7?null:self,mode===6?undefined:other);}catch(e){result={error:e.name};}
 return {kind,mode,result,trace};
}
function metadata(kind){const fn=classes[kind].prototype.equals;let constructible=true;try{Reflect.construct(function(){},[],fn);}catch{constructible=false;}
 return {metadata:kind,name:fn.name,length:fn.length,own:Object.getOwnPropertyNames(fn),descriptors:['name','length'].map(k=>{const d=Object.getOwnPropertyDescriptor(fn,k);return [d.writable,d.enumerable,d.configurable];}),constructible,source:fn.toString()};}
const expected=[];for(let k=0;k<6;k++){for(let m=0;m<12;m++)expected.push(scenario(k,m));expected.push(metadata(k));}
fs.writeFileSync(path.join(output,'expected.jsonl'),expected.map(v=>JSON.stringify(v)).join('\n')+'\n');
const run=spawnSync(probe,[],{encoding:'utf8',maxBuffer:16*1024*1024});
fs.writeFileSync(path.join(output,'actual.jsonl'),run.stdout??'');fs.writeFileSync(path.join(output,'probe.stderr.log'),run.stderr??'');assert.equal(run.status,0,run.stderr);
const actual=run.stdout.trim().split(/\r?\n/).map(v=>JSON.parse(v));assert.equal(actual.length,expected.length);let comparisons=0;
function compare(a,b,p){comparisons++;if(a===null||b===null||typeof a!=='object'||typeof b!=='object'){assert.deepEqual(a,b,p);return;}
 assert.equal(Array.isArray(a),Array.isArray(b),p);assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),p);for(const k of Object.keys(b))compare(a[k],b[k],`${p}.${k}`);}
for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],`scenario[${i}]`);
console.log(`Native math equals: ${expected.length} scenarios, ${comparisons} recursive comparisons passed`);
