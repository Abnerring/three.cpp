import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]);
const binary=process.argv[3]&&process.argv[3]!=='--oracle'?path.resolve(process.argv[3]):undefined;
const outputDirectory=process.argv[4]&&path.resolve(process.argv[4]);
const relativeNodeIds=process.argv.includes('--relative-node-ids');
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=relative=>import(pathToFileURL(path.join(upstream,'src/'+relative+'.js')));
const [codeModule,functionModule,callModule,tsl,utils]=await Promise.all(['nodes/code/CodeNode','nodes/code/FunctionNode','nodes/code/FunctionCallNode','nodes/tsl/TSLCore','utils'].map(load));
const FunctionCallNode=callModule.default;
const [{default:Node},{default:NodeBuilder},{default:GLSLNodeParser},{default:WGSLNodeParser}]=await Promise.all(['nodes/core/Node','nodes/core/NodeBuilder','nodes/parsers/GLSLNodeParser','renderers/webgpu/nodes/WGSLNodeParser'].map(load));
// This mode measures real node allocations after each side has loaded its
// actual modules. The default mode keeps absolute IDs for import-graph checks.
const nodeIdBaseline=relativeNodeIds?new Node().id:undefined;
const enc=v=>v===undefined?'#undefined':typeof v==='symbol'?'#symbol':typeof v==='function'?'#function':v!==null&&typeof v==='object'?'#object':v;
function normal(v,seen=[]) {
    if(v===null||(typeof v!=='object'&&typeof v!=='function'))return enc(v);
    if(v._tag)return '@'+v._tag;
    if(v.isConstNode===true)return ['const',normal(v.value),v.nodeType];
    if(typeof v==='function')return '#function';
    if(seen.includes(v))return '#cycle';seen=[...seen,v];
    if(Array.isArray(v))return Array.from(v,x=>normal(x,seen));
    const result={};for(const key of Object.keys(v)){const field=v[key];Object.defineProperty(result,key,{value:relativeNodeIds&&key==='id'&&v.isNode===true&&typeof field==='number'?field-nodeIdBaseline:normal(field,seen),writable:true,enumerable:true,configurable:true});}return result;
}
function decode(v){if(v&&v.tag==='undefined')return undefined;if(v&&v.tag==='symbol')return Symbol('test');return v;}
function attempt(object,key,args=[]){try{return normal(object[key](...args));}catch(e){return '#'+e.name;}}
function descriptor(object,key){const d=Object.getOwnPropertyDescriptor(object,key);return [key,Object.hasOwn(d,'value')?'data':'accessor',enc(d.value),!!d.writable,!!d.enumerable,!!d.configurable,typeof d.value==='function'?d.value.name:enc(d.get),typeof d.value==='function'?d.value.length:enc(d.set)];}
function snapshot(node){const out={};for(const key of ['type','nodeType','code','language','includes','functionNode','parameters','value','isNode','isCodeNode']){const value=node[key];out[key]=key==='functionNode'&&value?[value.type,normal(value.code),normal(value.language)]:normal(value);}return out;}
function metadata(){
    const ctor=FunctionCallNode,node=new ctor(),proto=ctor.prototype,out={};
    out.own=Object.getOwnPropertyNames(node).map(key=>[key,key==='id'?'#id':normal(node[key]),...descriptor(node,key).slice(3,6)]);
    out.prototype=Object.getOwnPropertyNames(proto).map(key=>descriptor(proto,key));out.constructor=Object.getOwnPropertyNames(ctor).map(key=>descriptor(ctor,key));
    out.parent=Object.getPrototypeOf(proto).constructor.name;out.staticParent=Object.getPrototypeOf(ctor).name;out.call=attempt(proto,'constructor');out.parametersDistinct=node.parameters!==new ctor().parameters;
    const replacement={};out.setIdentity=node.setParameters(replacement)===node;out.getIdentity=node.getParameters()===replacement;return out;
}
function factory(r){const module=r.module==='code'?codeModule:functionModule,f=module[r.factory],out={};try{
    const node=f(...r.args.map(decode));out.snapshot=snapshot(node);out.callable=typeof node==='function';
    try{Reflect.construct(function(){},[],node);out.constructible=true;}catch{out.constructible=false;}
    if(typeof node==='function'){
        out.own=Object.getOwnPropertyNames(node).map(key=>descriptor(node,key));out.prototypeIsFunction=Object.getPrototypeOf(node)===Function.prototype;out.constructorIsFunctionNode=node.constructor===functionModule.default;
        out.called=snapshot(node({}));const marker={};node.marker=marker;out.markerIdentity=node.marker===marker;out.targetOwnMarker=Object.hasOwn(node,'marker');out.name=node.name;
    }
}catch(e){out.error=e.name;}return out;}
function callFactory(r){
    const func=new functionModule.default('float f(){return 1.;}',[],'glsl'),node=new Node('float');node._tag='argument';const map={a:2,b:false,node};
    const args=[func];if(r.shape==='map')args.push(map);else if(r.shape==='node')args.push(node);else if(r.shape==='multiple')args.push(3,true,node);else if(r.shape==='array')args.push([1,2]);else if(r.shape==='null')args.push(null);else if(r.shape==='false')args.push(false);else if(r.shape==='zero')args.push(0);else if(r.shape==='string')args.push('ab');
    const out={};try{const result=r.chained?func.call(...args.slice(1)):callModule.call(...args);out.parameters=normal(result.parameters);out.functionIdentity=result.functionNode===func;out.mapIdentity=result.parameters===map;out.mapAfter=normal(map);out.type=result.type;}catch(e){out.error=e.name;}return out;
}
function integration(r){
    const source=r.language==='glsl'?'float sumFn(float left, float right) { return left + right; }':'fn sumFn(left: f32, right: f32) -> f32 { return left + right; }';
    const native=functionModule[r.language+'Fn'](source),parser=r.language==='glsl'?new GLSLNodeParser():new WGSLNodeParser(),builder=new NodeBuilder(null,{backend:{},debug:{diagnostics:{keywords:false}}},parser);builder.setShaderStage('fragment');
    const input={isNode:true,nodeType:'float',_tag:'input',build(){return 'valueInput';}};const call=native([input,input]);const before=normal(call.parameters);builder.setBuildStage('generate');
    const result=attempt(call,'generate',[builder]);return {result,parameters:before,codes:builder.codes.fragment.map(code=>[code.name,code.type,code.code]),source:attempt(builder,'getCodes',['fragment'])};
}
function observe(s,object,key,label){const value=object[key];Object.defineProperty(object,key,{enumerable:true,configurable:true,get(){s.events.push(['get',label]);if(s.r.throwAt===label)throw new TypeError('get');return value;}});}
function protocol(r){
    const s={r,events:[]},builder={_tag:'builder'},fn={_tag:'function'};
    function fixture(name){return function(...args){s.events.push([name,...args.map(x=>normal(x))]);if(r.throwAt===name)throw new TypeError(name);if(name==='getInputs')return s.inputs;if(name==='getNodeType')return 'vec3';if(name==='getMemberType')return 'member:'+args[1];if(name==='function.build')return 'nativeFn';if(name==='zero.build')return 'zero';return name+':'+(args.length>1?args[1]:'implicit');};}
    for(const key of ['getInputs','getNodeType','getMemberType'])fn[key]=fixture(key);fn.build=fixture('function.build');
    const inputs=s.inputs=[],parameters=r.mode==='array'?[]:{};
    for(let i=0;i<r.count;i++){const input={name:'p'+i,type:r.pointer===i?'pointer':'float'};inputs.push(input);if(r.observed){observe(s,input,'name','input'+i+'.name');observe(s,input,'type','input'+i+'.type');}}
    for(let i=0;i<r.supplied;i++){const node={_tag:'param'+i,build:fixture('p'+i+'.build')};if(r.observed)observe(s,node,'build','p'+i+'.getbuild');if(Array.isArray(parameters))parameters.push(node);else parameters['p'+i]=node;}
    if(r.iterable){const iterable={[Symbol.iterator](){let i=0;return {next(){if(r.throwAt==='next')throw new TypeError('next');return {done:i>=inputs.length,value:inputs[i++]};},return(){s.events.push(['closeInputs']);if(r.throwClose)throw new TypeError('close');return {};}};}};fn.getInputs=function(...args){s.events.push(['getInputs',normal(args[0])]);return iterable;};}
    const node=new FunctionCallNode(fn,parameters);node._tag='call';if(r.observed){for(const key of ['getInputs','getNodeType','getMemberType','build'])observe(s,fn,key,'fn.'+key);observe(s,node,'functionNode','call.functionNode');observe(s,node,'parameters','call.parameters');}
    const zero=tsl.float(0),old=Object.getOwnPropertyDescriptor(zero,'build');zero.build=fixture('zero.build');
    const previousConsole=utils.getConsoleFunction();utils.setConsoleFunction((...args)=>{s.events.push(['console',...args.map(x=>normal(x))]);if(r.throwAt==='console')throw new TypeError('console');});
    const out={result:attempt(node,r.operation,[builder,'member']),events:s.events,parameters:normal(parameters)};
    utils.setConsoleFunction(previousConsole);if(old)Object.defineProperty(zero,'build',old);else delete zero.build;return out;
}
const cases=[{kind:'metadata'}];
for(const factoryName of ['code','glsl','wgsl','js'])for(const args of [[],['code'],['code',[]],['code',[], 'custom'],['code',[], 'custom','extra'],[7],[null],[{tag:'undefined'}],[{tag:'symbol'}]])cases.push({kind:'factory',module:'code',factory:factoryName,args});
for(const factoryName of ['glslFn','wgslFn'])for(const args of [[],['float f(){return 1.;}'],['float f(){return 1.;}',[]],['code',null],['code',false],['code',{tag:'undefined'}]])cases.push({kind:'factory',module:'function',factory:factoryName,args});
for(const mode of ['array','object'])for(const count of [0,1,2,3])for(const supplied of [0,1,2,4])for(const observed of [false,true])cases.push({kind:'protocol',operation:'generate',mode,count,supplied,observed,pointer:count>1?1:-1});
for(const operation of ['generateNodeType','getMemberType'])for(const observed of [false,true])for(const throwAt of [undefined,'call.functionNode',operation==='getMemberType'?'getMemberType':'getNodeType'])cases.push({kind:'protocol',operation,mode:'object',count:2,supplied:2,observed,pointer:-1,throwAt});
for(const mode of ['array','object'])for(const throwAt of ['call.functionNode','fn.getInputs','getInputs','call.parameters','input0.type','p0.getbuild','p0.build','function.build','fn.build','console','zero.build'])cases.push({kind:'protocol',operation:'generate',mode,count:2,supplied:throwAt==='console'||throwAt==='zero.build'?0:2,observed:true,pointer:1,throwAt});
for(const throwAt of [undefined,'next','p0.build','input0.type','function.build','zero.build'])for(const throwClose of [false,true])cases.push({kind:'protocol',operation:'generate',mode:'object',count:2,supplied:throwAt==='zero.build'?0:2,observed:true,iterable:true,pointer:1,throwAt,throwClose});
for(const shape of ['map','node','multiple','array','null','false','zero','string','empty'])for(const chained of [false,true])cases.push({kind:'callFactory',shape,chained});
for(const language of ['glsl','wgsl'])cases.push({kind:'integration',language});
const expected=cases.map(r=>r.kind==='metadata'?metadata(r):r.kind==='factory'?factory(r):r.kind==='protocol'?protocol(r):r.kind==='callFactory'?callFactory(r):integration(r));
if(outputDirectory){fs.mkdirSync(outputDirectory,{recursive:true});fs.writeFileSync(path.join(outputDirectory,'input.json'),JSON.stringify(cases));fs.writeFileSync(path.join(outputDirectory,'expected.json'),JSON.stringify(expected,null,2));}
if(!binary){console.log(`Prepared ${cases.length} actual pinned-upstream native function factory scenarios`);process.exit(0);}
const actual=JSON.parse(execFileSync(binary,relativeNodeIds?['--relative-node-ids']:[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:64*1024*1024}));
if(outputDirectory)fs.writeFileSync(path.join(outputDirectory,'actual.json'),JSON.stringify(actual,null,2));
assert.equal(actual.length,expected.length);const differences=[];for(let i=0;i<cases.length;i++)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({index:i,input:cases[i],expected:expected[i],actual:actual[i]});}
if(outputDirectory)fs.writeFileSync(path.join(outputDirectory,'differences.json'),JSON.stringify(differences,null,2));
assert.equal(differences.length,0,JSON.stringify(differences.slice(0,4),null,2));console.log(`Native function factories and FunctionCallNode (${relativeNodeIds?'relative to a real Node created after imports':'absolute Node IDs'}): ${cases.length} actual pinned-upstream scenarios passed`);
