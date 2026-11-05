// Differential checks for the canonical source classes, not the typed facades.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync, spawnSync} from 'node:child_process';
const upstream=path.resolve(process.argv[2]??'D:/Temp/surui/threejs-pinned-11738a-20261104');
const binary=process.argv[3]==='--expected-only'?undefined:process.argv[3],directory=process.argv[4];
const commit='11738a9d55e502fb484cefffcea3a66dc44e9fe9';
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),commit);
assert.equal(execFileSync('git',['-C',upstream,'status','--porcelain','--untracked-files=no','--','src'],{encoding:'utf8'}).trim(),'','Pinned source must be clean');
const names=['AmbientLight','DirectionalLight','HemisphereLight','PointLight','SpotLight','LightShadow','DirectionalLightShadow','PointLightShadow','SpotLightShadow'];
const classes=await Promise.all(names.map(async name=>(await import(pathToFileURL(path.join(upstream,'src/lights',name+'.js')).href))[name]));
const {Light}=await import(pathToFileURL(path.join(upstream,'src/lights/Light.js')).href);
const {Object3D}=await import(pathToFileURL(path.join(upstream,'src/core/Object3D.js')).href);
const {ObjectLoader}=await import(pathToFileURL(path.join(upstream,'src/loaders/ObjectLoader.js')).href);
const select=(value,keys)=>Object.fromEntries(keys.map(key=>[key,value[key]]));
function shadowState(value){
    const result=select(value,['intensity','bias','biasNode','normalBias','radius','blurSamples','mapType','map','mapPass','autoUpdate','needsUpdate','focus','aspect']);
    result.mapSize=value.mapSize.toArray();result.matrix=value.matrix.toArray();result.frameExtents=value.getFrameExtents().toArray();result.viewports=value.getViewportCount();
    result.camera=value.camera?{...select(value.camera,['type','near','far','left','right','top','bottom','fov','aspect']),projection:value.camera.projectionMatrix.elements}:{};return result;
}
function state(value){
    if(!value.isLight)return shadowState(value);
    const result=select(value,['type','intensity','distance','decay','angle','penumbra','map','power']);result.position=value.position.toArray();result.color=value.color.toArray();
    if(value.groundColor)result.groundColor=value.groundColor.toArray();if(value.shadow)result.shadow=shadowState(value.shadow);return result;
}
function metadata(C,parent){
    const methods={};
    for(const key of Object.getOwnPropertyNames(C.prototype)){
        if(key==='constructor')continue;
        const d=Object.getOwnPropertyDescriptor(C.prototype,key),method={enumerable:d.enumerable,configurable:d.configurable};
        if(d.value){Object.assign(method,{name:d.value.name,length:d.value.length,source:String(d.value),writable:d.writable});}
        if(d.get){method.getName=d.get.name;method.getSource=String(d.get);}if(d.set){method.setName=d.set.name;method.setSource=String(d.set);}methods[key]=method;
    }
    return {name:C.name,length:C.length,source:String(C),parent:Object.getPrototypeOf(C)===parent,methods,initial:state(new C())};
}
function dynamicConstructor(C,fields,fail){
    const data={events:[],fail};
    function Parent(...args){
        data.newTarget=new.target===C;data.superArgs=args;const self={},position={copy(value){data.events.push(['copy position',this===position,value===Object3D.DEFAULT_UP]);return this;}};
        Object.defineProperty(self,'position',{configurable:true,get(){data.events.push('get position');return position;}});
        self.updateMatrix=function(){data.events.push('updateMatrix');return this;};
        for(const field of fields)Object.defineProperty(self,field,{configurable:true,set(){data.events.push(field);if(fail===field)throw 'field sentinel';}});
        data.instance=self;return self;
    }
    const saved=Object.getPrototypeOf(C);Object.setPrototypeOf(C,Parent);let threw=false,returnsParentObject=false;
    try{returnsParentObject=new C(17,23,'third',null,false,'last')===data.instance;}catch(error){assert.equal(error,'field sentinel');threw=true;}finally{Object.setPrototypeOf(C,saved);}
    return {fail,events:data.events,newTarget:data.newTarget,superArgs:data.superArgs,threw,returnsParentObject};
}
function dynamicCopy(C,fields){
    const target=new C(),source=new C(),events=[],parent={},prototype=C.prototype,saved=Object.getPrototypeOf(prototype);
    Object.defineProperty(parent,'copy',{configurable:true,get(){events.push(['get super.copy',this===target]);return function(...args){events.push(['call super.copy',this===target,args[0]===source,args.length,args.length>1?args[1]:undefined]);return 17;};}});
    for(const field of fields){let value=source[field];if(field==='target'||field==='shadow'){value={clone(){events.push(['clone',field,this===value]);return {};}};}Object.defineProperty(source,field,{configurable:true,get(){events.push('get source.'+field);return value;}});}
    Object.setPrototypeOf(prototype,parent);let copied;try{copied=target.copy(source,false);}finally{Object.setPrototypeOf(prototype,saved);}return {events,returnsSelf:copied===target};
}
function cloneCase(C,recursive){
    const source=new C();source.color.setRGB(.2,.3,.4);source.name='source';source.add(new Object3D());
    if(source.target){source.target.name='target';source.target.add(new Object3D());}
    const bias={},map={},shadow=source.shadow;
    if(shadow){shadow.bias=-.2;shadow.biasNode=bias;shadow.map=map;shadow.mapType=1015;shadow.mapSize.set(128,256);}if(source.isSpotLight)source.map=map;
    const copy=source.clone(recursive),out={derived:copy instanceof C,distinctColor:copy.color!==source.color,children:copy.children.length,state:state(copy)};
    if(shadow){out.distinctShadow=copy.shadow!==shadow;out.distinctCamera=copy.shadow.camera!==shadow.camera;out.biasNodeAlias=copy.shadow.biasNode===bias;}
    if(source.target){out.distinctTarget=copy.target!==source.target;out.targetChildren=copy.target.children.length;}if(source.isSpotLight)out.mapAlias=copy.map===map;return out;
}
function primitivePrototype(){
    const OriginalObject=Object;function NewTarget(){}NewTarget.prototype=17;globalThis.Object={prototype:OriginalObject.create(null)};
    let shadow;try{shadow=Reflect.construct(classes[5],[],NewTarget);}finally{globalThis.Object=OriginalObject;}
    return {intrinsicPrototype:Object.getPrototypeOf(shadow)===Object.prototype,initial:{...select(shadow,['intensity','bias','mapType','map','mapPass']),cameraUndefined:shadow.camera===undefined,mapSize:shadow.mapSize.toArray()}};
}
function disposal(fail){
    const shadow=new classes[5](),events=[],replacement={dispose(){events.push('new mapPass');}},old={dispose(){events.push('old mapPass');}};
    let pass=old;const map={dispose(){events.push('map');shadow.mapPass=replacement;if(fail)throw 'dispose sentinel';}};
    Object.defineProperty(shadow,'map',{configurable:true,get(){events.push('get map');return map;}});
    Object.defineProperty(shadow,'mapPass',{configurable:true,get(){events.push('get mapPass');return pass;},set(value){events.push('set mapPass');pass=value;}});
    let threw=false;try{shadow.dispose();}catch(error){assert.equal(error,'dispose sentinel');threw=true;}return {events,threw,replacementRetained:pass===replacement};
}
const expected={metadata:{},construction:{},copies:{},clones:{},loader:{}};
for(let i=0;i<classes.length;++i)expected.metadata[names[i]]=metadata(classes[i],i<5?Light:i===5?Function.prototype:classes[5]);
const fields=[['isAmbientLight','type'],['isDirectionalLight','type','target','shadow'],['isHemisphereLight','type','groundColor'],['isPointLight','type','distance','decay','shadow'],['isSpotLight','type','target','distance','angle','penumbra','decay','map','shadow']];
const loader=new ObjectLoader();
for(let i=0;i<5;++i){
    const C=classes[i],name=names[i];expected.construction[name]=['',...fields[i]].map(fail=>dynamicConstructor(C,fields[i],fail));
    for(const recursive of [false,true])expected.clones[name+':'+recursive]=cloneCase(C,recursive);
    const parsed=loader.parseObject({type:name,uuid:name},{},{},{},{});expected.loader[name]={binding:true,prototype:Object.getPrototypeOf(parsed)===C.prototype,state:state(parsed)};
}
for(const [index,fields]of [[1,['target','shadow']],[2,['groundColor']],[3,['distance','decay','shadow']],[4,['distance','angle','penumbra','decay','map','target','shadow']],[8,['focus','aspect']]])expected.copies[names[index]]=dynamicCopy(classes[index],fields);
expected.primitivePrototype=primitivePrototype();
expected.disposal=[disposal(false),disposal(true)];
const normalized=JSON.parse(JSON.stringify(expected));
if(directory){fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(normalized,null,2));}
if(!binary){console.log('Generated pinned canonical light/shadow class oracle: metadata, defaults, dynamic super/newTarget/throws, copies, clones, loader and intrinsic allocation');process.exit(0);}
const run=spawnSync(path.resolve(binary),[],{encoding:'utf8',maxBuffer:4*1024*1024,env:process.env});assert.equal(run.status,0,run.stderr??run.error?.message);
const actual=JSON.parse(run.stdout);if(directory)fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));
let comparisons=0;
function compare(a,b,location='root'){
    ++comparisons;if(typeof a==='number'&&typeof b==='number'){assert.ok(Math.abs(a-b)<=2e-11*Math.max(1,Math.abs(a),Math.abs(b)),`${location}: ${a} vs ${b}`);return;}
    if(a&&b&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),location+' keys');for(const key of Object.keys(b))compare(a[key],b[key],location+'.'+key);return;}assert.deepEqual(a,b,location);
}
compare(actual,normalized);console.log(`Canonical native light classes: ${comparisons} comparisons match ${commit}`);
