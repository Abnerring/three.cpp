import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync,execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
if(process.argv[2]==='--fresh-import'){
 const upstream=path.resolve(process.argv[3]),family=process.argv[4],load=async name=>await import(pathToFileURL(path.join(upstream,'src/math',name+'.js')));
 assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
 const {Vector2}=await load('Vector2'),{Vector3}=await load('Vector3');await load('Sphere');await load('Plane');const {Matrix4}=await load('Matrix4');
 const events=[];for(const [C,name,key] of [[Vector2,'Vector2','x'],[Vector3,'Vector3','x'],[Matrix4,'Matrix4','elements']])Object.defineProperty(C.prototype,key,{configurable:true,set(value){events.push(['set:'+name+'.'+key,value]);}});
 const module=await load(family==='array'?'FrustumArray':'Frustum');console.log(JSON.stringify({events,exports:Object.keys(module)}));process.exit(0);
}
const [upstreamArgument,binaryArgument,directoryArgument]=process.argv.slice(2);
assert(upstreamArgument&&binaryArgument&&directoryArgument,'Usage: node --expose-gc compare-frustum.mjs <pinned-upstream> <binary|--oracle> <new-output-directory>');
assert.equal(typeof gc,'function','Run with --expose-gc');
const upstream=path.resolve(upstreamArgument),binary=binaryArgument==='--oracle'?null:path.resolve(binaryArgument),directory=path.resolve(directoryArgument);
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
assert(!fs.existsSync(directory),'Choose a fresh output directory to preserve earlier evidence');fs.mkdirSync(directory,{recursive:true});
const load=async name=>await import(pathToFileURL(path.join(upstream,'src/math',name+'.js')));
const {Frustum}=await load('Frustum'),{FrustumArray}=await load('FrustumArray'),{Vector2}=await load('Vector2'),{Vector3}=await load('Vector3'),{Sphere}=await load('Sphere'),{Plane}=await load('Plane'),{Matrix4}=await load('Matrix4');
const ctor=request=>request.family==='array'?FrustumArray:Frustum;
const own=(object,key,value)=>Object.defineProperty(object,key,{value,writable:true,enumerable:true,configurable:true});
function normal(value,state,depth=0){
 if(value===undefined)return '#undefined';if(typeof value==='symbol')return '#symbol';
 if(typeof value==='number'){if(Number.isNaN(value))return '#NaN';if(!Number.isFinite(value))return value>0?'#Infinity':'#-Infinity';if(Object.is(value,-0))return '#-0';}
 if(value===null||typeof value!=='object'&&typeof value!=='function')return value;
 if(state)for(const [ref,name] of state.refs)if(ref===value)return '@'+name;
 if(typeof value==='function')return '#function:'+value.name;
 if(depth>10)return '#depth';
 if(Array.isArray(value))return Array.from({length:value.length},(_,index)=>normal(value[index],state,depth+1));
 return Object.fromEntries(Object.keys(value).map(key=>[key,normal(value[key],state,depth+1)]));
}
const ref=(state,value,name)=>state.refs.push([value,name]);
function event(state,name,args=[]){
 if(!state.active)return;state.events.push([name,...args.map(value=>normal(value,state))]);
 if(state.request.throwAt===name)throw state.throwValue;
 if(state.request.gcAt===name){gc();state.events.push(['gc']);}
}
function state(request){const result={request,events:[],refs:[],active:false,throwValue:{token:'fixture-throw'}};ref(result,result.throwValue,'throwValue');return result;}
function decode(value,state){
 if(value===null||typeof value!=='object')return value;
 switch(value.tag){case 'undefined':return undefined;case 'NaN':return NaN;case 'Infinity':return Infinity;case '-Infinity':return -Infinity;case '-0':return -0;case 'symbol':return Symbol('fixture');case 'primitive':{const object={[Symbol.toPrimitive](hint){event(state,'convert:'+value.name,[hint]);return decode(value.value,state);}};ref(state,object,'primitive:'+value.name);return object;}}
 return Array.isArray(value)?value.map(item=>decode(item,state)):Object.fromEntries(Object.keys(value).map(key=>[key,decode(value[key],state)]));
}
function caught(state,error){
 if(error===state.throwValue)return {value:normal(error,state),fixtureIdentity:true};
 if(error instanceof TypeError||error instanceof ReferenceError||error instanceof RangeError)return {name:error.name};
 if(error!==null&&typeof error==='object')return {value:normal(error,state),fixtureIdentity:false,name:error.name,message:error.message};
 return {value:normal(error,state),fixtureIdentity:false};
}
function attempt(state,fn){try{return {value:normal(fn(),state)};}catch(error){return {throw:caught(state,error)};}}
function getter(state,object,key,label,value){Object.defineProperty(object,key,{enumerable:true,configurable:true,get(){event(state,'get:'+label);return value;}});}
function cell(state,object,key,label,value){Object.defineProperty(object,key,{enumerable:true,configurable:true,get(){event(state,'get:'+label);return value;},set(input){event(state,'set:'+label,[input]);value=input;}});}
function method(state,object,name,label,result,returnSelf=false){Object.defineProperty(object,name,{configurable:true,get(){event(state,'get:'+label);const receiver=this;return function(...args){
 event(state,'call:'+label,[this===receiver,args]);
 if(!state.reentering&&state.request.reenterAt==='call:'+label){state.reentering=true;try{event(state,'reenter:return',[Frustum.prototype.containsPoint.call(state.self,state.point)]);}finally{state.reentering=false;}}
 return returnSelf?this:result;
};}});}
const observe=(state,object,key,label)=>getter(state,object,key,label,object[key]);
const vector=(values,count=3)=>count===2?new Vector2(values[0],values[1]):new Vector3(values[0],values[1],values[2]);
function matrix(values){const result=new Matrix4();if(values&&typeof values==='object')result.elements=values;return result;}
const planes=values=>values.map(value=>new Plane(vector(value),value[3]));
const snapshot=value=>normal(value);
function describe(object){return Object.getOwnPropertyNames(object).map(key=>{const d=Object.getOwnPropertyDescriptor(object,key),value=d.value;return [key,value!==null&&typeof value==='object'?'#object':normal(value),typeof value==='function'?value.name:null,typeof value==='function'?value.length:null,(()=>{try{Reflect.construct(function(){},[],value);return true;}catch{return false;}})(),d.writable??false,d.enumerable??false,d.configurable??false];});}
function metadata(request){const C=ctor(request);return {constructor:describe(C),prototype:describe(C.prototype),constructorSource:C.toString(),methodSources:Object.fromEntries(Object.getOwnPropertyNames(C.prototype).filter(name=>name!=='constructor').map(name=>[name,C.prototype[name].toString()]))};}
function numeric(request){
 const s=state(request),object=new Frustum(),out={},name=request.method;let args=[];if(Object.hasOwn(request,'planes'))object.planes=planes(decode(request.planes,s));
 if(name==='setFromProjectionMatrix')args=[matrix(decode(request.elements,s)),decode(request.coordinateSystem,s),decode(request.reversedDepth,s)];
 else if(name==='containsPoint')args=[vector(decode(request.point,s))];
 else if(name==='intersectsSphere')args=[new Sphere(vector(decode(request.point,s)),decode(request.radius,s))];
 else if(name==='intersectsBox')args=[{min:vector(decode(request.min,s)),max:vector(decode(request.max,s))}];
 else if(name==='intersectsObject'){const sphere=new Sphere(vector(decode(request.point,s)),decode(request.radius,s)),input={matrixWorld:matrix(decode(request.elements,s))};if(request.geometry)input.geometry={boundingSphere:sphere};else input.boundingSphere=sphere;args=[input];}
 else if(name==='intersectsSprite')args=[{center:vector(decode(request.center,s),2),matrixWorld:matrix(decode(request.elements,s))}];
 else if(name==='set')args=planes(decode(request.sourcePlanes,s));
 else if(name==='copy'){const source=new Frustum();source.planes=planes(decode(request.sourcePlanes,s));args=[source];}
 try{const result=Reflect.apply(object[name],object,args);out.result=result===object?'@self':snapshot(result);if(name==='clone')out.cloneIndependent=result.planes!==object.planes&&result.planes[0]!==object.planes[0];}catch(error){out.throw=caught(s,error);}
 out.self=snapshot(object);return out;
}
function arrayNumeric(request){
 const s=state(request),object=new FrustumArray(),out={},passes=[];let first;
 for(const count of request.passes){const cameras=Array.from({length:count},(_,i)=>({projectionMatrix:matrix(request.elements),matrixWorldInverse:matrix(),coordinateSystem:decode(request.coordinateSystem,s),reversedDepth:decode(request.reversedDepth,s)})),row={};try{row.returnedSelf=object.setFromArrayCamera({cameras})===object;}catch(error){row.throw=caught(s,error);}if(first===undefined&&object._frustums.length)first=object._frustums[0];row.count=object._count;row.poolLength=object._frustums.length;row.firstReused=first===undefined||object._frustums[0]===first;passes.push(row);}
 out.passes=passes;const name=request.method;
 if(name==='copy'||name==='clone'){const result=name==='copy'?new FrustumArray().copy(object):object.clone();out.result=snapshot(result);out.independent=result._count===0||result._frustums[0]!==object._frustums[0];}
 else if(name!=='setFromArrayCamera'){
  let value=name==='containsPoint'?vector(request.point):name==='intersectsSphere'?new Sphere(vector(request.point),decode(request.radius,s)):{};
  if(name==='intersectsBox')value={min:vector(request.min),max:vector(request.max)};
  if(name==='intersectsObject')value={boundingSphere:new Sphere(vector(request.point),request.radius),matrixWorld:matrix()};
  if(name==='intersectsSprite')value={center:vector(request.center,2),matrixWorld:matrix()};out.result=object[name](value);
 }
 out.self=snapshot(object);return out;
}
function restore(entries){for(const {object,key,descriptor} of entries.toReversed())if(descriptor)Object.defineProperty(object,key,descriptor);else delete object[key];}
function observedVector(state,name,values){const result={};ref(state,result,name);for(let i=0;i<3;++i)getter(state,result,'xyz'[i],name+'.'+'xyz'[i],decode(values[i],state));return result;}
function sequenceGetter(state,object,key,name,input){const values=input.map(value=>decode(value,state));let index=0;Object.defineProperty(object,key,{enumerable:true,configurable:true,get(){event(state,'get:'+name);return values[Math.min(index++,values.length-1)];},set(value){event(state,'set:'+name,[value]);}});}
function protocol(request){
 const s=state(request),point={},out={},name=request.method,isArray=request.family==='array',changes=[],pool={};let self={},args=[];
 ref(s,self,'self');ref(s,point,'point');s.self=self;s.point=point;ref(s,pool,isArray?'pool':'planes');
 if(!isArray){
  getter(s,self,'planes','self.planes',pool);
  for(let i=0;i<6;++i){const plane={},label='p'+i;ref(s,plane,label);getter(s,pool,i,'planes.'+i,plane);method(s,plane,'copy',label+'.copy',undefined,true);method(s,plane,'setComponents',label+'.setComponents',undefined,true);method(s,plane,'normalize',label+'.normalize',undefined,true);method(s,plane,'distanceToPoint',label+'.distanceToPoint',decode(request.distances?request.distances[i]:1,s));getter(s,plane,'normal',label+'.normal',observedVector(s,label+'.normal',request.normal??[1,-1,0]));}
  if(name==='set')for(let i=0;i<6;++i){const value={};ref(s,value,'arg'+i);args.push(value);}
  else if(name==='copy'){const source={},sourcePlanes={};ref(s,source,'source');ref(s,sourcePlanes,'sourcePlanes');getter(s,source,'planes','source.planes',sourcePlanes);for(let i=0;i<6;++i){const value={};ref(s,value,'arg'+i);getter(s,sourcePlanes,i,'sourcePlanes.'+i,value);}args=[source];}
  else if(name==='setFromProjectionMatrix'){
   const value={},elements={};ref(s,value,'matrix');ref(s,elements,'elements');getter(s,value,'elements','matrix.elements',elements);for(let i=0;i<16;++i)getter(s,elements,i,'elements.'+i,decode(request.elements[i],s));args=[value,decode(request.coordinateSystem,s),decode(request.reversedDepth,s)];
   if(Object.hasOwn(request,'errorMode')){changes.push({object:globalThis,key:'Error',descriptor:Object.getOwnPropertyDescriptor(globalThis,'Error')});if(request.errorMode==='missing')delete globalThis.Error;else {const replacement=request.errorMode==='nonconstructible'?()=>{}:function Error(...args){event(s,'construct:Error',args);return s.throwValue;};getter(s,globalThis,'Error','global.Error',replacement);}}
  }
  else if(name==='containsPoint')args=[point];
  else if(name==='intersectsSphere'){const sphere={};ref(s,sphere,'sphere');getter(s,sphere,'center','sphere.center',point);getter(s,sphere,'radius','sphere.radius',decode(request.radius,s));args=[sphere];}
  else if(name==='intersectsBox'){const box={};ref(s,box,'box');getter(s,box,'min','box.min',observedVector(s,'box.min',request.min));getter(s,box,'max','box.max',observedVector(s,'box.max',request.max));args=[box];}
  else if(name==='intersectsObject'||name==='intersectsSprite'){
   const input={};ref(s,input,'input');getter(s,input,'matrixWorld','input.matrixWorld',matrix(request.matrix));method(s,self,'intersectsSphere','self.intersectsSphere',decode(request.result,s));
   if(name==='intersectsSprite')getter(s,input,'center','input.center',vector(request.center,2));
   else {const sphere=new Sphere(new Vector3(1,2,3),2);ref(s,sphere,'inputSphere');let holder=input;if(request.branch==='geometry'||request.branch==='geometryNull'){getter(s,input,'boundingSphere','input.boundingSphere',undefined);holder={};ref(s,holder,'geometry');getter(s,input,'geometry','input.geometry',holder);}cell(s,holder,'boundingSphere',holder===input?'input.boundingSphere':'geometry.boundingSphere',request.branch==='null'||request.branch==='geometryNull'?null:sphere);own(holder,'computeBoundingSphere',function(){event(s,'call:computeBoundingSphere');this.boundingSphere=sphere;});}
   const prototype=name==='intersectsSprite'?Vector2.prototype:Sphere.prototype,key=name==='intersectsSprite'?'distanceTo':'copy';changes.push({object:prototype,key,descriptor:Object.getOwnPropertyDescriptor(prototype,key)});observe(s,prototype,key,'prototype.'+key);changes.push({object:Sphere.prototype,key:'applyMatrix4',descriptor:Object.getOwnPropertyDescriptor(Sphere.prototype,'applyMatrix4')});observe(s,Sphere.prototype,'applyMatrix4','prototype.applyMatrix4');args=[input];
  }
  else if(name==='clone'){const target={};ref(s,target,'cloneTarget');method(s,target,'copy','cloneTarget.copy',request.result);const constructor=function Custom(){event(s,'call:constructor');return target;};getter(s,self,'constructor','self.constructor',constructor);}
 }else{
  getter(s,self,'_frustums','self._frustums',pool);cell(s,self,'coordinateSystem','self.coordinateSystem',2000);const counts=request.counts??[3];sequenceGetter(s,self,'_count','self._count',counts);
  for(let i=0;i<3;++i){const frustum={},label='f'+i;ref(s,frustum,label);getter(s,pool,i,'pool.'+i,frustum);for(const key of ['intersectsObject','intersectsSprite','intersectsSphere','intersectsBox','containsPoint'])method(s,frustum,key,label+'.'+key,decode(request.results?.[i],s));method(s,frustum,'copy',label+'.copy',undefined,true);method(s,frustum,'setFromProjectionMatrix',label+'.setFromProjectionMatrix',undefined,true);}
  if(name==='copy'){const source={},sourcePool={};ref(s,source,'source');ref(s,sourcePool,'sourcePool');getter(s,source,'coordinateSystem','source.coordinateSystem',2001);getter(s,source,'_frustums','source._frustums',sourcePool);sequenceGetter(s,source,'_count','source._count',counts);for(let i=0;i<3;++i){const item={};ref(s,item,'source'+i);getter(s,sourcePool,i,'sourcePool.'+i,item);}args=[source];}
  else if(name==='setFromArrayCamera'){
   const input={},cameras={};ref(s,input,'cameraArray');ref(s,cameras,'cameras');getter(s,input,'cameras','cameraArray.cameras',cameras);sequenceGetter(s,cameras,'length','cameras.length',request.lengths??[2]);
   for(let i=0;i<3;++i){const camera={},label='camera'+i;ref(s,camera,label);getter(s,cameras,i,'cameras.'+i,camera);getter(s,camera,'projectionMatrix',label+'.projectionMatrix',matrix());getter(s,camera,'matrixWorldInverse',label+'.matrixWorldInverse',matrix());getter(s,camera,'coordinateSystem',label+'.coordinateSystem',2001);getter(s,camera,'reversedDepth',label+'.reversedDepth',i%2===0);}changes.push({object:Matrix4.prototype,key:'multiplyMatrices',descriptor:Object.getOwnPropertyDescriptor(Matrix4.prototype,'multiplyMatrices')});observe(s,Matrix4.prototype,'multiplyMatrices','prototype.multiplyMatrices');args=[input];
  }
  else if(name==='clone'){self={};ref(s,self,'cloneSelf');getter(s,self,'constructor','mustNotRead.constructor',1);getter(s,self,'coordinateSystem','cloneSelf.coordinateSystem',2001);getter(s,self,'_frustums','cloneSelf._frustums',[]);getter(s,self,'_count','cloneSelf._count',0);}
  else args=[point];
 }
 s.active=true;out.result=attempt(s,()=>Reflect.apply(ctor(request).prototype[name],self,args));s.active=false;restore(changes);out.events=s.events;return out;
}
function construction(request){
 const s=state(request),C=ctor(request),out={},prototype={},args=request.args.map(value=>decode(value,s)),custom=request.newTarget;let target;
 if(custom){for(const key of request.family==='array'?['coordinateSystem','_frustums','_count']:['planes'])Object.defineProperty(prototype,key,{configurable:true,set(value){event(s,'set:target.'+key,[value]);}});target=new Proxy(function Target(){},{get(object,key,receiver){if(key==='prototype'){event(s,'get:newTarget.prototype');return prototype;}return Reflect.get(object,key,receiver);}});}
 s.active=true;try{const result=request.call?Reflect.apply(C,null,args):custom?Reflect.construct(C,args,target):Reflect.construct(C,args);s.active=false;out.result=snapshot(result);if(custom)out.prototypeIdentity=Object.getPrototypeOf(result)===prototype;}catch(error){s.active=false;out.throw=caught(s,error);}out.events=s.events;return out;
}
const run=request=>({metadata,numeric,arrayNumeric,construct:construction,protocol}[request.kind])(request);

