import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
const upstream=path.resolve(process.argv[2]);
const commit='11738a9d55e502fb484cefffcea3a66dc44e9fe9';
assert.equal(spawnSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).stdout.trim(),commit);
assert.equal(createHash('sha256').update(fs.readFileSync(path.join(upstream,'src/renderers/webgl/WebGLUniformsGroups.js'))).digest('hex'),'02392701664a8386da60a0b1e797b28342ff4036f4cf317737c8cf654406257b','raw upstream source SHA256');
const {WebGLUniformsGroups}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLUniformsGroups.js')));
const {setConsoleFunction}=await import(pathToFileURL(path.join(upstream,'src/utils.js')));
function encode(v){
 if(v===undefined)return 'undefined';if(v===null||typeof v==='boolean'||typeof v==='string')return v;
 if(typeof v==='number'){if(Number.isNaN(v))return 'NaN';if(!Number.isFinite(v))return v<0?'-Infinity':'Infinity';return Object.is(v,-0)?'-0':v;}
 if(typeof v==='function')return {function:v.name};if(Object.hasOwn(v,'tag'))return {ref:String(v.tag)};
 if(ArrayBuffer.isView(v))return {typed:v.constructor.name,values:Array.from(v,encode)};
 if(Array.isArray(v))return Array.from(v,encode);
 const out={};for(const key of Object.keys(v)){const d=Object.getOwnPropertyDescriptor(v,key);out[key]=Object.hasOwn(d,'value')?encode(d.value):'accessor';}return out;
}
function outcome(action){try{return encode(action());}catch(e){return e.name==='TypeError'||e.name==='RangeError'?{error:e.name}:{error:e.name,message:e.message};}}
const numbers=(n,start=.25)=>Array.from({length:n},(_,i)=>start+i);
function value(state,flag,n,start=.25){
 const result={flag,[flag]:true,values:numbers(n,start)};result.elements=result.values;
 for(const name of ['clone','equals','copy','toArray'])result[name]=({[name](...args){
  state.log.push([name]);if(state.throw===name)throw new RangeError(name);const values=this.values;
  if(name==='clone'){const result=value(state,this.flag,values.length);result.values=values.slice();result.elements=result.values;return result;}
  if(name==='equals'){if(state.equalsReturn!==undefined)return state.equalsReturn;const other=args[0].values;if(values.length!==other.length)return false;for(let i=0;i<values.length;i++)if(values[i]!==other[i])return false;return true;}
  if(name==='copy'){this.values=args[0].values.slice();this.elements=this.values;return this;}
  for(let i=0;i<values.length;i++)args[0][args[1]+i]=values[i];return args[0];
 }})[name];return result;
}
const uniform=(value,type)=>({value,type});
class Fixture{
 constructor(){
  const state=this.state={log:[],listeners:[],next:0,throw:'',acted:false,reenter:'',reenterAction:'dispose'};
  const gl=this.gl={UNIFORM_BUFFER:35345},info=this.info={render:{frame:0}},binding=this.binding={},group=this.group={tag:'group',id:1,usage:35044,uniforms:[]},program=this.program={program:{tag:'program'}};
  Object.assign(state,{gl,binding,group,program});
  const command=(name,target)=>({[name](...args){state.log.push([name,this===target,...args.map(encode)]);if(state.throw===name)throw new RangeError(name);if(name==='createBuffer'){if(state.bufferReturnEnabled)return state.bufferReturn;return {tag:`buffer:${state.next++}`};}if(state.reenter===name&&!state.acted){state.acted=true;state.manager[state.reenterAction](state.group,state.program);}}})[name];
  for(const name of ['createBuffer','bindBuffer','bufferData','bufferSubData','deleteBuffer'])gl[name]=command(name,gl);
  for(const name of ['uniformBlockBinding','updateUBOMapping'])binding[name]=command(name,binding);
  for(const name of ['addEventListener','removeEventListener'])group[name]=({[name](type,fn){state.log.push([name,this===state.group,type,encode(fn)]);if(state.throw===name)throw new RangeError(name);const listeners=state.listeners,index=listeners.indexOf(fn);if(name==='addEventListener'){if(index===-1)listeners.push(fn);}else if(index!==-1)listeners.splice(index,1);}})[name];
  this.manager=state.manager=WebGLUniformsGroups(gl,info,{},binding);
  setConsoleFunction((...args)=>state.log.push(['warning',...args.map(encode)]));
 }
 update(){return this.manager.update(this.group,this.program);}bind(){return this.manager.bind(this.group,this.program);}frame(value){this.info.render.frame=value;}
 dispatch(){const event={target:this.group},listeners=this.state.listeners.slice();for(let i=0;i<listeners.length;i++)listeners[i].call(this.group,event);}
 snapshot(){return {size:encode(this.group.__size),cache:encode(this.group.__cache),uniforms:this.group.uniforms.map(item=>(Array.isArray(item)?item:[item]).map(u=>({offset:encode(u.__offset),data:encode(u.__data)}))),listeners:encode(this.state.listeners),trace:encode(this.state.log)};}
}
function scalarCase(mode){const f=new Fixture(),values=[1,true,false,-0,NaN,Infinity,-Infinity],u=uniform(values[mode%values.length],mode<7?undefined:mode<14?'int':'uint');f.group.uniforms=[u];const results=[outcome(()=>f.bind())];for(let i=0;i<3;i++){f.frame(i);results.push(outcome(()=>f.update()),outcome(()=>f.update()));}results.push(outcome(()=>f.bind()));return {kind:'scalar',mode,results,snapshot:f.snapshot()};}
function packedCase(mode){const f=new Fixture(),flags=['isVector2','isVector3','isColor','isVector4','isMatrix3','isMatrix4'],widths=[2,3,3,4,9,16],us=[uniform(1)];for(let i=0;i<6;i++){us.push(uniform(value(f.state,flags[i],widths[i]),mode===1?'int':mode===2?'uint':undefined),uniform(true));}f.group.uniforms=us;const results=[];for(let i=0;i<3;i++){f.frame(i);if(i===2)us[3].value.values[0]=99;results.push(outcome(()=>f.update()));}return {kind:'packed',mode,results,snapshot:f.snapshot()};}
function arrayCase(mode){const f=new Fixture();let v=[1,2,3];if(mode===1)v=[value(f.state,'isVector2',2),value(f.state,'isVector2',2,5)];if(mode===2)v=[value(f.state,'isVector3',3),value(f.state,'isVector3',3,5)];if(mode===3)v=[value(f.state,'isMatrix3',9),value(f.state,'isMatrix3',9,20)];if(mode===4)v=[];const u=uniform(v);if(mode===5)f.group.uniforms=[[uniform(1),uniform(2),uniform(3)],uniform(4)];else if(mode===6)f.group.uniforms=[[],u];else f.group.uniforms=[uniform(9),u,uniform(4)];const results=[];for(let i=0;i<4;i++){f.frame(i);if(i===1)v.length=1;if(i===2)v.length=0;if(i===3)v.push(7);results.push(outcome(()=>f.update()));}return {kind:'array',mode,results,snapshot:f.snapshot()};}
const typed=[Int8Array,Uint8Array,Int16Array,Uint16Array,Int32Array,Uint32Array,Float32Array,Float64Array,Uint8ClampedArray];
function typedCase(kind,mode){const f=new Fixture(),v=new typed[kind](numbers(mode===3?0:mode===2?3:4));if(mode===1)v.isVector3=true;const u=uniform(v);f.group.uniforms=[uniform(1),u,uniform(2)];const results=[];for(let i=0;i<3;i++){f.frame(i);if(i===2)v[0]=77;results.push(outcome(()=>f.update()));}return {kind:'typed',type:kind,mode,results,snapshot:f.snapshot()};}
function failureCase(mode){const f=new Fixture(),names=['createBuffer','bindBuffer','bufferData','addEventListener','updateUBOMapping','clone','toArray','bufferSubData','deleteBuffer','removeEventListener','copy','equals'],v=value(f.state,'isVector3',3);f.group.uniforms=[uniform(v)];const results=[];if(mode>=8)results.push(outcome(()=>f.update()));if(mode>=10){f.frame(1);v.values[0]=88;}f.state.throw=names[mode];results.push(outcome(()=>mode<8||mode>=10?f.update():f.dispatch()));const failed=f.snapshot();f.state.throw='';results.push(outcome(()=>f.update()));return {kind:'failure',mode,results,failed,snapshot:f.snapshot()};}
function lifecycleCase(mode){const f=new Fixture(),u=uniform(1);f.group.uniforms=[u];const results=[outcome(()=>f.update())];if(mode===0)results.push(outcome(()=>f.dispatch()));if(mode===1)results.push(outcome(()=>f.manager.dispose()));if(mode===2){f.group.id=2;results.push(outcome(()=>f.update()),outcome(()=>f.manager.dispose()));}if(mode===3){f.group.id='__proto__';results.push(outcome(()=>f.update()));}if(mode===4){f.state.reenter='bufferSubData';f.state.acted=false;f.frame(1);u.value=2;results.push(outcome(()=>f.update()));}results.push(outcome(()=>f.bind()),outcome(()=>f.update()),outcome(()=>f.dispatch()),outcome(()=>f.manager.dispose()));return {kind:'lifecycle',mode,results,snapshot:f.snapshot()};}
function watch(f,object,property,label,fail=false){const saved=object[property];Object.defineProperty(object,property,{enumerable:true,configurable:true,get(){f.state.log.push(['get',label,property,this===object]);if(fail)throw new RangeError('getter');return saved;},set(v){f.state.log.push(['set',label,property,this===object,encode(v)]);}});}
function observedCase(mode){const f=new Fixture(),v=value(f.state,'isMatrix3',9),u=uniform(v);f.group.uniforms=[u];if(mode===0)watch(f,u,'value','uniform');if(mode===1)watch(f,v,'elements','matrix');if(mode===2)watch(f,v,'isVector2','value');if(mode===3)watch(f,f.group,'id','group');if(mode===4)watch(f,f.gl,'UNIFORM_BUFFER','gl');if(mode===5)watch(f,f.gl,'bufferSubData','gl');if(mode===6)watch(f,f.group,'uniforms','group');if(mode===7)watch(f,u,'type','uniform');if(mode===8)watch(f,f.program,'program','program');if(mode===9)watch(f,f.info.render,'frame','render');if(mode===10)watch(f,u,'value','uniform',true);if(mode===11){watch(f,v,'clone','value');watch(f,v,'toArray','value');}const results=[outcome(()=>f.update())];f.frame(1);results.push(outcome(()=>f.update()),outcome(()=>f.bind()));return {kind:'observed',mode,results,snapshot:f.snapshot()};}
function unsupportedCase(mode){const f=new Fixture();let v;if(mode===0)v=null;if(mode===1)v=undefined;if(mode===2)v='x';if(mode>=3){v=value(f.state,mode===3?'isTexture':'unsupported',0);if(mode===5)f.state.equalsReturn=0;if(mode===6)f.state.equalsReturn=false;}f.group.uniforms=[uniform(v)];const results=[];for(let i=0;i<2;i++){f.frame(i);results.push(outcome(()=>f.update()));}return {kind:'unsupported',mode,results,snapshot:f.snapshot()};}
function edgeCase(mode){const f=new Fixture();let v=value(f.state,'isVector3',3);const u=uniform(v);f.group.uniforms=[u];
 if(mode===0)f.group.uniforms=[];if(mode===1)u.value=[];if(mode===2)f.group.uniforms=[u,u];if(mode===4)f.frame(undefined);if(mode===5)f.frame(NaN);
 if(mode===7)f.state.equalsReturn=0;if(mode===8)v.clone=function clone(){return undefined;};
 if(mode>=9&&mode<=11){f.state.bufferReturnEnabled=true;f.state.bufferReturn=mode===9?null:mode===10?false:undefined;}
 if(mode===12)f.group.id=undefined;if(mode===13)f.group.id='__proto__';if(mode===14)Object.preventExtensions(f.group);if(mode===15)watch(f,u,'__data','uniform');
 if(mode===16)f.group.uniforms=new Array(1);if(mode===17)f.group.uniforms=[[u],uniform(2)];if(mode===18)u.value=[1,v];if(mode===20)f.gl.bufferSubData=9;if(mode===21)f.gl.UNIFORM_BUFFER=null;if(mode===22)f.state.reenter='bufferData';
 if(mode===25||mode===27){v=new Float32Array(numbers(4));u.value=v;watch(f,v,'byteLength','typed');if(mode===27){watch(f,v,'slice','typed');watch(f,v,'subarray','typed');}}
 const results=[outcome(()=>mode===19?f.manager.update(f.group,undefined):f.update())];
 if(mode===3)f.group.uniforms.push(uniform(4));if(mode===6)u.type='uint';if(mode===18)u.value=[v,1];if(mode===23)watch(f,f.group,'id','group');
 if(mode===24){v.values[0]=99;watch(f,v,'clone','value');watch(f,v,'equals','value');watch(f,v,'copy','value');}
 if(mode===26){v=new Float32Array(numbers(4));u.value=v;watch(f,u.__data,'set','data');watch(f,v,'subarray','typed');watch(f,v,'slice','typed');}
 if(mode!==4&&mode!==5)f.frame(1);results.push(outcome(()=>f.update()),outcome(()=>f.update()));
 return {kind:'edge',mode,results,snapshot:outcome(()=>f.snapshot())};
}
function metadata(fn){const out={name:encode(fn.name),length:encode(fn.length),own:Object.getOwnPropertyNames(fn),constructible:typeof fn==='function'&&Object.hasOwn(fn,'prototype'),source:fn.toString()};const d=Object.getOwnPropertyDescriptor(fn,'prototype');if(d)out.prototypeFlags=[d.writable,d.enumerable,d.configurable];return out;}
function metadataCase(){const f=new Fixture(),out={kind:'metadata',constructor:metadata(WebGLUniformsGroups),identity:WebGLUniformsGroups===WebGLUniformsGroups};for(const name of ['bind','update','dispose'])out[name]=metadata(f.manager[name]);out.constructResult=encode(new WebGLUniformsGroups(f.gl,f.info,{},f.binding));out.methodConstruct=outcome(()=>new f.manager.dispose());return out;}
function identityCase(mode){const f=new Fixture(),v=mode===0?numbers(3):mode===1?new Float32Array(numbers(4)):mode===2?value(f.state,'isMatrix3',9):[value(f.state,'isMatrix4',16),value(f.state,'isMatrix4',16,20)],u=uniform(v);f.group.uniforms=[u];const retained=[],original=f.gl.bufferSubData;f.gl.bufferSubData=function bufferSubData(...args){retained.push(args[2]);return original.apply(this,args);};const results=[outcome(()=>f.update())],cache=f.group.__cache,cached=cache['0_0'],data=u.__data,before=encode(cached);if(mode===0||mode===1)v[0]=99;if(mode===2)v.values[0]=99;if(mode===3)v[0].values[0]=99;f.frame(1);results.push(outcome(()=>f.update()));const out={kind:'identity',mode,results,cacheSame:cache===f.group.__cache,cachedSame:cached===cache['0_0'],dataSame:data===u.__data,cachedAliasedValue:cached===v,retainedSame:retained[0]===data&&retained[1]===data,cacheBefore:before,cacheAfter:encode(cached),snapshot:f.snapshot()};if(mode===1)out.bufferAliased=cached.buffer===v.buffer;if(mode===2)out.elementsAliased=cached.elements===v.elements;if(mode===3){out.firstAliased=cached[0]===v[0];out.elementsAliased=cached[0].elements===v[0].elements;}return out;}
const expected=[];
for(let i=0;i<21;i++)expected.push(scalarCase(i));
for(let i=0;i<3;i++)expected.push(packedCase(i));
for(let i=0;i<7;i++)expected.push(arrayCase(i));
for(let i=0;i<9;i++)for(let j=0;j<4;j++)expected.push(typedCase(i,j));
for(let i=0;i<12;i++)expected.push(failureCase(i));
for(let i=0;i<5;i++)expected.push(lifecycleCase(i));
for(let i=0;i<12;i++)expected.push(observedCase(i));
for(let i=0;i<7;i++)expected.push(unsupportedCase(i));
for(let i=0;i<28;i++)expected.push(edgeCase(i));
expected.push(metadataCase());
for(let i=0;i<4;i++)expected.push(identityCase(i));
setConsoleFunction(null);
if(process.argv[3]==='--oracle-only'){console.log(`Pinned Node WebGLUniformsGroups oracle executed ${expected.length} scenarios`);process.exit(0);}
const native=spawnSync(process.argv[3],[],{encoding:'utf8',maxBuffer:128*1024*1024});
const logDir=process.argv[4];if(logDir){fs.mkdirSync(logDir,{recursive:true});fs.writeFileSync(path.join(logDir,'expected.jsonl'),expected.map(v=>JSON.stringify(v)).join('\n')+'\n');fs.writeFileSync(path.join(logDir,'actual.jsonl'),native.stdout);fs.writeFileSync(path.join(logDir,'native.stderr.log'),native.stderr);}
assert.equal(native.status,0,native.stderr||'native probe failed');
const actual=native.stdout.trim().split(/\r?\n/).filter(Boolean).map(JSON.parse);assert.equal(actual.length,expected.length,'scenario count');let comparisons=0;
function compare(a,b,key){comparisons++;if(a&&b&&typeof a==='object'&&typeof b==='object'){assert.equal(Array.isArray(a),Array.isArray(b),key);const ka=Object.keys(a).sort(),kb=Object.keys(b).sort();assert.deepEqual(ka,kb,key+' keys');for(const k of ka)compare(a[k],b[k],key+'.'+k);}else assert.deepEqual(a,b,key);}
for(let i=0;i<expected.length;i++){try{compare(actual[i],expected[i],`scenario[${i}]`);}catch(e){console.error('Expected scenario',JSON.stringify(expected[i]));console.error('Actual scenario',JSON.stringify(actual[i]));throw e;}}
console.log(`WebGLUniformsGroups parity: ${expected.length} scenarios / ${comparisons} comparisons passed against ${commit} (CPU native command protocol)`);
