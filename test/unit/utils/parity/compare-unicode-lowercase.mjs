import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {execFileSync} from 'node:child_process';

const probe=path.resolve(process.argv[2]);
const directory=process.argv[3]&&path.resolve(process.argv[3]);
assert.equal(process.versions.unicode,'17.0','This oracle requires the pinned Unicode 17 runtime');
if(directory)fs.mkdirSync(directory,{recursive:true});
const units=text=>Array.from({length:text.length},(_,i)=>text.charCodeAt(i));
const encode=text=>units(text).map(x=>x.toString(16)).join(',');
const manifest={node:process.version,unicode:process.versions.unicode,runs:[]};
function compare(mode,sources,label){
    const raw=execFileSync(probe,[mode],{encoding:'utf8',maxBuffer:256*1024*1024}).replaceAll('\r\n','\n');
    if(directory)fs.writeFileSync(path.join(directory,label+'.actual.hex'),raw);
    const hash=createHash('sha256');let cursor=0,count=0;const failures=[];
    for(const source of sources){
        const expected=encode(source.toLowerCase()),end=raw.indexOf('\n',cursor);
        const actual=end<0?raw.slice(cursor):raw.slice(cursor,end);
        hash.update(expected+'\n');
        if(actual!==expected&&failures.length<8)failures.push({index:count,input:encode(source),expected,actual});
        cursor=end<0?raw.length:end+1;++count;
    }
    const run={label,count,expectedSHA256:hash.digest('hex'),actualSHA256:createHash('sha256').update(raw).digest('hex'),trailingBytes:raw.length-cursor,failures};
    manifest.runs.push(run);if(directory)fs.writeFileSync(path.join(directory,'manifest.json'),JSON.stringify(manifest,null,2));
    assert.equal(cursor,raw.length,label+' trailing output');assert.equal(run.actualSHA256,run.expectedSHA256,JSON.stringify(run,null,2));
    return count;
}
function* allPoints(){for(let p=0;p<=0x10ffff;++p)yield String.fromCodePoint(p);}
function* contexts(){for(let p=0;p<=0x10ffff;++p){const text=String.fromCodePoint(p);yield text+'Σ';yield 'A'+text+'Σ';yield 'AΣ'+text;yield 'AΣ'+text+'A';}}
const pointCount=compare('--lower-all',allPoints(),'all-codepoints');
const contextCount=compare('--lower-context-all',contexts(),'all-sigma-contexts');

let random=0x8437c159;
function next(){random^=random<<13;random^=random>>>17;random^=random<<5;return random>>>0;}
const samples=['','ΟΣ','ΟΣΑ','Σ','ΣΣ','AΣΣ','AΣΣA','\u0345Σ','a\u0345Σ','aΣ\u0345','aΣ\u02b0','\u02b0Σ','Iİıi','\ud800','\udc00','\ud800AΣ\udc00','\ud801\udc00Σ','A\u0301\u0345Σ\u0301\u0345','AΣ\u0301\u0345B','AΣ\u0301\u0345!','A'+('\u0301\u0345').repeat(20000)+'Σ'+('\u0301\u0345').repeat(20000)+'A'];
for(let i=0;i<10000;++i){let text='';const length=next()%65;for(let j=0;j<length;++j)text+=next()%4===0?'Σ':next()%2?String.fromCharCode(next()&0xffff):String.fromCodePoint(next()%0x110000);samples.push(text);}
const sampleInput=samples.map(s=>encode(s).replaceAll(',',' ')).join('\n')+'\n';
const sampleOutput=execFileSync(probe,['--lower'],{input:sampleInput,encoding:'utf8',maxBuffer:64*1024*1024}).replaceAll('\r\n','\n');
const sampleExpected=samples.map(s=>encode(s.toLowerCase())).join('\n')+'\n';
if(directory){fs.writeFileSync(path.join(directory,'context-strings.input.hex'),sampleInput);fs.writeFileSync(path.join(directory,'context-strings.expected.hex'),sampleExpected);fs.writeFileSync(path.join(directory,'context-strings.actual.hex'),sampleOutput);}
assert.equal(sampleOutput,sampleExpected,'UTF-16 context strings');

