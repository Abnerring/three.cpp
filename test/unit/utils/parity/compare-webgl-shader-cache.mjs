import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
import {writeFileSync} from 'node:fs';
const upstream=process.argv[2],probe=process.argv[3];
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const {WebGLShaderCache}=await import(pathToFileURL(`${upstream}/src/renderers/webgl/WebGLShaderCache.js`));
const encode=v=>v===undefined?'undefined':typeof v==='symbol'?'symbol:'+(v.description??''):typeof v==='function'?'function':typeof v==='number'?Number.isNaN(v)?'NaN':Object.is(v,-0)?'-0':v:v!==null&&typeof v==='object'?'object':v;
const outcome=f=>{try{return f()}catch(e){return e.name}};
const descriptor=(object,key)=>{const d=Object.getOwnPropertyDescriptor(object,key);return d?{writable:d.writable??false,enumerable:d.enumerable??false,configurable:d.configurable??false}:null};
const expected=[],emit=v=>expected.push(v);
{
 const constructor=WebGLShaderCache,manager=new constructor(),prototype=constructor.prototype,stage=manager._getShaderStage('metadata'),stageConstructor=stage.constructor;
 const methods=Object.getOwnPropertyNames(prototype).map(key=>{const method=prototype[key];return {key,name:encode(method.name),length:encode(method.length),descriptor:descriptor(prototype,key),construct:outcome(()=>{new method();return 'ok'})}});
 emit({kind:'metadata',name:constructor.name,length:constructor.length,prototype:descriptor(constructor,'prototype'),own:Object.getOwnPropertyNames(manager),classCall:outcome(()=>{constructor();return 'ok'}),methods,stageName:stageConstructor.name,stageLength:stageConstructor.length,stageOwn:Object.getOwnPropertyNames(stage),stagePrototype:Object.getOwnPropertyNames(stageConstructor.prototype),stageId:encode(stage.id),stageClassCall:outcome(()=>{stageConstructor();return 'ok'}),maps:manager.shaderCache instanceof Map&&manager.materialCache instanceof Map});
}
const sameValue=(a,b)=>Object.is(a,b);
const locate=(v,values)=>{const i=values.findIndex(x=>sameValue(x,v));return i<0?999:i};
const snapshot=(manager,materials,codes,stages)=>({shaders:[...manager.shaderCache.entries()].map(([code,stage])=>[locate(code,codes),encode(stage.id),encode(stage.usedTimes)]),materials:[...manager.materialCache.entries()].map(([material,set])=>[locate(material,materials),[...set].map(stage=>encode(stage.id))]),saved:stages.map(stage=>[encode(stage.id),locate(stage.code,codes),encode(stage.usedTimes)])});
for(let seed=0;seed<48;seed++){
 let rng=seed+1;const random=()=>rng=(Math.imul(rng,1664525)+1013904223)>>>0,manager=new WebGLShaderCache(),codes=[undefined,null,'alpha','beta',0,-0,NaN,3,{},Symbol('code')],materials=Array.from({length:6},(_,i)=>({vertexShader:codes[i],fragmentShader:codes[(i+2)%codes.length]})),stages=[];
 for(let step=0;step<160;step++){
  const op=random()%9,m=random()%materials.length,c=random()%codes.length,d=random()%codes.length,material=materials[m];
  const result=outcome(()=>{switch(op){
   case 0:{const stage=manager.getVertexShaderStage(material);stages.push(stage);return encode(stage.id)}
   case 1:{const stage=manager.getFragmentShaderStage(material);stages.push(stage);return encode(stage.id)}
   case 2:{const a=manager._getShaderStage(codes[c]),b=manager._getShaderStage(codes[d]);stages.push(a,b);return manager.update(material,a,b)===manager}
   case 3:return manager.remove(material)===manager;
   case 4:material.vertexShader=codes[c];material.fragmentShader=codes[d];return 'ok';
   case 5:return manager._getShaderCacheForMaterial(material) instanceof Set;
   case 6:return encode(manager.dispose());
   case 7:{const stage=manager._getShaderStage(codes[c]);stages.push(stage);return encode(stage.id)}
   case 8:{if(!stages.length)return 'empty';const stage=stages[random()%stages.length];stage.usedTimes=c%3-1;return 'ok'}
  }});
  emit({kind:'random',seed,step,operation:op,result,state:snapshot(manager,materials,codes,stages)});
 }
}
const traceGetter=(log,label,result,error=0)=>function(){log.push(label);if(error===1)throw RangeError('getter');if(error===2)throw TypeError('getter');return result};
const traceMethod=(log,label,result,error=0)=>function(){log.push(label);if(error===1)throw RangeError('method');if(error===2)throw TypeError('method');return result};
const accessor=(object,key,get,set=undefined)=>Object.defineProperty(object,key,{get,set,enumerable:true,configurable:true});
for(let has=0;has<8;has++)for(let numeric=0;numeric<9;numeric++)for(let reused=0;reused<2;reused++){
 const log=[],stored={},shaders={},manager=new WebGLShaderCache(),hasResults=[false,true,0,'',undefined,null,NaN,{}];
 const number={[Symbol.toPrimitive](hint){log.push('hint:'+hint);return 3}},numbers=[undefined,null,false,3,'2',NaN,-0,Symbol('count'),number],vertex={},fragment={};
 for(const [stage,label]of [[vertex,'vertex'],[fragment,'fragment']])accessor(stage,'usedTimes',traceGetter(log,'get:'+label,numbers[numeric]),function(value){log.push('set:'+label);stored[label]=value});
 accessor(shaders,'has',traceGetter(log,'get:has',traceMethod(log,'call:has',hasResults[has])));
 accessor(shaders,'add',traceGetter(log,'get:add',traceMethod(log,'call:add',undefined)));
 accessor(manager,'_getShaderCacheForMaterial',traceGetter(log,'get:material-helper',traceMethod(log,'call:material-helper',shaders)));
 const result=outcome(()=>manager.update({},vertex,reused?vertex:fragment)===manager);
 emit({kind:'update-protocol',has,numeric,reused,result,log,vertex:encode(stored.vertex),fragment:encode(stored.fragment)});
}
for(let which=0;which<2;which++)for(let mode=0;mode<12;mode++){
 const manager=new WebGLShaderCache(),log=[],material={},key=which?'fragmentShader':'vertexShader';
 const helper=(name,value)=>function(code){log.push(name,'arg:'+String(code));return value},old=helper('old-helper',17),replacement=helper('new-helper',23);
 accessor(manager,'_getShaderStage',function(){log.push('get:helper');if(mode===1)throw RangeError('helper');if(mode===2)return 7;if(mode===3)Object.defineProperty(material,key,{value:'changed',writable:true,enumerable:true,configurable:true});return old});
 accessor(material,key,function(){log.push('get:shader');if(mode===4)throw RangeError('shader');if(mode===5)Object.defineProperty(manager,'_getShaderStage',{value:replacement,writable:true,enumerable:true,configurable:true});return 'code'});
 const receiver=mode>=6?[undefined,null,false,3,'',Symbol('material')][mode-6]:material;
 const result=outcome(()=>encode(manager[which?'getFragmentShaderStage':'getVertexShaderStage'](receiver)));
 emit({kind:'lookup-protocol',which,mode,result,log});
}
for(let mode=0;mode<25;mode++){
 const log=[],state={next:0,count:1,reads:0},manager=new WebGLShaderCache(),stage={code:'old-code'},iterator={},iterable={},materialMap={},shaderMap={},otherMap={};
 const erase=function(code){log.push('call:shader-delete','code:'+String(code));return false};
 accessor(otherMap,'delete',traceGetter(log,'get:other-delete',traceMethod(log,'call:other-delete',false)));
 accessor(shaderMap,'delete',function(){log.push('get:shader-delete');if(mode===16)throw RangeError('delete');if(mode===21)Object.defineProperty(stage,'code',{value:'changed-code',writable:true,enumerable:true,configurable:true});if(mode===22)Object.defineProperty(manager,'materialCache',{value:otherMap,writable:true,enumerable:true,configurable:true});return erase});
 accessor(materialMap,'get',traceGetter(log,'get:material-get',traceMethod(log,'call:material-get',iterable)));
 accessor(materialMap,'delete',traceGetter(log,'get:material-delete',traceMethod(log,'call:material-delete',false),mode===17?1:0));
 accessor(manager,'materialCache',traceGetter(log,'get:materialCache',materialMap));
 accessor(manager,'shaderCache',traceGetter(log,'get:shaderCache',shaderMap));
 Object.defineProperty(iterable,Symbol.iterator,{get(){log.push('get:iterator');if(mode===1)throw RangeError('iterator');return function(){log.push('call:iterator');return mode===2?7:iterator}}});
 accessor(iterator,'next',function(){log.push('get:next');if(mode===3)throw RangeError('next-get');return function(){log.push('call:next');if(mode===4)throw RangeError('next-call');if(mode===5)return 7;const cursor=state.next++;const result={};accessor(result,'done',traceGetter(log,'get:done',cursor>0,mode===6?1:0));accessor(result,'value',traceGetter(log,'get:value',stage,mode===7?1:0));return result}});
 accessor(iterator,'return',function(){log.push('get:return');if(mode===12)throw TypeError('return-get');if(mode===18)return null;return function(){log.push('call:return');if(mode===13)throw TypeError('return-call');return mode===14?7:{}}});
 accessor(stage,'usedTimes',function(){log.push('get:count');const read=state.reads++;if([8,12,13,14,18].includes(mode))throw RangeError('count');if(mode===9)return Symbol('count');if(mode===23&&read===1)return '0';if(mode===24&&read===1)return -0;return state.count},function(value){log.push('set:count');if(mode===10)throw RangeError('count-set');state.count=value;if(mode===20)Object.defineProperty(manager,'shaderCache',{value:otherMap,writable:true,enumerable:true,configurable:true})});
 if(mode===15)accessor(stage,'code',traceGetter(log,'get:code',undefined,1));
 const result=outcome(()=>manager.remove({})===manager);
 emit({kind:'remove-protocol',mode,result,log,count:encode(state.count),next:encode(state.next),reads:encode(state.reads)});
}
for(let which=0;which<2;which++)for(let mode=0;mode<11;mode++){
 const manager=new WebGLShaderCache(),log=[],cache={},other={},stored={},field=which?'shaderCache':'materialCache',resultValue=mode<5?[undefined,null,false,7,{}][mode]:undefined;
 accessor(cache,'get',function(){log.push('get:get');if(mode===5)throw RangeError('get');if(mode===6)Object.defineProperty(manager,field,{value:other,writable:true,enumerable:true,configurable:true});return function(key){log.push('call:get','key:'+String(key));if(mode===7)throw RangeError('get-call');return resultValue}});
 accessor(cache,'set',function(){log.push('get:set');if(mode===8)throw RangeError('set');if(mode===9)return 7;return function(key,value){log.push('call:set');stored.key=key;stored.value=value;if(mode===10)throw RangeError('set-call');return false}});
 accessor(manager,field,traceGetter(log,'get:cache',cache));
 const result=outcome(()=>encode(manager[which?'_getShaderStage':'_getShaderCacheForMaterial']('code'))),value=stored.value;
 emit({kind:'cache-protocol',which,mode,result,log,stored:encode(value),id:value!==null&&typeof value==='object'?encode(value.id):'none',set:value instanceof Set});
}
for(let mode=0;mode<8;mode++){
 const manager=new WebGLShaderCache(),log=[],shader={},material={},other={};
 accessor(other,'clear',traceGetter(log,'get:other-clear',traceMethod(log,'call:other-clear',undefined)));
 accessor(shader,'clear',function(){log.push('get:shader-clear');if(mode===1)throw RangeError('shader-clear');if(mode===6)Object.defineProperty(manager,'materialCache',{value:other,writable:true,enumerable:true,configurable:true});return function(){log.push('call:shader-clear');if(mode===2)throw RangeError('shader-clear-call');if(mode===7)Object.defineProperty(manager,'materialCache',{value:other,writable:true,enumerable:true,configurable:true})}});
 accessor(material,'clear',traceGetter(log,'get:material-clear',traceMethod(log,'call:material-clear',undefined,mode===5?1:0),mode===4?1:0));
 accessor(manager,'shaderCache',traceGetter(log,'get:shaderCache',shader,mode===0?1:0));
 accessor(manager,'materialCache',traceGetter(log,'get:materialCache',material,mode===3?1:0));
 const result=outcome(()=>encode(manager.dispose()));emit({kind:'dispose-protocol',mode,result,log});
}
{
 const manager=new WebGLShaderCache(),exemplar=manager._getShaderStage('stage-prototype'),prototype=exemplar.constructor.prototype;
 for(let mode=0;mode<7;mode++){
  const log=[],state={entered:false},cache=new WebGLShaderCache();for(const name of ['id','code','usedTimes'])delete prototype[name];
  accessor(prototype,'id',undefined,function(value){log.push('set:id',value);if(mode===1)throw RangeError('id');if(mode===2&&!state.entered){state.entered=true;cache._getShaderStage('nested-stage')}if(mode===3)Object.defineProperty(this,'code',{value:'before-code',writable:true,enumerable:true,configurable:true});Object.defineProperty(this,'id',{value,writable:true,enumerable:true,configurable:true})});
  accessor(prototype,'code',undefined,function(value){log.push('set:code');if(mode===4)throw RangeError('code');Object.defineProperty(this,'code',{value,writable:true,enumerable:true,configurable:true})});
  accessor(prototype,'usedTimes',undefined,function(value){log.push('set:usedTimes');if(mode===5)throw RangeError('usedTimes');Object.defineProperty(this,'usedTimes',{value,writable:true,enumerable:true,configurable:true})});
  if(mode===6)cache.shaderCache.set=traceMethod(log,'cache:set',undefined,1);
  const result=outcome(()=>{const stage=cache._getShaderStage('code');return {id:encode(stage.id),code:encode(stage.code),usedTimes:encode(stage.usedTimes)}}),sizeBefore=cache.shaderCache.size;
  for(const name of ['id','code','usedTimes'])delete prototype[name];
  if(mode===6)delete cache.shaderCache.set;
  const next=cache._getShaderStage('after');emit({kind:'stage-construction',mode,result,log,sizeBefore,nextId:encode(next.id)});
 }
}
for(let mode=0;mode<8;mode++){
 const constructor=WebGLShaderCache,log=[],stored={entered:false},prototype=Object.create(constructor.prototype),newTarget=function Derived(){};
 accessor(prototype,'shaderCache',undefined,function(value){log.push('set:shaderCache',value instanceof Map);if(mode===1)throw RangeError('shader');if(mode===2)WebGLShaderCache.prototype.dispose.call(this);if(mode===3)Object.defineProperty(this,'materialCache',{value:'before',writable:true,enumerable:true,configurable:true});Object.defineProperty(this,'shaderCache',{value,writable:true,enumerable:true,configurable:true})});
 accessor(prototype,'materialCache',undefined,function(value){log.push('set:materialCache',value instanceof Map);if(mode===4)throw RangeError('material');if(mode===5&&!stored.entered){stored.entered=true;Reflect.construct(WebGLShaderCache,[],newTarget)}Object.defineProperty(this,'materialCache',{value,writable:true,enumerable:true,configurable:true})});
 newTarget.prototype=mode===6?null:mode===7?3:prototype;
 const result=outcome(()=>{const value=Reflect.construct(constructor,[],newTarget);return {own:Object.getOwnPropertyNames(value),shader:value.shaderCache instanceof Map,material:value.materialCache instanceof Map,derived:Object.getPrototypeOf(value)===prototype}});
 emit({kind:'constructor-protocol',mode,result,log});
}
for(let mode=0;mode<8;mode++){
 const manager=new WebGLShaderCache(),material={},log=[],state={entered:false,count:0},vertex=manager._getShaderStage('vertex'),fragment=manager._getShaderStage('fragment'),shaders=manager._getShaderCacheForMaterial(material);
 accessor(vertex,'usedTimes',function(){log.push('get:vertex');if(!state.entered){state.entered=true;if(mode===0)manager.dispose();if(mode===1)shaders.clear();if(mode===2)shaders.add(fragment);if(mode===3)manager.remove(material);if(mode===4)manager.update(material,vertex,fragment);if(mode===5)throw RangeError('vertex')}return state.count},function(value){log.push('set:vertex');if(mode===6)throw RangeError('vertex-set');state.count=value;if(mode===7)manager.dispose()});
 const result=outcome(()=>manager.update(material,vertex,fragment)===manager);
 emit({kind:'reentry',mode,result,log,vertex:encode(state.count),fragment:encode(fragment.usedTimes),shaderSize:manager.shaderCache.size,materialSize:manager.materialCache.size,setCodes:[...shaders].map(stage=>encode(stage.code))});
}
{
 const {ShaderMaterial}=await import(pathToFileURL(`${upstream}/src/materials/ShaderMaterial.js`)),{RawShaderMaterial}=await import(pathToFileURL(`${upstream}/src/materials/RawShaderMaterial.js`));
 const shader=new ShaderMaterial(),raw=new RawShaderMaterial(),material=shader,rawMaterial=raw,manager=new WebGLShaderCache(),vertex=manager.getVertexShaderStage(material),fragment=manager.getFragmentShaderStage(material);
 manager.update(material,vertex,fragment);manager.update(rawMaterial,vertex,fragment);
 emit({kind:'native-material',mode:0,vertexCode:vertex.code,fragmentCode:fragment.code,vertexId:vertex.id,fragmentId:fragment.id,rawSame:manager.getVertexShaderStage(rawMaterial)===vertex&&manager.getFragmentShaderStage(rawMaterial)===fragment,vertexCount:vertex.usedTimes,fragmentCount:fragment.usedTimes});
 shader.vertexShader='vertex-b';material.fragmentShader='fragment-b';const v2=manager.getVertexShaderStage(material),f2=manager.getFragmentShaderStage(material);manager.update(material,v2,f2);
 emit({kind:'native-material',mode:1,vertexCode:v2.code,fragmentCode:f2.code,vertexId:v2.id,fragmentId:f2.id,nativeVertex:shader.vertexShader,nativeFragment:shader.fragmentShader,identity:material===shader,vertexCount:v2.usedTimes,fragmentCount:f2.usedTimes});
 manager.remove(material);manager.remove(rawMaterial);
 emit({kind:'native-material',mode:2,vertexCount:vertex.usedTimes,fragmentCount:fragment.usedTimes,newVertexCount:v2.usedTimes,newFragmentCount:f2.usedTimes,shaderSize:manager.shaderCache.size,materialSize:manager.materialCache.size});
}
const actual=execFileSync(probe,{encoding:'utf8',maxBuffer:128*1024*1024}).trim().split(/\r?\n/).map(JSON.parse);
if(process.argv[4]){writeFileSync(`${process.argv[4]}-node.json`,JSON.stringify(expected,null,2));writeFileSync(`${process.argv[4]}-native.json`,JSON.stringify(actual,null,2));}
assert.equal(actual.length,expected.length);
for(let i=0;i<actual.length;i++)assert.deepEqual(actual[i],expected[i],`scenario ${i}`);
let comparisons=0;const count=x=>{comparisons++;if(x&&typeof x==='object')for(const v of Object.values(x))count(v)};expected.forEach(count);
console.log(`WebGLShaderCache: ${expected.length} actual pinned source scenarios / ${comparisons} comparisons passed (${process.version}).`);
