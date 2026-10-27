import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync,spawnSync} from 'node:child_process';
import {fileURLToPath,pathToFileURL} from 'node:url';

const child=process.argv[2]==='--oracle-child';
const relativeResourceIds=process.argv.includes('--relative-import-resource-ids');
const upstream=path.resolve(process.argv[child?3:2]);
const load=p=>import(pathToFileURL(path.join(upstream,'src',p+'.js')));

if(child){
    const [observerExports,{Material},{Object3D},{BufferGeometry},{BufferAttribute},{Color},{Vector2},{Euler},{Texture},{SpotLight},{EventDispatcher},{default:NodeFrame},constants]=await Promise.all([
        load('materials/nodes/manager/NodeMaterialObserver'),load('materials/Material'),load('core/Object3D'),load('core/BufferGeometry'),load('core/BufferAttribute'),load('math/Color'),load('math/Vector2'),load('math/Euler'),load('textures/Texture'),load('lights/SpotLight'),load('core/EventDispatcher'),load('nodes/core/NodeFrame'),load('constants')
    ]);
    const Observer=observerExports.default;
    // Import the actual builder only after its public graph dependencies have initialized.
    const {default:NodeBuilder}=await load('nodes/core/NodeBuilder'),{default:Node}=await load('nodes/core/Node');
    const r=JSON.parse(fs.readFileSync(0,'utf8'));
    let textureOrigin;const actualTextureIds=new Set();
    function texture(){const value=new Texture();if(relativeResourceIds)actualTextureIds.add(value.id);return value;}
    function normal(v,seen=[]){
        if(v===undefined)return '#undefined';if(typeof v==='function')return '#function';if(typeof v==='symbol')return '#symbol';
        if(typeof v==='number'){if(Number.isNaN(v))return '#NaN';if(!Number.isFinite(v))return v>0?'#Infinity':'#-Infinity';}
        if(v===null||typeof v!=='object')return v;if(v instanceof WeakMap)return '#WeakMap';
        const i=seen.indexOf(v);if(i!==-1)return ['#ref',i];seen.push(v);const out=Array.isArray(v)?new Array(v.length):{};for(const key of Object.keys(v))out[key]=normal(v[key],seen);return out;
    }
    function constructible(v){try{Reflect.construct(function(){},[],v);return true;}catch{return false;}}
    function descriptors(o){return Object.getOwnPropertyNames(o).map(name=>{const d=Object.getOwnPropertyDescriptor(o,name),out={name,writable:!!d.writable,enumerable:!!d.enumerable,configurable:!!d.configurable};if(d.get)out.get=d.get.name;if(Object.hasOwn(d,'value')){const v=d.value;out.value=typeof v==='function'?'#function':v&&typeof v==='object'?'#object':normal(v);if(typeof v==='function'){out.functionName=v.name;out.length=v.length;out.constructible=constructible(v);}}return out;});}
    function attribute(){return new BufferAttribute(new Float32Array([0,1,2,3,4,5]),3);}
    function fixture(){
        const object=new Object3D(),geometry=new BufferGeometry(),material=new Material();
        Object.assign(material,{envMap:null,map:null,color:new Color(0x123456),normalScale:new Vector2(1,1),opacity:1,anisotropyRotation:null});geometry.attributes.position=attribute();
        const scene={environment:null,environmentIntensity:1,environmentRotation:new Euler()},context={width:640,height:480},lights=[],lightsNode={getBuiltinLights(){return lights;}},host={mrt:null},renderer={getMRT(){return host.mrt;}};
        object.material=material;const builder=new NodeBuilder(object,renderer,null),observer=new Observer(builder),frame=new NodeFrame();frame.renderer=renderer;frame.renderId=0;
        const renderObject={object,geometry,material,context,scene,lightsNode,bundle:null};
        if(relativeResourceIds&&textureOrigin===undefined){textureOrigin=texture().id;process.stderr.write('#TextureBaseline '+textureOrigin+'\n');}
        return{object,geometry,material,scene,context,lights,lightsNode,renderer,host,builder,observer,frame,renderObject};
    }
    function configure(f,kind){const o=f.object;if(kind==='center')o.center=new Vector2(.5,.5);if(kind==='morph')o.morphTargetInfluences=[0,.25,.5];if(kind==='instance')Object.assign(o,{isInstancedMesh:true,instanceMatrix:attribute(),instanceColor:null,morphTexture:null});if(kind==='batch')Object.assign(o,{isBatchedMesh:true,_matricesTexture:texture(),_colorsTexture:null,_indirectTexture:texture()});if(kind==='bundle')f.renderObject.bundle={version:0,static:true};if(kind==='environment')f.scene.environment=texture();if(kind==='lights'){const light=new SpotLight();Object.defineProperty(light,'map',{value:texture(),writable:true,enumerable:true,configurable:true});light.castShadow=true;f.lights.push(light);}if(kind==='texture')f.material.map=texture();}
    function action(f,a){const {object:o,geometry:g,material:m,renderObject:ro,frame}=f;
        if(a==='next')frame.renderId++;else if(a==='matrix')o.matrixWorld.elements[12]=3;else if(a==='opacity')m.opacity=.25;else if(a==='color')m.color.setRGB(.2,.3,.4);else if(a==='nullColor')m.color=null;else if(a==='newColor')m.color=new Color(0xff0000);else if(a==='removeOpacity')delete m.opacity;else if(a==='newOpacity')m.opacity=.7;
        else if(a==='width')f.context.width=800;else if(a==='height')f.context.height=600;else if(a==='geometry')ro.geometry=new BufferGeometry();else if(a==='attribute')g.attributes.position.needsUpdate=true;else if(a==='removeAttribute')delete g.attributes.position;else if(a==='addAttribute')g.attributes.normal=attribute();else if(a==='replaceAttribute'){delete g.attributes.position;g.attributes.normal=attribute();}else if(a==='index')g.index=attribute();else if(a==='indexVersion')g.index.needsUpdate=true;else if(a==='nullIndex')g.index=null;else if(a==='range')g.drawRange.count=3;else if(a==='disposeGeometry')g.dispatchEvent({type:'dispose'});
        else if(a==='morph')o.morphTargetInfluences[1]=.75;else if(a==='instanceMatrix')o.instanceMatrix.needsUpdate=true;else if(a==='instanceColor')o.instanceColor=attribute();else if(a==='instanceColorVersion')o.instanceColor.needsUpdate=true;else if(a==='morphTexture')o.morphTexture=texture();else if(a==='textureVersion')m.map.needsUpdate=true;else if(a==='newTexture')m.map=texture();else if(a==='nullTexture')m.map=null;else if(a==='disposeTexture')m.map.dispose();else if(a==='matricesTexture')o._matricesTexture.needsUpdate=true;else if(a==='colorsTexture')o._colorsTexture=texture();else if(a==='indirectTexture')o._indirectTexture.needsUpdate=true;
        else if(a==='center')o.center.set(.2,.4);else if(a==='environmentIntensity')f.scene.environmentIntensity=2;else if(a==='environmentRotation')f.scene.environmentRotation.x=.3;else if(a==='lightMap')f.lights[0].map.needsUpdate=true;else if(a==='lightDispose')f.lights[0].map.dispose();else if(a==='shadowWidth')f.lights[0].shadow.mapSize.width=1024;else if(a==='shadowHeight')f.lights[0].shadow.mapSize.height=2048;else if(a==='bundleVersion')ro.bundle.version=1;else if(a==='static')o.static=true;else if(a==='dynamic')o.instanceMatrix.usage=35048;else if(a==='hasNode')f.observer.hasNode=true;else if(a==='hasAnimation')f.observer.hasAnimation=true;else if(a==='velocity')f.host.mrt={has(name){return name==='velocity';}};else if(a==='noVelocity')f.host.mrt=null;
    }
    function materialSnapshot(data){const out=normal(data);if(relativeResourceIds&&data.map&&typeof data.map==='object'){
        // Scope is exactly the observer's real material.map Texture snapshot.
        assert.deepEqual(Object.keys(data.map),['id','version','cacheVersion']);assert.ok(actualTextureIds.has(data.map.id),'Snapshot must originate from an actual fixture Texture');out.map.id=data.map.id-textureOrigin;
    }return out;}
    function state(f){const {observer:o,renderObject:ro}=f,out={renderId:o.renderId,initialized:o.renderObjects.has(ro)};if(out.initialized){out.renderObject=normal(o.renderObjects.get(ro));out.geometry=normal(o.getGeometryData(ro.geometry));out.material=materialSnapshot(o.getMaterialData(f.material));}return out;}
    function metadata(){const f=fixture(),o=f.observer,other=new Observer(f.builder),out={keys:Object.keys(observerExports),constructor:descriptors(Observer),prototype:descriptors(Observer.prototype),own:normal(o),sharedUniforms:o.refreshUniforms===other.refreshUniforms,separateMaps:o.renderObjects!==other.renderObjects,objectPrototype:Object.getPrototypeOf(Observer.prototype)===Object.prototype};const facade={...observerExports};facade.default=null;out.privateBinding=Observer===o.constructor;out.privateCtor=Observer===o.constructor;try{Observer(f.builder);}catch(e){out.callError='#'+e.name;}return out;}
    function refresh(){const f=fixture();configure(f,r.configure);return r.steps.map(a=>{const out={};try{action(f,a);out.result=normal(f.observer.needsRefresh(f.renderObject,f.frame));}catch(e){out.error='#'+e.name;}out.state=state(f);return out;});}
    function event(s,name){if(!s.active)return;s.events.push(name);if(s.request.throwAt===name)throw new TypeError('fixture getter');}
    function observe(s,o,key,name){const old=Object.getOwnPropertyDescriptor(o,key);if(old&&!old.configurable)return;const slot={value:o[key]};s.slots.push([o,key,slot]);Object.defineProperty(o,key,{get(){event(s,'get '+name);return slot.value;},set(v){event(s,'set '+name);slot.value=v;},enumerable:true,configurable:true});}
    function observeMethod(s,o,key,name){const original=o[key];o[key]=function(...args){event(s,'call '+name);return original.apply(this,args);};}
    function protocol(){const f=fixture(),op=r.operation;configure(f,r.configure);const {observer:o,renderObject:ro}=f;if(op==='equals'||op==='needsRefresh')o.getRenderObjectData(ro);const s={request:r,events:[],slots:[],active:false},out={};
        for(const key of ['renderObjects','hasNode','hasAnimation','refreshUniforms','renderId'])observe(s,o,key,'observer.'+key);
        for(const key of ['object','geometry','material','context','scene','lightsNode','bundle'])observe(s,ro,key,'renderObject.'+key);
        for(const key of ['isInstancedMesh','isBatchedMesh','instanceMatrix','instanceColor','morphTexture','_matricesTexture','_colorsTexture','_indirectTexture','center','morphTargetInfluences','matrixWorld','static'])observe(s,f.object,key,'object.'+key);
        for(const key of ['id','attributes','index','drawRange'])observe(s,f.geometry,key,'geometry.'+key);
        for(const key of ['start','count'])observe(s,f.geometry.drawRange,key,'drawRange.'+key);
        for(const key of ['opacity','color','map','envMap','_renderId','_version'])observe(s,f.material,key,'material.'+key);
        for(const key of ['width','height'])observe(s,f.context,key,'context.'+key);
        for(const key of ['environment','environmentIntensity','environmentRotation'])observe(s,f.scene,key,'scene.'+key);
        for(const key of ['renderId','renderer'])observe(s,f.frame,key,'frame.'+key);
        for(const key of ['getRenderObjectData','getGeometryData','getTextureData','getMaterialData','getLightsData','getLights','getAttributesData','firstInitialization','hasDynamicInstancing','needsVelocity','equals'])observeMethod(s,o,key,'observer.'+key);
        observeMethod(s,f.lightsNode,'getBuiltinLights','lightsNode.getBuiltinLights');observeMethod(s,f.geometry,'addEventListener','geometry.addEventListener');observeMethod(s,f.renderer,'getMRT','renderer.getMRT');s.events=[];s.active=true;
        try{const args=op==='getGeometryData'?[f.geometry]:op==='getMaterialData'?[f.material]:op==='getLights'?[f.lightsNode,1]:op==='containsNode'?[f.builder]:[ro,op==='equals'?[]:f.frame,1];out.result=normal(o[op](...args));}catch(e){out.error='#'+e.name;}
        s.active=false;for(const[o,key,slot]of s.slots)Object.defineProperty(o,key,{value:slot.value,writable:true,enumerable:true,configurable:true});out.events=s.events;out.state=state(f);return out;
    }
    function cache(){const f=fixture(),o=f.observer,out={};
        if(r.case==='geometry'){const g=f.geometry,a=o.getGeometryData(g),other=new Observer(f.builder);out.initial=normal(a);out.shared=a===other.getGeometryData(g);g.dispatchEvent({type:'dispose'});g.dispatchEvent({type:'dispose'});out.disposed=normal(a);}
        if(r.case==='texture'||r.case==='renderTarget'){const t=new Texture(),target=new EventDispatcher();if(r.case==='renderTarget')t.renderTarget=target;const a=o.getTextureData(t),other=new Observer(f.builder);out.initial=normal(a);out.shared=a===other.getTextureData(t);(r.case==='renderTarget'?target:t).dispatchEvent({type:'dispose'});out.disposed=normal(a);if(r.case==='renderTarget'){t.dispatchEvent({type:'dispose'});out.ownDisposeIgnored=normal(a);}}
        if(r.case==='sharedMaterial'){const ro=f.renderObject,ro2={...ro},other=new Observer(f.builder);o.getRenderObjectData(ro);other.getRenderObjectData(ro2);f.material.opacity=.3;out.first=o.equals(ro,[],1);out.second=other.equals(ro2,[],1);out.sameData=o.getMaterialData(f.material)===other.getMaterialData(f.material);out.material=normal(o.getMaterialData(f.material));}
        if(r.case==='lightsReentry'){const node=f.lightsNode,guard={once:true,nested:null};node.getBuiltinLights=function(){if(guard.once){guard.once=false;guard.nested=o.getLights(this,9);}return f.lights;};const a=o.getLights(node,9);out.sameNested=a===guard.nested;out.value=normal(a);}
        if(r.case==='geometryReentry'){const g=f.geometry,guard={once:true,nested:null},original=g.addEventListener;g.addEventListener=function(...args){if(guard.once){guard.once=false;guard.nested=o.getGeometryData(this);}return original.apply(this,args);};const a=o.getGeometryData(g);out.sameNested=a===guard.nested;out.cachedOuter=a===o.getGeometryData(g);g.dispatchEvent({type:'dispose'});out.outer=normal(a);out.nested=normal(guard.nested);}
        if(r.case==='enum'){const events=[];for(const key of ['NONE','SHARED','FULL'])Object.defineProperty(constants.RenderObjectRefreshType,key,{get(){events.push(key);return 'enum-'+key;},enumerable:true,configurable:true});const values=[];for(const op of ['','','next','hasNode']){action(f,op);values.push(o.needsRefresh(f.renderObject,f.frame));}out.values=values;out.events=events;}
        return out;
    }
    function edge(){const f=fixture(),o=f.observer,out={};
        if(r.case==='dynamic'){Object.defineProperty(f.object,'isInstancedMesh',{value:r.instanced,writable:true,enumerable:true,configurable:true});f.object.instanceMatrix={usage:r.matrixUsage};f.object.instanceColor=r.color==='undefined'?undefined:r.color===null?null:{usage:r.color};out.value=o.hasDynamicInstancing(f.object);}
        if(r.case==='velocity'){let mrt=r.value;out.calls=[];if(mrt==='undefined-mrt')mrt=undefined;else if(mrt!==null){const value=mrt;mrt={has(...args){out.calls.push(args);return value;}};}f.host.mrt=mrt;out.value=normal(o.needsVelocity(f.renderer));}
        if(r.case==='containsContext'){f.builder.context[r.key]=r.value;out.value=o.containsNode(f.builder);}
        if(r.case==='containsInherited'){const proto=Object.create(Object.getPrototypeOf(f.material));proto.inheritedNode=new Node();Object.setPrototypeOf(f.material,proto);out.value=o.containsNode(f.builder);}
        if(r.case==='firstStrict'){const calls=[];o.renderObjects={has(){return r.value==='undefined'?undefined:r.value;}};o.getRenderObjectData=()=>{calls.push('getRenderObjectData');};out.value=o.firstInitialization(f.renderObject);out.calls=calls;}
        if(r.case==='constructorOverride'){out.calls=[];Observer.prototype.containsNode=function(builder){out.calls.push(builder===f.builder);return 'contains-sentinel';};out.own=normal(new Observer(f.builder));}
        if(r.case==='worldStrict'){const data=o.getRenderObjectData(f.renderObject),calls=[];data.worldMatrix.equals=()=>r.value==='undefined'?undefined:r.value;data.worldMatrix.copy=()=>{calls.push('copy');};out.value=o.equals(f.renderObject,[],1);out.calls=calls;}
        if(r.case==='uniformList'){const other=new Observer(f.builder);o.refreshUniforms.push('custom');f.material.custom=17;out.shared=other.refreshUniforms.at(-1);out.material=normal(o.getMaterialData(f.material));}
        return out;
    }
    let out;try{out=r.kind==='metadata'?metadata():r.kind==='refresh'?refresh():r.kind==='protocol'?protocol():r.kind==='edge'?edge():cache();}catch(e){out={error:'#'+e.name};}process.stdout.write(JSON.stringify(out));process.exit(0);
}

assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const binary=process.argv[3]&&process.argv[3]!=='--oracle'?path.resolve(process.argv[3]):null;
const directory=path.resolve(process.argv[4]??path.join(process.env.TEMP,'node-material-observer-parity'));
fs.mkdirSync(directory,{recursive:true});
let oracleOrigins=[];
function runProcess(file,args,request,origins){
    const result=spawnSync(file,args,{input:JSON.stringify(request),encoding:'utf8',maxBuffer:12*1024*1024});
    assert.equal(result.status,0,`${file}: ${result.stderr??result.error}`);
    if(relativeResourceIds){const matches=[...result.stderr.matchAll(/^#TextureBaseline (\d+)\r?$/gm)];assert.equal(matches.length,1,'Exactly one real Texture allocation baseline is required');origins.push(Number(matches[0][1]));}
    return JSON.parse(result.stdout);
}
function compare(cases,results){
    const nativeOrigins=[];const actual=cases.map(r=>runProcess(binary,relativeResourceIds?['--relative-import-resource-ids']:[],r,nativeOrigins));fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));
    if(relativeResourceIds)fs.writeFileSync(path.join(directory,'resource-id-baselines.json'),JSON.stringify({scope:'Only actual material.map Texture snapshots use id minus a real post-import Texture allocation; every other id and lights.map version stays absolute.',oracle:oracleOrigins,native:nativeOrigins},null,2));
    const differences=[];for(let i=0;i<cases.length;++i)try{assert.deepEqual(actual[i],results[i]);}catch{differences.push({index:i,input:cases[i],expected:results[i],actual:actual[i]});}
    fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(differences,null,2));assert.equal(differences.length,0,JSON.stringify(differences.slice(0,2),null,2));console.log(`NodeMaterialObserver: ${cases.length} fresh-process scenarios passed (${relativeResourceIds?'explicit relative material.map Texture IDs; all other IDs absolute':'absolute IDs'}, states, getter/callback order)`);
}
// Explicit replay retains the original upstream process output and fixture source without regeneration.
const replay=process.argv.indexOf('--replay-oracle');
if(replay!==-1){
    assert.ok(binary,'Replay requires a native probe binary');const source=path.resolve(process.argv[replay+1]);assert.notEqual(source,directory,'Replay output must use a new evidence directory');
    const scope=fs.existsSync(path.join(source,'scope.json'))?JSON.parse(fs.readFileSync(path.join(source,'scope.json'))).mode:'absolute';assert.equal(scope,relativeResourceIds?'relative-import-resource-ids':'absolute','Replay mode must match the recorded oracle scope');
    for(const file of ['input.json','expected.json','oracle.mjs'])fs.copyFileSync(path.join(source,file),path.join(directory,file));
    fs.writeFileSync(path.join(directory,'scope.json'),JSON.stringify({mode:scope},null,2));
    if(relativeResourceIds){fs.copyFileSync(path.join(source,'oracle-resource-id-baselines.json'),path.join(directory,'oracle-resource-id-baselines.json'));oracleOrigins=JSON.parse(fs.readFileSync(path.join(source,'oracle-resource-id-baselines.json')));}
    const cases=JSON.parse(fs.readFileSync(path.join(directory,'input.json'))),results=JSON.parse(fs.readFileSync(path.join(directory,'expected.json')));assert.equal(cases.length,results.length);compare(cases,results);process.exit(0);
}
const oracle=path.join(directory,'oracle.mjs');fs.copyFileSync(fileURLToPath(import.meta.url),oracle);
const cases=[{kind:'metadata'},...['geometry','texture','renderTarget','sharedMaterial','lightsReentry','geometryReentry','enum'].map(c=>({kind:'cache',case:c}))];
for(const instanced of [true,false,1])for(const matrixUsage of [35044,35048])for(const color of ['undefined',null,35044,35048])cases.push({kind:'edge',case:'dynamic',instanced,matrixUsage,color});
for(const value of [null,false,true,0,1,'sentinel','undefined-mrt'])cases.push({kind:'edge',case:'velocity',value});
for(const key of ['modelViewMatrix','modelNormalViewMatrix','getAO','getShadow'])for(const value of [false,0,null,1,'sentinel'])cases.push({kind:'edge',case:'containsContext',key,value});
for(const value of ['undefined',false,true,0,1,'sentinel'])for(const c of ['firstStrict','worldStrict'])cases.push({kind:'edge',case:c,value});
for(const c of ['containsInherited','constructorOverride','uniformList'])cases.push({kind:'edge',case:c});
const refreshCase=(configure,actions)=>({kind:'refresh',configure,steps:['','','next','',...actions.flatMap(action=>['next',action,'','next',''])]});
cases.push(refreshCase('', ['matrix','opacity','color','nullColor','newColor','removeOpacity','newOpacity','width','height','attribute','index','indexVersion','nullIndex','range','disposeGeometry','addAttribute','removeAttribute','geometry']));
for(const [configure,actions]of Object.entries({center:['center'],morph:['morph'],instance:['instanceMatrix','instanceColor','instanceColorVersion','morphTexture','dynamic'],batch:['matricesTexture','colorsTexture','indirectTexture'],bundle:['opacity','bundleVersion'],environment:['environmentIntensity','environmentRotation'],lights:['lightMap','lightDispose','shadowWidth','shadowHeight'],texture:['textureVersion','newTexture','disposeTexture','nullTexture','newTexture']}))cases.push(refreshCase(configure,actions));
cases.push(refreshCase('', ['replaceAttribute']),refreshCase('', ['velocity','opacity','noVelocity']),refreshCase('', ['static','matrix','opacity']),refreshCase('', ['hasNode','opacity']),refreshCase('', ['hasAnimation','opacity']));
for(const operation of ['getRenderObjectData','getGeometryData','getMaterialData','containsNode','getLights','equals','needsRefresh'])for(const configure of ['',...(operation==='equals'||operation==='getRenderObjectData'?['center','morph','instance','batch','bundle','environment','lights','texture']:[])])cases.push({kind:'protocol',operation,configure});
if(process.argv.includes('--only-edge-cases'))for(let i=cases.length-1;i>=0;--i)if(cases[i].kind!=='edge')cases.splice(i,1);
function expected(r){return runProcess(process.execPath,[oracle,'--oracle-child',upstream,...(relativeResourceIds?['--relative-import-resource-ids']:[])],r,oracleOrigins);}
const results=cases.map(expected);
// Derive exception cut points from the real upstream event stream; every distinct getter/call is exercised.
for(let i=0,n=cases.length;i<n;++i)if(cases[i].kind==='protocol')for(const throwAt of new Set(results[i].events??[])){const r={...cases[i],throwAt};cases.push(r);results.push(expected(r));}
fs.writeFileSync(path.join(directory,'input.json'),JSON.stringify(cases,null,2));fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(results,null,2));
fs.writeFileSync(path.join(directory,'scope.json'),JSON.stringify({mode:relativeResourceIds?'relative-import-resource-ids':'absolute'},null,2));if(relativeResourceIds)fs.writeFileSync(path.join(directory,'oracle-resource-id-baselines.json'),JSON.stringify(oracleOrigins));
if(!binary){console.log(`Prepared ${cases.length} real NodeMaterialObserver fresh-process scenarios`);process.exit(0);}
compare(cases,results);