const U={tag:'undefined'},N={tag:'NaN'},P={tag:'Infinity'},M={tag:'-Infinity'},Z={tag:'-0'},S={tag:'symbol'},primitive=(name,value)=>({tag:'primitive',name,value}),identity=[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],translated=[2,0,0,0,0,3,0,0,0,0,4,0,1,-2,3,1],cube=[[1,0,0,1],[-1,0,0,1],[0,1,0,1],[0,-1,0,1],[0,0,1,1],[0,0,-1,1]],sourcePlanes=cube.map((values,index)=>values.map((value,column)=>column===3?index-2:value)),cases=[];
for(const family of ['frustum','array']){
 cases.push({kind:'metadata',family});
 for(const args of [[],[null],[U,3,false,'raw',null,U],[1,2,3,4,5,6]])for(const call of [false,true])cases.push({kind:'construct',family,args,call});
 cases.push({kind:'construct',family,args:[],newTarget:true});
 for(const throwAt of ['get:newTarget.prototype',...(family==='array'?['set:target.coordinateSystem','set:target._frustums','set:target._count']:['set:target.planes'])])cases.push({kind:'construct',family,args:[],newTarget:true,throwAt});
}
const numericBase={kind:'numeric',family:'frustum',planes:cube,point:[0,0,0],radius:.5,min:[-.5,-.5,-.5],max:[.5,.5,.5],center:[.5,.5],elements:identity};
for(const method of ['set','copy','clone'])cases.push({...numericBase,method,sourcePlanes});
for(const point of [[0,0,0],[1,1,1],[-1,-1,-1],[1.000001,0,0],[0,0,100],[N,0,0],[Z,0,0]])for(const method of ['containsPoint','intersectsSphere','intersectsObject'])for(const radius of method==='containsPoint'?[0]:[0,.5,2,-1])for(const geometry of method==='intersectsObject'?[false,true]:[false])cases.push({...numericBase,method,point,radius,geometry});
for(const [min,max] of [[[-.5,-.5,-.5],[.5,.5,.5]],[[1,1,1],[2,2,2]],[[2,2,2],[3,3,3]],[[P,P,P],[M,M,M]],[[N,0,0],[1,1,1]]])cases.push({...numericBase,method:'intersectsBox',min,max});
for(const center of [[.5,.5],[0,0],[2,-1]])for(const elements of [identity,translated])cases.push({...numericBase,method:'intersectsSprite',center,elements});
for(const elements of [identity,Array.from({length:16},(_,index)=>index+1),Array(16).fill(0),translated])for(const coordinateSystem of [U,2000,2001,0,'2000',S])for(const reversedDepth of [false,true,0,1,'truthy'])cases.push({kind:'numeric',family:'frustum',method:'setFromProjectionMatrix',elements,coordinateSystem,reversedDepth});
const arrayBase={kind:'arrayNumeric',family:'array',elements:identity,coordinateSystem:2000,reversedDepth:false,passes:[2,1,3,0,2],point:[0,0,0],radius:.5,min:[-.5,-.5,-.5],max:[.5,.5,.5],center:[.5,.5]};
for(const method of ['setFromArrayCamera','copy','clone','containsPoint','intersectsSphere','intersectsBox','intersectsObject','intersectsSprite'])for(const passes of [[],[0],[1],[2,1,3,0,2]])cases.push({...arrayBase,method,passes});
for(const coordinateSystem of [2000,2001,0])for(const reversedDepth of [false,true])cases.push({...arrayBase,method:'setFromArrayCamera',passes:[1],coordinateSystem,reversedDepth});

