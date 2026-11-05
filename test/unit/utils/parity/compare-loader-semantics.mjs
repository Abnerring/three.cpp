import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';
const [upstream,probe,oraclePath]=process.argv.slice(2);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url),'utf8'));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=rel=>import(pathToFileURL(path.join(upstream,'src',rel)).href);
const [{default:NodeLoader},{default:NodeMaterialLoader},{MaterialLoader},{FileLoader},{default:MeshBasicNodeMaterial},{default:ConstNode}]=await Promise.all([load('loaders/nodes/NodeLoader.js'),load('loaders/nodes/NodeMaterialLoader.js'),load('loaders/MaterialLoader.js'),load('loaders/FileLoader.js'),load('materials/nodes/MeshBasicNodeMaterial.js'),load('nodes/core/ConstNode.js')]);
const ctors={NodeLoader,NodeMaterialLoader,MaterialLoader};
const outcome=(fn,reason)=>{try{return fn();}catch(e){return e instanceof TypeError?'TypeError':e===reason?'reason':'other';}};
const getter=(o,k,get,enumerable=true)=>Object.defineProperty(o,k,{get,enumerable,configurable:true});
function metadata(module){
 const ctor=ctors[module],p=ctor.prototype,methods={};
 const names=module==='NodeLoader'?['load','parseNodes','parse','setTextures','setNodes','createNodeFromType']:module==='NodeMaterialLoader'?['parse','setNodes','setNodeMaterials','createMaterialFromType']:['load','parse','setTextures','createMaterialFromType'];
 for(const name of names){const d=Object.getOwnPropertyDescriptor(p,name);methods[name]={name:d.value.name,length:d.value.length,source:d.value.toString(),flags:[d.writable,d.enumerable,d.configurable]};}
 if(module==='MaterialLoader')for(const name of ['createMaterialFromType','registerMaterial']){const f=ctor[name];methods[`static:${name}`]={name:f.name,length:f.length,source:f.toString()};}
 const loader=new ctor(),out={name:ctor.name,length:ctor.length,source:ctor.toString(),methods,texturesOrdinary:Object.getPrototypeOf(loader.textures)===Object.prototype};
 if(module!=='MaterialLoader')out.nodesOrdinary=Object.getPrototypeOf(loader.nodes)===Object.prototype;
 if(module==='NodeMaterialLoader')out.materialsOrdinary=Object.getPrototypeOf(loader.nodeMaterials)===Object.prototype;
 return out;
}
function iteratorScenario(variant){
 const loader=new NodeLoader(),trace=[],reason={},state={pass:0},data={uuid:'u',type:'T'};
 const deserialize=function(data){trace.push(['deserialize',this.uuid,Object.getPrototypeOf(data.meta.nodes)===Object.prototype,Object.getPrototypeOf(data.meta)===Object.prototype]);if(variant==='deserializeThrow')throw reason;};
 loader.createNodeFromType=function(type){trace.push(['create',type]);if(variant==='createThrow'||variant==='returnThrow')throw reason;return {deserialize};};
 for(const name of ['uuid','type'])getter(data,name,()=>{trace.push(`get:${name}`);if(variant===`${name}Throw`)throw reason;return name==='uuid'?'u':'T';});
 const iterable={[Symbol.iterator](){const pass=++state.pass;trace.push(['iterator',pass,this===iterable]);let i=0;const it={next(){const index=i++;trace.push(['next',pass,index,this===it]);if(variant==='nextThrow'||(variant==='secondNextThrow'&&pass===2))throw reason;return {done:index>0,value:data};}};
 getter(it,'return',function(){trace.push(['get:return',pass,this===it]);if(variant==='returnThrow')throw 'close failed';return function(){trace.push(['return',pass,this===it]);return {};};});return it;}};
 return {result:outcome(()=>loader.parseNodes(iterable).u.uuid,reason),trace,metaRetained:Object.hasOwn(data,'meta')};
}
function registryScenario(){
 const loader=new NodeLoader(),trace=[],first={},key=Symbol('type'),second={[key]:class {constructor(){trace.push('construct');this.tag='second';}}};let reads=0;
 getter(first,key,()=>{trace.push('first:type');return class {};});getter(loader,'nodes',()=>{trace.push('get:nodes');return reads++===0?first:second;});
 return {tag:loader.createNodeFromType(key).tag,trace};
}
function keyScenario(){
 const loader=new NodeLoader(),trace=[],key={toString(){trace.push('key');return 'u';}},data={uuid:key,type:'T'};
 getter(loader,'createNodeFromType',()=>{trace.push('get:create');return function(){trace.push('create');return {set uuid(v){trace.push('set:uuid');},deserialize(){trace.push('deserialize');}};};});
 loader.parseNodes([data]);return trace;
}
function arrayScenario(variant){
 const loader=new NodeLoader(),trace=[],first={uuid:variant==='duplicate'?'same':'a',type:'T',tag:1},second={uuid:variant==='duplicate'?'same':'b',type:'T',tag:2},values=variant==='grow'?[first]:[first,second];let count=0;
 loader.createNodeFromType=()=>{const id=++count;trace.push(['create',id]);if(variant==='grow'&&id===1)values.push(second);return {deserialize(data){trace.push(['deserialize',id,data.tag]);}};};
 const nodes=loader.parseNodes(values);return {count,keys:Object.keys(nodes),trace};
}
function factoryScenario(variant){
 const loader=new MaterialLoader(),trace=[],material={};
 if(variant==='static'){const d=Object.getOwnPropertyDescriptor(MaterialLoader,'createMaterialFromType');getter(MaterialLoader,'createMaterialFromType',function(){trace.push(['get:static',this===MaterialLoader]);return function(type){trace.push(['call:static',this===MaterialLoader,type]);return material;};});try{return {same:loader.createMaterialFromType('custom')===material,trace};}finally{Object.defineProperty(MaterialLoader,'createMaterialFromType',d);}}
 const key=Symbol('registered'),klass=class{constructor(){trace.push('construct');return material;}};return {registerUndefined:MaterialLoader.registerMaterial(key,klass)===undefined,same:loader.createMaterialFromType(key)===material,trace};
}
function forInScenario(variant){
 const loader=new NodeMaterialLoader(),trace=[],material={fromJSON(){trace.push('fromJSON');return this;}},nodes={},proto={inherited:'b',hidden:'c'},input=Object.create(proto),reason={};
 for(const k of ['a','b','c','x','y'])nodes[k]=`node:${k}`;
 loader.createMaterialFromType=()=>material;loader.nodes=nodes;input.own='a';Object.defineProperty(input,'hidden',{value:'x',enumerable:false});input[Symbol('ignored')]='y';
 if(variant==='mutate'){proto.deleted='c';input.deleted='x';getter(input,'own',()=>{trace.push('get:own');delete input.deleted;input.added='y';return 'a';});}
 if(variant==='throw')getter(input,'own',()=>{throw reason;});
 const json={type:'T',inputNodes:variant==='string'?'ab':variant==='null'?null:variant==='undefined'?undefined:input};
 const result=outcome(()=>loader.parse(json)===material,reason),assigned={};for(const k of Object.keys(material))if(k!=='fromJSON')assigned[k]=material[k];return {result,assigned,trace};
}
function dynamicSuperScenario(){
 const home=NodeMaterialLoader.prototype,base=Object.getPrototypeOf(home),replacement={},loader=new NodeMaterialLoader(),trace=[],material={};
 getter(replacement,'parse',function(){trace.push(['get:super',this===loader]);return function(json){trace.push(['call:super',this===loader,json.tag]);return material;};});
 getter(replacement,'createMaterialFromType',function(){trace.push(['get:create',this===loader]);return function(type){trace.push(['call:create',this===loader,type]);return material;};});
 Object.setPrototypeOf(home,replacement);try{return {parseSame:loader.parse({tag:'json'})===material,createSame:loader.createMaterialFromType('missing')===material,trace};}finally{Object.setPrototypeOf(home,base);}
}
function materialParseScenario(variant){
 const loader=new MaterialLoader(),json={},trace=[],material={},reason={},textures={};
 getter(loader,'createMaterialFromType',()=>{trace.push('get:create');return function(type){trace.push(['create',this===loader,type]);return material;};});
 getter(json,'type',()=>{trace.push('get:type');return 'T';});getter(loader,'textures',()=>{trace.push('get:textures');return textures;});
 if(variant!=='missing')getter(material,'fromJSON',()=>{trace.push('get:fromJSON');return function(a,b){trace.push(['fromJSON',this===material,a===json,b===textures]);if(variant==='throw')throw reason;return 'ignored';};});
 return {result:outcome(()=>loader.parse(json)===material,reason),trace};
}
function loadScenario(module,variant){
 const old=Object.getOwnPropertyDescriptor(FileLoader.prototype,'load'),oldJSON=Object.getOwnPropertyDescriptor(JSON,'parse'),originalJSON=JSON.parse;
 const trace=[],reason={},manager={itemError(url){trace.push(['itemError',this===manager,url]);}},loader=new ctors[module](manager),parsed={},state={},headers={custom:'yes'};
 loader.path='base/';loader.requestHeader=headers;loader.withCredentials=true;
 getter(loader,'parse',()=>{trace.push('get:parse');return function(data){trace.push(['parse',this===loader,data.value]);if(variant==='parseThrow'||variant==='onErrorThrow')throw reason;return parsed;};});
 let entered=false;const onLoad=function(data){trace.push(['onLoad',this===undefined,data===parsed]);if(variant==='onLoadThrow')throw reason;if(variant==='reenter'&&!entered){entered=true;loader.load('nested.json',onLoad,progress,onError);}};
 const onError=function(e){trace.push(['onError',this===undefined,e===reason]);if(variant==='onErrorThrow')throw reason;},progress=()=>{};
 JSON.parse=function(text){trace.push(['JSON.parse',this===JSON,text]);return originalJSON.call(this,text);};
 FileLoader.prototype.load=function(url,callback,onProgress,onErr){trace.push(['file.load',url,this.path,this.requestHeader===headers,this.withCredentials,onProgress===progress,onErr===onError]);state.callbackName=callback.name;state.callbackLength=callback.length;state.callbackSource=callback.toString();callback.call(this,'{"value":7}');return 'ignored file return';};
 try{return {result:outcome(()=>loader.load('asset.json',onLoad,progress,onError)===undefined,reason),trace,callback:state};}finally{Object.defineProperty(FileLoader.prototype,'load',old);Object.defineProperty(JSON,'parse',oldJSON);}
}
function realRoundtrip(){const loader=new NodeMaterialLoader(),value=new ConstNode(.375,'float');loader.setNodes({'node-1':value});loader.setNodeMaterials({MeshBasicNodeMaterial});const material=loader.parse({type:'MeshBasicNodeMaterial',uuid:'material-1',opacity:.75,inputNodes:{opacityNode:'node-1'}});return {isNodeMaterial:material.isNodeMaterial,uuid:material.uuid,opacity:material.opacity,sameNode:material.opacityNode===value,value:material.opacityNode.value};}
function run({mode,variant='',module='NodeLoader'}){
 if(mode==='metadata')return metadata(module);if(mode==='iterator')return iteratorScenario(variant);if(mode==='registry')return registryScenario();if(mode==='array')return arrayScenario(variant);if(mode==='factory')return factoryScenario(variant);if(mode==='keys')return keyScenario();if(mode==='forIn')return forInScenario(variant);if(mode==='super')return dynamicSuperScenario();if(mode==='parse')return materialParseScenario(variant);if(mode==='load')return loadScenario(module,variant);if(mode==='real')return realRoundtrip();
 if(mode==='realNode'){const loader=new NodeLoader();loader.setNodes({ConstNode});return outcome(()=>loader.parseNodes([{uuid:'node-1',type:'ConstNode'}]));}
 if(mode==='nullish'){const loader=new NodeLoader(),out={undefined:Object.getPrototypeOf(loader.parseNodes())===Object.prototype,null:outcome(()=>loader.parseNodes(null)),number:outcome(()=>loader.parseNodes(3))};loader.setTextures();loader.setNodes();out.texturesUndefined=loader.textures===undefined;out.nodesUndefined=loader.nodes===undefined;return out;}
 throw new Error(`Unknown scenario ${mode}`);
}
const requests=[];
for(const module of Object.keys(ctors))requests.push({mode:'metadata',module});
for(const variant of ['', 'uuidThrow','typeThrow','createThrow','deserializeThrow','nextThrow','secondNextThrow','returnThrow'])requests.push({mode:'iterator',variant});
for(const mode of ['registry','keys','super','real','realNode','nullish'])requests.push({mode});
for(const variant of ['grow','duplicate'])requests.push({mode:'array',variant});
for(const variant of ['static','symbol'])requests.push({mode:'factory',variant});
for(const variant of ['', 'mutate','throw','string','null','undefined'])requests.push({mode:'forIn',variant});
for(const variant of ['', 'missing','throw'])requests.push({mode:'parse',variant});
for(const module of Object.keys(ctors))for(const variant of ['', 'parseThrow','onLoadThrow','onErrorThrow','reenter'])requests.push({mode:'load',module,variant});
const expected=requests.map(run);
if(oraclePath)fs.writeFileSync(oraclePath,JSON.stringify({requests,expected},null,2));
if(probe==='--source-only'){console.log(`Loader semantics source oracle passed: ${requests.length} scenarios`);process.exit(0);}
const stdout=execFileSync(path.resolve(probe),{input:JSON.stringify(requests),encoding:'utf8',maxBuffer:32*1024*1024});
const actual=JSON.parse(stdout.trim().split('\n').at(-1));
for(let i=0;i<requests.length;i++)assert.deepEqual(actual[i],JSON.parse(JSON.stringify(expected[i])),JSON.stringify(requests[i]));
assert.equal(actual.length,expected.length);console.log(`Loader semantics parity passed: ${requests.length} scenarios`);
