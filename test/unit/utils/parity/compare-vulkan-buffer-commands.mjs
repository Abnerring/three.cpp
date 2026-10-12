import assert from 'node:assert/strict';
import {spawnSync,execFileSync} from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
// Independent test oracle for the supported view overload. It uses BigInt for
// WebIDL's mathematical modulo, never the native adapter's floating algorithm.
// It is a specification-derived byte oracle, not a browser WebGL execution.
// https://webidl.spec.whatwg.org/#abstract-opdef-converttoint
// https://registry.khronos.org/webgl/specs/latest/2.0/#3.7.3
const numbers=[0,-0,.25,-.25,1,4,5,-1,NaN,Infinity,-Infinity,2**32,2**32+1,2**64,-(2**64),'2',null,undefined];
const integer=(v,bits,signed=false)=>{
    const n=Number(v),i=Number.isFinite(n)?BigInt(Math.trunc(n)):0n;
    return signed?BigInt.asIntN(bits,i):BigInt.asUintN(bits,i);
};
function expected(kind,mode,a,b){
    const width=[1,1,2,2,4,4,4,8,1][kind],bytes=Array(32).fill(160);
    const source=Array.from({length:4*width},(_,i)=>(i+2*width)*3+1);
    let dst=0,offset=0,length=1;
    if(mode===0)dst=numbers[a];if(mode===1)offset=numbers[a];if(mode===2){offset=1;length=numbers[a];}
    if(mode===3){dst=numbers[a];offset=numbers[b];length=0;}
    const d=integer(dst,64,true),s=integer(offset,64),l=integer(length,32),count=l===0n?4n-s:l,n=count*BigInt(width);
    const error=d<0n||s>4n||count<0n||s+count>4n||d+n>32n?1281:0;
    if(!error&&n>0n)bytes.splice(Number(d),Number(n),...source.slice(Number(s)*width,Number(s+count)*width));
    return {kind,mode,a,b,error,revision:!error&&n>0n?2:1,bytes};
}
if(process.argv.length!==4)throw Error('Usage: node compare-vulkan-buffer-commands.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]),lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {WebGLAttributes}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLAttributes.js')));
const run=spawnSync(process.argv[3],[],{encoding:'utf8',maxBuffer:32*1024*1024});if(run.error)throw run.error;assert.equal(run.status,0,run.stderr);
const actual=run.stdout.trim().split(/\r?\n/).map(JSON.parse);let index=0,comparisons=0;
function check(expected){assert.deepEqual(actual[index],expected,`case ${index}`);comparisons+=Object.keys(expected).length+expected.bytes.length;++index;}
for(let kind=0;kind<9;++kind){
    for(let mode=0;mode<3;++mode)for(let a=0;a<18;++a)check(expected(kind,mode,a,0));
    for(let a=0;a<18;++a)for(let b=0;b<18;++b)check(expected(kind,3,a,b));
}
function integration(mode){
    let bound,flag=0;const gl={UNSIGNED_BYTE:5121,
        createBuffer(){return {bytes:[],revision:0};},bindBuffer(target,value){bound=value;},deleteBuffer(){},
        bufferData(target,source){bound.bytes=Array.from(new Uint8Array(source.buffer,source.byteOffset,source.byteLength));++bound.revision;},
        bufferSubData(target,dst,source,offset=0,length=0){
            const d=integer(dst,64,true),s=integer(offset,64),l=integer(length,32),count=l===0n?BigInt(source.length)-s:l,n=count*BigInt(source.BYTES_PER_ELEMENT);
            if(d<0n||s>BigInt(source.length)||count<0n||s+count>BigInt(source.length)||d+n>BigInt(bound.bytes.length)){flag=1281;return;}
            if(n===0n)return;const bytes=new Uint8Array(source.buffer,source.byteOffset+Number(s)*source.BYTES_PER_ELEMENT,Number(n));bound.bytes.splice(Number(d),Number(n),...bytes);++bound.revision;
        },getError(){const e=flag;flag=0;return e;}};
    const values=new Uint8Array(12),a={array:values,usage:35044,version:0,uploads:0,updateRanges:[],
        onUploadCallback(){++this.uploads;},clearUpdateRanges(){this.updateRanges.length=0;}};
    const manager=new WebGLAttributes(gl);manager.update(a,34962);const buffer=manager.get(a).buffer;
    values.set(Array.from({length:12},(_,i)=>i+1));a.updateRanges=[{start:0,count:1},{start:4,count:-1},{start:8,count:1}];a.version=1;manager.update(a,34962);
    const firstError=gl.getError();let exception='none';
    if(mode){let reads=0;const start={[Symbol.toPrimitive](){throw RangeError('source offset');}},second={count:1,get start(){return ++reads>=3?start:4;}};
        const ranges=[{start:0,count:1},second];ranges.sort=function(){return this;};values[0]=11;values[4]=22;a.updateRanges=ranges;a.version=2;
        try{manager.update(a,34962);}catch(e){exception=e.name;}}
    return {integration:mode,firstError,exception,error:gl.getError(),revision:buffer.revision,version:manager.get(a).version,uploads:a.uploads,ranges:a.updateRanges.length,bytes:buffer.bytes};
}
for(let mode=0;mode<2;mode++)check(integration(mode));
assert.equal(actual.length,index);console.log(`VulkanBufferCommands: 3402 specification byte cases + 2 pinned-upstream control-flow cases / ${comparisons} field and byte comparisons passed (no browser WebGL context)`);