const string=text=>({kind:'string',units:units(text)});
const object=(result,extra={})=>({kind:'object',result,...extra});
const cases=['charAt','toLowerCase'].map(method=>({kind:'metadata',method}));
for(const text of ['','A😀Z','\ud800A\udc00','\u0000A\u0000','ΟΣ','İ','\ud801\udc00'])for(const position of [undefined,{kind:'undefined'},{kind:'null'},{kind:'nan'},{kind:'infinity'},{kind:'negativeInfinity'},{kind:'negativeZero'},...[-3,-0.5,0,0.9,1,1.9,2,3,4,99].map(value=>({kind:'number',value})),string('2'),string('bad'),{kind:'boolean',value:true}])cases.push({method:'charAt',receiver:string(text),...(position?{position}:{})});
for(const method of ['charAt','toLowerCase'])for(const receiver of [string('ΟΣİ'),{kind:'boxed',units:units('ΟΣİ')},{kind:'number',value:123.5},{kind:'boolean',value:false},{kind:'null'},{kind:'undefined'},{kind:'symbol'},{kind:'plainObject'},object(string('ΟΣİ')),object({kind:'number',value:123}),object({kind:'symbol'}),object({kind:'plainObject'}),object(string('A'),{throwGet:true}),object(string('A'),{throwConvert:true})])for(const position of [{kind:'undefined'},object({kind:'number',value:1}),object({kind:'number',value:1},{throwGet:true}),object({kind:'number',value:1},{throwConvert:true}),{kind:'symbol'}])cases.push({method,receiver,position});
function decode(spec,state,role){
    if(spec.kind==='undefined')return undefined;if(spec.kind==='null')return null;
    if(spec.kind==='number'||spec.kind==='boolean')return spec.value;
    if(spec.kind==='nan')return NaN;if(spec.kind==='infinity')return Infinity;if(spec.kind==='negativeInfinity')return -Infinity;if(spec.kind==='negativeZero')return -0;
    if(spec.kind==='symbol')return Symbol('value');if(spec.kind==='string')return String.fromCharCode(...spec.units);if(spec.kind==='boxed')return new String(String.fromCharCode(...spec.units));
    const value={};if(spec.kind!=='object')return value;const result=decode(spec.result,state,role+'.result');
    Object.defineProperty(value,Symbol.toPrimitive,{configurable:true,get(){state.events.push(['get',role]);if(spec.throwGet)throw new TypeError('get converter');return function(hint){state.events.push(['convert',role,hint,this===value]);if(spec.throwConvert)throw new TypeError('conversion');return result;};}});return value;
}
const expected=cases.map(r=>{const method=String.prototype[r.method];if(r.kind==='metadata'){const descriptor=Object.getOwnPropertyDescriptor(String.prototype,r.method);return {name:method.name,length:method.length,constructible:Object.hasOwn(method,'prototype'),prototypeOwn:Object.hasOwn(method,'prototype'),writable:descriptor.writable,enumerable:descriptor.enumerable,configurable:descriptor.configurable};}const state={events:[]},receiver=decode(r.receiver,state,'receiver'),args=Object.hasOwn(r,'position')?[decode(r.position,state,'position')]:[];let result;try{result=units(method.call(receiver,...args));}catch(e){result='#'+e.name;}return {result,events:state.events};});
if(directory){fs.writeFileSync(path.join(directory,'methods.input.json'),JSON.stringify(cases));fs.writeFileSync(path.join(directory,'methods.expected.json'),JSON.stringify(expected,null,2));}
const actual=JSON.parse(execFileSync(probe,['--methods'],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:32*1024*1024}));
if(directory)fs.writeFileSync(path.join(directory,'methods.actual.json'),JSON.stringify(actual,null,2));
assert.equal(actual.length,expected.length);const differences=[];for(let i=0;i<cases.length;++i)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({index:i,input:cases[i],expected:expected[i],actual:actual[i]});}
if(directory)fs.writeFileSync(path.join(directory,'methods.differences.json'),JSON.stringify(differences,null,2));assert.equal(differences.length,0,JSON.stringify(differences.slice(0,5),null,2));
console.log(`Unicode lower: ${pointCount} codepoints, ${contextCount} Sigma contexts, ${samples.length} UTF-16 strings and ${cases.length} native String method scenarios match Node ${process.version} / Unicode ${process.versions.unicode}`);
