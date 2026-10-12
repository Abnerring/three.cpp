import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {PropertyBinding:P}=await import(pathToFileURL(path.join(upstream,'src/animation/PropertyBinding.js')));
const encode=v=>v===undefined?'$undefined':typeof v==='function'?'$function':typeof v==='number'?(Number.isNaN(v)?'$NaN':v===Infinity?'$Infinity':v===-Infinity?'$-Infinity':Object.is(v,-0)?'$-0':v):Array.isArray(v)?Array.from(v,encode):v!==null&&typeof v==='object'?Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])])):v;
const wire=(_,v)=>v===undefined?{$undefined:true}:typeof v==='number'&&(!Number.isFinite(v)||Object.is(v,-0))?{$number:Object.is(v,-0)?'-0':String(v)}:v;
const state=(b,root,buffer)=>({root:encode(root),parsed:encode(b.parsedPath),node:encode(b.node),target:encode(b.targetObject),resolved:encode(b.resolvedProperty),propertyName:encode(b.propertyName),propertyIndex:encode(b.propertyIndex),buffer:encode(buffer),bufferProperties:Object.fromEntries(Object.keys(buffer).map(k=>[k,encode(buffer[k])]))});
function normal(r){
    const root=r.root;if(r.typedProperty)root.value=new Float32Array(root.value);
    const b=new P(root,r.path,r.parsed);let buffer=Object.hasOwn(r,'buffer')?r.buffer:[99,98,97,96,95];if(r.typedBuffer)buffer=new Float64Array(buffer);
    const results=[state(b,root,buffer)];
    for(const a of r.actions){let error='';try{
        const offset=Object.hasOwn(a,'offset')?a.offset:0;
        if(a.op==='get')b.getValue(buffer,offset);else if(a.op==='set')b.setValue(buffer,offset);else if(a.op==='bind')b.bind();else if(a.op==='unbind')b.unbind();
        else if(a.op==='buffer')buffer=a.value;else if(a.op==='replace'){let object=root;for(const key of a.keys.slice(0,-1))object=object[key];object[a.keys.at(-1)]=a.value;}
    }catch(e){error=e.name;}results.push({error,state:state(b,root,buffer)});}
    return results;
}
function callback(name){
    const trace=[],root={name:'root',isMaterial:true,value:1};let b;
    const accessor=(o,k,get,set)=>Object.defineProperty(o,k,{get,set,enumerable:true,configurable:true});
    if(name==='sharedTables'){
        const a=new P(root,'.value');b=new P(root,'.value');const previousGet=a.GetterByBindingType[0],previousSet=a.SetterByBindingTypeAndVersioning[0][1];
        const same=a.GetterByBindingType===b.GetterByBindingType,sameRow=a.SetterByBindingTypeAndVersioning[0]===b.SetterByBindingTypeAndVersioning[0];
        a.GetterByBindingType[0]=function(buffer,offset){trace.push('getShared');buffer[offset]=77;};a.SetterByBindingTypeAndVersioning[0][1]=function(){trace.push('setShared');};
        const buffer=[0];b.getValue(buffer,0);b.setValue(buffer,0);a.GetterByBindingType=a.GetterByBindingType.slice();a.GetterByBindingType[0]=previousGet;a.getValue(buffer,0);
        b.GetterByBindingType[0]=previousGet;b.SetterByBindingTypeAndVersioning[0][1]=previousSet;
        return {trace,same,sameRow,separate:a.GetterByBindingType!==b.GetterByBindingType,buffer};
    }
    if(name==='versionTarget'){
        const other={};accessor(other,'needsUpdate',undefined,v=>trace.push(['newVersion',v]));
        accessor(root,'value',()=>{trace.push('get');return 1;},v=>{trace.push(['set',v]);b.targetObject=other;});
        accessor(root,'needsUpdate',undefined,()=>trace.push('oldVersion'));b=new P(root,'.value');b.setValue([7],0);return trace;
    }
    if(name==='bindReentry'){accessor(root,'value',()=>{trace.push('get');b.setValue([8],0);trace.push('returned');return 1;});b=new P(root,'.value');b.bind();return trace;}
    if(name==='arrayLength'){
        const values=[1,2,3],buffer=[];root.value=values;accessor(buffer,'0',undefined,v=>{trace.push(v);values.push(4);});b=new P(root,'.value');b.getValue(buffer,0);return {trace:encode(trace),buffer:encode(buffer),values:encode(values)};
    }
    if(name==='fromToArray'){
        const buffer=new Float64Array([3,4,5]),vector={x:1,y:2};
        vector.fromArray=function(a,o){trace.push(['from',this===vector,a===buffer,o]);this.x=a[o];this.y=a[o+1];return this;};
        vector.toArray=function(a,o){trace.push(['to',this===vector,a===buffer,o]);a[o]=this.x;a[o+1]=this.y;return a;};
        root.value=vector;b=new P(root,'.value');b.setValue(buffer,1);b.getValue(buffer,0);return {trace:encode(trace),vector:encode(vector),buffer:encode(buffer),needsUpdate:encode(root.needsUpdate)};
    }
    if(name==='findGetters'){
        const child={name:'child',uuid:'id',children:[]},children=[child];accessor(root,'children',()=>{trace.push('children');return children;});accessor(root,'skeleton',()=>{trace.push('skeleton');return undefined;});return {trace,same:P.findNode(root,'id')===child};
    }
    if(name==='lookupOrder'){
        const material={isMaterial:true,opacity:1};accessor(root,'material',()=>{trace.push('material');return material;});
        accessor(material,'opacity',()=>{trace.push('opacity');return 1;},v=>trace.push(['write',v]));b=new P(root,'.material.opacity');b.getValue([0],0);b.setValue([7],0);return trace;
    }
    if(name==='offsetCoercion'){
        const other={value:8},buffer=[],offset={toString(){trace.push('key');b.targetObject=other;return '1';}};
        accessor(root,'value',()=>{trace.push('rootRead');return 1;});b=new P(root,'.value');b.bind();b.getValue(buffer,offset);return {trace,buffer:encode(buffer)};
    }
    if(['boneUndefined','boneNull','boneFound'].includes(name)){
        const child={name:'child',uuid:'id',children:[]},skeleton={getBoneByName(n){trace.push(['bone',this===skeleton,n]);return name==='boneUndefined'?undefined:name==='boneNull'?null:child;}};
        root.skeleton=skeleton;root.children=[child];const found=P.findNode(root,'id');return {trace,found:encode(found),same:found===child};
    }
    throw new Error('Unknown callback fixture');
}
function composite(r){
    const trace=[],bindings=[],group={isAnimationObjectGroup:true,nCachedObjects_:r.cached};
    for(let i=0;i<3;i++){
        const b={id:i};for(const method of ['bind','unbind','getValue','setValue'])b[method]=function(...a){trace.push([method,this.id]);if(method==='bind'&&this.id===1&&r.mutate)group.nCachedObjects_=2;if(method==='getValue')a[0][a[1]]=this.id+10;if(method==='setValue')trace.push(a[0][a[1]]);};bindings.push(b);
    }
    group.subscribe_=function(p,parsed){trace.push(['subscribe',this===group,p,parsed]);return bindings;};
    const b=P.create(group,'.value'),buffer=new Float64Array([7]);b.getValue(buffer,0);b.setValue(buffer,0);b.unbind();return {trace:encode(trace),buffer:encode(buffer),cached:encode(group.nCachedObjects_)};
}
function run(r){if(r.kind==='parse')return encode(P.parseTrackName(r.value));if(r.kind==='sanitize')return P.sanitizeNodeName(r.value);if(r.kind==='find')return encode(P.findNode(r.root,r.name));if(r.kind==='callback')return callback(r.name);if(r.kind==='composite')return composite(r);return normal(r);}
const cases=[];
const parseNames=['.property','node.property','foo.bar.baz','foo.material.opacity','foo.materials[2].opacity','foo.material[2].opacity','parent:dir/node.position[x]','.quaternion[w]','.bones[Armature.DEF_cog].position','a.custom[key].value','a.prop[a][b]','a.prop[]','.value[a\nb]','.value[a\rb]','.value[a\u2028b]','.value[a\u2029b]','a\n.value','a.value\n','a..value','.map.offset','',null,undefined,123,true,{},['a.value'],'😇.value[急須]','a.value[ ]','a.value[[]','a.value[]]','a.value[.]','a.value[../foo]'];
for(const value of parseNames)cases.push({kind:'parse',value});
const spaces=[' ','\t','\n','\r','\v','\f','\u00a0','\u1680',...Array.from({length:11},(_,i)=>String.fromCharCode(0x2000+i)),'\u2028','\u2029','\u202f','\u205f','\u3000','\ufeff','\u0085','\u180e','\u200b'];
for(const s of spaces)cases.push({kind:'sanitize',value:'a'+s+'b.[x]:/😇'});
for(const value of [null,undefined,1,true,[],{},'急須','"Mátyás" %_* 😇'])cases.push({kind:'sanitize',value});
const leaf=(name,value)=>({name,uuid:'id-'+name,value,children:[]});
const base=()=>({name:'root',uuid:'id-root',isObject3D:true,value:1,children:[{...leaf('a',2),children:[leaf('same',3)]},leaf('same',4)],material:{isMaterial:true,opacity:1,color:[1,2,3]},geometry:{morphAttributes:{}},morphTargetInfluences:[.2,.3],morphTargetDictionary:{smile:1}});
for(const name of [undefined,'','.',-1,'-1','root','id-root','a','id-a','same','id-same','missing',null,0])cases.push({kind:'find',root:base(),name});
for(const name of ['none',undefined,'x'])for(const root of [null,{}, {name:'n',children:[{name:'x'}]}])cases.push({kind:'find',root,name});
for(const value of [0,-0,1,true,false,'hello',null,undefined,[],[1,2,3],{x:1,y:2},{fromArray:0,toArray:0},{fromArray:0}])for(const flags of [{},{isMaterial:true},{isObject3D:true},{isMaterial:1,isObject3D:true}]){
    cases.push({root:{value,...flags},path:'.value',actions:[{op:'get',offset:1},{op:'set'},{op:'unbind'},{op:'get'}]});
}
for(const path of ['.value','root.value','id-root.value','a.value','same.value','missing.value','.material.opacity','.material.color[1]','.morphTargetInfluences[smile]','.morphTargetInfluences[0]','.morphTargetInfluences[unknown]','.map.offset','.materials[0].opacity','.bones[0].value','.custom[key].value'])cases.push({root:base(),path,actions:[{op:'get'},{op:'set'},{op:'bind'},{op:'get'}]});
for(const offset of [-1,0,1,.5,'0','01','x',null,undefined,NaN,Infinity])for(const typedBuffer of [false,true])for(const value of [2,[1,2]])cases.push({root:{value},path:'.value',typedBuffer,actions:[{op:'get',offset},{op:'set',offset}]});
cases.push({root:{value:[1,2]},path:'.value',typedProperty:true,actions:[{op:'get'},{op:'set'}]});
cases.push({root:{},path:'.value',actions:[{op:'get'},{op:'replace',keys:['value'],value:7},{op:'get'},{op:'unbind'},{op:'get'}]});
cases.push({root:base(),path:'.material.opacity',actions:[{op:'bind'},{op:'replace',keys:['material'],value:{isMaterial:true,opacity:8}},{op:'set'},{op:'unbind'},{op:'set'}]});
cases.push({root:base(),path:'.morphTargetInfluences[smile]',actions:[{op:'bind'},{op:'replace',keys:['morphTargetInfluences'],value:[8,9]},{op:'set'},{op:'unbind'},{op:'set'}]});
for(const map of [null,undefined,false,{offset:[1,2]}])cases.push({root:{map,material:{map:{offset:[3,4]}}},path:'.map.offset',actions:[{op:'get'},{op:'set'}]});
for(const name of ['versionTarget','bindReentry','arrayLength','fromToArray','findGetters','sharedTables','lookupOrder','offsetCoercion','boneUndefined','boneNull','boneFound'])cases.push({kind:'callback',name});
for(const cached of [0,1,2,3,'0'])for(const mutate of [false,true])cases.push({kind:'composite',cached,mutate});
let seed=712;const rand=()=>((seed=Math.imul(seed,1664525)+1013904223>>>0)/4294967296);
const chars='abc[].:/_-012';for(let i=0;i<300;i++){let value='';for(let n=0,count=1+Math.floor(rand()*28);n<count;n++)value+=chars[Math.floor(rand()*chars.length)];cases.push({kind:'parse',value});}
const expected=cases.map(r=>{const messages=[],warn=console.warn,error=console.error;console.warn=console.error=(m)=>messages.push(m);let result;try{result=run(structuredClone(r));}catch(e){result={error:e.name};}finally{console.warn=warn;console.error=error;}return {result,messages};});
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases,wire),encoding:'utf8',maxBuffer:64*1024*1024}));
let comparisons=0;function compare(a,b,p){if(a!==null&&b!==null&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),p+' keys');for(const key of Object.keys(a))compare(a[key],b[key],p+'.'+key);return;}assert.deepEqual(a,b,p);comparisons++;}
assert.equal(actual.length,cases.length);const failures=[];for(let i=0;i<cases.length;i++){try{compare(actual[i],expected[i],`case[${i}]`);}catch(e){failures.push({case:i,input:cases[i],message:e.message});}}
if(failures.length){console.error(JSON.stringify(failures.slice(0,15),wire,2));throw new Error(`${failures.length} PropertyBinding scenarios differ`);}
console.log(`PropertyBinding parity: ${cases.length} scenarios / ${comparisons} comparisons; upstream ${lock.commit}`);