const protocolBase={kind:'protocol',family:'frustum',radius:1,min:[-2,-3,-4],max:[2,3,4],center:[0,0],matrix:translated,result:'callback-result',elements:identity,coordinateSystem:2000,reversedDepth:false},cutpointScenarios=[];
function scenario(request,cutpoints=false){const value={...protocolBase,...request};cases.push(value);if(cutpoints)cutpointScenarios.push(value);return value;}
for(const method of ['set','copy','containsPoint','intersectsSphere','intersectsBox','intersectsObject','intersectsSprite','clone','setFromProjectionMatrix'])scenario({method},true);
for(const reversedDepth of [false,true,0,1,'truthy'])for(const coordinateSystem of [U,2000,2001,0,'2000',N,S,primitive('coordinate','custom')])scenario({method:'setFromProjectionMatrix',coordinateSystem,reversedDepth});
for(const errorMode of ['replace','nonconstructible','missing'])scenario({method:'setFromProjectionMatrix',coordinateSystem:primitive('coordinate','invalid'),errorMode},true);
scenario({method:'setFromProjectionMatrix',coordinateSystem:0,errorMode:'replace',throwAt:'get:global.Error'});
scenario({method:'setFromProjectionMatrix',coordinateSystem:primitive('coordinate','invalid'),errorMode:'replace',throwAt:'convert:coordinate'});
for(const values of [Array.from({length:16},(_,index)=>primitive('e'+index,index+1)),Array.from({length:16},(_,index)=>String(index+1)),identity.with(0,S)])scenario({method:'setFromProjectionMatrix',elements:values},true);
for(const method of ['containsPoint','intersectsSphere','intersectsBox'])for(let outside=0;outside<6;++outside){const distances=Array(6).fill(1);distances[outside]=-2;scenario({method,distances},outside===3);}
for(const radius of [Z,0,-1,N,P,M,S,primitive('radius',2)])scenario({method:'intersectsSphere',radius},radius?.tag==='primitive');
for(const normal of [[0,Z,N],[1,-1,1],[primitive('nx',1),primitive('ny',-1),primitive('nz',0)],[S,0,0]])scenario({method:'intersectsBox',normal},normal[0]?.tag==='primitive');
for(const branch of ['own','null','geometry','geometryNull'])for(const result of [false,true,0,'truthy',U])scenario({method:'intersectsObject',branch,result},result===true);
for(const center of [[.5,.5],[0,0],[2,-1]])for(const result of [false,true,'callback'])scenario({method:'intersectsSprite',center,result});
for(const result of [false,true,0,'copy-result'])scenario({method:'clone',result});
for(const method of ['containsPoint','intersectsSphere','intersectsBox'])scenario({method,gcAt:'call:p3.distanceToPoint'});
scenario({method:'intersectsBox',reenterAt:'call:p0.distanceToPoint'});

