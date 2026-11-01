import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const [upstream,mode='cases']=process.argv.slice(2);
let randomCalls=0;Math.random=()=>((++randomCalls)%257)/257;
const load=p=>import(pathToFileURL(path.join(upstream,'src',p+'.js')));
const {default:QuadMesh}=await load('renderers/common/QuadMesh');
const utils=await load('utils');
const events=[];utils.setConsoleFunction((...args)=>events.push(['console',...args]));
const normal=v=>v===undefined?'#undefined':typeof v==='function'?'#function':typeof v==='symbol'?'#symbol':v===null?null:typeof v==='object'?'#object':v;
const descriptor=(o,k)=>{const d=Object.getOwnPropertyDescriptor(o,k);return d?{value:'value'in d?normal(d.value):'#accessor',writable:d.writable??null,enumerable:d.enumerable,configurable:d.configurable}:null;};
const metadata=()=>{const out={};for(const [name,ctor]of [['QuadMesh',QuadMesh],['QuadGeometry',new QuadMesh({}).geometry.constructor]]){
 const methods=[];for(const key of Reflect.ownKeys(ctor.prototype)){if(key==='constructor')continue;const d=Object.getOwnPropertyDescriptor(ctor.prototype,key);if(typeof d.value==='function'){const f=d.value;methods.push({key,name:f.name,length:f.length,source:Function.prototype.toString.call(f),parentName:Object.getPrototypeOf(f).constructor.name,writable:d.writable,enumerable:d.enumerable,configurable:d.configurable,constructible:(()=>{try{Reflect.construct(function(){},[],f);return true;}catch{return false;}})()});}}
 out[name]={name:ctor.name,length:ctor.length,parentName:Object.getPrototypeOf(ctor).name,source:Function.prototype.toString.call(ctor),prototypeKeys:Reflect.ownKeys(ctor.prototype),methods};}return out;};
const geometry=g=>({type:g.type,id:g.id,uuid:g.uuid,constructorName:g.constructor.name,keys:Reflect.ownKeys(g.attributes),position:{array:Array.from(g.attributes.position.array),itemSize:g.attributes.position.itemSize,count:g.attributes.position.count,normalized:g.attributes.position.normalized},uv:{array:Array.from(g.attributes.uv.array),itemSize:g.attributes.uv.itemSize,count:g.attributes.uv.count,normalized:g.attributes.uv.normalized}});
const nodeInfo=n=>n&&typeof n==='object'&&n.isNode?{type:n.type,id:n.id,nodeType:n.nodeType}:normal(n);
async function run(request){
 events.length=0;const result={kind:request.kind};
 if(request.kind==='constructor'){
  const m=request.material==='undefined'?undefined:request.material==='null'?null:{vertexNode:'previous'};const q=new QuadMesh(m),other=new QuadMesh({});
  Object.assign(result,{isQuadMesh:q.isQuadMesh,isMesh:q.isMesh,type:q.type,materialSame:q.material===m,materialType:q.material?.type??null,defaultMaterialName:q.material?.constructor.name??null,sharedGeometry:q.geometry===other.geometry,sharedCamera:q.camera===other.camera,camera:{name:q.camera.constructor.name,left:q.camera.left,right:q.camera.right,top:q.camera.top,bottom:q.camera.bottom,near:q.camera.near,far:q.camera.far,id:q.camera.id,uuid:q.camera.uuid},geometry:geometry(q.geometry),cameraDescriptor:descriptor(q,'camera'),flagDescriptor:descriptor(q,'isQuadMesh'),instanceKeys:Reflect.ownKeys(q)});
 }else if(request.kind==='geometry'){
  const ctor=new QuadMesh({}).geometry.constructor,flip=request.flipY==='undefined'?undefined:request.flipY,g=new ctor(flip);result.geometry=geometry(g);
 }else if(request.kind==='render'){
  const material={vertexNode:'previous'},second={vertexNode:'second'},q=new QuadMesh(material),reason={marker:'render-throw'},renderer={};let observed,called=0;
  const rendering=function(mesh,camera){events.push('render');observed=material.vertexNode;events.push(['receiver',this===renderer,'mesh',mesh===q,'camera',camera===q.camera]);
   if(request.mode==='throw')throw reason;
   if(request.mode==='material-change')q.material=second;
   if(request.mode==='reentrant'&&called++===0)q.render({render(mesh,camera){events.push(['inner',mesh===q,camera===q.camera,nodeInfo(material.vertexNode)]);}});
  };
  Object.defineProperty(renderer,'render',{configurable:true,get(){events.push('render.get');if(request.mode==='render-getter-throw')throw reason;return rendering;}});
  if(request.mode==='material-getter')Object.defineProperty(q,'material',{configurable:true,get(){events.push('material.get');return material;}});
  let error=null;try{result.returned=normal(q.render(renderer));}catch(e){error=e===reason?'reason':e.name;}
  Object.assign(result,{error,observed:nodeInfo(observed),materialVertex:nodeInfo(material.vertexNode),secondVertex:nodeInfo(second.vertexNode)});
 }else if(request.kind==='async'){
  const q=new QuadMesh({vertexNode:'previous'}),reason={marker:'async-reason'},renderer={};
  Object.defineProperty(renderer,'init',{get(){events.push('init.get');if(request.mode==='init-getter-throw')throw reason;return function(){events.push(['init.call',this===renderer]);if(request.mode==='init-call-throw')throw reason;if(request.mode==='reject')return Promise.reject(reason);if(request.mode==='primitive')return 7;if(request.mode==='then-getter-throw')return Object.defineProperty({},'then',{get(){events.push('then.get');throw reason;}});if(request.mode==='thenable')return {then(resolve){events.push('then.call');resolve(9);}};return Promise.resolve(9);};}});
  Object.defineProperty(renderer,'render',{get(){events.push('render.get');if(request.mode==='render-getter-throw')throw reason;return function(mesh,camera){events.push(['render.call',this===renderer,mesh===q,camera===q.camera]);if(request.mode==='render-throw')throw reason;};}});
  let p;try{p=q.renderAsync(renderer);result.callThrew=false;}catch(e){result.callThrew=true;result.callError=e===reason?'reason':e.name;}
  result.before=structuredClone(events);result.isPromise=p instanceof Promise;result.promiseTag=Object.prototype.toString.call(p);
  try{result.fulfilled=normal(await p);}catch(e){result.rejected=e===reason?'reason':e.name;}
  result.materialVertex=nodeInfo(q.material.vertexNode);
 }else if(request.kind==='module-private-bindings'){
  const a=new QuadMesh({vertexNode:1}),b=new QuadMesh({vertexNode:2});let first,second;a.render({render(){first=a.material.vertexNode;}});b.render({render(){second=b.material.vertexNode;}});Object.assign(result,{sameSharedVertex:first===second,vertex:nodeInfo(first),sameCamera:a.camera===b.camera,sameGeometry:a.geometry===b.geometry});
 }else throw Error('Unknown QuadMesh fixture kind');
 result.events=structuredClone(events);result.randomCalls=randomCalls;return result;
}
if(mode==='metadata')process.stdout.write(JSON.stringify(metadata()));
else if(mode==='cases'){const input=JSON.parse(fs.readFileSync(0,'utf8')),output=[];for(const request of input)output.push(await run(request));process.stdout.write(JSON.stringify(output));}
else throw Error('Unknown QuadMesh oracle mode');
utils.setConsoleFunction(null);
