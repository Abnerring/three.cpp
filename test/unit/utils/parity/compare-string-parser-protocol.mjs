import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { mkdirSync,writeFileSync } from 'node:fs';
import path from 'node:path';
const [ executable,output ]=process.argv.slice(2),fixtures=[];
const decode=v=>v==='#undefined'?undefined:v==='#NaN'?NaN:v==='#Infinity'?Infinity:v==='#-Infinity'?-Infinity:v;
for(const text of ['', 'abcdef','A😀B','\ud800x\udfff'])for(const start of [-Infinity,-8,-1,-0,0.5,2,5,Infinity,NaN])for(const end of [undefined,-3,1,4,Infinity,NaN])fixtures.push({kind:'substring',text,start:Number.isNaN(start)?'#NaN':start===Infinity?'#Infinity':start===-Infinity?'#-Infinity':start,end:end===undefined?'#undefined':Number.isNaN(end)?'#NaN':end===Infinity?'#Infinity':end});
for(const text of ['', 'ab abc','123 ab 456'])for(const [pattern,flags] of [['(a)(b)?',''],['[a-z]+','g'],['(\\d+)',''],['^z',''],['','g']])fixtures.push({kind:'match',text,pattern,flags,regexp:true});
for(const pattern of ['#undefined',null,'(a)(b)?','x'])fixtures.push({kind:'match',text:'ab abc',pattern});
fixtures.push({kind:'protocol',mode:'match-custom'},{kind:'protocol',mode:'substring-coercion'});
const expected=fixtures.map(r=>{
    const out={},events=[];
    try {
        let value;
        if(r.kind==='substring')value=String.prototype.substring.call(r.text,decode(r.start),decode(r.end));
        else if(r.kind==='match'){
            value=r.text.match(r.regexp?new RegExp(decode(r.pattern),r.flags):decode(r.pattern));
            if(value!==null){out.index=value.index===undefined?'#undefined':value.index;out.input=value.input===undefined?'#undefined':value.input;}
        }else{
            const receiver={toString(){events.push('text');return 'abcdef';}};
            if(r.mode==='match-custom'){
                const pattern={get [Symbol.match](){events.push('get-match');return function(value){events.push('call-match');return value===receiver&&this===pattern;};}};
                value=String.prototype.match.call(receiver,pattern);
            }else value=String.prototype.substring.call(receiver,{valueOf(){events.push('start');return 4;}},{valueOf(){events.push('end');return 1;}});
        }
        out.value=value===undefined?'#undefined':value;
    }catch(e){out.error=e.name;}
    out.events=events;return JSON.parse(JSON.stringify(out));
});
const input=fixtures.map(x=>JSON.stringify(x)).join('\n')+'\n';
if(output){mkdirSync(output,{recursive:true});writeFileSync(path.join(output,'fixtures.jsonl'),input);writeFileSync(path.join(output,'expected.jsonl'),expected.map(x=>JSON.stringify(x)).join('\n')+'\n');}
const text=execFileSync(executable,[],{input,encoding:'utf8',maxBuffer:4*1024*1024});
const actual=text.trim().split(/\r?\n/).map(x=>JSON.parse(x));
if(output)writeFileSync(path.join(output,'actual.jsonl'),text);
assert.equal(actual.length,expected.length);for(let i=0;i<expected.length;++i)assert.deepEqual(actual[i],expected[i],`row ${i}: ${JSON.stringify(fixtures[i])}`);
console.log(`String match/substring: ${expected.length} native JavaScript protocol scenarios match.`);