for(const method of ['intersectsObject','intersectsSprite','intersectsSphere','intersectsBox','containsPoint']){
 for(const results of [[false,false,false],[true,false,false],[false,1,false],[0,'yes',true],[null,N,{}]])scenario({family:'array',method,results,counts:[3]},results[1]===1);
 for(const counts of [[0],[1],[2,1],[primitive('count',2)],[S],[4]])scenario({family:'array',method,results:[false,false,true],counts});
}
for(const method of ['copy','clone','setFromArrayCamera'])scenario({family:'array',method,results:[false,false,false]},true);
for(const counts of [[0],[1],[2,1],[primitive('sourceCount',2)],[S]])scenario({family:'array',method:'copy',counts});
for(const lengths of [[0],[1],[2,1,1],[primitive('cameraLength',2)],[S]])scenario({family:'array',method:'setFromArrayCamera',lengths});
scenario({family:'array',method:'copy',gcAt:'call:f1.copy'});
scenario({family:'array',method:'setFromArrayCamera',gcAt:'get:camera0.matrixWorldInverse'});

// Derive exception cutpoints from genuine fixed-source executions; no expected
// algorithm is reimplemented. Every observed getter/call/coercion can stop it.
for(const request of cutpointScenarios){const observed=run(request),names=new Set(observed.events.filter(row=>typeof row[0]==='string'&&/^(get:|call:|convert:|construct:)/.test(row[0])).map(row=>row[0]));for(const throwAt of names)cases.push({...request,throwAt});}
const input=cases.map(value=>JSON.stringify(value)).join('\n')+'\n',expected=cases.map(run),digest=value=>createHash('sha256').update(value).digest('hex');
fs.writeFileSync(path.join(directory,'input.jsonl'),input);fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2)+'\n');
const summary={status:'oracle-only-uncompiled',cases:cases.length,node:process.version,v8:process.versions.v8,upstreamCommit:'11738a9d55e502fb484cefffcea3a66dc44e9fe9',absoluteComparison:true,relativeIdNormalization:false,inputSha256:digest(input),expectedSha256:digest(fs.readFileSync(path.join(directory,'expected.json'))),categories:Object.fromEntries(['metadata','construct','numeric','arrayNumeric','protocol'].map(kind=>[kind,cases.filter(value=>value.kind===kind).length])),boundaries:['CPU source protocols and real managed Plane/Sphere/Matrix algorithms; no GPU execution is claimed.','Protocol host methods are explicit arbitrary-receiver fixtures; numeric scenarios separately invoke real class algorithms.','Special numeric values use matching lossless JSON tags; no identity/id fields are normalized.']};
const freshImports=[];for(const family of ['frustum','array']){
 const child=spawnSync(process.execPath,['--expose-gc',fileURLToPath(import.meta.url),'--fresh-import',upstream,family],{encoding:'utf8',maxBuffer:1024*1024});fs.writeFileSync(path.join(directory,`fresh-import-${family}-expected.json`),child.stdout??'');fs.writeFileSync(path.join(directory,`fresh-import-${family}-oracle-stderr.log`),child.stderr??'');assert.equal(child.status,0,child.stderr);const expected=JSON.parse(child.stdout),record={family,expected,oracleExitCode:child.status};
 if(binary){const native=spawnSync(binary,['--fresh-import',family],{encoding:'utf8',maxBuffer:1024*1024});fs.writeFileSync(path.join(directory,`fresh-import-${family}-actual.json`),native.stdout??'');fs.writeFileSync(path.join(directory,`fresh-import-${family}-native-stderr.log`),native.stderr??'');record.nativeExitCode=native.status;if(native.status===0){record.actual=JSON.parse(native.stdout);try{assert.deepEqual(record.actual,expected);record.equal=true;}catch{record.equal=false;}}else record.equal=false;}
 freshImports.push(record);
}
fs.writeFileSync(path.join(directory,'fresh-import.json'),JSON.stringify(freshImports,null,2)+'\n');summary.freshImports=freshImports.length;
if(binary){
 summary.binarySha256=digest(fs.readFileSync(binary));const child=spawnSync(binary,[],{input,encoding:'utf8',maxBuffer:128*1024*1024});fs.writeFileSync(path.join(directory,'actual.jsonl'),child.stdout??'');fs.writeFileSync(path.join(directory,'native-stderr.log'),child.stderr??'');summary.nativeExitCode=child.status;
 if(child.status!==0){summary.status='native-process-failed';fs.writeFileSync(path.join(directory,'summary.json'),JSON.stringify(summary,null,2)+'\n');assert.equal(child.status,0,String(child.error??child.stderr));}
 const actual=child.stdout.trimEnd().split(/\r?\n/).map(JSON.parse),differences=[];assert.equal(actual.length,cases.length);for(let index=0;index<cases.length;++index)try{assert.deepEqual(actual[index],expected[index]);}catch{differences.push({index,input:cases[index],expected:expected[index],actual:actual[index]});}
 fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2)+'\n');fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(differences,null,2)+'\n');summary.differences=differences.length;
 const fresh=spawnSync(binary,['--fresh-gc'],{encoding:'utf8',maxBuffer:1024*1024});fs.writeFileSync(path.join(directory,'fresh-gc-stdout.json'),fresh.stdout??'');fs.writeFileSync(path.join(directory,'fresh-gc-stderr.log'),fresh.stderr??'');summary.freshGcExitCode=fresh.status;
 summary.status=differences.length?'native-differences':fresh.status===0?'native-passed':'fresh-gc-failed';summary.freshImportsEqual=freshImports.every(value=>value.equal);if(!summary.freshImportsEqual)summary.status='fresh-import-differences';fs.writeFileSync(path.join(directory,'summary.json'),JSON.stringify(summary,null,2)+'\n');assert.equal(differences.length,0,JSON.stringify(differences.slice(0,2),null,2));assert.equal(fresh.status,0,fresh.stderr);assert.deepEqual(JSON.parse(fresh.stdout),[true,true,true]);assert(summary.freshImportsEqual,'fresh-import.json retains true import differences');
}else fs.writeFileSync(path.join(directory,'summary.json'),JSON.stringify(summary,null,2)+'\n');
console.log(JSON.stringify(summary));
