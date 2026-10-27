import fs from 'node:fs';import path from 'node:path';import assert from 'node:assert/strict';import {execFileSync} from 'node:child_process';import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),binary=process.argv[3]&&process.argv[3]!=='--oracle'?path.resolve(process.argv[3]):null,directory=process.argv[4]&&path.resolve(process.argv[4]),relative=process.argv.includes('--relative-node-ids');
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=name=>import(pathToFileURL(path.join(upstream,'src',name+'.js')));
const {default:NodeBuilder}=await load('nodes/core/NodeBuilder');
const [{default:Node},{default:StackNode},tsl,utils,expression,discard,remap,debug]=await Promise.all([load('nodes/core/Node'),load('nodes/core/StackNode'),load('nodes/tsl/TSLCore'),load('utils'),load('nodes/code/ExpressionNode'),load('nodes/utils/Discard'),load('nodes/utils/Remap'),load('nodes/utils/DebugNode')]);
const modules={expression,discard,remap,debug};let baseline=0;
function normal(v,seen=[]){
 if(v===undefined)return '#undefined';if(typeof v==='function')return '#function';if(typeof v==='symbol')return '#symbol';if(typeof v==='number'){if(Number.isNaN(v))return '#NaN';if(!Number.isFinite(v))return v>0?'#Infinity':'#-Infinity';if(Object.is(v,-0))return '#-0';}
 if(v===null||typeof v!=='object')return v;if(v.fixtureTag)return '@'+v.fixtureTag;const index=seen.indexOf(v);if(index!==-1)return ['#ref',index];seen.push(v);if(Array.isArray(v))return v.map(x=>normal(x,seen));
 return Object.fromEntries(Object.keys(v).map(k=>[k,relative&&k==='id'&&v.isNode===true?(v.id>baseline?['#local',v.id-baseline]:['#existing',seen.indexOf(v)]):normal(v[k],seen)]));
}
const attempt=fn=>{try{return normal(fn());}catch(e){return '#'+e.name;}};
function observe(s,o,key,label){const value=o[key];Object.defineProperty(o,key,{enumerable:true,configurable:true,get(){s.events.push(['get',label]);if(s.r.throwAt===label)throw new TypeError('getter');return value;}});}
function primitive(s,label,value){return {[Symbol.toPrimitive](hint){s.events.push(['primitive',label,hint]);if(s.r.throwAt==='primitive.'+label)throw new TypeError('primitive');return value;}};}
function metadata(r){const c=modules[r.module].default,p=c.prototype,n=new c(...(r.module==='debug'?[1]:[]));n.id=100;return {exports:Object.keys(modules[r.module]),name:c.name,length:c.length,type:c.type,parent:Object.getPrototypeOf(c).name,fields:Object.getOwnPropertyNames(n).map(k=>{const d=Object.getOwnPropertyDescriptor(n,k);return [k,normal(n[k]),!!d.writable,!!d.enumerable,!!d.configurable];}),methods:Object.getOwnPropertyNames(p).map(k=>{const d=Object.getOwnPropertyDescriptor(p,k),v=d.value;return [k,normal(v),typeof v==='function'?v.name:null,typeof v==='function'?v.length:null,!!d.writable,!!d.enumerable,!!d.configurable];})};}
function protocol(r){
 const s={r,events:[]},flow={code:r.code??'\t\tfloat x;\n\treturn x;'},builder={fixtureTag:'builder',shaderStage:r.stage??'fragment',flow};
 const cb=name=>function(...args){s.events.push(['call',name,this===undefined,...args.map(x=>normal(x))]);if(r.throwAt===name+'()')throw new TypeError('call');if(name==='child.getNodeType')return r.type;if(name==='child.build')return r.objects?primitive(s,'snippet','SNIP'):'SNIP';if(name==='format')return args;};
 let n;
 if(r.module==='expression'){n=new expression.default(r.objects?primitive(s,'snippet','SNIP'):'SNIP',r.type);n.getNodeType=cb('child.getNodeType');builder.addLineFlowCode=cb('addLineFlowCode');builder.format=cb('format');}
 else{const child={fixtureTag:'child',getNodeType:cb('child.getNodeType'),build:cb('child.build')};const callback=r.callback==='function'?cb('callback'):r.callback==='false'?false:r.callback==='undefined'?undefined:null;n=new debug.default(child,callback);if(r.objects)builder.shaderStage=primitive(s,'stage',r.stage??'fragment');for(const k of ['getNodeType','build'])observe(s,child,k,'child.'+k);}
 n.fixtureTag='self';
 if(r.observed!==false){for(const k of r.module==='expression'?['getNodeType','snippet']:['node','callback'])observe(s,n,k,'self.'+k);for(const k of Object.keys(builder))if(k!=='fixtureTag')observe(s,builder,k,'builder.'+k);observe(s,flow,'code','flow.code');}
 const previous=utils.getConsoleFunction();utils.setConsoleFunction((...args)=>{s.events.push(['console',...args.map(x=>normal(x))]);if(r.throwAt==='console')throw new TypeError('console');});
 const result=attempt(()=>n[r.method](builder,r.output));utils.setConsoleFunction(previous);return {result,events:s.events};
}
function remapFixture(s,tag){const node=new Node('float');node.fixtureTag=tag;for(const key of ['sub','div','clamp','mul','add'])Object.defineProperty(node,key,{configurable:true,get(){s.events.push(['get',tag+'.'+key]);if(s.r.throwAt===tag+'.'+key)throw new TypeError('getter');return function(...args){s.events.push(['call',tag+'.'+key,...args.map(x=>normal(x))]);if(s.r.throwAt===tag+'.'+key+'()')throw new TypeError('call');return remapFixture(s,tag+'.'+key+'()');};}});return node;}
function remapBody(r){
 const s={r,events:[]};let args;
 if(r.fixture)args=['node','low','high','outLow','outHigh'].map(x=>remapFixture(s,x));else args=(r.args??[.4,.3,.5]).map(x=>x===null?undefined:tsl.float(x));
 if(r.fixture)args.push(r.clamp==='node'?new Node('bool'):r.clamp==='null'?null:tsl.bool(r.clamp===true));
 const iterable=r.iterator==='array'?args:{[Symbol.iterator](){s.events.push(['iterator']);let i=0;return {next(){s.events.push(['next',i]);if(r.throwAt==='next:'+i)throw new TypeError('next');return i<args.length?{done:false,value:args[i++]}:{done:true};},get return(){s.events.push(['getReturn']);if(r.throwAt==='getReturn')throw new TypeError('return');return ()=>{s.events.push(['return']);if(r.throwAt==='return')throw new TypeError('return');return {};};}};}};
 return {result:attempt(()=>remap.remap.shaderNode.jsFunc(iterable)),events:s.events};
}
function factory(r){const old=tsl.getCurrentStack(),stack=new StackNode();tsl.setCurrentStack(r.noStack?null:stack);try{let result;if(r.module==='discard')result=r.name==='Return'?discard.Return():discard.Discard(r.condition);else if(r.module==='debug')result=debug.debug(tsl.float(2));else if(r.module==='expression')result=expression.expression(...r.args);else result=remap[r.name](...r.args);return {result:normal(result),stack:normal(stack.nodes)};}finally{tsl.setCurrentStack(old);}}
function integration(r){
 const renderer={coordinateSystem:2001,debug:{diagnostics:{keywords:false}}},builder=new NodeBuilder(null,renderer,null);builder.shaderStage='fragment';
 let node;
 if(r.module==='expression')node=expression.expression(r.snippet,r.type);
 else if(r.module==='debug')node=new debug.default(tsl.float(2),()=>{});
 else if(r.module==='remap')node=remap[r.name](...r.args);
 else{const old=tsl.getCurrentStack(),stack=new StackNode();tsl.setCurrentStack(stack);try{discard[r.name](r.condition);node=stack;}finally{tsl.setCurrentStack(old);}}
 const values=[];for(const stage of ['setup','analyze','generate']){builder.buildStage=stage;values.push(attempt(()=>{const v=node.build(builder,r.output);return v?.isNode?'#'+v.type:v;}));}return {values,flow:normal(builder.flow),stackRestored:tsl.getCurrentStack()===null};
}
function logging(r){const events=[],old=utils.getConsoleFunction(),saved=console.log;let text='';if(r.fallback){utils.setConsoleFunction(null);console.log=(...args)=>{text+=args.join(' ')+'\n';};}else utils.setConsoleFunction((...args)=>{events.push(args.map(x=>normal(x)));if(r.throwAt==='console')throw new TypeError('console');});try{return {result:attempt(()=>utils.log(...r.args)),events,text};}finally{utils.setConsoleFunction(old);console.log=saved;}}
const cases=[{kind:'metadata',module:'expression'},{kind:'metadata',module:'debug'}];
for(const module of ['expression','debug'])for(const type of ['void','float','vec3',null])for(const method of module==='expression'?['isCacheable','generate']:['generateNodeType','setup','analyze','generate'])for(const callback of module==='debug'?['null','function','false','undefined']:['null'])cases.push({kind:'protocol',module,type,method,callback,output:'vec4',objects:true});
for(const stage of ['vertex','😀','阶段'])for(const code of ['\t\tx\r\t\ty\u2028\tz\u2029\tw','\tx\nx\ty',''])cases.push({kind:'protocol',module:'debug',method:'generate',callback:'null',stage,code});
for(const throwAt of ['self.getNodeType','child.getNodeType()','self.snippet','builder.format','format()','builder.addLineFlowCode','addLineFlowCode()'])cases.push({kind:'protocol',module:'expression',type:'float',method:'generate',throwAt});
for(const throwAt of ['self.callback','self.node','child.build','child.build()','builder.shaderStage','primitive.stage','builder.flow','flow.code','primitive.snippet','console'])cases.push({kind:'protocol',module:'debug',method:'generate',callback:'null',objects:true,throwAt});
for(const clamp of [false,true,'node','null'])for(const iterator of ['array','custom'])cases.push({kind:'remapBody',fixture:true,clamp,iterator});
for(const args of [[.4,.3,.5],[2,0,1],[2,0,1,10,20],[2,0,1,null,null]])cases.push({kind:'remapBody',args,iterator:'custom'});
for(const throwAt of ['next:0','next:3','next:5','getReturn','return','node.sub','node.sub()','node.sub().div','high.sub','high.sub()','node.sub().div().clamp','node.sub().div().clamp()','outHigh.sub','outHigh.sub()','node.sub().div().mul','node.sub().div().mul()','node.sub().div().mul().add'])cases.push({kind:'remapBody',fixture:true,clamp:true,iterator:'custom',throwAt});
for(const condition of [null,false,0,1,true])for(const noStack of [false,true])cases.push({kind:'factory',module:'discard',name:'Discard',condition,noStack});
for(const module of ['discard','debug'])for(const noStack of [false,true])cases.push({kind:'factory',module,name:module==='discard'?'Return':'debug',noStack});
for(const name of ['remap','remapClamp'])for(const args of [[.4,.3,.5],[2,0,1],[2,0,1,10,20]])cases.push({kind:'factory',module:'remap',name,args});
for(const type of ['void','float'])cases.push({kind:'integration',module:'expression',snippet:type==='void'?'continue':'2.0',type,output:type});
cases.push({kind:'integration',module:'debug',output:'float'});
for(const name of ['Discard','Return'])cases.push({kind:'integration',module:'discard',name,condition:false,output:'void'});
for(const name of ['remap','remapClamp'])for(const args of [[.4,.3,.5],[2,0,1]])cases.push({kind:'integration',module:'remap',name,args,output:'float'});
for(const args of [[],['hello'],['TSL: hello'],['hello',1,null]])cases.push({kind:'logging',args});cases.push({kind:'logging',args:['hello'],throwAt:'console'},{kind:'logging',args:['hello'],fallback:true});
if(relative)for(const r of cases)r.relativeNodeIds=true;
const expected=cases.map(r=>{baseline=new Node().id;return r.kind==='metadata'?metadata(r):r.kind==='protocol'?protocol(r):r.kind==='remapBody'?remapBody(r):r.kind==='factory'?factory(r):r.kind==='integration'?integration(r):logging(r);});
if(directory){fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'input.json'),JSON.stringify(cases));fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2));}
if(!binary){console.log(`Prepared ${cases.length} actual utility shader-node scenarios (${relative?'relative Node.id':'absolute Node.id'})`);process.exit(0);}
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:256*1024*1024}));if(directory)fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);const differences=[];for(let i=0;i<cases.length;++i)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({index:i,input:cases[i],expected:expected[i],actual:actual[i]});}if(directory)fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(differences,null,2));assert.equal(differences.length,0,JSON.stringify(differences.slice(0,2),null,2));console.log(`Utility shader nodes: ${cases.length} actual pinned-upstream cases passed`);
