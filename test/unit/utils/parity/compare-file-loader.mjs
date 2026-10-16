// Differential check for the native src/loaders/FileLoader.js port.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-file-loader.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const url=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
const {FileLoader}=await import(url('src/loaders/FileLoader.js'));
const {Cache}=await import(url('src/loaders/Cache.js'));
if(typeof globalThis.ProgressEvent==='undefined')
    globalThis.ProgressEvent=class ProgressEvent{constructor(type,init={}){this.type=type;Object.assign(this,init);}};
const asset='https://host/asset.txt';
const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const place=value=>value===''||value===undefined?'-':String(value).replace(/ /g,'+');
const scenarios=[
    {status:200,statusText:'OK',body:'hello world',contentLength:'11',responseType:'',cache:0,loads:1},
    {status:200,statusText:'OK',body:'hello world',contentLength:'11',responseType:'',cache:0,loads:2},
    {status:200,statusText:'OK',body:'hello world',contentLength:'11',responseType:'',cache:1,loads:2},
    {status:200,statusText:'OK',body:'{"a":1}',contentLength:'7',responseType:'json',cache:0,loads:1},
    {status:404,statusText:'Not Found',body:'',contentLength:'',responseType:'',cache:0,loads:1},
    {status:0,statusText:'',body:'zero status',contentLength:'11',responseType:'',cache:0,loads:1}
];
const lines=scenarios.map(s=>[s.status,place(s.statusText),place(s.body),place(s.contentLength),
    place(s.responseType),s.cache,s.loads].map(token).join(' '));
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
for(let index=0;index<scenarios.length;++index){
    const scenario=scenarios[index];
    let calls=0,loads=0,received='',error='',lastUrl='',progressLoaded=-1,progressTotal=-1,progressComputable=false;
    const originalFetch=globalThis.fetch;
    globalThis.fetch=async request=>{
        calls+=1;
        lastUrl=typeof request==='string'?request:String(request.url);
        const headers=new Headers(scenario.contentLength?{'Content-Length':scenario.contentLength}:{});
        const response=new Response(scenario.body,{status:scenario.status||200,statusText:scenario.statusText,headers});
        // Response.url is read only, and the class the mock builds has it empty, so define it:
        // upstream reports response.url in its error text.
        Object.defineProperty(response,'url',{value:asset});
        // The Response constructor rejects a status outside 200..599, but a status of 0 is a real
        // case for the pinned loader (a non-http protocol), so the mock sets it directly.
        if(scenario.status===0) Object.defineProperty(response,'status',{value:0});
        return response;
    };
    Cache.clear();
    Cache.enabled=scenario.cache===1;
    const loader=new FileLoader();
    if(scenario.responseType) loader.setResponseType(scenario.responseType);
    loader.setPath('');
    for(let i=0;i<scenario.loads;++i){
        await new Promise(resolve=>{
            loader.load(asset,data=>{loads+=1;received=data==null?'':(typeof data==='object'?JSON.stringify(data):String(data));resolve();},
                event=>{progressLoaded=event.loaded;progressTotal=event.total;progressComputable=event.lengthComputable;},
                err=>{error=err&&err.message?err.message:String(err);resolve();});
            setTimeout(resolve,0);
        });
    }
    Cache.enabled=false;
    Cache.clear();
    globalThis.fetch=originalFetch;
    const expected={calls,loads,received,error,lastUrl,
        progressLoaded:Number.isNaN(progressLoaded)?'NaN':progressLoaded,
        progressTotal:progressTotal,progressComputable};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH scenario',index,'line:',lines[index]);
        console.log('  probe :',JSON.stringify(actual));
        console.log('  oracle:',JSON.stringify(expected));
    }
    assert.deepEqual(actual,expected,`scenario ${index}: ${lines[index]}`);
}
console.log(`FileLoader parity: ${scenarios.length} scenarios over the injected transport`);