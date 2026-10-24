import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { mkdirSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';
const [ upstream, executable, output ] = process.argv.slice(2);
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const load=relative=>import(pathToFileURL(path.join(upstream,'src',relative+'.js')));
const {default:NodeBuilder}=await load('nodes/core/NodeBuilder');
const {default:ConstNode}=await load('nodes/core/ConstNode');
const constants=await load('nodes/core/constants');
const fixtures=[];
for(const async of [false,true])for(const material of [false,true])for(const real of [false,true])fixtures.push({async,material,real,yield:'plain'});
for(const material of [false,true])for(const yieldMode of ['plain','resolved','poison','thenable','deferred','default','null'])fixtures.push({async:true,material,yield:yieldMode});
for(const async of [false,true])for(const iterators of [false,true]) {
    for(const nodeFail of [1,2])fixtures.push({async,iterators,nodeFail,yield:'plain'});
    for(const failure of ['preFail','codeFail'])fixtures.push({async,iterators,[failure]:true,yield:'plain'});
}
for(const yieldMode of ['throw','reject'])for(const failAfter of [1,2,3,4,8,9])for(const iterators of [false,true])fixtures.push({async:true,yield:yieldMode,failAfter,iterators});
for(const async of [false,true])for(const stepFail of ['build','shader'])fixtures.push({async,iterators:true,stepFail,yield:'plain'});
for(const yieldMode of ['throw','reject'])fixtures.push({async:true,iterators:true,closeFail:true,yield:yieldMode,failAfter:2});
for(const async of [false,true])for(const iterators of [false,true])for(const positionMode of ['replace','method-throw','argument-throw'])fixtures.push({async,iterators,positionMode,yield:'plain'});
for(const iterators of [false,true])for(const speciesMode of ['undefined','null','intrinsic','invalid','throw','custom','constructor-throw','constructor-undefined','constructor-null','constructor-primitive'])fixtures.push({async:true,iterators,yield:'species',speciesMode});
const expected=[];
const field=value=>value===undefined?'#undefined':value;
const checkpoint=()=>new Promise(resolve=>setImmediate(resolve));
for(const config of fixtures) {
    const events=[],host=[];let yields=0,nodeCalls=0,status='pending',reason,same,sync;
    const context=label=>({isContextNode:true,getFlowContextData(){events.push(label);return {from:label};}});
    const renderer={contextNode:context('renderer-context')};
    const b=new NodeBuilder(null,renderer,null);
    const node=config.real?new ConstNode(7):{build(builder){events.push(`node:${builder.buildStage}:${builder.shaderStage}`);if(++nodeCalls===config.nodeFail)throw 'node-stop';}};
    if(!config.real)b.flowNode=function(){events.push(`flow:${this.buildStage}:${this.shaderStage}`);return {};};
    b.compute=node;
    if(config.material) {
        b.material={contextNode:context('material-context')};
        renderer.library={fromMaterial(){events.push('from-material');return {build(builder){events.push('material-build');builder.addFlow('fragment',node);}};}};
    }
    if(config.preFail)b.prebuild=()=>{throw 'pre-stop';};
    if(config.positionMode) {
        const prebuild=b.prebuild;let position,positionReads=0;
        b.prebuild=function(){
            prebuild.call(this);position={isNode:true,tag:'first'};positionReads=0;
            Object.defineProperty(this.context,'position',{configurable:true,get(){
                events.push(`position-get:${++positionReads}`);
                if(positionReads===3&&config.positionMode!=='replace')throw 'position-stop';
                return position;
            }});
        };
        Object.defineProperty(b,'flowNodeFromShaderStage',{configurable:true,get(){
            events.push('flow-method-get');
            if(config.positionMode==='method-throw')throw 'method-stop';
            position={isNode:true,tag:'second'};
            return function(stage,value){events.push(`position-flow:${value.tag}`);};
        }});
    }
    b.buildCode=function(){events.push('code');if(config.codeFail)throw 'code-stop';if(config.real)this.computeShader=this.getFlowData(node).result;};
    const update=b.buildUpdateNodes;
    b.buildUpdateNodes=function(){events.push('updates');return update.call(this);};
    const iterator=(values,label)=>function(){
        events.push(`${label}:iterator`);let i=0;
        return {next(){events.push(`${label}:next:${i}`);if(config.stepFail===label&&i===1){++i;throw 'next-stop';}return i<values.length?{value:values[i++],done:false}:{value:undefined,done:true};},return(){events.push(`${label}:close`);if(config.closeFail)throw 'close-stop';return {};}};
    };
    const stages=constants.defaultBuildStages,shaders=constants.shaderStages;
    if(config.iterators){stages[Symbol.iterator]=iterator(stages,'build');shaders[Symbol.iterator]=iterator(shaders,'shader');}
    const yieldFn=()=>{
        events.push(`yield:${++yields}`);
        if(yields===config.failAfter){if(config.yield==='reject')return Promise.reject('yield-stop');if(config.yield==='throw')throw 'yield-stop';}
        if(config.yield==='deferred')return new Promise(resolve=>host.push(resolve));
        if(config.yield==='species') {
            const promise=Promise.resolve(yields),constructor={get [Symbol.species](){
                events.push('species-get');
                if(config.speciesMode==='null')return null;
                if(config.speciesMode==='intrinsic')return Promise;
                if(config.speciesMode==='invalid')return 17;
                if(config.speciesMode==='throw')throw 'species-stop';
                if(config.speciesMode==='custom')return function Species(executor){events.push('species-construct');return new Promise(executor);};
            }};
            Object.defineProperty(promise,'constructor',{configurable:true,get(){
                events.push('promise-constructor-get');
                if(config.speciesMode==='constructor-throw')throw 'constructor-stop';
                if(config.speciesMode==='constructor-undefined')return undefined;
                if(config.speciesMode==='constructor-null')return null;
                if(config.speciesMode==='constructor-primitive')return 17;
                return constructor;
            }});return promise;
        }
        if(config.yield==='resolved'||config.yield==='poison'){
            const promise=Promise.resolve(yields);
            if(config.yield==='poison')Object.defineProperty(promise,'then',{get(){events.push('then-get');throw 'then-stop';},configurable:true});
            return promise;
        }
        if(config.yield==='thenable')return {get then(){events.push('get-then');return (resolve,reject)=>{events.push('call-then');resolve(1);reject('ignored');};}};
    };
    const previousRAF=Object.getOwnPropertyDescriptor(globalThis,'requestAnimationFrame');
    globalThis.requestAnimationFrame=resolve=>host.push(resolve);
    try {
        if(config.async) {
            const promise=config.yield==='default'?b.buildAsync():b.buildAsync(config.yield==='null'?null:yieldFn);
            events.push('returned');sync=[...events];
            promise.then(value=>{status='fulfilled';same=value===b;events.push('fulfilled');},value=>{status='rejected';reason=value&&typeof value==='object'?value.name:value;events.push('rejected');});
            queueMicrotask(()=>events.push('micro'));await checkpoint();
            let tasks=0;
            while(host.length&&tasks++<30){events.push('host');host.shift()();await checkpoint();}
        }else{same=b.build()===b;status='fulfilled';events.push('returned');sync=[...events];}
    }catch(value){status='thrown';reason=value&&typeof value==='object'?value.name:value;sync=[...events];}
    finally {
        if(config.iterators){delete stages[Symbol.iterator];delete shaders[Symbol.iterator];}
        if(previousRAF)Object.defineProperty(globalThis,'requestAnimationFrame',previousRAF);else delete globalThis.requestAnimationFrame;
    }
    expected.push({status,reason:field(reason),same:field(same),events,yields,sync,stage:field(b.buildStage),shader:field(b.shaderStage),context:field(b.context.from),code:field(b.computeShader)});
}
if(output){mkdirSync(output,{recursive:true});writeFileSync(path.join(output,'fixtures.jsonl'),fixtures.map(x=>JSON.stringify(x)).join('\n')+'\n');writeFileSync(path.join(output,'expected.jsonl'),expected.map(x=>JSON.stringify(x)).join('\n')+'\n');}
const text=execFileSync(executable,[],{input:fixtures.map(x=>JSON.stringify(x)).join('\n')+'\n',encoding:'utf8',maxBuffer:16*1024*1024});
const actual=text.trim().split(/\r?\n/).map(line=>JSON.parse(line));
if(output)writeFileSync(path.join(output,'actual.jsonl'),text);
assert.equal(actual.length,expected.length);
for(let i=0;i<expected.length;++i)assert.deepEqual(actual[i],expected[i],`row ${i}: ${JSON.stringify(fixtures[i])}`);
console.log(`NodeBuilder build/prebuild/buildAsync: ${expected.length} actual upstream scenarios match.`);
