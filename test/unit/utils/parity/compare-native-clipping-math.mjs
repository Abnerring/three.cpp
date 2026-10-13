import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
import path from 'node:path';
import fs from 'node:fs';
const [root,probe]=process.argv.slice(2),pinned='11738a9d55e502fb484cefffcea3a66dc44e9fe9';
assert.equal(execFileSync('git',['-C',root,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),pinned);
const load=async name=>(await import(pathToFileURL(path.join(root,'src/math',name+'.js'))))[name];
const [Vector3,Matrix3,Matrix4,Plane]=await Promise.all(['Vector3','Matrix3','Matrix4','Plane'].map(load));
const encode=value=>value===undefined?'@undefined':typeof value==='function'?'@function':typeof value==='number'?(Number.isNaN(value)?'@NaN':!Number.isFinite(value)?value<0?'@-Infinity':'@Infinity':Object.is(value,-0)?'@-0':value):Array.isArray(value)?Array.from(value,encode):value&&typeof value==='object'?Object.fromEntries(Object.keys(value).map(key=>[key,encode(value[key])])):value;
function attempt(action){try{return {value:encode(action())}}catch(e){return {error:e.name}}}
function constructible(fn){try{Reflect.construct(function(){},[],fn);return true}catch{return false}}
const vector=(values=[0,0,0])=>Object.assign(Object.create(Vector3.prototype),{x:values[0],y:values[1],z:values[2]});
const matrix=(values,size=3)=>Object.assign(Object.create(size===3?Matrix3.prototype:Matrix4.prototype),{elements:[...values]});
const plane=(normal,constant)=>Object.assign(Object.create(Plane.prototype),{isPlane:true,normal,constant});
const vectorValues=v=>[v.x,v.y,v.z];
function numeric(seed,native){const values=Array.from({length:16},(_,i)=>{let value=((seed*37+i*17)%29-14)/8;if(i%5===0)value+=2;if(seed%16===0)value=0;if(seed%16===1)value=i%5===0?1:-0;if(seed%16===2&&i===5)value=NaN;if(seed%16===3&&i===7)value=Infinity;return value});const source=matrix(values,4),square=values.slice(0,9),m=native?new Matrix3():matrix(square);if(native)for(let i=0;i<9;i++)m.elements[i]=square[i];const out={kind:'numeric',seed,native};
 let r=m.invert();out.inverse=encode(m.elements);out.inverseSelf=r===m;
 r=m.transpose();out.transpose=encode(m.elements);out.transposeSelf=r===m;
 r=m.setFromMatrix4(source);out.upper=encode(m.elements);out.upperSelf=r===m;
 r=m.getNormalMatrix(source);out.normalMatrix=encode(m.elements);out.normalSelf=r===m;
 const v=native?new Vector3(1,-2,3):vector([1,-2,3]),b=vector([-0,seed/7,-4]);out.dot=encode(v.dot(b));v.applyMatrix3(m);out.vector=encode(vectorValues(v));
 const p=native?new Plane():plane(vector([1,0,0]),0),initial=plane(vector([1,-2,3]),seed%7===0?-0:seed-5),normal=p.normal;r=p.copy(initial);out.copySelf=r===p;out.copyNormalIdentity=normal===p.normal;const point=vector();r=p.coplanarPoint(point);out.point=encode(vectorValues(point));out.pointIdentity=r===point;r=p.applyMatrix4(source,seed%2?m:undefined);out.planeNormal=encode(vectorValues(p.normal));out.planeConstant=encode(p.constant);out.planeSelf=r===p;return out;
}
function trace(state,row){state.log.push(row);if(state.mode===2&&state.log.length===10)throw new RangeError('trace')}
const observedNumber=(state,label,value)=>({valueOf(){trace(state,['number',label]);return value}});
function accessor(target,key,store,state,label){Object.defineProperty(target,key,{enumerable:true,configurable:true,get(){trace(state,['get',label,key,this===target]);return store[key]},set(value){trace(state,['set',label,key,value,this===target]);if(state.mode===3&&(key==='1'||key==='y'))throw new RangeError('setter');store[key]=value}})}
function observed(kind,mode){const names=['set','setFromMatrix4','invert','transpose','getNormalMatrix','applyMatrix3','dot','copy','coplanarPoint','applyMatrix4'],name=names[kind],state={log:[],mode},stores=[],targets=[];
 for(let which=0;which<4;which++){const store={},target=which<2?[]:vector(),count=which<2?(which===0?9:16):3;for(let i=0;i<count;i++){const key=which<2?String(i):'xyz'[i];let n=which<2?(i%4===0?2:i*.125):i+1;if(mode===4)n=i===0?-0:i===1?NaN:n;store[key]=mode===1?observedNumber(state,`${which}:${key}`,n):n;accessor(target,key,store,state,String(which))}stores.push(store);targets.push(target)}
 const self=matrix([]),source=matrix([],4),selfBacking={elements:targets[0]},sourceBacking={elements:targets[1]};accessor(self,'elements',selfBacking,state,'self');accessor(source,'elements',sourceBacking,state,'source');
 const p=plane(targets[2],0),q=plane(targets[3],0),ps={normal:targets[2],constant:mode===1?observedNumber(state,'constant',-3):-3},qs={normal:targets[3],constant:mode===1?observedNumber(state,'inputConstant',2):2};for(const key of ['normal','constant']){accessor(p,key,ps,state,'plane');accessor(q,key,qs,state,'inputPlane')}
 let receiver=self,args=[];if(kind===0)args=Array.from({length:9},(_,i)=>mode===1?observedNumber(state,'arg'+i,i+1):i+1);else if(kind===1||kind===4)args=[source];else if(kind===5||kind===6){receiver=targets[2];args=[kind===5?self:targets[3]]}else if(kind>=7){receiver=p;args=[kind===7?q:kind===8?targets[3]:source]}
 const fn=receiver[name],result=attempt(()=>{const out=fn.call(receiver,...args);if(out===receiver)return 'self';if(kind===8&&out===args[0])return 'target';return out});
 return {kind:'observed',method:name,mode,result,stores:encode(stores),constant:encode(ps.constant),trace:encode(state.log),name:fn.name,length:fn.length,constructible:constructible(fn)};
}
function callback(target,name,state,returned){const fn=function(...args){trace(state,['call',name,this===target,args.length]);return returned};Object.defineProperty(target,name,{configurable:true,enumerable:true,get(){trace(state,['method',name,this===target]);return fn}})}
function chaining(mode){const state={log:[],mode:0},source=matrix([1,0,0,0,0,1,0,0,0,0,1,0,2,3,4,1],4),self=matrix([0,0,0,0,0,0,0,0,0]),first={},second={};let receiver=self,fn,args=[];
 if(mode===0){fn=self.setFromMatrix4;callback(self,'set',state,'ignored');args=[source]}
 if(mode===1){fn=self.getNormalMatrix;callback(self,'setFromMatrix4',state,first);callback(first,'invert',state,second);callback(second,'transpose',state,'chain end');args=[source]}
 if(mode===2){fn=self.invert;callback(self,'set',state,'singular result')}
 const normal=vector([1,2,3]),p=plane(normal,observedNumber(state,'constant',2)),q=plane(vector([4,5,6]),7);let matrixPrototype,oldNormal;
 if(mode===3){receiver=p;fn=p.copy;callback(normal,'copy',state,'ignored');args=[q]}
 if(mode===4){receiver=p;fn=p.coplanarPoint;callback(first,'copy',state,second);callback(second,'multiplyScalar',state,'point result');args=[first]}
 if(mode===5||mode===6){receiver=p;fn=p.applyMatrix4;const transformed={},normalized={},reference={},normalMatrix={};callback(p,'coplanarPoint',state,first);callback(first,'applyMatrix4',state,reference);callback(normal,'applyMatrix3',state,transformed);callback(transformed,'normalize',state,normalized);callback(reference,'dot',state,observedNumber(state,'distance',13));matrixPrototype=Matrix3.prototype;oldNormal=Object.getOwnPropertyDescriptor(matrixPrototype,'getNormalMatrix');callback(matrixPrototype,'getNormalMatrix',state,normalMatrix);args=[source,mode===5?false:normalMatrix]}
 let result;try{result=attempt(()=>{const returned=fn.call(receiver,...args);return returned===receiver?'self':returned})}finally{if(oldNormal)Object.defineProperty(matrixPrototype,'getNormalMatrix',oldNormal)}
 return {kind:'chaining',mode,result,constant:encode(p.constant),trace:encode(state.log)};
}
function reentry(mode){const state={acted:false,calls:0},outer=plane(vector([1,2,3]),2),inner=plane(vector([-1,2,1]),-3),a=matrix([2,0,0,0,0,3,0,0,0,0,4,0,1,2,3,1],4),b=matrix([1,0,0,0,0,2,0,0,0,0,1,0,4,5,6,1],4),original=outer.coplanarPoint;outer.coplanarPoint=function(...args){state.calls++;const target=original.call(this,...args);if(!state.acted){state.acted=true;inner.applyMatrix4(b);if(mode===1)throw new RangeError('nested')}return target};const result=attempt(()=>outer.applyMatrix4(a)===outer?'self':undefined);return {kind:'reentry',mode,result,outerNormal:encode(vectorValues(outer.normal)),outerConstant:encode(outer.constant),innerNormal:encode(vectorValues(inner.normal)),innerConstant:encode(inner.constant),calls:state.calls}}
// Plane.js initializes its private _normalMatrix with this actual constructor.
// Native each case starts a fresh worker so its module scratch is still cold.
function initialization(mode){let initialized=false,calls=0,error='';const previous=Object.getOwnPropertyDescriptor(Array.prototype,'0'),d={configurable:true};if(mode===1){d.value=17;d.writable=false}else if(mode===2)d.get=function(){return 17};else d.set=function(){calls++;if(mode===3)throw new RangeError('inherited index')};Object.defineProperty(Array.prototype,'0',d);try{new Vector3();new Matrix3();initialized=true}catch(e){error=e.name}finally{if(previous)Object.defineProperty(Array.prototype,'0',previous);else delete Array.prototype[0]}return {kind:'initialization',mode,initialized,calls,error}}
const expected=[];for(let mode=0;mode<4;mode++)expected.push(initialization(mode));for(let seed=0;seed<64;seed++)for(const native of [false,true])expected.push(numeric(seed,native));for(let kind=0;kind<10;kind++)for(let mode=0;mode<5;mode++)expected.push(observed(kind,mode));for(let mode=0;mode<7;mode++)expected.push(chaining(mode));for(let mode=0;mode<2;mode++)expected.push(reentry(mode));
if(probe==='--oracle-only'){console.log(`${expected.length} actual upstream scenarios executed`);process.exit(0)}
const actual=JSON.parse(execFileSync(probe,[],{encoding:'utf8',maxBuffer:256*1024*1024}));fs.writeFileSync(probe+'.expected.json',JSON.stringify(expected));fs.writeFileSync(probe+'.actual.json',JSON.stringify(actual));let comparisons=0;
function compare(a,e,p){comparisons++;if(typeof a==='number'&&typeof e==='number'){assert.equal(a,e,p);return}if(Array.isArray(e)){assert.ok(Array.isArray(a),p);assert.equal(a.length,e.length,p+'.length');e.forEach((x,i)=>compare(a[i],x,p+'.'+i));return}if(e&&typeof e==='object'){assert.deepEqual(Object.keys(a),Object.keys(e),p+'.keys');for(const k of Object.keys(e))compare(a[k],e[k],p+'.'+k);return}assert.equal(a,e,p)}
compare(actual,expected,'root');console.log(`Native clipping math parity: ${expected.length} scenarios / ${comparisons} comparisons passed; pinned ${pinned}; exact signed-zero and getter/conversion/write order`);
