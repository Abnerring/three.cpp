import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';

const upstream=path.resolve(process.argv[2]);
const binary=process.argv[3]&&process.argv[3]!=='--oracle'?path.resolve(process.argv[3]):undefined;
const directory=process.argv[4]&&path.resolve(process.argv[4]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=name=>import(pathToFileURL(path.join(upstream,'src',name+'.js')));
const [{default:Node},tsl,utils]=await Promise.all([load('nodes/core/Node'),load('nodes/tsl/TSLCore'),load('utils')]);
const before=new Node();
const module=await load('nodes/math/OperatorNode');
const after=new Node();
const OperatorNode=module.default;
function normal(v){
    if(v===undefined)return '#undefined';if(typeof v==='function')return '#function';if(typeof v==='symbol')return '#symbol';
    if(typeof v==='number'){if(Number.isNaN(v))return '#NaN';if(!Number.isFinite(v))return v>0?'#Infinity':'#-Infinity';if(Object.is(v,-0))return '#-0';}
    if(v===null||typeof v!=='object')return v;if(v._tag)return '@'+v._tag;
    if(v.isConstNode)return ['const',normal(v.value),normal(v.nodeType)];
    if(v.isOperatorNode)return ['operator',normal(v.op),normal(v.aNode),normal(v.bNode)];
    if(v.isVarNode)return ['var',normal(v.node),v.name,v.readOnly,v.intent];
    if(v.isConvertNode)return ['convert',normal(v.node),normal(v.convertTo)];
    if(Array.isArray(v))return v.map(normal);
    const out={};for(const k of Object.keys(v))Object.defineProperty(out,k,{value:normal(v[k]),writable:true,enumerable:true,configurable:true});return out;
}
function attempt(body){try{return normal(body());}catch(e){return '#'+e.name;}}
function observe(s,o,key,label){const v=o[key];Object.defineProperty(o,key,{configurable:true,enumerable:true,get(){s.events.push(['get',label]);if(s.r.throwAt===label)throw new TypeError(label);return v;}});}
const isVector=v=>typeof v==='string'&&/^(?:[iub]?vec)[234]$/.test(v);
const isMatrix=v=>typeof v==='string'&&v.startsWith('mat');
const isScalar=v=>['float','int','uint','bool','half'].includes(v);
const length=v=>v==null?0:isMatrix(v)?Number(v[3])**2:isVector(v)?Number(v.at(-1)):v==='void'?0:1;
function protocol(r){
    const s={r,events:[]};
    const special=(label,text)=>({[Symbol.toPrimitive](hint){s.events.push(['primitive',label,hint]);if(r.throwAt==='primitive.'+label)throw new TypeError(label);return text;}});
    const callback=name=>function(...args){s.events.push([name,...args.map(normal)]);if(r.throwAt===name)throw new TypeError(name);
        if(name==='a.getNodeType')return r.typeA;if(name==='b.getNodeType')return r.typeB;
        if(name==='a.build')return r.snippetObject?special('a','A'):'A';if(name==='b.build')return r.snippetObject?special('b','B'):'B';
        if(name==='getNodeType')return Object.hasOwn(r,'nodeType')?r.nodeType:'vec3';
        if(name==='getTypeLength'){const value=length(args[0]);return r.lengthObject?special('length:'+args[0],value):value;}
        if(name==='getIntegerType')return 'integer:'+args[0];if(name==='isMatrix')return isMatrix(args[0]);if(name==='isVector')return r.forceVectors||isVector(args[0]);if(name==='isScalar')return isScalar(args[0]);if(name==='isInteger')return typeof args[0]==='string'&&/^(?:int|uint|[iu]vec[234])$/.test(args[0]);
        if(name==='getVectorFromMatrix')return 'vec'+args[0][3];if(name==='getPromotedComponentType')return r.promoted??'float';
        if(name==='changeComponentType')return String(args[0])+':'+args[1];
        if(name==='getTypeFromLength')return args[0]>1?(args[1]==='float'?'':args[1]==='bool'?'b':args[1]==='uint'?'u':'i')+'vec'+args[0]:args[1];
        if(name==='getFunctionOperator')return r.functionOperator?(r.snippetObject?special('fn','customFn'):'customFn'):null;
        if(name==='getMethod')return 'method:'+String(args[0])+':'+String(args[1]);
        if(name==='format')return args;
    };
    const a={_tag:'a',getNodeType:callback('a.getNodeType'),build:callback('a.build')};
    const b={_tag:'b',getNodeType:callback('b.getNodeType'),build:callback('b.build')};
    const n=new OperatorNode(r.op,a,r.noB?null:b);n._tag='operator';
    n.getNodeType=callback('getNodeType');
    const builder={_tag:'builder',renderer:{coordinateSystem:r.glsl?2000:2001}};
    for(const name of ['getMethod','getTypeLength','getIntegerType','isMatrix','isVector','isScalar','isInteger','getVectorFromMatrix','getPromotedComponentType','changeComponentType','getTypeFromLength','getFunctionOperator','format'])builder[name]=callback(name);
    if(r.observed){for(const key of ['op','aNode','bNode','getNodeType','getOperatorMethod'])observe(s,n,key,'operator.'+key);for(const key of ['getNodeType','build']){observe(s,a,key,'a.'+key);observe(s,b,key,'b.'+key);}observe(s,builder.renderer,'coordinateSystem','renderer.coordinateSystem');for(const key of Object.keys(builder))if(key!=='_tag')observe(s,builder,key,'builder.'+key);}
    const previous=utils.getConsoleFunction();utils.setConsoleFunction((...a)=>s.events.push(['console',...a.map(normal)]));
    const result=attempt(()=>n[r.operation](builder,r.output));utils.setConsoleFunction(previous);return {result,events:s.events};
}
function metadata(){
    const ctor=OperatorNode,proto=ctor.prototype,node=new ctor('+',1,2);node.id=100;
    return {arity:ctor.length,name:ctor.name,parent:Object.getPrototypeOf(ctor).name,staticType:ctor.type,
        own:Object.getOwnPropertyNames(node).map(k=>{const d=Object.getOwnPropertyDescriptor(node,k);return [k,normal(node[k]),!!d.writable,!!d.enumerable,!!d.configurable];}),
        methods:Object.getOwnPropertyNames(proto).map(k=>{const d=Object.getOwnPropertyDescriptor(proto,k),v=d.value;return [k,normal(v),typeof v==='function'?v.name:null,typeof v==='function'?v.length:null,!!d.writable,!!d.enumerable,!!d.configurable];})};
}
function construction(r){
    const n=new OperatorNode(...r.args);
    const graph=v=>v&&v.isOperatorNode?{id:v.id-n.id,op:normal(v.op),a:graph(v.aNode),b:graph(v.bNode)}:normal(v);
    return graph(n);
}
function serial(r){
    const s={r,events:[]},n=new OperatorNode('+',1,2),data={_tag:'data',op:'-'};n._tag='node';
    const old=Object.getOwnPropertyDescriptor(Node.prototype,r.operation);
    Object.defineProperty(Node.prototype,r.operation,{configurable:true,writable:true,value:function(...args){s.events.push(['super',this===n,...args.map(normal)]);if(r.throwAt==='super')throw new TypeError('super');}});
    observe(s,n,'op','node.op');if(r.observeData)observe(s,data,'op','data.op');
    const result=attempt(()=>n[r.operation](data));Object.defineProperty(Node.prototype,r.operation,old);
    return {result,events:s.events,op:attempt(()=>n.op),dataOp:attempt(()=>data.op)};
}
function factory(r){
    // Use one explicit host capture input while retaining parsed StackTrace
    // frames and the complete error text, including its formatted location.
    const prepare=Object.getOwnPropertyDescriptor(Error,'prepareStackTrace');
    Object.defineProperty(Error,'prepareStackTrace',{configurable:true,writable:true,value:()=> 'Error\n    at operatorFactory (OperatorFixture.js:40:2)'});
    const previous=utils.getConsoleFunction(),events=[];utils.setConsoleFunction((...a)=>events.push(a.map(v=>v&&v.constructor?.name==='StackTrace'?['StackTrace',normal(v.stack)]:normal(v))));
    const input=r.args??[1,2,3];const out={result:attempt(()=>module[r.name](...input)),events};utils.setConsoleFunction(previous);
    if(prepare)Object.defineProperty(Error,'prepareStackTrace',prepare);else delete Error.prepareStackTrace;
    return out;
}
function fnBody(r){
    const events=[],a=new Node('int'),temp={_tag:'temp'};a._tag='a';
    for(const method of ['addAssign','subAssign','toConst'])Object.defineProperty(a,method,{configurable:true,get(){events.push(['get',method]);if(r.throwAt==='get.'+method)throw new TypeError(method);return function(...args){events.push([method,this===a,...args.map(normal)]);if(r.throwAt===method)throw new TypeError(method);return method==='toConst'?temp:a;};}});
    let count=0;
    const inputs={[Symbol.iterator](){events.push(['iterator']);return {next(){events.push(['next']);if(r.throwAt==='next')throw new TypeError('next');return count++===0?{done:false,value:a}:{done:true};},get return(){events.push(['getReturn']);if(r.throwAt==='getReturn')throw new TypeError('getReturn');return function(){events.push(['return']);if(r.throwAt==='return')throw new TypeError('return');return {};};}};}};
    const result=attempt(()=>module[r.name].shaderNode.jsFunc(inputs));return {result,events};
}
async function integration(r){
    const [{default:NodeBuilder},{default:GLSLNodeParser}]=await Promise.all([load('nodes/core/NodeBuilder'),load('nodes/parsers/GLSLNodeParser')]);
    const renderer={coordinateSystem:r.glsl?2000:2001,debug:{diagnostics:{keywords:false}}};
    const builder=new NodeBuilder(null,renderer,new GLSLNodeParser());builder.shaderStage='fragment';
    const args=r.variable?[tsl.int(r.args[0]).toVar('inputValue')]:r.args;
    const n=module[r.name](...args),values=[];
    for(const stage of ['setup','analyze','generate']){builder.buildStage=stage;values.push(attempt(()=>{const value=n.build(builder,r.output);return r.variable&&value?.isStackNode?'#stack':value;}));}
    return {values,flow:normal(builder.flow),stackRestored:tsl.getCurrentStack()===null};
}
const cases=[{kind:'imports'},{kind:'metadata'}];
for(const args of [[],['+'],['+',1],['+',1,2],['+',1,2,3],['+',1,2,3,4,5],['!',1],['%',null,0],['custom','text',false]])cases.push({kind:'construction',args});
const operations=['+','-','*','/','%','==','!=','<','>','<=','>=','&&','||','!','^^','&','~','|','^','<<','>>','custom'];
const types=[['float','float'],['vec2','vec4'],['ivec3','uint'],['mat3','float'],['mat3','vec3'],['mat3','mat4'],['float','mat4'],['vec3','mat4'],['void','float'],['float','void']];
for(const op of operations)for(const [typeA,typeB] of types)cases.push({kind:'protocol',operation:'generateNodeType',op,typeA,typeB,observed:true,output:null});
for(const op of operations)for(const [typeA,typeB] of types)for(const glsl of [false,true])for(const output of ['vec4','void'])cases.push({kind:'protocol',operation:'generate',op,typeA,typeB,nodeType:['==','!=','<','>','<=','>=','&&','||','^^','!'].includes(op)?'bool':typeA,glsl,output,observed:true,functionOperator:false});
for(const operation of ['generateNodeType','generate'])for(const op of ['+','%','!','<<','=='])for(const noB of [false,true])cases.push({kind:'protocol',operation,op,typeA:'vec3',typeB:'vec2',nodeType:'bool',observed:true,glsl:true,output:'vec3',noB,lengthObject:true,snippetObject:true,functionOperator:true});
for(const throwAt of ['operator.op','operator.aNode','operator.bNode','a.getNodeType','b.getNodeType','builder.getTypeLength','getTypeLength','builder.isMatrix','isMatrix','builder.getPromotedComponentType','getPromotedComponentType','builder.changeComponentType','changeComponentType'])cases.push({kind:'protocol',operation:'generateNodeType',op:'+',typeA:'vec2',typeB:'vec4',observed:true,throwAt});
for(const throwAt of ['operator.getNodeType','getNodeType','a.build','b.build','builder.getFunctionOperator','getFunctionOperator','builder.renderer','renderer.coordinateSystem','builder.format','format','primitive.a','primitive.b','primitive.fn'])cases.push({kind:'protocol',operation:'generate',op:'+',typeA:'vec2',typeB:'vec4',nodeType:'vec4',observed:true,glsl:true,output:'vec4',functionOperator:true,snippetObject:true,throwAt});
for(const operation of ['serialize','deserialize'])for(const observeData of [false,true])for(const throwAt of [undefined,'super','node.op','data.op'])cases.push({kind:'serial',operation,observeData,throwAt});
for(const name of Object.keys(module).filter(k=>!['default','incrementBefore','decrementBefore','increment','decrement'].includes(k)))for(const args of [[1],[1,2],[1,2,3,4]])cases.push({kind:'factory',name,args});
for(const name of ['incrementBefore','decrementBefore','increment','decrement'])for(const throwAt of [undefined,'next','getReturn','return','get.addAssign','addAssign','get.subAssign','subAssign','get.toConst','toConst'])cases.push({kind:'fnBody',name,throwAt});
for(const name of ['add','mul','equal','mod','not','shiftLeft'])for(const glsl of [true,false])cases.push({kind:'integration',name,args:name==='not'?[true]:[3,2],glsl,output:'float'});
for(const name of ['incrementBefore','decrementBefore','increment','decrement'])for(const glsl of [true,false])cases.push({kind:'integration',name,args:[3],variable:true,glsl,output:'int'});
for(const op of operations)cases.push({kind:'protocol',operation:'getOperatorMethod',op,typeA:'float',typeB:'float',output:'vec3',observed:true});
for(const glsl of [false,true])for(const noB of [false,true])cases.push({kind:'protocol',operation:'generate',op:'!',typeA:'vec3',typeB:'vec3',nodeType:'bvec3',glsl,noB,output:'bvec3',observed:true});
for(const op of ['&&','||','^^','custom'])cases.push({kind:'protocol',operation:'generate',op,typeA:'vec3',typeB:'vec3',nodeType:'bool',glsl:false,output:'bool',observed:true,forceVectors:true});
const expected=[];
for(const r of cases)expected.push(r.kind==='imports'?{keys:Object.keys(module),nodeCount:after.id-before.id,fnIds:['incrementBefore','decrementBefore','increment','decrement'].map(k=>[k,module[k].id-before.id,module[k].shaderNode.id-before.id])}:r.kind==='metadata'?metadata():r.kind==='construction'?construction(r):r.kind==='protocol'?protocol(r):r.kind==='serial'?serial(r):r.kind==='factory'?factory(r):r.kind==='fnBody'?fnBody(r):await integration(r));
if(directory){fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'input.json'),JSON.stringify(cases));fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2));}
if(!binary){console.log(`Prepared ${cases.length} real OperatorNode oracle cases`);process.exit(0);}
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:256*1024*1024}));
if(directory)fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);
const differences=[];for(let i=0;i<cases.length;++i)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({index:i,input:cases[i],expected:expected[i],actual:actual[i]});}
if(directory)fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(differences,null,2));
assert.equal(differences.length,0,JSON.stringify(differences.slice(0,3),null,2));console.log(`OperatorNode: ${cases.length} real pinned-upstream scenarios passed`);
