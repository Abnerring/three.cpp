import fs from 'node:fs';import path from 'node:path';import assert from 'node:assert/strict';import {execFileSync} from 'node:child_process';import {pathToFileURL,fileURLToPath} from 'node:url';
const upstream=path.resolve(process.argv[2]);
const registry={
    ArrayNode:{file:'core/ArrayNode',args:['float',2],chains:['toArray']},
    IsolateNode:{file:'core/IsolateNode',args:[null],chains:['cache','isolate']},
    BypassNode:{file:'core/BypassNode',args:[null,null],chains:['bypass']},
    SubBuildNode:{file:'core/SubBuildNode',args:[null,'TEST'],chains:[],absent:'subBuild'},
    VaryingNode:{file:'core/VaryingNode',args:[null],chains:['toVarying','toVertexStage']},
    ExpressionNode:{file:'code/ExpressionNode',args:['','void'],chains:[],absent:'expression'},
    DebugNode:{file:'utils/DebugNode',args:[null],chains:['debug']},
    ColorSpaceNode:{file:'display/ColorSpaceNode',args:[null,'srgb-linear','srgb'],chains:['workingToColorSpace','colorSpaceToWorking'],absent:'convertColorSpace'}
};
function normal(v){if(v===undefined)return '#undefined';if(typeof v==='function')return '#function';if(typeof v==='symbol')return '#symbol';if(typeof v==='number'){if(Number.isNaN(v))return '#NaN';if(!Number.isFinite(v))return v>0?'#Infinity':'#-Infinity';if(Object.is(v,-0))return '#-0';}if(v===null||typeof v!=='object')return v;if(Array.isArray(v))return v.map(normal);return Object.fromEntries(Object.keys(v).map(key=>[key,normal(v[key])]));}
function constructible(v){try{Reflect.construct(function(){},[],v);return true}catch{return false;}}
if(process.argv[3]==='--oracle-case'){
    const name=process.argv[4],definition=registry[name];assert(definition);const {default:C}=await import(pathToFileURL(path.join(upstream,'src/nodes',definition.file+'.js')));const node=new C(...definition.args),nodePrototype=Object.getPrototypeOf(C.prototype),out={instance:normal(node),id:node.id,chains:definition.chains.map(key=>{const fn=node[key],d=Object.getOwnPropertyDescriptor(nodePrototype,key),item={name:key,callable:typeof fn==='function',ownOnNodePrototype:!!d,sameAsNodePrototype:fn===nodePrototype[key],function:typeof fn==='function'?[fn.name,fn.length,constructible(fn)]:normal(fn)};if(d)item.descriptor=[!!d.writable,!!d.enumerable,!!d.configurable];return item;})};if(definition.absent)out.unregisteredFactoryMethod=normal(node[definition.absent]);out.wrapperConstructorIdentity=Object.getPrototypeOf(node).constructor===C;process.stdout.write(JSON.stringify(out));process.exit(0);
}
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const binary=process.argv[3]&&process.argv[3]!=='--oracle'?path.resolve(process.argv[3]):undefined,directory=process.argv[4]&&path.resolve(process.argv[4]),script=fileURLToPath(import.meta.url),input=Object.keys(registry),expected=[],actual=[];
// Every upstream and native wrapper case runs in a new process. No fixture
// creates a Node or imports another source module before the selected module.
for(const name of input){expected.push(JSON.parse(execFileSync(process.execPath,[script,upstream,'--oracle-case',name],{encoding:'utf8'})));if(binary)actual.push(JSON.parse(execFileSync(binary,[name],{encoding:'utf8'})));}
if(directory){fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'input.json'),JSON.stringify(input,null,2));fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2));if(binary)fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));}
if(!binary){console.log(`Prepared ${input.length} real fresh-process wrapper import oracle scenarios`);process.exit(0);}
const differences=[];for(let i=0;i<input.length;++i)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({name:input[i],expected:expected[i],actual:actual[i]});}if(directory)fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(differences,null,2));assert.equal(differences.length,0,JSON.stringify(differences.slice(0,2),null,2));console.log(`TSL base wrapper imports: ${input.length} real fresh-process scenarios passed (absolute node ids and registered chains)`);
