import fs from 'node:fs';import assert from 'node:assert/strict';import {spawnSync} from 'node:child_process';import {strictDifferences} from './strict-parity.mjs';
assert.match(process.version,/^v26\./);assert.equal(typeof globalThis.gc,'function','Run Node with --expose-gc');
const expected=[],inputs=[],labels=['undefined','null','false','true','zero','negative-zero','number','NaN','Infinity','empty-string','string','astral-string','symbol','object','array'];
function input(i,symbol){return [undefined,null,false,true,0,-0,3,NaN,Infinity,'','ab','a😀b',symbol,{k:5},[5,6]][i]}
const valueDescriptor=value=>({value,writable:true,enumerable:true,configurable:true});
function encode(v,symbols){
 if(v===undefined)return {type:'undefined'};if(v===null)return null;if(typeof v==='function')return {type:'function'};
 if(typeof v==='symbol'){const index=symbols.findIndex(x=>x===v);assert.ok(index>=0,'Unexpected Symbol identity in output');return {symbol:index}}
 if(typeof v==='boolean')return v;if(typeof v==='number'){if(Number.isNaN(v))return {number:'NaN'};if(Object.is(v,-0))return {number:'-0'};if(!Number.isFinite(v))return {number:v<0?'-Infinity':'Infinity'};return v}
 if(typeof v==='string')return {stringUTF16:Array.from({length:v.length},(_,i)=>v.charCodeAt(i))};
 if(Array.isArray(v))return {array:Array.from({length:v.length},(_,i)=>encode(v[i],symbols))};
 return {object:Reflect.ownKeys(v).filter(key=>Object.getOwnPropertyDescriptor(v,key).enumerable).map(key=>[encode(key,symbols),encode(v[key],symbols)])};
}
function snapshot(o,symbols){return Reflect.ownKeys(o).map(key=>{const d=Object.getOwnPropertyDescriptor(o,key);return [encode(key,symbols),'value'in d?{value:encode(d.value,symbols),writable:d.writable,enumerable:d.enumerable,configurable:d.configurable}:{get:encode(d.get,symbols),set:encode(d.set,symbols),enumerable:d.enumerable,configurable:d.configurable}]})}
function attempt(f,symbols){try{return encode(f(),symbols)}catch(e){if(e instanceof TypeError||e instanceof RangeError)return {error:e.name};throw e}}
function keyObject(log,symbol,mode){return {get [Symbol.toPrimitive](){log.push('get');gc();if(mode===2)throw RangeError('key getter');if(mode===4)return 7;return hint=>{log.push(hint);gc();if(mode===3)throw RangeError('key call');if(mode===5)return {};return mode===0?'0':symbol}}}}
function emit(input,value){inputs.push(input);expected.push(value)}
const methods=['assign-target','assign-source','keys','values','getPrototypeOf','defineProperties-source','getOwnPropertyNames','getOwnPropertyDescriptor','getOwnPropertyDescriptors','defineProperty-target','defineProperties-target'];
for(let i=0;i<15;i++)for(const method of methods){
 const symbol=Symbol('fixture'),sourceSymbol=Symbol('source'),symbols=[symbol,sourceSymbol],value=input(i,symbol);let result;
 try{
  if(method==='assign-target'){const output=Object.assign(value,{a:1,[sourceSymbol]:2});result={value:encode(output,symbols),ownDescriptors:snapshot(output,symbols),valueOfSame:Object.is(output.valueOf(),value),sameTarget:Object.is(output,value)}}
  else if(method==='assign-source')result=encode(Object.assign({},value),symbols);
  else if(method==='getPrototypeOf'){const output=Object.getPrototypeOf(value);let prototype;if(i===2||i===3)prototype=Boolean.prototype;else if(i>=4&&i<=8)prototype=Number.prototype;else if(i===12)prototype=Symbol.prototype;else prototype=Object.getPrototypeOf(Object(value));result={object:output!==null&&typeof output==='object',samePrototype:output===prototype}}
  else if(method==='defineProperties-source')result=encode(Object.defineProperties({},value),symbols);
  else if(method==='getOwnPropertyDescriptor')result=encode(Object.getOwnPropertyDescriptor(value,'0'),symbols);
  else if(method==='defineProperty-target')result=encode(Object.defineProperty(value,'x',valueDescriptor(7)),symbols);
  else if(method==='defineProperties-target')result=encode(Object.defineProperties(value,{x:valueDescriptor(7)}),symbols);
  else result=encode(Object[method](value),symbols);
 }catch(e){if(e instanceof TypeError||e instanceof RangeError)result={error:e.name};else throw e}
 emit({kind:'primitive',input:labels[i],method},{kind:'primitive',input:labels[i],method,result});
}
for(let i=0;i<15;i++)for(const method of ['getOwnPropertyDescriptor','defineProperty'])for(let mode=0;mode<6;mode++){
 const symbol=Symbol('fixture'),keySymbol=Symbol('key'),symbols=[symbol,keySymbol],value=input(i,symbol),log=[],key=keyObject(log,keySymbol,mode);
 const result=attempt(()=>method==='defineProperty'?Object.defineProperty(value,key,valueDescriptor(9)):Object.getOwnPropertyDescriptor(value,key),symbols);
 emit({kind:'dynamic-key',input:labels[i],method,mode},{kind:'dynamic-key',input:labels[i],method,mode,result,events:encode(log,symbols)});
}
function inheritedDescriptor(log,state,label,failure,value){const prototype={},descriptor=Object.create(prototype);state[label]=descriptor;for(const field of ['enumerable','configurable','value','writable'])Object.defineProperty(prototype,field,{configurable:true,get(){log.push([label+'.'+field,this===state[label]]);gc();if(label+'.'+field===failure)throw RangeError('descriptor getter');return field==='value'?value:true}});return descriptor}
for(const method of ['defineProperty','defineProperties'])for(const failure of ['none','first.enumerable','first.value','first.writable','second.value']){
 const log=[],state={},target={prior:1},first=inheritedDescriptor(log,state,'first',failure,7),second=inheritedDescriptor(log,state,'second',failure,9),keySymbol=Symbol('key'),symbols=[keySymbol],key=keyObject(log,keySymbol,0),properties={a:first,b:second};
 const result=attempt(()=>method==='defineProperty'?Object.defineProperty(target,key,first):Object.defineProperties(target,properties),symbols);
 emit({kind:'inherited-descriptor',method,failure},{kind:'inherited-descriptor',method,failure,result,target:snapshot(target,symbols),events:encode(log,symbols)});
}
for(const method of ['assign','keys','values','getOwnPropertyNames','getOwnPropertyDescriptors','defineProperties'])for(let mode=0;mode<2;mode++){
 const log=[],base={},symbol=Symbol('source'),symbols=[symbol],define=method==='defineProperties';let proxy;
 Object.defineProperty(base,'a',{enumerable:true,configurable:true,get(){log.push(['a.get',this===proxy]);gc();if(mode)Reflect.deleteProperty(base,'b');return define?valueDescriptor(1):1}});
 base.b=define?valueDescriptor(2):2;Object.defineProperty(base,'hidden',{value:define?valueDescriptor(3):3,configurable:true});base[symbol]=define?valueDescriptor(4):4;
 proxy=new Proxy(base,{ownKeys(){log.push('ownKeys');gc();return Reflect.ownKeys(base)},getOwnPropertyDescriptor(_,key){log.push(['descriptor',key]);gc();return Object.getOwnPropertyDescriptor(base,key)},get(_,key,receiver){log.push(['get',key]);gc();return Reflect.get(base,key,receiver)}});
 const result=attempt(()=>method==='assign'||method==='defineProperties'?Object[method]({},proxy):Object[method](proxy),symbols);
 emit({kind:'proxy-source',method,mode},{kind:'proxy-source',method,mode,result,events:encode(log,symbols)});
}
{
 const prototype={},base=Object.create(prototype),log=[],proxy=new Proxy(base,{getPrototypeOf(){log.push('getPrototypeOf');gc();return prototype}}),result=Object.getPrototypeOf(proxy);
 emit({kind:'proxy-prototype'},{kind:'proxy-prototype',samePrototype:result===prototype,events:encode(log,[])});
 const symbolPrototype=Symbol.prototype,saved=Object.getOwnPropertyDescriptor(symbolPrototype,'constructor'),sentinelLog=[];Object.defineProperty(symbolPrototype,'constructor',{configurable:true,get(){sentinelLog.push('unexpected constructor getter');return 9}});const symbol=Symbol('sentinel');
 let symbolResult,names;try{symbolResult=Object.getPrototypeOf(symbol);names=Object.getOwnPropertyNames(symbol)}finally{Object.defineProperty(symbolPrototype,'constructor',saved)}
 emit({kind:'symbol-prototype-getter'},{kind:'symbol-prototype-getter',samePrototype:symbolResult===symbolPrototype,ownNames:encode(names,[]),events:encode(sentinelLog,[])});
}
assert.equal(expected.length,369);
if(process.argv[2]==='--oracle'){
 const directory=process.argv[3];assert.ok(directory);assert.equal(fs.existsSync(directory),false);fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(directory+'/input.json',JSON.stringify(inputs,null,2)+'\n');fs.writeFileSync(directory+'/expected.json',JSON.stringify(expected,null,2)+'\n');fs.writeFileSync(directory+'/metadata.json',JSON.stringify({node:process.version,scenarios:expected.length,explicitGc:true,symbolTransport:'identity index within actual fixture Symbol handles',stringTransport:'all UTF-16 code units',numberTransport:'exact finite numbers plus distinct NaN/negative-zero/infinity tags',nativeExecuted:false},null,2)+'\n');console.log(JSON.stringify({node:process.version,scenarios:expected.length,explicitGc:true,nativeExecuted:false}));
}else{
 assert.ok(process.argv[2]&&process.argv[3],'native executable and fresh D result directory required');const directory=process.argv[3];assert.equal(fs.existsSync(directory),false);assert.match(directory,/^D:[\\/]/i);fs.mkdirSync(directory,{recursive:true});const binary=process.argv[2];
 const child=spawnSync(binary,[],{encoding:'utf8',maxBuffer:64*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});fs.writeFileSync(directory+'/native-stdout.jsonl',child.stdout??'');fs.writeFileSync(directory+'/native-stderr.log',child.stderr??'');fs.writeFileSync(directory+'/process.json',JSON.stringify({binary,pid:child.pid??null,status:child.status,error:child.error?String(child.error):null,nativeExecuted:Number.isInteger(child.pid)&&child.pid>0},null,2)+'\n');
 assert.equal(child.error,undefined);assert.equal(child.status,0,child.stderr);const actual=child.stdout.trim().split(/\r?\n/).map(JSON.parse),differences=strictDifferences(expected,actual);fs.writeFileSync(directory+'/input.json',JSON.stringify(inputs,null,2)+'\n');fs.writeFileSync(directory+'/expected.json',JSON.stringify(expected,null,2)+'\n');fs.writeFileSync(directory+'/actual.json',JSON.stringify(actual,null,2)+'\n');fs.writeFileSync(directory+'/differences.json',JSON.stringify(differences,null,2)+'\n');assert.equal(differences.length,0,JSON.stringify(differences.slice(0,10)));console.log(JSON.stringify({node:process.version,scenarios:expected.length,strictDifferences:0,nativeExecuted:true}));
}
