import {upstreamModule} from './threejs-source.mjs';
const {Scene}=await upstreamModule('src/scenes/Scene.js'),{BoxGeometry}=await upstreamModule('src/geometries/BoxGeometry.js'),{Object3D}=await upstreamModule('src/core/Object3D.js'),{BufferGeometry}=await upstreamModule('src/core/BufferGeometry.js');
export const constructors={Scene,BoxGeometry};
const scalar=v=>v===undefined?'#undefined':typeof v==='function'?'#function':typeof v==='symbol'?'#symbol':typeof v==='number'?(Number.isNaN(v)?'#NaN':v===Infinity?'#Infinity':v===-Infinity?'#-Infinity':Object.is(v,-0)?'#-0':v):v;
const error=e=>e instanceof Error?'#'+e.name:{thrown:scalar(e)};
function normal(v,seen=new Set()){
 if(v===null||typeof v!=='object')return scalar(v);if(seen.has(v))return '#cycle';seen=new Set(seen);seen.add(v);
 if(v.isEuler||v.isVector3||v.isQuaternion)return v.toArray().map(scalar);
 if(Array.isArray(v)||ArrayBuffer.isView(v))return Array.from(v,x=>normal(x,seen));
 const out={};for(const key of Object.keys(v))out[key]=key==='uuid'?'#uuid':normal(v[key],seen);return out;
}
function decode(v,s){if(v===null||typeof v!=='object')return v;if(v.$undefined)return undefined;if(v.$symbol)return Symbol(v.$symbol);if(v.$number)return v.$number==='-0'?-0:Number(v.$number);if(v.$coerce)return {[Symbol.toPrimitive](hint){s.events.push('convert:'+v.$coerce.label+':'+hint);if(v.$coerce.throw)throw 43;return decode(v.$coerce.value,s);}};if(Array.isArray(v))return v.map(x=>decode(x,s));return Object.fromEntries(Object.entries(v).map(([k,x])=>[k,decode(x,s)]));}
function descriptor(d){return Object.fromEntries(Object.entries(d).map(([k,v])=>[k,scalar(v)]));}
function metadata(C){const method=(home,key)=>{const d=Object.getOwnPropertyDescriptor(home,key),f=d.value??d.get??d.set;return {key,descriptor:descriptor(d),name:f.name,length:f.length,source:Function.prototype.toString.call(f)};};return {name:C.name,length:C.length,source:Function.prototype.toString.call(C),constructorKeys:Reflect.ownKeys(C),prototypeKeys:Reflect.ownKeys(C.prototype),methods:Reflect.ownKeys(C.prototype).filter(k=>k!=='constructor').map(k=>method(C.prototype,k)),statics:Reflect.ownKeys(C).filter(k=>!['length','name','prototype'].includes(k)).map(k=>method(C,k))};}
function sceneSnapshot(v){const out={keys:Object.keys(v)};for(const key of ['type','isScene','background','environment','fog','backgroundBlurriness','backgroundIntensity','backgroundRotation','environmentIntensity','environmentRotation','overrideMaterial','matrixAutoUpdate','name','userData','position','rotation','quaternion','scale'])out[key]=normal(v[key]);out.children=v.children.map(c=>({type:c.type,name:c.name,position:normal(c.position)}));return out;}
function attribute(a,compact){if(a===null||a===undefined)return scalar(a);let values=Array.from(a.array,scalar);if(compact&&values.length>40)values=[...values.slice(0,20),'#middle',...values.slice(-20)];return {arrayType:a.array.constructor.name,length:a.array.length,itemSize:a.itemSize,count:a.count,normalized:a.normalized,array:values};}
function boxSnapshot(v,compact=false){return {keys:Object.keys(v),type:v.type,isBufferGeometry:v.isBufferGeometry,parameters:normal(v.parameters),groups:normal(v.groups),index:attribute(v.index,compact),attributes:Object.fromEntries(Object.entries(v.attributes).map(([k,a])=>[k,attribute(a,compact)]))};}
function observe(object,key,s,label=key){let value=object[key];const d=Object.getOwnPropertyDescriptor(object,key);Object.defineProperty(object,key,{configurable:true,enumerable:d?.enumerable??true,get(){s.events.push('get:'+label);if(s.q.throwAt==='get:'+label)throw 41;return value;},set(v){s.events.push('set:'+label);if(s.q.throwAt==='set:'+label)throw 41;value=v;}});return()=>value;}
function withHome(C,method,s,callback){const home=C.prototype,previous=Object.getPrototypeOf(home),replacement=Object.create(previous);Object.defineProperty(replacement,method,{configurable:true,get(){s.events.push('super.get:'+method+':'+(this===s.self));if(s.q.throwAt==='super.get')throw 44;return function(...a){s.events.push(['super.call:'+method,this===s.self,...a.map(x=>x===s.source?'#source':scalar(x))]);if(s.q.throwAt==='super.call')throw 44;return method==='toJSON'?{object:{tag:'super'}}:81;};}});Object.setPrototypeOf(home,replacement);try{return callback();}finally{Object.setPrototypeOf(home,previous);}}
function devtools(q){const events=[],oldDev=Object.getOwnPropertyDescriptor(globalThis,'__THREE_DEVTOOLS__'),oldEvent=Object.getOwnPropertyDescriptor(globalThis,'CustomEvent');let reads=0,entered=false;
 const dispatch=function(e){events.push(['dispatch',this===tools,e.type,e.detail.isScene]);if(q.mode==='dispatchThrow')throw 46;if(q.mode==='reentry'&&!entered){entered=true;new Scene();}};
 const tools={};Object.defineProperty(tools,'dispatchEvent',{get(){events.push('get:dispatch');if(q.mode==='dispatchGetThrow')throw 45;return dispatch;}});
 try{if(q.mode==='absent')delete globalThis.__THREE_DEVTOOLS__;else Object.defineProperty(globalThis,'__THREE_DEVTOOLS__',{configurable:true,get(){events.push('get:devtools:'+(++reads));if(q.mode==='globalThrow'+reads)throw 47;return q.mode==='undefined'?undefined:q.mode==='null'?null:tools;}});
 if(q.mode==='missingEvent')delete globalThis.CustomEvent;else Object.defineProperty(globalThis,'CustomEvent',{configurable:true,value:function CustomEvent(type,init){events.push(['event',type,init.detail.isScene]);if(q.mode==='eventThrow')throw 48;this.type=type;this.detail=init.detail;}});
 const out={};try{out.value=sceneSnapshot(new Scene());}catch(e){out.error=error(e);}out.events=events;return out;
 }finally{if(oldDev)Object.defineProperty(globalThis,'__THREE_DEVTOOLS__',oldDev);else delete globalThis.__THREE_DEVTOOLS__;if(oldEvent)Object.defineProperty(globalThis,'CustomEvent',oldEvent);else delete globalThis.CustomEvent;}}
