import assert from 'node:assert/strict';
import {execFileSync,spawnSync} from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),probe=path.resolve(process.argv[3]);
const output=path.resolve(process.argv[4]);fs.mkdirSync(output,{recursive:true});
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const load=async file=>import(pathToFileURL(path.join(upstream,file)));
const {Uniform}=await load('src/core/Uniform.js');
const {UniformsGroup}=await load('src/core/UniformsGroup.js');
const {WebGLUniformsGroups}=await load('src/renderers/webgl/WebGLUniformsGroups.js');
const classes={};
for(const name of ['Vector2','Vector3','Vector4','Color','Matrix3','Matrix4'])classes[name]=(await load(`src/math/${name}.js`))[name];
function encode(v){
 if(v===undefined)return 'undefined';if(v===null)return null;
 if(typeof v==='number'){if(Number.isNaN(v))return 'NaN';if(!Number.isFinite(v))return String(v);if(Object.is(v,-0))return '-0';return v;}
 if(typeof v!=='object')return v;
 if(Object.hasOwn(v,'tag'))return {ref:v.tag};
 for(const flag of ['isVector2','isVector3','isVector4','isColor','isMatrix3','isMatrix4'])if(v[flag]===true)return {math:flag,values:encode(v.toArray())};
 if(ArrayBuffer.isView(v))return {typed:v.constructor.name,values:[...v].map(encode)};
 if(Array.isArray(v))return v.map(encode);
 return Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])]));
}
function fixture(){
 const state={trace:[],next:0},gl={UNIFORM_BUFFER:35345},info={render:{frame:0}},binding={},program={program:{tag:'program'}},group=new UniformsGroup();
 group.tag='group';
 for(const name of ['createBuffer','bindBuffer','bufferData','bufferSubData','deleteBuffer'])gl[name]=function(...args){
  state.trace.push([name,this===gl,...args.map(encode)]);
  if(name==='createBuffer')return {tag:`buffer:${state.next++}`};
 };
 for(const name of ['uniformBlockBinding','updateUBOMapping'])binding[name]=function(...args){state.trace.push([name,this===binding,...args.map(encode)]);};
 const manager=WebGLUniformsGroups(gl,info,{},binding);
 return {group,manager,program,info,update(){manager.update(group,program);},frame(n){info.render.frame=n;},snapshot(){return {
  size:encode(group.__size),uniforms:group.uniforms.map(u=>({offset:encode(u.__offset),data:encode(u.__data)})),
  cache:encode(group.__cache),listeners:group._listeners?.dispose?.length??0,trace:encode(state.trace)
 };}};
}
function math(kind){
 if(kind===1)return new classes.Vector2(.25,1.25);
 if(kind===2)return new classes.Vector3(.25,1.25,2.25);
 if(kind===3)return new classes.Vector4(.25,1.25,2.25,3.25);
 if(kind===4)return new classes.Color(.25,1.25,2.25);
 const v=kind===5?new classes.Matrix3():new classes.Matrix4();return v.fromArray(Array.from({length:kind===5?9:16},(_,i)=>i+.25));
}
function scenario(kind){
 const f=fixture(),v=kind===0?-0:kind<7?math(kind):kind===7?[math(2),math(2)]:new Uint32Array([1,2,3,4]);
 const uniform=new Uniform(v);if(kind===8)uniform.type='uint';f.group.add(new Uniform(true)).add(uniform).add(new Uniform(17));
 const steps=[],save=()=>steps.push(f.snapshot());
 f.update();save();f.update();save();f.frame(1);f.update();save();
 if(kind===0)uniform.value=3.5;else if(kind<=3)v.x=99;else if(kind===4)v.r=99;else if(kind<=6)v.elements[0]=99;else if(kind===7)v[0].x=99;else v[0]=99;
 f.update();save();f.frame(2);f.update();save();f.manager.bind(f.group,f.program);save();
 f.group.dispose();save();f.frame(3);f.update();save();f.manager.dispose();save();f.frame(4);f.update();save();f.group.dispose();save();
 return {kind,steps};
}
const expected=Array.from({length:9},(_,i)=>scenario(i));
fs.writeFileSync(path.join(output,'expected.jsonl'),expected.map(v=>JSON.stringify(v)).join('\n')+'\n');
const run=spawnSync(probe,[],{encoding:'utf8',maxBuffer:64*1024*1024});
fs.writeFileSync(path.join(output,'actual.jsonl'),run.stdout??'');fs.writeFileSync(path.join(output,'probe.stderr.log'),run.stderr??'');
assert.equal(run.status,0,run.stderr);
const actual=run.stdout.trim().split(/\r?\n/).map(v=>JSON.parse(v));assert.equal(actual.length,expected.length);
let comparisons=0;
function compare(a,b,p){comparisons++;if(a===null||b===null||typeof a!=='object'||typeof b!=='object'){assert.deepEqual(a,b,p);return;}
 assert.equal(Array.isArray(a),Array.isArray(b),p);assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),p);
 for(const k of Object.keys(b))compare(a[k],b[k],`${p}.${k}`);
}
for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],`scenario[${i}]`);
console.log(`Native UniformsGroup/math: ${expected.length} scenarios, ${comparisons} recursive comparisons passed`);
