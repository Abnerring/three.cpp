import fs from 'node:fs';import path from 'node:path';import assert from 'node:assert/strict';import {execFileSync} from 'node:child_process';import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),binary=process.argv[3]&&process.argv[3]!=='--oracle'?path.resolve(process.argv[3]):undefined,directory=process.argv[4]&&path.resolve(process.argv[4]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=name=>import(pathToFileURL(path.join(upstream,'src',name+'.js')));
const [module,{default:Node},tsl]=await Promise.all([load('nodes/core/AssignNode'),load('nodes/core/Node'),load('nodes/tsl/TSLCore')]);const AssignNode=module.default;
function normal(v){if(v===undefined)return '#undefined';if(typeof v==='function')return '#function';if(typeof v==='symbol')return '#symbol';if(v===null||typeof v!=='object')return v;if(v._tag)return '@'+v._tag;if(v.isConstNode)return ['const',normal(v.value),normal(v.nodeType)];if(v.isAssignNode)return ['assign',normal(v.targetNode),normal(v.sourceNode)];if(Array.isArray(v))return v.map(normal);const out={};for(const k of Object.keys(v))Object.defineProperty(out,k,{value:normal(v[k]),writable:true,enumerable:true,configurable:true});return out;}
function attempt(body){try{return normal(body());}catch(e){return '#'+e.name;}}
function observe(s,o,key,label){const value=o[key];Object.defineProperty(o,key,{configurable:true,enumerable:true,get(){s.events.push(['get',label]);if(s.r.throwAt===label)throw new TypeError(label);return value;}});}
function protocol(r){
    const s={r,events:[],lines:0};
    const special=(label,text)=>({[Symbol.toPrimitive](hint){s.events.push(['primitive',label,hint]);if(r.throwAt==='primitive.'+label)throw new TypeError(label);return text;}});
    const callback=name=>function(...args){s.events.push([name,...args.map(normal)]);if(r.throwAt===name)throw new TypeError(name);
        if(name==='target.getNodeType'||name==='context.getNodeType')return r.targetType??'vec2';if(name==='root.getNodeType')return 'vec4';if(name==='source.getNodeType')return r.sourceType??'vec2';
        if(name==='target.getScope')return s.scope;if(name==='target.context')return s.context;if(name==='root.context')return s.rootContext;
        if(name==='context.build')return r.snippetObject?special('target','target'):'target';if(name==='source.build')return r.snippetObject?special('source','source'):'source';if(name==='rootContext.build')return r.snippetObject?special('root','root'):'root';
        if(name==='isAvailable')return r.available;if(name==='getTypeLength')return r.rootLength??4;
        if(name==='getDataFromNode')return args[0]===s.scope?s.scopeData:s.data;if(name==='getNodeProperties')return s.properties;
        if(name==='getVarFromNode')return s.variable;if(name==='getPropertyName')return r.snippetObject?special('property','temp'):'temp';
        if(name==='addLineFlowCode'){++s.lines;if(r.clearAfterLine===s.lines)Object.defineProperty(s.target,'components',{value:'',writable:true,enumerable:true,configurable:true});if(r.throwAt==='line:'+s.lines)throw new TypeError('line');}
        if(name==='format')return args;
    };
    const root={_tag:'root',getNodeType:callback('root.getNodeType'),context:callback('root.context')};
    const target=s.target={_tag:'target',getScope:callback('target.getScope'),context:callback('target.context'),getNodeType:callback('target.getNodeType'),isSplitNode:r.isSplit,components:r.components??'yx',node:root};
    s.context={_tag:'context',build:callback('context.build'),getNodeType:callback('context.getNodeType'),node:target};s.rootContext={_tag:'rootContext',build:callback('rootContext.build')};
    const source={_tag:'source',build:callback('source.build'),getNodeType:callback('source.getNodeType')},n=new AssignNode(target,source);n._tag='assign';s.scope={_tag:'scope'};
    s.scopeData={};if(r.readonlyScope)Object.defineProperty(s.scopeData,'assign',{value:false,enumerable:true});
    s.data={};if(Object.hasOwn(r,'property'))s.data.propertyName=r.property;
    if(r.observeProperty){let value=s.data.propertyName;Object.defineProperty(s.data,'propertyName',{enumerable:true,configurable:true,get(){s.events.push(['getProperty']);if(r.throwAt==='getProperty')throw new TypeError('get');return value;},set(v){s.events.push(['setProperty',normal(v)]);if(r.throwAt==='setProperty')throw new TypeError('set');value=v;}});}
    s.properties=r.operation==='setup'?{}:{targetNode:s.context,sourceNode:source};s.variable={_tag:'variable'};
    if(r.throwWrite)Object.defineProperty(s.properties,r.throwWrite,{enumerable:true,configurable:true,set(value){s.events.push(['write',r.throwWrite,normal(value)]);throw new TypeError('write');}});
    const builder={_tag:'builder'};for(const name of ['isAvailable','getTypeLength','getDataFromNode','getNodeProperties','getVarFromNode','getPropertyName','addLineFlowCode','format'])builder[name]=callback(name);
    if(r.observed){for(const key of ['targetNode','sourceNode','needsSplitAssign'])observe(s,n,key,'assign.'+key);for(const key of ['isSplitNode','components','node','getScope','context','getNodeType'])observe(s,target,key,'target.'+key);for(const key of ['node','build','getNodeType'])observe(s,s.context,key,'context.'+key);for(const key of ['getNodeType','context'])observe(s,root,key,'root.'+key);observe(s,s.rootContext,'build','rootContext.build');for(const key of ['build','getNodeType'])observe(s,source,key,'source.'+key);for(const key of Object.keys(builder))if(key!=='_tag')observe(s,builder,key,'builder.'+key);}
    const results=[];for(let i=0;i<(r.repeat??1);++i)results.push(attempt(()=>n[r.operation](builder,r.output)));
    return {results,events:s.events,scopeData:normal(s.scopeData),data:attempt(()=>s.data),properties:normal(s.properties)};
}
function metadata(){const c=AssignNode,p=c.prototype,n=new c(1,2);n.id=100;return {name:c.name,arity:c.length,type:c.type,parent:Object.getPrototypeOf(c).name,own:Object.getOwnPropertyNames(n).map(k=>{const d=Object.getOwnPropertyDescriptor(n,k);return [k,normal(n[k]),!!d.writable,!!d.enumerable,!!d.configurable];}),methods:Object.getOwnPropertyNames(p).map(k=>{const d=Object.getOwnPropertyDescriptor(p,k),v=d.value;return [k,normal(v),typeof v==='function'?v.name:null,typeof v==='function'?v.length:null,!!d.writable,!!d.enumerable,!!d.configurable];}),exports:Object.keys(module)};}
function factory(){const target=new Node('float'),source=new Node('float');target._tag='target';source._tag='source';const n=module.assign(target,source);return {value:normal(n),target:n.targetNode===target,source:n.sourceNode===source};}
async function integration(r){
    const [{default:NodeBuilder},{property}]=await Promise.all([load('nodes/core/NodeBuilder'),load('nodes/core/PropertyNode'),load('nodes/core/ContextNode')]);
    const renderer={coordinateSystem:2001,debug:{diagnostics:{keywords:false}}},builder=new NodeBuilder(null,renderer,null);builder.shaderStage='fragment';builder.isAvailable=()=>r.available;
    const root=property('vec3','targetValue'),target=r.components?root[r.components]:root,source=r.components?tsl.vec2(1,2):tsl.vec3(1,2,3),n=module.assign(target,source),values=[];
    const encode=v=>v&&v.isNode?'#'+v.type:normal(v);
    for(const stage of ['setup','analyze','generate']){builder.buildStage=stage;try{values.push(encode(n.build(builder,r.output)));}catch(e){values.push('#'+e.name);}}
    if(r.repeat){builder.buildStage='generate';values.push(attempt(()=>n.build(builder,r.output)));}
    return {values,flow:normal(builder.flow),assign:normal(builder.getDataFromNode(root).assign),contextAssign:normal(builder.context.assign),targetContext:builder.getNodeProperties(n).targetNode?.isContextNode===true};
}
const cases=[{kind:'metadata'},{kind:'factory'}];
for(const operation of ['isCacheable','generateNodeType','needsSplitAssign','setup','generate'])for(const output of [undefined,'void','vec2'])for(const available of [false,true,null,0])for(const isSplit of [false,true])cases.push({kind:'protocol',operation,output,available,isSplit,observed:true});
for(const components of ['x','xy','yx','zw','xyzw','','xxx'])for(const rootLength of [1,2,3,4])cases.push({kind:'protocol',operation:'needsSplitAssign',available:false,isSplit:true,components,rootLength,observed:true});
for(const available of [false,true])for(const output of ['void','vec2'])for(const sourceType of ['void','vec2'])for(const property of [undefined,null,false,'existing'])cases.push({kind:'protocol',operation:'generate',available,isSplit:true,output,sourceType,property,repeat:2,observed:true});
for(const clearAfterLine of [1,2])cases.push({kind:'protocol',operation:'generate',available:false,isSplit:true,output:'vec2',observed:true,clearAfterLine});
for(const output of ['void','vec2'])for(const throwAt of [undefined,'getProperty','setProperty'])cases.push({kind:'protocol',operation:'generate',available:false,isSplit:true,output,observed:true,observeProperty:true,throwAt});
for(const throwAt of ['assign.targetNode','assign.sourceNode','target.getScope','target.getScope','builder.getDataFromNode','getDataFromNode','builder.getNodeProperties','getNodeProperties','target.context','target.context'])cases.push({kind:'protocol',operation:'setup',available:false,isSplit:true,observed:true,throwAt});
for(const readonlyScope of [true])cases.push({kind:'protocol',operation:'setup',available:false,isSplit:true,observed:true,readonlyScope});
for(const throwWrite of ['sourceNode','targetNode'])cases.push({kind:'protocol',operation:'setup',available:false,isSplit:true,observed:true,throwWrite});
for(const throwAt of ['assign.needsSplitAssign','builder.isAvailable','isAvailable','target.isSplitNode','target.components','target.node','root.getNodeType','getTypeLength','context.build','context.getNodeType','source.build','source.getNodeType','getVarFromNode','getPropertyName','addLineFlowCode','line:2','line:3','root.context','rootContext.build','builder.format','format','primitive.target','primitive.source','primitive.root','primitive.property'])cases.push({kind:'protocol',operation:'generate',available:false,isSplit:true,output:'vec2',observed:true,snippetObject:true,throwAt});
for(const components of [undefined,'xy','yx','zx'])for(const available of [false,true])for(const output of ['void','vec2'])cases.push({kind:'integration',components,available,output,repeat:true});
const expected=[];for(const r of cases)expected.push(r.kind==='metadata'?metadata():r.kind==='factory'?factory():r.kind==='protocol'?protocol(r):await integration(r));
if(directory){fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'input.json'),JSON.stringify(cases));fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2));}if(!binary){console.log(`Prepared ${cases.length} real AssignNode oracle cases`);process.exit(0);}
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:128*1024*1024}));if(directory)fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);const differences=[];for(let i=0;i<cases.length;++i)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({index:i,input:cases[i],expected:expected[i],actual:actual[i]});}if(directory)fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(differences,null,2));assert.equal(differences.length,0,JSON.stringify(differences.slice(0,3),null,2));console.log(`AssignNode: ${cases.length} real pinned-upstream scenarios passed`);
