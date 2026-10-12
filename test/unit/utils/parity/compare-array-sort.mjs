import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
if(process.argv.length!==3)throw new Error('Usage: node compare-array-sort.mjs <probe.exe>');
assert.equal(process.version,'v26.5.0','Array sort comparison order is pinned to this Node release');
assert.equal(process.versions.v8,'14.6.202.34-node.24');
let state=0x7f4a7c15;
function next(){state^=state<<13;state^=state>>>17;state^=state<<5;return state>>>0;}
const cases=[];
for(const count of [0,1,2,3,7,16,31,32,33,63,64,65,127,128,129,255,256,257,511,512,1023,1024,2048,4096]){
    for(let mode=0;mode<8;++mode){
        for(let pattern=0;pattern<6;++pattern){
            const values=Array.from({length:count},(_,i)=>{
                if(pattern===0)return next()%257-128;
                if(pattern===1)return i;
                if(pattern===2)return count-i;
                if(pattern===3)return Math.floor(i/32)%2===0?count-i:i;
                if(pattern===4)return [NaN,Infinity,-Infinity,-0,0,next()%13][next()%6];
                return Math.floor(i/67)%5*1000+i%67;
            });cases.push({values,mode,seed:next()});
        }
    }
}
for(let i=0;i<120;++i)cases.push({values:Array.from({length:next()%1500},()=>next()%11===0?NaN:next()%103),mode:i%8,seed:next()});
const number=v=>Object.is(v,-0)?'-0':String(v);
const input=cases.map(({values,mode,seed})=>`${values.length} ${mode} ${seed} ${values.map(number).join(' ')}`).join('\n');
const output=execFileSync(path.resolve(process.argv[2]),[],{input,encoding:'utf8',maxBuffer:256*1024*1024}).trim().split('\n');
assert.equal(output.length,cases.length);let comparisons=0;
for(let c=0;c<cases.length;++c){
    const {values:keys,mode}=cases[c];let random=cases[c].seed;
    const values=keys.map((key,id)=>({key,id})),trace=[];let threw=false;
    try{values.sort((a,b)=>{
        trace.push([a.id,b.id]);
        if(mode===7&&trace.length===17)throw new Error('comparison');
        if(mode===6&&trace.length===1){values.push({id:keys.length,key:-9});values[0].key=123;}
        if(mode===1){const d=(a.id%3-b.id%3+3)%3;return d===0?0:d===1?-1:1;}
        if(mode===2){random^=random<<13;random^=random>>>17;random^=random<<5;const code=(random>>>0)%7;return code===0?NaN:code-3;}
        if(mode===3&&(a.id%7===0||b.id%7===0))return NaN;
        if(mode===5)return NaN;
        return mode===4?b.key-a.key:a.key-b.key;
    });}catch(error){assert.equal(error.message,'comparison');threw=true;}
    const actual=JSON.parse(output[c]);
    assert.deepEqual(actual,{order:values.map(v=>v.id),comparisons:trace,threw},`case ${c}, length ${keys.length}, mode ${mode}`);
    comparisons+=trace.length;
}
console.log(`V8 array sort parity: ${cases.length} cases, ${comparisons} comparator calls and exact resulting identities; ${process.version}`);
