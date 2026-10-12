import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {PropertyMixer}=await import(pathToFileURL(path.join(upstream,'src/animation/PropertyMixer.js')));
const encode=v=>v===undefined?'$undefined':typeof v==='number'?(Number.isNaN(v)?'$NaN':v===Infinity?'$Infinity':v===-Infinity?'$-Infinity':Object.is(v,-0)?'$-0':v):
    Array.isArray(v)?Array.from(v,encode):v!==null&&typeof v==='object'?Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])])):v;
const wire=(_,v)=>v===undefined?{$undefined:true}:typeof v==='number'&&(!Number.isFinite(v)||Object.is(v,-0))?{$number:Object.is(v,-0)?'-0':String(v)}:v;
function dump(a){return {values:Array.from(a,encode),holes:Array.isArray(a)?Array.from({length:a.length},(_,i)=>i).filter(i=>!Object.hasOwn(a,i)):[],extras:Array.isArray(a)?Object.fromEntries(Object.keys(a).filter(k=>!(+k>=0&&+k<a.length&&Math.floor(+k)===+k&&String(+k)===k)).map(k=>[k,encode(a[k])])):{}};}
function binding(value){return {value,events:[],mixer:null,event(name,a,offset){this.events.push([name,encode(offset),encode(this.mixer.cumulativeWeight),encode(this.mixer.cumulativeWeightAdditive),a===this.mixer.buffer]);},
    getValue(a,offset){this.event('get',a,offset);this.value.forEach((v,i)=>a[offset+i]=v);this.getHook?.(a,offset);},
    setValue(a,offset){this.event('set',a,offset);for(let i=0,n=this.value.length;i<n;i++)this.value[i]=a[offset+i];this.setHook?.(a,offset);}};}
