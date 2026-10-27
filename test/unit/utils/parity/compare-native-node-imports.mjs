import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {fileURLToPath,pathToFileURL} from 'node:url';

const paths={TSL:'nodes/tsl/TSLCore',NodeUtils:'nodes/core/NodeUtils',Property:'nodes/core/PropertyNode',Conditional:'nodes/math/ConditionalNode',Operator:'nodes/math/OperatorNode',Math:'nodes/math/MathNode',Context:'nodes/core/ContextNode',Stack:'nodes/core/StackNode',Color:'nodes/display/ColorSpaceFunctions',Alpha:'nodes/functions/material/getAlphaHashThreshold',Constants:'nodes/core/constants',NodeBuilder:'nodes/core/NodeBuilder'};
const pinned='11738a9d55e502fb484cefffcea3a66dc44e9fe9';
function normal(v,seen=[],ids=new Map()){
    if(v===undefined)return '#undefined';if(typeof v==='function')return '#function';if(typeof v==='symbol')return '#symbol';
    if(typeof v==='number'){if(Number.isNaN(v))return '#NaN';if(!Number.isFinite(v))return v>0?'#Infinity':'#-Infinity';if(Object.is(v,-0))return '#-0';}
    if(v===null||typeof v!=='object')return v;const index=seen.indexOf(v);if(index!==-1)return ['#ref',index];seen.push(v);
    const result=Array.isArray(v)?new Array(v.length):{};
    for(const key of Object.keys(v)){const value=v[key];let field;if(key==='id'&&v.isNode===true&&typeof value==='number'){if(!ids.has(value))ids.set(value,ids.size);field=['#nodeId',ids.get(value)];}else field=normal(value,seen,ids);Object.defineProperty(result,key,{value:field,writable:true,enumerable:true,configurable:true});}
    return result;
}
async function run(upstream,r){
    const load=name=>import(pathToFileURL(path.join(upstream,'src',paths[name]+'.js')));
    const source=await load(r.source);let poisonCalls=0;const poison=()=>{poisonCalls++;throw new TypeError('facade export must not be imported');};
    if(r.mutationTiming==='afterTarget')await load(r.target);
    const output={mutationAccepted:r.mutationKind==='contents'?Reflect.set(source[r.key],'0',r.value):Reflect.set(source,r.key,poison),sourceReplaced:source[r.key]===poison};
    const utils=await import(pathToFileURL(path.join(upstream,'src/utils.js'))),events=[],oldConsole=utils.getConsoleFunction();utils.setConsoleFunction((...args)=>events.push(normal(args)));
    try{
        const target=await load(r.target),tsl=await load('TSL'),result={};
        if(r.target==='TSL'){
            result.values=[true,7,.25,null].map(v=>normal(target.nodeObject(v)));result.vector=normal(target.vec3(1,2,3));
        }else if(r.target==='Property'){
            const property=target.property('vec3','output',4);result.property=normal(property);result.cacheKey=property.customCacheKey();result.diffuseColor=normal(target.diffuseColor);result.ambientOcclusion=normal(target.ambientOcclusion);
        }else if(r.target==='Operator'){
            result.mul=normal(target.mul(2,3,4));result.equal=normal(target.equal(1,2));result.incrementFn=target.increment.isFn;
        }else if(r.target==='Math'){
            result.pow3=normal(target.pow3(2));result.clamp=normal(target.clamp(.5));result.transform=normal(target.transformNormalByViewMatrix(1,2));result.rand=normal(target.rand.shaderNode.jsFunc([tsl.vec2(.2,.4)]));
            const builder={getNodeProperties:()=>({})};result.setup=['oneMinus','reciprocal','difference'].map(name=>normal(target[name](2,3).setup(builder)));
        }else if(r.target==='Conditional'){
            result.select=normal(target.select(true,2,3));const propertyModule=await load('Property'),prototype=propertyModule.default.prototype,old=Object.getOwnPropertyDescriptor(prototype,'build'),properties=[];
            prototype.build=function(){properties.push(normal(this));throw new TypeError('stop after real property allocation');};
            const node=target.select(true,2,3);node.getNodeType=()=> 'float';const builder={getDataFromNode:()=>({}),getNodeProperties:()=>({condNode:node.condNode,ifNode:node.ifNode,elseNode:node.elseNode}),currentFunctionNode:null};
            try{node.generate(builder,'float');result.generate='returned';}catch(e){result.generate='#'+e.name;}
            if(old)Object.defineProperty(prototype,'build',old);else delete prototype.build;result.properties=properties;
        }else if(r.target==='Context'){
            const {default:Node}=await import(pathToFileURL(path.join(upstream,'src/nodes/core/Node.js'))),n=new Node('float');result.context=normal(target.context(n));result.uniform=normal(tsl.nodeObject(n).uniformFlow());
        }else if(r.target==='Stack'){
            const n=target.stack(),fn=()=>{};n.If(true,fn).ElseIf(false,fn).Else(fn).Switch(2);result.nodes=normal(n.nodes);result.current=normal(n._currentCond);result.expression=normal(n._expressionNode);
        }else if(r.target==='Color'){
            const color=tsl.vec3(.1,.2,.3);for(const name of ['sRGBTransferEOTF','sRGBTransferOETF'])result[name]=normal(target[name].shaderNode.jsFunc([color]));
        }else if(r.target==='Alpha'){
            const {default:StackNode}=await load('Stack'),position=tsl.vec3(.1,.2,.3),previous=tsl.getCurrentStack(),stack=new StackNode();tsl.setCurrentStack(stack);
            try{result.result=normal(target.default.shaderNode.jsFunc([position]));result.stack=normal(stack.nodes);}finally{tsl.setCurrentStack(previous);}
        }else if(r.target==='NodeBuilder'){
            const stages=[],receiver={context:{},flowNodes:{vertex:[],fragment:[],compute:[],customShader:[]}};
            for(const name of ['prebuild','setBuildStage','setShaderStage','buildCode','buildUpdateNodes'])receiver[name]=(...args)=>{stages.push([name,...args.map(value=>normal(value))]);};
            result.returnedSelf=target.default.prototype.build.call(receiver)===receiver;result.stages=stages;
        }
        output.behavior=result;
    }catch(e){output.behavior='#'+e.name;}
    utils.setConsoleFunction(oldConsole);output.poisonCalls=poisonCalls;output.events=events;return output;
}
if(process.argv[2]==='--case'){
    Error.stackTraceLimit=0;process.stdout.write(JSON.stringify(await run(path.resolve(process.argv[3]),JSON.parse(process.argv[4]))));process.exit(0);
}
const upstream=path.resolve(process.argv[2]),binary=process.argv[3]&&process.argv[3]!=='--oracle'?path.resolve(process.argv[3]):null,directory=process.argv[4]&&path.resolve(process.argv[4]);
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),pinned);
const cases=[];const add=(source,target,keys)=>{for(const key of keys)cases.push({source,target,key});};
add('NodeUtils','TSL',['getValueType']);add('NodeUtils','Property',['getValueType','hashString']);
add('TSL','Property',['nodeObject','nodeImmutable']);add('TSL','Operator',['nodeProxyIntent','addMethodChaining','int','Fn']);
add('TSL','Math',['nodeObject','nodeProxyIntent','addMethodChaining','float','vec2','vec3','vec4','Fn']);
add('TSL','Conditional',['nodeProxy','addMethodChaining']);add('TSL','Context',['addMethodChaining']);add('TSL','Stack',['ShaderNode','nodeObject','nodeProxy']);
add('Property','Conditional',['property','default']);add('Conditional','Stack',['select','default']);
add('Operator','Math',['sub','mul','div','mod']);add('Math','Color',['mix']);
add('Math','Alpha',['abs','ceil','clamp','dFdx','dFdy','exp2','floor','fract','length','log2','max','min','sin']);
add('TSL','Color',['Fn']);add('TSL','Alpha',['Fn','float','vec2','vec3','nodeProxy','addMethodChaining']);
add('Constants','NodeBuilder',['defaultBuildStages','shaderStages']);
cases.push({source:'Constants',target:'NodeBuilder',key:'defaultBuildStages',mutationKind:'contents',mutationTiming:'afterTarget',value:'customBuild'});
cases.push({source:'Constants',target:'NodeBuilder',key:'shaderStages',mutationKind:'contents',mutationTiming:'afterTarget',value:'customShader'});
const pending=[];
const expected=[],actual=[],differences=[];
for(let i=0;i<cases.length;i++){
    const r=cases[i],oracle=JSON.parse(execFileSync(process.execPath,[fileURLToPath(import.meta.url),'--case',upstream,JSON.stringify(r)],{encoding:'utf8',maxBuffer:64*1024*1024}));
    assert.equal(oracle.mutationAccepted,r.mutationKind==='contents');assert.equal(oracle.sourceReplaced,false);assert.equal(oracle.poisonCalls,0);assert.equal(typeof oracle.behavior,'object',JSON.stringify(r));expected.push(oracle);
    if(binary){const native=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(r),encoding:'utf8',maxBuffer:64*1024*1024}));actual.push(native);try{assert.equal(native.mutationAccepted,true);assert.equal(native.sourceReplaced,r.mutationKind!=='contents');assert.equal(native.poisonCalls,0);assert.deepEqual(native.behavior,oracle.behavior);assert.deepEqual(native.events,oracle.events);}catch{differences.push({index:i,input:r,expected:oracle,actual:native});}}
}
if(directory){fs.mkdirSync(directory,{recursive:true});for(const [name,value]of Object.entries({input:cases,expected,actual,differences,pending}))fs.writeFileSync(path.join(directory,name+'.json'),JSON.stringify(value,null,2));}
if(!binary){console.log(`Prepared ${cases.length} fresh-process native import cases; ${pending.length} recorded boundary`);process.exit(0);}
assert.equal(differences.length,0,JSON.stringify(differences.slice(0,2),null,2));console.log(`Native node imports: ${cases.length} fresh-process scenarios passed; facade/ESM mutation differences recorded explicitly`);
