import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),binary=process.argv[3]&&process.argv[3]!=='--oracle'?path.resolve(process.argv[3]):undefined,directory=process.argv[4]&&path.resolve(process.argv[4]);
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const HostError=globalThis.Error;globalThis.Error=new Proxy(HostError,{construct(target,args,newTarget){const error=Reflect.construct(target,args,newTarget);error.stack='';return error;}});
const load=file=>import(pathToFileURL(path.join(upstream,'src',file+'.js')));
const module=await load('nodes/core/PropertyNode'),PropertyNode=module.default,{default:Node}=await load('nodes/core/Node'),tsl=await load('nodes/tsl/TSLCore'),utils=await load('utils');
function normal(value,seen=[]){
    if(value===undefined)return '#undefined';if(typeof value==='symbol')return '#symbol';if(typeof value==='function')return '#function';
    if(typeof value==='number'){if(Number.isNaN(value))return '#NaN';if(!Number.isFinite(value))return value>0?'#Infinity':'#-Infinity';if(Object.is(value,-0))return '#-0';}
    if(value===null||typeof value!=='object')return value;if(seen.includes(value))return '#repeat';seen.push(value);
    const out=Array.isArray(value)?new Array(value.length):{};for(const key of Object.keys(value))Object.defineProperty(out,key,{value:normal(value[key],seen),writable:true,enumerable:true,configurable:true});seen.pop();return out;
}
function decode(value,state){
    if(value===null||typeof value!=='object')return value;
    if(value.tag==='undefined')return undefined;if(value.tag==='NaN')return NaN;if(value.tag==='negativeZero')return -0;if(value.tag==='infinity')return Infinity;if(value.tag==='symbol')return Symbol('fixture');
    if(value.tag==='node')return new Node(Object.hasOwn(value,'nodeType')?value.nodeType:'float');if(value.tag==='function')return ()=>{};
    if(value.tag==='flags'){
        const object={};for(const flag of ['isNode','isVector2','isVector3','isVector4','isMatrix2','isMatrix3','isMatrix4','isColor'])Object.defineProperty(object,flag,{configurable:true,get(){state.events.push(['get',flag,this===object]);if(state.request.throwAt===flag)throw new TypeError('flag');return value.flag===flag;}});return object;
    }
    if(Array.isArray(value))return value.map(item=>decode(item,state));return value;
}
function descriptors(value){return Object.getOwnPropertyNames(value).map(key=>{const d=Object.getOwnPropertyDescriptor(value,key);return [key,normal(d.value),!!d.writable,!!d.enumerable,!!d.configurable,typeof d.get==='function'?d.get.name:null];});}
function constructible(value){try{Reflect.construct(function(){},[],value);return true}catch{return false;}}
function metadata(){return {keys:Object.getOwnPropertyNames(module),functions:Object.getOwnPropertyNames(module).filter(key=>typeof module[key]==='function').map(key=>[key,module[key].name,module[key].length,constructible(module[key])]),nodes:Object.getOwnPropertyNames(module).filter(key=>typeof module[key]!=='function'),prototype:descriptors(PropertyNode.prototype),constructor:descriptors(PropertyNode),moduleIdentity:true,defaultIdentity:module.default===PropertyNode};}
function snapshot(request){const node=module[request.name];return {node:normal(node),fields:descriptors(node),type:node.type,prototype:Object.getPrototypeOf(node)===PropertyNode.prototype,repeatIdentity:node===module[request.name],nodeObjectIdentity:node===tsl.nodeObject(node),hash:normal(node.getHash()),cacheKey:normal(node.customCacheKey()),cacheable:node.isCacheable()};}
function factory(request){const state={request,events:[]},baseline=new Node().id,out={},console=utils.getConsoleFunction();utils.setConsoleFunction((...args)=>state.events.push(['console',normal(args)]));
    try{
        const args=request.args.map(value=>decode(value,state)),target=module[request.name],create=()=>request.invokeMode==='construct'||request.invokeMode!=='call'&&request.name==='default'?Reflect.construct(target,args):Reflect.apply(target,undefined,args);
        const first=create();out.first=normal(first);out.fields=descriptors(first);
        if(request.repeat){const second=create();out.second=normal(second);out.sameNode=first===second;out.samePlaceholder=Object.is(first.placeholderNode,second.placeholderNode);const index=request.name==='default'?3:2;out.placeholderIsArgument=Object.is(first.placeholderNode,index<args.length?args[index]:null);out.nodeObjectIdentity=tsl.nodeObject(first)===first;}
    }catch(e){out.error='#'+e.name;}
    out.allocationDelta=new Node().id-baseline;out.events=state.events;utils.setConsoleFunction(console);return out;
}
function protocol(request){const state={request,events:[]},node=module[request.factory](request.nodeType,request.propertyName,null);node.id=900;node._fixtureTag='node';
    const variable={_fixtureTag:'variable',needsInterpolation:false},builder={_fixtureTag:'builder'},placeholder={_fixtureTag:'placeholder'},data={},properties={};
    function fn(name){return function(...args){state.events.push([name,...args.map(value=>value&&typeof value==='object'&&value._fixtureTag?'@'+value._fixtureTag:normal(value))]);if(request.throwAt===name)throw new TypeError('callback');if(name==='getVarFromNode'||name==='getVaryingFromNode')return variable;if(name==='getDataFromNode')return data;if(name==='getNodeProperties')return properties;if(name==='getOutputType')return 'vec4';if(name==='hasWriteUsage')return request.write;if(name==='getPropertyName')return 'variable';if(name==='placeholder.build')return 'initial';};}
    placeholder.build=fn('placeholder.build');if(request.placeholder)node.placeholderNode=placeholder;
    for(const key of ['getVarFromNode','getVaryingFromNode','getDataFromNode','getNodeProperties','getOutputType','hasWriteUsage','getPropertyName','addLineFlowCode'])builder[key]=fn(key);
    function observe(object,key,label){const value=object[key];Object.defineProperty(object,key,{configurable:true,get(){state.events.push(['get',label,this===object]);if(request.throwAt===label)throw new TypeError('get');return value;}});}
    if(request.observed){for(const key of ['name','varying','placeholderNode','getNodeType'])observe(node,key,'node.'+key);for(const key of ['getVarFromNode','getVaryingFromNode','getDataFromNode','getNodeProperties','getOutputType','hasWriteUsage','getPropertyName','addLineFlowCode'])observe(builder,key,'builder.'+key);observe(placeholder,'build','placeholder.build');}
    const out={};try{out.result=normal(node[request.operation](builder));}catch(e){out.error='#'+e.name;}out.events=state.events;out.needsInterpolation=variable.needsInterpolation;return out;
}
const cases=[{kind:'metadata'}];
for(const name of Object.keys(module).filter(key=>typeof module[key]!=='function'))cases.push({kind:'snapshot',name});
const U={tag:'undefined'},scalars=[null,U,false,true,0,{tag:'negativeZero'},1,3,-1,.5,8.75,{tag:'NaN'},{tag:'infinity'},'', 'float',{},[],{tag:'node'}, {tag:'function'},{tag:'symbol'}];
for(const name of ['property','varyingProperty']){
    for(const args of [[],['float'],['float',U],['output','color'],['vec3',null,null],['float','x',null,99]])cases.push({kind:'factory',name,args,repeat:true});
    for(const type of [U,null,'float','output','vec3',0,false])for(const propertyName of [U,null,'', 'named',0,false])cases.push({kind:'factory',name,args:[type,propertyName,1],repeat:true});
    for(const placeholder of scalars)cases.push({kind:'factory',name,args:['float','initial',placeholder],repeat:true});
    for(const flag of ['isNode','isVector2','isVector3','isVector4','isMatrix2','isMatrix3','isMatrix4','isColor','none'])for(const throwAt of ['',flag])cases.push({kind:'factory',name,args:['float','flags',{tag:'flags',flag}],throwAt,repeat:false});
}
for(const args of [[],['float'],['output','color'],['vec3',null,true,1],['float','x',false,false],['float','x',U,U],['float','x',null,null],['float','x',1,8.75]])cases.push({kind:'factory',name:'default',args,repeat:true});
for(const placeholder of scalars)for(const varying of [false,true,1,null])cases.push({kind:'factory',name:'default',args:['float','x',varying,placeholder],repeat:true});
for(const name of ['default','property','varyingProperty'])for(const invokeMode of ['call','construct'])cases.push({kind:'factory',name,args:['float','x'],invokeMode,repeat:false});
for(const factory of ['property','varyingProperty'])for(const nodeType of ['float','output'])for(const operation of ['getHash','customCacheKey','isCacheable','getNodeType','generate'])for(const observed of [false,true])for(const placeholder of [false,true])cases.push({kind:'protocol',factory,nodeType,operation,observed,placeholder,write:false,propertyName:'named'});
for(const factory of ['property','varyingProperty'])for(const write of [false,true,0,null,U])for(const throwAt of ['','getVarFromNode','getVaryingFromNode','hasWriteUsage','getPropertyName','addLineFlowCode','placeholder.build','node.name','node.varying','node.placeholderNode','node.getNodeType','builder.getPropertyName','builder.addLineFlowCode'])cases.push({kind:'protocol',factory,nodeType:'output',operation:'generate',observed:true,placeholder:true,write,throwAt,propertyName:'initial'});
const expected=cases.map(request=>request.kind==='metadata'?metadata():request.kind==='snapshot'?snapshot(request):request.kind==='factory'?factory(request):protocol(request));
if(directory){fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'input.json'),JSON.stringify(cases));fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2));}
if(!binary){console.log(`Prepared ${cases.length} real PropertyNode module oracle scenarios`);process.exit(0);}
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:128*1024*1024}));if(directory)fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);
const differences=[];for(let i=0;i<cases.length;++i)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({index:i,input:cases[i],expected:expected[i],actual:actual[i]});}if(directory)fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(differences,null,2));assert.equal(differences.length,0,JSON.stringify(differences.slice(0,2),null,2));console.log(`PropertyNode module: ${cases.length} real pinned-upstream scenarios passed (absolute node ids)`);