function state(m,b){return {buffer:dump(m.buffer),kind:Array.isArray(m.buffer)?'array':m.buffer instanceof Float32Array?'float32':'float64',size:encode(m.valueSize),weight:encode(m.cumulativeWeight),additive:encode(m.cumulativeWeightAdditive),work:encode(m._workIndex),original:m._origIndex,addIndex:m._addIndex,useCount:m.useCount,referenceCount:m.referenceCount,value:encode(b.value),events:structuredClone(b.events)};}
function normal(r){
    const b=binding(r.original??[]),m=new PropertyMixer(b,r.type,r.size);b.mixer=m;
    if(r.bufferKind)m.buffer=r.bufferKind==='array'?r.initialBuffer:new (r.bufferKind==='float32'?Float32Array:Float64Array)(r.initialBuffer);
    if(r.nullBinding)m.binding=null;
    const states=[state(m,b)];
    for(const a of r.actions??[]){
        a.incoming?.forEach((v,i)=>m.buffer[i]=v);
        if(a.op==='save')m.saveOriginalState();else if(a.op==='restore')m.restoreOriginalState();
        else if(a.op==='accumulate')m.accumulate(a.index,a.weight);else if(a.op==='add')m.accumulateAdditive(a.weight);else if(a.op==='apply')m.apply(a.index);
        else if(a.op==='set'){for(const [key,field] of Object.entries({weight:'cumulativeWeight',additive:'cumulativeWeightAdditive',size:'valueSize',origIndex:'_origIndex',addIndex:'_addIndex',workIndex:'_workIndex'}))if(Object.hasOwn(a,key))m[field]=a[key];}
        else if(a.op==='numericIdentity')m._setAdditiveIdentityNumeric();else if(a.op==='quaternionIdentity')m._setAdditiveIdentityQuaternion();else if(a.op==='otherIdentity')m._setAdditiveIdentityOther();
        else m[`_${a.op}`](m.buffer,a.destination,a.source,a.weight,a.stride??m.valueSize);
        states.push(state(m,b));
    }
    return states;
}
const weighted=(trace,label,value)=>({valueOf(){trace.push(label);return value;}});
function callbacks(kind){
    const b=binding([10]),m=new PropertyMixer(b,'number',1);b.mixer=m;const trace=[],old=m.buffer;
    if(kind==='save'){
        const replacement=binding([90]);replacement.mixer=m;
        b.getHook=()=>{m.buffer=new Float64Array([1,2,3,4,5]);m.binding=replacement;m.cumulativeWeight=9;m.valueSize=1;};
        m.saveOriginalState();m.restoreOriginalState();return {state:state(m,b),old:dump(old),replacement:encode(replacement.value),replacementEvents:replacement.events};
    }
    m.saveOriginalState();b.events=[];
    if(kind==='mutable'){
        const other=new PropertyMixer(null,'number',1),sharedBuiltin=m._mixBufferRegion===other._mixBufferRegion;
        let count=0;m._mixBufferRegion=function(){trace.push(++count);if(count===2)this._mixBufferRegion=function(){trace.push(99);};trace.push(-count);};
        const copied=m._mixBufferRegion;m.cumulativeWeight=1;
        for(let i=0;i<3;i++)m.accumulate(0,1);copied.call(m,m.buffer,1,0,.5,1);
        let identities=0;m._setIdentity=function(){trace.push(10+ ++identities);if(identities===2)this._setIdentity=function(){trace.push(199);};trace.push(-10-identities);};
        for(let i=0;i<3;i++){m.cumulativeWeightAdditive=0;m.accumulateAdditive(1);}
        return {state:state(m,b),trace,sharedBuiltin,sameFunction:copied===m._mixBufferRegion};
    }
    if(kind==='applyBinding'){
        const replacement=binding([90]);replacement.mixer=m;m.buffer[0]=2;m.accumulate(0,.25);const original=m._mixBufferRegion;
        m._mixBufferRegion=function(...args){trace.push('mix');m.binding=replacement;m.buffer=new Float64Array([1,2,3,4,5]);original.apply(this,args);};
        b.setHook=()=>{trace.push(m.cumulativeWeight);m.buffer[0]=8;m.accumulate(1,.75);};
        m.apply(0);return {state:state(m,b),old:dump(old),trace,replacementEvents:replacement.events};
    }
    if(kind==='identity'){
        const original=m._setIdentity;m._setIdentity=function(){trace.push('identity');m.buffer=new Float64Array([3,4,5,6,7]);m.valueSize=2;m.cumulativeWeightAdditive=5;original.call(this);};
        m.buffer[0]=2;m.accumulateAdditive(.5);return {state:state(m,b),old:dump(old),trace};
    }
    if(kind==='weights'){
        m.buffer[0]=2;m.accumulate(0,weighted(trace,'first',.25));m.buffer[0]=6;m.accumulate(0,weighted(trace,'second',.5));m.apply(0);
        m.buffer[0]=3;m.accumulateAdditive(weighted(trace,'add',.25));m.apply(1);return {state:state(m,b),trace};
    }
    if(kind==='index'){
        const index={valueOf(){trace.push('index');m.buffer=new Float64Array([3,4,5,6,7]);m.valueSize=2;return 0;}};
        m.buffer[0]=2;m.accumulate(index,.5);return {state:state(m,b),old:dump(old),trace};
    }
    if(kind==='applyWeight'){
        const weight={valueOf(){trace.push(m.cumulativeWeight);m.cumulativeWeight=9;return .5;}};
        m.buffer[0]=2;m.accumulate(0,weight);m.apply(0);return {state:state(m,b),trace};
    }
    throw new Error('Unknown callback');
}
const cases=[];
for(const callback of ['save','applyBinding','identity','weights','index','applyWeight','mutable'])cases.push({callback});
const sample=(size,offset=0)=>Array.from({length:size},(_,i)=>i+offset);
for(const type of ['number','vector','color','string','bool','quaternion','unknown',undefined,null])for(const size of [0,1,2,3,4,8]){
    const original=type==='string'?sample(size).map(i=>'s'+i):type==='bool'?sample(size).map(i=>i%2===0):type==='quaternion'?sample(size).map(i=>i%4===3?1:0):sample(size,2);
    const incoming=type==='string'?sample(size).map(i=>'next'+i):type==='bool'?sample(size).map(i=>i%2!==0):type==='quaternion'?sample(size).map(i=>i%4===1?1:0):sample(size,10);
    cases.push({type,size,original,actions:[{op:'save'},{op:'accumulate',index:0,weight:.25,incoming},{op:'apply',index:0},{op:'accumulate',index:1,weight:.25,incoming},{op:'apply',index:1},{op:'add',weight:.5,incoming},{op:'apply',index:0},{op:'restore'}]});
}
for(const type of ['number','string','bool','quaternion'])for(const value of [0,-0,.25,.5,1,2,-1,NaN,Infinity,-Infinity,'0','.5',null,undefined]){
    const size=type==='quaternion'?4:2,original=type==='quaternion'?[0,0,0,1]:type==='string'?['a','b']:type==='bool'?[true,false]:[2,4];
    const incoming=type==='quaternion'?[0,1,0,0]:type==='string'?['x','y']:type==='bool'?[false,true]:[6,8];
    cases.push({type,size,original,actions:[{op:'save'},{op:'accumulate',index:0,weight:value,incoming},{op:'accumulate',index:0,weight:value,incoming:original},{op:'apply',index:0},{op:'add',weight:value,incoming},{op:'apply',index:1},{op:'restore'}]});
}
for(const bufferKind of ['array','float32','float64'])for(const index of [-3,-1,0,1,2,.5,NaN,Infinity,'1',null])cases.push({type:'number',size:2,original:[1,2],bufferKind,initialBuffer:sample(10),actions:[{op:'save'},{op:'accumulate',index,weight:.25,incoming:[4,8]},{op:'apply',index}]});
for(const type of ['number','string','quaternion'])for(const size of [-1,-.1,-0,.2,.25,NaN,Infinity,-Infinity])cases.push({type,size});
for(const size of [.5,1.5])cases.push({type:'number',size,original:[1],actions:[{op:'set',weight:1},{op:'apply',index:0}]});
cases.push({type:'number',size:.5,original:[1],bufferKind:'array',initialBuffer:[undefined,undefined,4],actions:[{op:'set',weight:1},{op:'apply',index:0}]});
cases.push({type:'number',size:2,original:[1],actions:[{op:'set',weight:1,size:-1},{op:'apply',index:0}]});
for(const bufferKind of ['array','float32','float64'])for(const op of ['lerp','lerpAdditive','select','slerp','slerpAdditive'])for(const weight of [0,.25,.5,1,-1,NaN]){
    cases.push({type:'quaternion',size:4,bufferKind,initialBuffer:[1,2,3,4,5,6,7,8,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0],actions:[{op,destination:1,source:0,weight,stride:4}]});
}
for(const q of [[0,0,0,0],[0,0,0,1],[0,0,0,-1],[1e-8,0,0,1],[NaN,0,0,1],[Infinity,0,0,1],[.1,.2,.3,.4]])cases.push({type:'quaternion',size:4,original:[0,0,0,1],actions:[{op:'save'},{op:'accumulate',index:0,weight:.2,incoming:q},{op:'accumulate',index:0,weight:.7,incoming:[0,0,0,1]},{op:'apply',index:0},{op:'add',weight:.5,incoming:q},{op:'apply',index:1}]});
for(const type of ['string','bool'])cases.push({type,size:2,original:[{id:1},null],actions:[{op:'save'},{op:'accumulate',index:0,weight:.5,incoming:[{id:1},undefined]},{op:'apply',index:0},{op:'add',weight:.75,incoming:[{id:2},true]},{op:'apply',index:1}]});
cases.push({type:'number',size:0,nullBinding:true,actions:[{op:'accumulate',index:0,weight:1},{op:'apply',index:0}]});
cases.push({type:'number',size:1,nullBinding:true,actions:[{op:'save'}]});
cases.push({type:'number',size:0,nullBinding:true,actions:[{op:'restore'}]});
let seed=1717;const rand=()=>((seed=Math.imul(seed,1664525)+1013904223>>>0)/4294967296);
for(let c=0;c<60;c++){
    const size=c%3+1,actions=[{op:'save'}];
    for(let frame=0;frame<5;frame++){for(let j=0;j<3;j++)actions.push({op:j===2?'add':'accumulate',index:frame%2,weight:rand(),incoming:Array.from({length:size},()=>rand()*20-10)});actions.push({op:'apply',index:frame%2});}
    actions.push({op:'restore'});cases.push({type:'number',size,original:sample(size,1),actions});
}
const mathValues=[0,-0,Number.MIN_VALUE,-Number.MIN_VALUE,Number.MAX_VALUE,-Number.MAX_VALUE,NaN,Infinity,-Infinity,1,-1,1-Number.EPSILON,-1+Number.EPSILON,.5,-.5,Math.PI/4,Math.PI/2,Math.PI,2**19*Math.PI/2,1e300];
for(let i=0;i<6000;i++){
    const data=Buffer.alloc(8);data.writeUInt32LE(Math.floor(rand()*4294967296),0);data.writeUInt32LE(Math.floor(rand()*4294967296),4);mathValues.push(data.readDoubleLE(0));
    mathValues.push(rand()*2-1);
}
const bits=v=>{if(Number.isNaN(v))return '$NaN';const b=Buffer.alloc(8);b.writeDoubleLE(v);return b.readBigUInt64LE().toString(16).padStart(16,'0');};
cases.push({math:mathValues});
const expected=cases.map(r=>{try{return r.math?r.math.map(v=>[bits(Math.sin(v)),bits(Math.acos(v))]):r.callback?callbacks(r.callback):normal(structuredClone(r));}catch(e){return {error:e.name};}});
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases,wire),encoding:'utf8',maxBuffer:64*1024*1024,stdio:['pipe','pipe','pipe']}));
let comparisons=0;
function compare(a,b,p){if(typeof a==='number'&&typeof b==='number'){assert.ok(Math.abs(a-b)<=3e-14*Math.max(1,Math.abs(a),Math.abs(b)),`${p}: ${a} != ${b}`);comparisons++;return;}
    if(a!==null&&b!==null&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),`${p} keys`);for(const key of Object.keys(a))compare(a[key],b[key],`${p}.${key}`);return;}
    assert.deepEqual(a,b,p);comparisons++;}
assert.equal(actual.length,cases.length);
for(let i=0;i<cases.length;i++){try{compare(actual[i],expected[i],`case[${i}]`);}catch(e){console.log(JSON.stringify(cases[i],wire));throw e;}}
console.log(`PropertyMixer parity: ${cases.length} scenarios / ${comparisons} comparisons; upstream ${lock.commit}`);