export function run(q){const s={q,events:[]},out={},C=constructors[q.class];
 if(q.kind==='metadata')return metadata(C);
 if(q.kind==='module'){const facade={[q.class]:C};facade[q.class]=null;return {facade:facade[q.class],same:C===constructors[q.class]};}
 if(q.kind==='newTarget'){function Target(){}Target.prototype=Object.create(C.prototype);const value=Reflect.construct(C,[],Target);return {prototype:Object.getPrototypeOf(value)===Target.prototype,ownConstructor:Object.hasOwn(value,'constructor'),value:q.class==='Scene'?sceneSnapshot(value):boxSnapshot(value)};}
 if(q.kind==='callWithoutNew'){try{C();}catch(e){out.error=error(e);}return out;}
 if(q.kind==='devtools')return devtools(q);
 if(q.kind==='sceneConstructor')return sceneSnapshot(new Scene(...(q.args??[])));
 if(q.kind==='boxConstructor'||q.kind==='boxHooks'||q.kind==='fromJSON'){
  const old=new Map(),proto=BoxGeometry.prototype;let group=0,entered=false;
  if(q.kind==='boxHooks')for(const key of ['addGroup','setIndex','setAttribute']){old.set(key,Object.getOwnPropertyDescriptor(proto,key));const original=proto[key];Object.defineProperty(proto,key,{configurable:true,get(){s.events.push('get:'+key);if(q.throwAt==='get:'+key)throw 42;return function(...a){const label='call:'+key+(key==='addGroup'?':'+group++:key==='setAttribute'?':'+a[0]:'');s.events.push([label,...(key==='addGroup'?a:key==='setIndex'?[a[0].length,a[0].slice(0,6)]:[a[0],a[1].count])]);if(q.throwAt===label)throw 42;if(q.reenter&&key==='addGroup'&&!entered){entered=true;out.nested=boxSnapshot(new BoxGeometry(2,3,4,1,2,1));}return original.apply(this,a);};}});}
  try{let value;if(q.kind==='fromJSON'){const data=decode(q.data,s);if(data!==null&&typeof data==='object')for(const k of ['width','height','depth','widthSegments','heightSegments','depthSegments'])observe(data,k,s,'data.'+k);value=BoxGeometry.fromJSON.call(q.foreignThis?function Foreign(){}:BoxGeometry,data);}else value=new BoxGeometry(...(q.args??[]).map(v=>decode(v,s)));out.value=boxSnapshot(value,!!q.compact);if(q.json)out.json=normal(value.toJSON());}catch(e){out.error=error(e);}finally{for(const[k,d]of old)if(d)Object.defineProperty(proto,k,d);else delete proto[k];}out.events=s.events;return out;
 }
 if(q.kind==='boxCopy'){
  const value=new BoxGeometry(),source=new BoxGeometry(2,3,4,2,1,3),marker={tag:'shared'},params=Object.create({inherited:8});params.width=2;params.nested=marker;Object.defineProperty(params,'height',{enumerable:true,get(){s.events.push('get:parameters.height');if(q.throwAt)throw 49;return 3;}});source.parameters=q.paramsNull?null:params;
  try{const r=value.copy(source);out.same=r===value;out.parametersSame=value.parameters===source.parameters;out.nestedSame=value.parameters.nested===marker;out.positionSame=value.attributes.position===source.attributes.position;out.positionArraySame=value.attributes.position.array===source.attributes.position.array;}catch(e){out.error=error(e);}out.value=boxSnapshot(value);out.events=s.events;return out;
 }
 if(q.kind==='sceneCopy'||q.kind==='sceneJSON'){
  const value=new Scene(),source=new Scene();s.self=value;s.source=source;source.name='source';source.position.set(1,2,3);source.rotation.set(.2,.3,-.1);source.backgroundBlurriness=.4;source.backgroundIntensity=2;source.environmentIntensity=3;source.backgroundRotation.set(.1,.2,.3,'ZYX');source.environmentRotation.set(.4,.5,.6,'YXZ');source.matrixAutoUpdate=false;const child=new Object3D();child.name='child';child.position.set(4,5,6);source.add(child);
  for(const key of ['background','environment','fog','overrideMaterial']){const resource={tag:key};resource.clone=function(){s.events.push('clone:'+key);if(q.throwAt==='clone:'+key)throw 50;return {tag:key+'.clone'};};resource.toJSON=function(){s.events.push('json:'+key);if(q.throwAt==='json:'+key)throw 51;return {type:'Fixture',tag:key};};if(q.resources)source[key]=resource;}
  const target=q.kind==='sceneJSON'?source:value;s.self=target;
  for(const key of q.observed??[])observe(q.observeSource?source:target,key,s,(q.observeSource?'source.':'self.')+key);
  const execute=()=>q.kind==='sceneCopy'?value.copy(source,decode(q.recursive,s)):target.toJSON(decode(q.meta,s));
  try{const result=q.superHome?withHome(Scene,q.kind==='sceneCopy'?'copy':'toJSON',s,execute):execute();out.result=result===target?'#self':normal(result);}catch(e){out.error=error(e);}out.events=s.events;
  // Observing getters should describe only source execution, not verification reads.
  s.events=[];out.value=sceneSnapshot(target);out.source=sceneSnapshot(source);return out;
 }
 throw Error('Unknown fixture kind '+q.kind);
}
export function fixtures(){const out=[],add=(name,q)=>out.push({name,...q});
 for(const cls of ['Scene','BoxGeometry'])for(const kind of ['metadata','module','newTarget','callWithoutNew'])add(cls+'.'+kind,{class:cls,kind});
 add('Scene.constructor',{class:'Scene',kind:'sceneConstructor'});add('Scene.constructor.extra',{class:'Scene',kind:'sceneConstructor',args:[1,2,3]});
 const args=[[],[2,3,4],[2,3,4,2,3,4],[-2,3,-4,2,1,3],[0,0,0],[1,2,3,0,1,2],[1,2,3,-1,2,1],[1,2,3,1.8,2.9,3.1],['2','3','4','2','1','3'],[null,null,null,null,null,null],[1,2,3,{$number:'NaN'},1,1],[1,2,3,1,{$number:'-Infinity'},1],[{$number:'-0'},1,2],[1,{$number:'Infinity'},3],[1,2,{$number:'NaN'}]];
 for(const[a,i]of args.map((v,i)=>[v,i]))add('Box.constructor.'+i,{class:'BoxGeometry',kind:'boxConstructor',args:a,json:true});
 for(let slot=0;slot<6;slot++)for(const token of [{$undefined:true},true,'2',{$coerce:{label:'arg'+slot,value:'2'}},{$coerce:{label:'arg'+slot,value:2,throw:true}},{$symbol:'argument'}]){const a=[1,2,3,1,1,1];a[slot]=token;add('Box.coerce.'+slot+'.'+JSON.stringify(token),{class:'BoxGeometry',kind:'boxConstructor',args:a});}
 add('Box.uint32',{class:'BoxGeometry',kind:'boxConstructor',args:[1,2,3,181,181,1],compact:true});
 for(const throwAt of [undefined,'get:addGroup','call:addGroup:0','call:addGroup:2','call:addGroup:5','get:setIndex','call:setIndex','get:setAttribute','call:setAttribute:position','call:setAttribute:normal','call:setAttribute:uv'])add('Box.hooks.'+throwAt,{class:'BoxGeometry',kind:'boxHooks',args:[2,3,4,2,3,1],throwAt});
 add('Box.hooks.reentry',{class:'BoxGeometry',kind:'boxHooks',args:[3,4,5,2,1,3],reenter:true});
 for(const data of [{},{width:2,height:3,depth:4,widthSegments:2,heightSegments:3,depthSegments:1},{width:'2',height:null,depth:-3,widthSegments:'2'},{width:{$coerce:{label:'width',value:2}},height:3,depth:4},null,{$undefined:true}])add('Box.fromJSON.'+JSON.stringify(data),{class:'BoxGeometry',kind:'fromJSON',data,foreignThis:true});
 for(const key of ['width','height','depth','widthSegments','heightSegments','depthSegments'])add('Box.fromJSON.throw.'+key,{class:'BoxGeometry',kind:'fromJSON',data:{width:2,height:3,depth:4},throwAt:'get:data.'+key});
 for(const q of [{},{paramsNull:true},{throwAt:'height'}])add('Box.copy.'+JSON.stringify(q),{class:'BoxGeometry',kind:'boxCopy',...q});
 for(const mode of ['absent','undefined','null','plain','globalThrow1','globalThrow2','dispatchGetThrow','missingEvent','eventThrow','dispatchThrow','reentry'])add('Scene.devtools.'+mode,{class:'Scene',kind:'devtools',mode});
 for(const recursive of [{$undefined:true},true,false,null,0,'yes'])for(const resources of [false,true])add('Scene.copy.'+JSON.stringify(recursive)+'.'+resources,{class:'Scene',kind:'sceneCopy',recursive,resources});
 for(const key of ['background','environment','fog','overrideMaterial'])add('Scene.copy.throw.'+key,{class:'Scene',kind:'sceneCopy',resources:true,throwAt:'clone:'+key});
 const copyFields=['background','environment','fog','backgroundBlurriness','backgroundIntensity','backgroundRotation','environmentIntensity','environmentRotation','overrideMaterial','matrixAutoUpdate'];
 add('Scene.copy.observed',{class:'Scene',kind:'sceneCopy',resources:true,observeSource:true,observed:copyFields});
 for(const key of ['background','backgroundRotation','overrideMaterial','matrixAutoUpdate'])add('Scene.copy.setter.throw.'+key,{class:'Scene',kind:'sceneCopy',resources:true,observed:copyFields,throwAt:'set:self.'+key});
 for(const throwAt of [undefined,'super.get','super.call'])add('Scene.copy.super.'+throwAt,{class:'Scene',kind:'sceneCopy',recursive:false,resources:true,superHome:true,throwAt});
 for(const meta of [{$undefined:true},'',{},null])for(const resources of [false,true])add('Scene.json.'+JSON.stringify(meta)+'.'+resources,{class:'Scene',kind:'sceneJSON',meta,resources});
 for(const throwAt of [undefined,'super.get','super.call','json:fog'])add('Scene.json.super.'+throwAt,{class:'Scene',kind:'sceneJSON',resources:true,superHome:true,meta:'meta',throwAt,observed:['fog','backgroundBlurriness','backgroundIntensity','backgroundRotation','environmentIntensity','environmentRotation']});
 return out;
}
