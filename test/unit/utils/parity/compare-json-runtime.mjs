import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync,execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
const allowed=new Set(['--probe','--output','--input','--upstream']),options=new Map(),argv=process.argv.slice(2);
for(let i=0;i<argv.length;i+=2){const key=argv[i],value=argv[i+1];if(!allowed.has(key)||options.has(key)||!value||value.startsWith('--'))throw new Error('Usage: node --expose-gc compare-json-runtime.mjs --output new-directory [--probe binary] [--input cases.json] [--upstream checkout]');options.set(key,value);}
assert(options.has('--output'),'--output is required');assert.equal(process.version,'v26.5.0');assert.equal(typeof gc,'function','Use --expose-gc');
process.env.TEMP='D:/Temp/surui';process.env.TMP='D:/Temp/surui';
const directory=path.resolve(options.get('--output')),binary=options.has('--probe')?path.resolve(options.get('--probe')):null;
assert(!fs.existsSync(directory),'Output must be a new directory');if(binary)assert(fs.statSync(binary).isFile());
if(options.has('--upstream'))assert.equal(execFileSync('git',['-C',path.resolve(options.get('--upstream')),'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const JSONIntrinsic=JSON, nativeParse=JSON.parse, nativeStringify=JSON.stringify;
const flags=d=>[d.writable??false,d.enumerable??false,d.configurable??false];
const keyName=key=>typeof key==='symbol'?'@@'+key.description:key;
const simple=value=>typeof value==='number'?{type:'number',value:Number.isNaN(value)?'NaN':!Number.isFinite(value)?value>0?'Infinity':'-Infinity':Object.is(value,-0)?'-0':value}:value===undefined?'#undefined':typeof value==='symbol'?'#symbol':typeof value==='function'?'#function':value!==null&&typeof value==='object'?'#object':value;
function normal(value,seen=new Map()){
 if(value===null||typeof value!=='object')return simple(value);
 if(seen.has(value))return {reference:seen.get(value)};seen.set(value,seen.size);
 const result={type:Array.isArray(value)?'array':'object',entries:[]};if(Array.isArray(value))result.length=value.length;
 for(const key of Reflect.ownKeys(value)){if(typeof key==='symbol'||key==='length'&&Array.isArray(value))continue;const d=Object.getOwnPropertyDescriptor(value,key);if(d?.enumerable)result.entries.push([key,normal(value[key],seen),flags(d)]);}
 return result;
}
function effects(state,definition,self){
 if(definition.gc){gc();state.events.push(['gc']);}
 if(definition.nested)state.events.push(['nested',nativeStringify(nativeParse('{"nested":[1,null]}'))]);
 for(const op of definition.mutations??[]){if(op.op==='delete')Reflect.deleteProperty(self,op.key);else if(op.op==='readonly')Object.defineProperty(self,op.key,{value:build(op.value,state),writable:false,enumerable:true,configurable:false});else if(op.op==='preventExtensions')Object.preventExtensions(self);else self[op.key]=build(op.value,state);state.events.push(['mutation',op.op,op.key??'']);}
 if(definition.throw)throw new TypeError(definition.name??'fixture');
}
function callback(state,definition,role){return function(...args){
 const key=args[0],value=role==='toJSON'?this:args[1];
 state.events.push([role,key,simple(value),args.length,role==='reviver'?(Object.hasOwn(args[2],'source')?args[2].source:'#none'):'#none']);
 effects(state,definition,this);
 for(const rule of definition.rules??[])if(rule.key===key){effects(state,rule,this);if(Object.hasOwn(rule,'result'))return build(rule.result,state);}
 if(definition.mode==='numbersPlus10'&&typeof value==='number')return value+10;
 if(definition.mode==='omitDrop'&&key==='drop')return undefined;
 if(Object.hasOwn(definition,'result'))return build(definition.result,state);
 return value;
};}
function build(definition,state){
 if(definition===null||typeof definition!=='object')return definition;
 const type=definition.$;switch(type){
 case 'undefined':case 'hole':return undefined;case 'NaN':return NaN;case 'Infinity':return Infinity;case '-Infinity':return -Infinity;case '-0':return -0;
 case 'symbol':return Symbol(definition.name??'fixture');case 'ref':return state.refs[definition.id];
 case 'raw':return JSONIntrinsic.rawJSON(build(definition.value,state));
 case 'function':return callback(state,definition,'function');
 case 'boxedNumber':case 'boxedString':case 'boxedBoolean':{const value=build(definition.value,state),object=type==='boxedNumber'?new Number(value):type==='boxedString'?new String(value):new Boolean(value);if(definition.id)state.refs[definition.id]=object;if(Object.hasOwn(definition,'convert')){object[Symbol.toPrimitive]=function(hint){state.events.push(['convert',definition.name??type,hint]);effects(state,definition,this);return build(definition.convert,state);};}return object;}
 case 'convert':{const object={};object[Symbol.toPrimitive]=function(hint){state.events.push(['convert',definition.name??'',hint]);effects(state,definition,this);return build(definition.value,state);};return object;}
 case 'proxy':{const target=build(definition.value,state),handler={get(t,k,r){state.events.push(['get',keyName(k)]);if(definition.throwGet===keyName(k))throw new TypeError('get');for(const override of definition.overrides??[])if(override.key===keyName(k))return build(override.value,state);return Reflect.get(t,k,r);},ownKeys(t){state.events.push(['ownKeys']);if(definition.throwKeys)throw new TypeError('keys');return definition.keys??Reflect.ownKeys(t);},getOwnPropertyDescriptor(t,k){state.events.push(['descriptor',keyName(k)]);if(definition.throwDescriptor===keyName(k))throw new TypeError('descriptor');return Reflect.getOwnPropertyDescriptor(t,k);},defineProperty(t,k,d){state.events.push(['define',keyName(k)]);return Reflect.defineProperty(t,k,d);},deleteProperty(t,k){state.events.push(['delete',keyName(k)]);return Reflect.deleteProperty(t,k);}};if(definition.revoked){const p=Proxy.revocable(target,handler);p.revoke();return p.proxy;}return new Proxy(target,handler);}
 case 'array':case 'object':{
  const object=type==='array'?[]:{};if(definition.id)state.refs[definition.id]=object;
  if(type==='array'){for(let i=0;i<(definition.items??[]).length;++i)if(definition.items[i]?.$!=='hole')Object.defineProperty(object,String(i),{value:build(definition.items[i],state),writable:true,enumerable:true,configurable:true});object.length=(definition.items??[]).length;}
  for(const property of definition.props??[]){const key=property.symbol?Symbol(property.key):property.key;if(property.getter){Object.defineProperty(object,key,{enumerable:property.enumerable??true,configurable:property.configurable??true,get(){state.events.push(['getter',keyName(key)]);effects(state,property,this);return build(property.value,state);}});}else Object.defineProperty(object,key,{value:build(property.value,state),writable:property.writable??true,enumerable:property.enumerable??true,configurable:property.configurable??true});}
  if(definition.prototype)Object.setPrototypeOf(object,build(definition.prototype,state));
  if(definition.toJSON)Object.defineProperty(object,'toJSON',{value:callback(state,definition.toJSON,'toJSON'),writable:true,configurable:true});
  return object;
 }
 default:throw new Error('Unknown fixture type '+type);
 }
}
function metadata(){const out={keys:Reflect.ownKeys(JSONIntrinsic).map(keyName),tag:JSONIntrinsic[Symbol.toStringTag],tagFlags:flags(Object.getOwnPropertyDescriptor(JSONIntrinsic,Symbol.toStringTag)),globalFlags:flags(Object.getOwnPropertyDescriptor(globalThis,'JSON')),objectPrototype:Object.getPrototypeOf(JSONIntrinsic)===Object.prototype};for(const name of ['parse','stringify','rawJSON','isRawJSON']){const fn=JSONIntrinsic[name];let constructible=true;try{Reflect.construct(function(){},[],fn);}catch{constructible=false;}out[name]={name:fn.name,length:fn.length,source:Function.prototype.toString.call(fn),identity:fn===JSONIntrinsic[name],constructible,prototype:Object.hasOwn(fn,'prototype'),nameFlags:flags(Object.getOwnPropertyDescriptor(fn,'name')),lengthFlags:flags(Object.getOwnPropertyDescriptor(fn,'length')),propertyFlags:flags(Object.getOwnPropertyDescriptor(JSONIntrinsic,name))};}return out;}
function run(request){
 if(request.kind==='metadata')return metadata();
 const state={events:[],refs:{}},out={};
 try{
  let result;
  if(request.kind==='parseDepth'){result=JSONIntrinsic.parse(request.text);let depth=0;while(Array.isArray(result)){++depth;result=result[0];}out.depth=depth;}
  else if(request.kind==='parse')result=Reflect.apply(JSONIntrinsic.parse,build(request.receiver,state),[build(request.text,state),request.reviver?callback(state,request.reviver,'reviver'):build(request.reviverValue,state),build(request.extra,state)]);
  else if(request.kind==='stringify')result=Reflect.apply(JSONIntrinsic.stringify,build(request.receiver,state),[build(request.value,state),request.replacer?callback(state,request.replacer,'replacer'):build(request.replacerValue,state),build(request.space,state),build(request.extra,state)]);
  else if(request.kind==='raw'){
   result=JSONIntrinsic.rawJSON(build(request.text,state));out.brand=JSONIntrinsic.isRawJSON(result);out.proxyBrand=JSONIntrinsic.isRawJSON(new Proxy(result,{}));out.nullPrototype=Object.getPrototypeOf(result)===null;out.extensible=Object.isExtensible(result);out.flags=flags(Object.getOwnPropertyDescriptor(result,'rawJSON'));out.stringified=JSONIntrinsic.stringify(result);
  }else if(request.kind==='isRaw')result=JSONIntrinsic.isRawJSON(build(request.value,state));
  else if(request.kind==='construct')result=Reflect.construct(JSONIntrinsic[request.name],[build(request.value,state)]);
  else if(request.kind==='binding'){
   const object=globalThis.JSON,fn=object.stringify;delete globalThis.JSON;out.saved=fn({x:1});try{JSON;}catch(e){out.lookup='#'+e.name;}Object.defineProperty(globalThis,'JSON',{value:object,writable:true,configurable:true});gc();result=object.stringify===fn;
  }else throw new Error('Unknown request kind');
  out.result=normal(result);
 }catch(error){out.result='#'+error.name;}
 out.events=state.events;return out;
}
const tag=($,extra={})=>({$,...extra}),U=tag('undefined'),H=tag('hole'),N=tag('NaN'),P=tag('Infinity'),M=tag('-Infinity'),Z=tag('-0'),S=tag('symbol'),F=tag('function');
const obj=(props=[],extra={})=>tag('object',{props:props.map(([key,value,options={}])=>({key,value,...options})),...extra}),arr=(items,extra={})=>tag('array',{items,...extra});
function generatedCases(){
 const cases=[{kind:'metadata'}],add=(kind,more)=>cases.push({kind,...more});
 const valid=['null','true','false','0','-0','1','-1','1e400','-1e400','1e-4000','-1e-4000','9007199254740993','1.234567890123456789','[1,null,true,"x"]','{}','[]','{"__proto__":1,"constructor":2}','{"b":1,"2":2,"1":3,"a":4}','{"a":1,"a":2}','{"x":"\\ud800","pair":"\\ud83d\\ude00"}','"\\u0000\\b\\f\\n\\r\\t\\/\\\\\\\""','"\ud83d\ude00\ud800\udfff"',' \t\r\n{" a ":[0,-0]}\n'];
 const invalid=['',' ','undefined','NaN','Infinity','+1','.1','1.','01','-01','--1','1e','1e+','0x10','true false','nullx','[1,]','[,1]','{"a":1,}','{a:1}',"{'a':1}",'[','{','"','"\\x20"','"\\u12"','"a\nb"','\ufeff0','\u00a00','0\u2028','[1 2]','{"a" 1}'];
 for(const text of valid){add('parse',{text});for(const reviver of [{},{mode:'numbersPlus10'},{mode:'omitDrop'},{gc:true,nested:true}])add('parse',{text,reviver});add('raw',{text});}
 for(const text of invalid){add('parse',{text});add('parse',{text,reviver:{throw:true}});add('raw',{text});}
 for(const primitive of [U,null,false,true,0,Z,1,N,P,M,S,F,arr([1,2]),obj()])add('parse',{text:primitive});
 const values=[U,null,false,true,0,Z,1,-1,1e-7,1e-6,1e20,1e21,Number.MIN_VALUE,Number.MAX_VALUE,N,P,M,S,F,'','quote"\\\n\u0000','\ud800','\udfff','\ud83d\ude00','\ud800x\udfff',obj(),arr([]),arr([U,H,F,S,N,P,M,Z]),obj([['drop',U],['fn',F],['sym',S],['n',N],['z',Z],['0','zero'],['x',7,{enumerable:false}],['s',2,{symbol:true}]]),obj([['a',1],['b',arr([2,obj([['c',3]])])]]),tag('boxedNumber',{value:3}),tag('boxedNumber',{value:N}),tag('boxedString',{value:'xy'}),tag('boxedBoolean',{value:false})];
 const spaces=[U,null,0,1,2,10,11,2.9,-2,N,P,M,'','\t','abcdefghijk','\ud83d\ude00\ud83d\ude00\ud83d\ude00\ud83d\ude00x\ud83d\ude00','\ud800x',tag('boxedNumber',{value:3}),tag('boxedString',{value:'--'}),tag('convert',{name:'unusedSpace',value:2,throw:true})];
 for(const value of values)for(const space of spaces)add('stringify',{value,space});
 const lists=[arr([]),arr(['a','a','b',1,'1',null,true,U,S,obj(),tag('boxedNumber',{value:2}),tag('boxedString',{value:'z'})]),tag('proxy',{value:arr(['b','a','a','absent'])}),obj([['0','a'],['length',1]])];
 for(const value of values){for(const replacerValue of lists)add('stringify',{value,replacerValue,space:2});for(const replacer of [{},{mode:'numbersPlus10'},{mode:'omitDrop'},{gc:true,nested:true}])add('stringify',{value,replacer});}
 for(const value of [0,S,obj(),U])for(const name of ['text','space','list']){const converted=tag('convert',{name,value});if(name==='text')add('parse',{text:converted,reviver:{}});if(name==='space')add('stringify',{value:obj([['x',1]]),space:tag('boxedNumber',{value:1,convert:converted,name:'space'})});if(name==='list')add('stringify',{value:obj([['a',1]]),replacerValue:arr([tag('boxedString',{value:'a',convert:converted,name:'item'})])});}
 for(const text of ['{"a":1,"b":2,"drop":3}','[1,2,3]','{"a":1,"b":{"x":2}}','{"a":1,"b":-0}','{"a":1,"b":1e400}','{"a":1,"b":"x"}'])for(const mutation of [{op:'delete',key:'b'},{op:'set',key:'b',value:7},{op:'set',key:'b',value:obj([['new',8]])},{op:'readonly',key:'b',value:2},{op:'preventExtensions'},{op:'set',key:'3',value:9}])add('parse',{text,reviver:{rules:[{key:'a',mutations:[mutation]},{key:'0',mutations:[mutation]}]}});
 for(const text of ['{"a":1,"b":2}','[1,2]','true'])for(const key of ['a','b','0','1',''])for(const result of [U,0,Z,S,F,obj([['r',1]])])add('parse',{text,reviver:{rules:[{key,result}]}});
 for(const key of ['a','b','0','1',''])add('parse',{text:'{"a":1,"b":[2,3]}',reviver:{rules:[{key,throw:true}]}});
 const getter=obj([['a',1,{getter:true,mutations:[{op:'delete',key:'b'},{op:'set',key:'c',value:3}]}],['b',2,{getter:true}]]);
 for(const replacer of [{},{gc:true,nested:true},{rules:[{key:'a',throw:true}]},{rules:[{key:'a',result:U}]}])add('stringify',{value:getter,replacer});
 for(const result of [U,null,7,S,F,arr([1,H]),obj([['a',2]])])for(const replacer of [{},{mode:'numbersPlus10'}])add('stringify',{value:obj([['ignored',1,{getter:true,throw:true}]],{toJSON:{result,gc:true,nested:true}}),replacer});
 for(const kind of ['object','array']){const value=kind==='object'?obj([['self',tag('ref',{id:'cycle'})]],{id:'cycle'}):arr([tag('ref',{id:'cycle'})],{id:'cycle'});add('stringify',{value});add('stringify',{value,replacer:{rules:[{key:kind==='object'?'self':'0',result:U}]}});}
 add('stringify',{value:arr([obj([['x',1]],{id:'shared'}),tag('ref',{id:'shared'})])});
 for(const value of [obj([['a',1],['b',2,{enumerable:false}],['c',3]]),arr([1,H,U]),F])for(const extra of [{},{throwGet:'toJSON'},{throwGet:'a'},{throwKeys:true},{throwDescriptor:'c'},{revoked:true}])add('stringify',{value:tag('proxy',{value,...extra})});
 for(const text of ['1','-0','1e400','9007199254740993','"\\ud800"','true','null','[]','{}',' 1','1 ']){add('raw',{text});add('stringify',{value:obj([['raw',tag('raw',{value:text})]])});}
 for(const value of values)add('isRaw',{value});for(const name of ['parse','stringify','rawJSON','isRawJSON'])add('construct',{name,value:tag('convert',{name:'mustNotConvert',value:'1',throw:true})});
 for(const method of [U,null,3,F,tag('function',{result:7}),tag('function',{throw:true})])for(const getter of [false,true])add('stringify',{value:obj([['toJSON',method,{getter}],['x',1,{getter:true}]])});
 for(const type of ['boxedNumber','boxedString','boxedBoolean'])for(const converted of [0,'abc',false,S,obj(),U])add('stringify',{value:tag(type,{value:type==='boxedString'?'hi':type==='boxedBoolean'?true:2,convert:converted,name:type,gc:true})});
 const inherited=obj([['inherited',3,{getter:true}],['1',7,{getter:true}]]),whitelist=arr(['hidden','inherited','own','absent']);
 add('stringify',{value:obj([['hidden',1,{enumerable:false}],['own',2]],{prototype:inherited}),replacerValue:whitelist});
 add('stringify',{value:arr([1,H,H],{prototype:inherited})});
 add('stringify',{value:arr([H,2,3],{props:[{key:'0',getter:true,value:9,mutations:[{op:'set',key:'length',value:1}]}]})});
 for(const length of [0,2,3.8,N,tag('convert',{name:'length',value:2,gc:true}),tag('convert',{name:'length',value:2,throw:true})])for(const kind of ['array','replacer']){
  const proxy=tag('proxy',{value:arr(kind==='array'?[1,2,3]:['a','b','c']),overrides:[{key:'length',value:length}]});
  if(kind==='array')add('stringify',{value:proxy});else add('stringify',{value:obj([['a',1],['b',2],['c',3]]),replacerValue:proxy});
 }
 add('stringify',{value:obj([['a',1]]),replacerValue:arr([tag('boxedString',{value:'a',name:'one',convert:'a'}),tag('boxedString',{value:'a',name:'two',convert:'a'})])});
 for(const value of [U,S,obj(),F]){add('parse',{text:'{"x":1}',reviverValue:value,extra:tag('convert',{name:'unused',value:2,throw:true})});add('stringify',{value:obj([['x',1]]),replacerValue:value,extra:tag('convert',{name:'unused',value:2,throw:true})});}
 for(const [text,replacement] of [['{"a":1,"b":2}',2],['{"a":1,"b":-0}',0],['{"a":1,"b":-0}',Z],['{"a":1,"b":1e400}',P],['{"a":1,"b":{"x":2}}',obj([['x',2]])]])add('parse',{text,reviver:{rules:[{key:'a',mutations:[{op:'set',key:'b',value:replacement}]}]}});
 for(const result of [U,7])add('parse',{text:'{"a":1,"b":2}',reviver:{rules:[{key:'a',mutations:[{op:'readonly',key:'b',value:2}]},{key:'b',result}]}});
 for(const receiver of [U,null,false,3,'ignored',S,obj()]){add('parse',{text:'1',receiver});add('stringify',{value:obj([['x',1]]),receiver});}
 for(const string of [Array.from({length:32},(_,i)=>String.fromCharCode(i)).join(''),'\u2028\u2029\ufeff\uffff','\ud800\ud800','\udc00\udc00','\udbff\udfff','\udbffx\udfff']){add('stringify',{value:string});add('parse',{text:nativeStringify(string),reviver:{}});add('raw',{text:nativeStringify(string)});}
 for(const text of ['-0e0','0e-9999','-0e-9999','4.9406564584124654e-324','-4.9406564584124654e-324','2.2250738585072014e-308','1.7976931348623157e308','1.7976931348623159e308','0.1000000000000000055511151231257827021181583404541015625']){add('parse',{text,reviver:{}});add('raw',{text});}
 for(const value of [tag('proxy',{value:obj(),revoked:true}),tag('proxy',{value:tag('raw',{value:'1'})}),obj([['rawJSON','1']])])add('isRaw',{value});
 for(const reviverValue of [tag('proxy',{value:tag('function')}),tag('proxy',{value:tag('function'),revoked:true})])add('parse',{text:'{"x":1}',reviverValue});
 for(const replacerValue of [tag('proxy',{value:tag('function')}),tag('proxy',{value:tag('function'),revoked:true}),tag('proxy',{value:arr(['x']),revoked:true}),tag('proxy',{value:arr(['x']),throwGet:'length'}),arr([tag('boxedString',{value:'x',convert:'x',throw:true})])])add('stringify',{value:obj([['x',1]]),replacerValue,space:tag('boxedString',{value:' ',convert:'--',name:'space'})});
 add('parse',{text:tag('convert',{name:'text',value:'1',throw:true}),reviver:{}});
 for(const depth of [1,128,1024,8192])add('parseDepth',{text:'['.repeat(depth)+'0'+']'.repeat(depth)});
 add('binding',{});cases.push({kind:'metadata'});
 // Append only: the frozen 1573 requests and their expectations remain intact.
 assert.equal(cases.length,1573);
 const roots=[obj([['toJSON',U,{getter:true}],['x',1,{getter:true}]]),obj([['x',1,{getter:true}]],{toJSON:{gc:true,nested:true,result:obj([['x',2,{getter:true}]])}})];
 const replacerSpaces=[['undefined',U],['string',tag('boxedString',{value:' ',convert:'--',name:'space'})],['stringThrow',tag('boxedString',{value:' ',convert:'--',name:'space',throw:true})],['number',tag('boxedNumber',{value:1,convert:2,name:'space'})],['numberThrow',tag('boxedNumber',{value:1,convert:2,name:'space',throw:true})],['ignoredObject',tag('convert',{name:'ignoredSpace',value:2,throw:true})]];
 const wrap=(value,revoked=false)=>tag('proxy',{value,revoked});
 for(const [targetKind,target] of [['callable',F],['array',arr(['x'])],['object',obj([['x',1]])]]){
  const shapes=[['direct',target],['live',wrap(target)],['nestedLive',wrap(wrap(target))],['revoked',wrap(target,true)],['innerRevoked',wrap(wrap(target,true))],['outerRevoked',wrap(wrap(target),true)],['bothRevoked',wrap(wrap(target,true),true)]];
  for(const [replacerShape,replacerValue] of shapes)for(const [spaceKind,space] of replacerSpaces)for(let rootKind=0;rootKind<roots.length;++rootKind)add('stringify',{value:roots[rootKind],replacerValue,space,targetKind,replacerShape,spaceKind,rootKind});
 }
 cases.push({kind:'metadata'});return cases;
}
const cases=options.has('--input')?nativeParse(fs.readFileSync(options.get('--input'),'utf8')):generatedCases();assert(Array.isArray(cases)&&cases.length>0);
for(const request of cases)assert(request&&['parse','parseDepth','stringify','raw','isRaw','construct','binding','metadata'].includes(request.kind),'Invalid case');
fs.mkdirSync(directory,{recursive:true});
const digest=data=>createHash('sha256').update(data).digest('hex'),write=(name,value)=>fs.writeFileSync(path.join(directory,name),nativeStringify(value,null,2)+'\n');
const input=cases.map(value=>nativeStringify(value)).join('\n')+'\n';write('input.json',cases);fs.writeFileSync(path.join(directory,'input.jsonl'),input);
const summary={status:'oracle-running',cases:cases.length,node:process.version,v8:process.versions.v8,scriptSha256:digest(fs.readFileSync(fileURLToPath(import.meta.url))),inputSha256:digest(input),nativeExecuted:false,categories:Object.fromEntries([...new Set(cases.map(x=>x.kind))].map(kind=>[kind,cases.filter(x=>x.kind===kind).length])),boundaries:['No BigInt JSValue or cross-realm values','SyntaxError class is asserted; engine-specific message wording is not','No native build or runtime success is inferred from Node oracle']};
try{
 const expected=cases.map(run);write('expected.json',expected);summary.expectedSha256=digest(fs.readFileSync(path.join(directory,'expected.json')));
 if(!options.has('--input')){assert.deepEqual(expected[0],expected.at(-1));assert.equal(expected[0].parse.length,2);assert.equal(expected[0].stringify.length,3);assert.equal(expected[0].rawJSON.length,1);assert.equal(expected[0].isRawJSON.length,1);assert(cases.some((r,i)=>r.kind==='parse'&&r.text==='-0'&&expected[i].result?.value==='-0'));assert(cases.some((r,i)=>r.kind==='parse'&&r.reviver&&expected[i].events.some(e=>e[0]==='reviver'&&e[4]==='9007199254740993')));assert(cases.some((r,i)=>r.kind==='stringify'&&r.value?.id==='cycle'&&expected[i].result==='#TypeError'));}
 if(binary){summary.binary=binary;summary.binaryBeforeSha256=digest(fs.readFileSync(binary));const child=spawnSync(binary,[],{input,encoding:'utf8',maxBuffer:256*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});summary.nativeExecuted=child.pid!==undefined;fs.writeFileSync(path.join(directory,'native-stdout.jsonl'),child.stdout??'');fs.writeFileSync(path.join(directory,'native-stderr.log'),child.stderr??'');summary.nativeExit={status:child.status,signal:child.signal,error:child.error?.message??null,pid:child.pid??null};write('native-exit.json',summary.nativeExit);try{summary.binaryAfterSha256=digest(fs.readFileSync(binary));}catch(e){summary.binaryAfterSha256=null;summary.hashError=String(e);}let actual=null;const differences=[];try{actual=(child.stdout??'').trimEnd().split(/\r?\n/).filter(Boolean).map(nativeParse);}catch(e){differences.push({kind:'invalid-output',error:String(e)});}write('actual.json',actual);if(actual){if(actual.length!==expected.length)differences.push({kind:'count',expected:expected.length,actual:actual.length});for(let i=0;i<expected.length;++i)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({index:i,input:cases[i],expected:expected[i],actual:actual[i]??null});}}write('differences.json',differences);summary.differences=differences.length;summary.status=child.status!==0?'native-process-failed':summary.binaryBeforeSha256!==summary.binaryAfterSha256?'binary-changed':differences.length?'native-differences':'native-passed';assert.equal(child.status,0);assert.equal(summary.binaryBeforeSha256,summary.binaryAfterSha256);assert.equal(differences.length,0,nativeStringify(differences.slice(0,2)));}
 else {summary.status='oracle-only-native-not-built';summary.binaryBeforeSha256=summary.binaryAfterSha256=null;write('actual.json',null);write('differences.json',[{kind:'native-not-executed'}]);write('native-exit.json',{executed:false,status:null,signal:null,pid:null});fs.writeFileSync(path.join(directory,'native-stdout.jsonl'),'');fs.writeFileSync(path.join(directory,'native-stderr.log'),'');}
}catch(error){summary.error=String(error);if(summary.status==='oracle-running')summary.status='oracle-failed';throw error;}finally{summary.scriptAfterSha256=digest(fs.readFileSync(fileURLToPath(import.meta.url)));summary.inputAfterSha256=digest(fs.readFileSync(path.join(directory,'input.jsonl')));write('hashes-before-after.json',{script:{before:summary.scriptSha256,after:summary.scriptAfterSha256},input:{before:summary.inputSha256,after:summary.inputAfterSha256},binary:{before:summary.binaryBeforeSha256??null,after:summary.binaryAfterSha256??null}});write('summary.json',summary);console.log(nativeStringify(summary));}
