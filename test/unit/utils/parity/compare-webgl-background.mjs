import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const [upstream,probe,requestedOutput]=process.argv.slice(2);
assert(upstream&&probe,'Usage: compare-webgl-background.mjs <pinned> <probe|--oracle-only> [fresh-directory]');
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=file=>import(pathToFileURL(path.join(upstream,'src',file)).href);
const {WebGLBackground}=await load('renderers/webgl/WebGLBackground.js');
const {Texture}=await load('textures/Texture.js'),{CubeTexture}=await load('textures/CubeTexture.js');
const {Color}=await load('math/Color.js'),{Euler}=await load('math/Euler.js'),{Matrix4}=await load('math/Matrix4.js');
const {ColorManagement}=await load('math/ColorManagement.js');
const requests=JSON.parse(fs.readFileSync(new URL('./webgl-background-fixtures.json',import.meta.url)));
const own=(object,key)=>Object.hasOwn(object,key);
function clean(value){if(value===undefined)return {$undefined:true};if(typeof value==='number'&&!Number.isFinite(value))return {$number:Number.isNaN(value)?'NaN':value<0?'-Infinity':'Infinity'};if(typeof value==='number'&&Object.is(value,-0))return {$number:'-0'};if(Array.isArray(value))return value.map(clean);if(value&&typeof value==='object')return Object.fromEntries(Object.keys(value).map(k=>[k,clean(value[k])]));return value;}
function fnMetadata(fn){return {name:fn.name,length:fn.length,source:fn.toString(),constructible:own(fn,'prototype')};}
function metadata(){const value=new WebGLBackground();return {factory:fnMetadata(WebGLBackground),keys:Object.keys(value),methods:Object.entries(value).map(([key,fn])=>({key,...fnMetadata(fn),flags:['writable','enumerable','configurable'].map(k=>Object.getOwnPropertyDescriptor(value,key)[k])}))};}
function texture(settings){const result=new (settings.kind==='cube'?CubeTexture:Texture)();result.colorSpace=settings.colorSpace??'srgb';result.isRenderTargetTexture=settings.renderTarget??false;result.matrixAutoUpdate=settings.matrixAutoUpdate??true;result.offset.set(.2,.3);result.repeat.set(2,3);result.rotation=.4;return result;}
function context(request,events){
    const c={request,events,meshes:[],draws:[],textures:(request.textures??[{kind:'plane',colorSpace:'srgb'},{kind:'cube',colorSpace:'srgb-linear'}]).map(texture),target:request.target==='target'?{}:request.target==='xr'?{isXRRenderTarget:true,texture:{colorSpace:'srgb'}}:null,mode:request.mode,toneMapping:0};
    const log=(name,...args)=>{events.push([name,...args]);};
    c.renderer={autoClear:own(request,'autoClear')?request.autoClear:true,autoClearColor:true,autoClearDepth:true,autoClearStencil:true,outputColorSpace:request.outputColorSpace??'srgb',toneMapping:0,xr:{getEnvironmentBlendMode(){log('xr');if(c.xrThrow){c.xrThrow=false;throw 'xr';}return c.mode;}},getRenderTarget(){log('target');return c.target;},clear(...args){log('clear',...args);if(c.clearThrow){c.clearThrow=false;throw 'clear';}}};
    c.state={buffers:{color:{setClear(...args){log('setClear',...args);},setMask(...args){log('color.setMask',...args);}},depth:{setTest(...args){log('depth.setTest',...args);},setMask(...args){log('depth.setMask',...args);}}}};
    c.environments={get(bg,blur){log('env',c.textures.indexOf(bg),blur);if(c.envThrow){c.envThrow=false;throw 'env';}return request.envResult==='null'?null:request.envResult==='undefined'?undefined:request.envResult==='alternate'?c.textures[1]:request.envResult==='first'?c.textures[0]:bg;}};
    c.objects={update(mesh){c.meshes.push(mesh);const index=c.meshes.length-1;log('objects',index,mesh.material.name);mesh.geometry.addEventListener('dispose',()=>{log('dispose:geometry',index);if(c.disposeThrow){c.disposeThrow=false;throw 'dispose';}});mesh.material.addEventListener('dispose',()=>log('dispose:material',index));if(c.updateThrow){c.updateThrow=false;throw 'objects';}return mesh.geometry;}};
    c.renderList={unshift(mesh,geometry,material,...args){log('unshift',c.meshes.indexOf(mesh),geometry===mesh.geometry,material===mesh.material,...args);c.draws.push(mesh);}};
    c.scene={isScene:own(request,'isScene')?request.isScene:true,background:null,backgroundBlurriness:0,backgroundIntensity:1,backgroundRotation:new Euler()};
    c.factory=new WebGLBackground(c.renderer,c.environments,c.state,c.objects,request.alpha,request.premultiplied);
    return c;
}
function meshSnapshot(c,mesh){
    const g=mesh.geometry,m=mesh.material,u=m.uniforms,cube=m.name==='BackgroundCubeMaterial',key=cube?'envMap':'map',d=Object.getOwnPropertyDescriptor(m,key);
    const result={identity:c.meshes.indexOf(mesh),geometry:{type:g.type,attributes:Object.keys(g.attributes),position:Array.from(g.attributes.position.array),index:Array.from(g.index.array)},material:{name:m.name,type:m.type,side:m.side,depthTest:m.depthTest,depthWrite:m.depthWrite,fog:m.fog,allowOverride:m.allowOverride,toneMapped:m.toneMapped,version:m.version,uniformKeys:Object.keys(u),vertexShader:m.vertexShader,fragmentShader:m.fragmentShader},layers:mesh.layers.mask,texture:c.textures.indexOf(m[key]),intensity:u.backgroundIntensity.value,getter:{...fnMetadata(d.get),flags:[d.enumerable,d.configurable]},matrixWorld:mesh.matrixWorld.toArray()};
    if(cube){result.rotation=u.backgroundRotation.value.toArray();result.blurriness=u.backgroundBlurriness.value;result.beforeRender=fnMetadata(mesh.onBeforeRender);}
    else result.uv=u.uvTransform.value.toArray();
    return result;
}
function act(c,action){
    if(own(action,'texture'))c.scene.background=c.textures[action.texture];
    if(action.background==='color')c.scene.background=new Color(.1,.4,.7);
    else if(action.background==='none')c.scene.background=null;
    else if(action.background==='texture')c.scene.background=c.textures[0];
    if(own(action,'intensity'))c.scene.backgroundIntensity=action.intensity;
    if(own(action,'blurriness'))c.scene.backgroundBlurriness=action.blurriness;
    if(action.rotation)c.scene.backgroundRotation.set(...action.rotation);
    if(action.bumpVersion)c.scene.background.version++;
    if(own(action,'toneMapping'))c.renderer.toneMapping=action.toneMapping;
    let result;
    if(action.op==='setColor')result=c.factory.setClearColor(action.color,action.alpha);
    else if(action.op==='setAlpha')result=c.factory.setClearAlpha(action.alpha);
    else if(action.op==='render')result=c.factory.render(c.scene);
    else if(action.op==='dispose')result=c.factory.dispose();
    else result=c.factory.addToRenderList(c.renderList,c.scene);
    if(action.op==='add'&&c.draws.length){const mesh=c.draws.at(-1);if(mesh.material.name==='BackgroundCubeMaterial')mesh.onBeforeRender(c.renderer,c.scene,{matrixWorld:new Matrix4().makeTranslation(3,4,5)});}
    return {undefined:result===undefined,color:[c.factory.getClearColor().r,c.factory.getClearColor().g,c.factory.getClearColor().b],alpha:c.factory.getClearAlpha(),meshes:c.meshes.map(m=>meshSnapshot(c,m)),draws:c.draws.map(m=>c.meshes.indexOf(m))};
}
function observable(request,events){
    const c=context(request,events),variant=request.variant,rows=[];
    function attempt(action){try{rows.push({value:act(c,action)});}catch(error){rows.push({error:typeof error==='object'?{name:error.name}:{raw:error}});}}
    if(['updateThrow','envThrow','clearThrow','xrThrow','disposeThrow'].includes(variant)){
        c[variant]=true;
        const method=variant==='clearThrow'||variant==='xrThrow'?'render':'add';
        if(variant==='disposeThrow'){attempt({op:'add',texture:1});attempt({op:'add',texture:0});attempt({op:'dispose'});attempt({op:'dispose'});attempt({op:'add',texture:1});}
        else{attempt({op:method,texture:variant==='updateThrow'?1:0});attempt({op:method,texture:variant==='updateThrow'?1:0});}
    }else if(variant==='cacheGetters'){
        let version=0;Object.defineProperty(c.textures[1],'version',{configurable:true,get(){events.push('get:version');return version++;}});
        Object.defineProperty(c.renderer,'toneMapping',{get(){events.push('get:toneMapping');return 0;}});
        attempt({op:'add',texture:1});attempt({op:'add',texture:1});attempt({op:'add',texture:1});
    }else if(variant==='rgbReentry'){
        const other=context({},events);other.factory.setClearColor(0x123456,.25);events.length=0;
        c.factory.getClearColor().getRGB=function(target){events.push('outer:getRGB');target.r=1;target.g=2;target.b=3;other.factory.render(other.scene);return target;};
        attempt({op:'render'});
    }else if(variant==='uniformAlias'){
        const token={raw:true};c.scene.backgroundIntensity=token;c.scene.backgroundBlurriness=token;
        attempt({op:'add',texture:1});rows.push({intensityAlias:c.meshes[0].material.uniforms.backgroundIntensity.value===token,blurrinessAlias:c.meshes[0].material.uniforms.backgroundBlurriness.value===token});
    }else if(variant==='mapGetter'){
        attempt({op:'add',texture:0});const mesh=c.meshes[0];mesh.material.uniforms.t2D.value=c.textures[1];rows.push({alias:mesh.material.map===c.textures[1]});
    }else if(variant==='beforeRenderReceiver'){
        attempt({op:'add',texture:1});const mesh=c.meshes[0],receiver={matrixWorld:new Matrix4()};mesh.onBeforeRender.call(receiver,c.renderer,c.scene,{matrixWorld:new Matrix4().makeTranslation(8,9,10)});rows.push({receiver:receiver.matrixWorld.toArray(),mesh:mesh.matrixWorld.toArray()});
    }else throw new Error(variant);
    return rows;
}
function colorRGB(request,events){
    const variant=request.variant,self=new Color(.1,.4,.7),target={},stored={},original=ColorManagement.workingToColorSpace;
    if(variant==='metadata')return fnMetadata(Color.prototype.getRGB);
    for(const key of ['r','g','b']){
        Object.defineProperty(target,key,{get(){return stored[key];},set(value){events.push(['set:'+key,value]);stored[key]=value;if(variant==='setterThrow'&&key==='g')throw 'setter';if(variant==='setterReentry'&&key==='r')new Color(.8,.2,.3).getRGB({});},enumerable:true});
        if(variant==='getterOrder')Object.defineProperty(self,key,{get(){events.push('get:'+key);return key==='r'?.1:key==='g'?.4:.7;}});
    }
    if(['returnIgnored','convertReentry','convertThrow'].includes(variant))ColorManagement.workingToColorSpace=function(color,space){events.push(['convert',space]);if(variant==='convertThrow')throw 'convert';if(variant==='convertReentry'){ColorManagement.workingToColorSpace=original;new Color(.6,.8,.2).getHex();}else {color.r=.2;color.g=.3;color.b=.4;}return {r:9,g:9,b:9};};
    try{const output=self.getRGB(variant==='targetNull'?null:target,request.colorSpace);return {same:output===target,stored};}finally{ColorManagement.workingToColorSpace=original;}
}
function run(request){if(request.op==='metadata')return metadata();const events=[];try{const c=['observable','colorRGB'].includes(request.op)?null:context(request,events);return clean({value:c?request.actions.map(a=>act(c,a)):request.op==='colorRGB'?colorRGB(request,events):observable(request,events),events});}catch(error){return clean({error:typeof error==='object'?{name:error.name}:{raw:error},events});}}
const expected=requests.map(run);for(let i=0;i<requests.length;i++){
    if(requests[i].expectedError)assert.deepEqual(expected[i].error,requests[i].expectedError,requests[i].case);
    else assert(!expected[i].error,'Unexpected source error '+requests[i].case+': '+JSON.stringify(expected[i].error));
    if(requests[i].op==='observable'){
        const wanted={updateThrow:['objects'],envThrow:['env'],clearThrow:['clear'],xrThrow:['xr'],disposeThrow:['dispose']}[requests[i].variant]??[];
        assert.deepEqual(expected[i].value.filter(row=>row.error).map(row=>row.error),wanted.map(raw=>({raw})),requests[i].case);
    }
}
const output=requestedOutput??path.join('D:/Temp/surui','webgl-background-'+Date.now());assert(!fs.existsSync(output),'Fresh output required');fs.mkdirSync(output,{recursive:true});
for(const [name,value]of[['requests',requests],['expected',expected]])fs.writeFileSync(path.join(output,name+'.json'),JSON.stringify(value,null,2));
const failures=[];
function compare(a,b,where){if(typeof a==='number'&&typeof b==='number')assert(Math.abs(a-b)<=1e-12*Math.max(1,Math.abs(a),Math.abs(b)),where+': '+a+' != '+b);else if(Array.isArray(b)){assert(Array.isArray(a),where);assert.equal(a.length,b.length,where);b.forEach((v,i)=>compare(a[i],v,where+'['+i+']'));}else if(b&&typeof b==='object'){assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),where);for(const key of Object.keys(b))compare(a[key],b[key],where+'.'+key);}else assert.deepEqual(a,b,where);}
if(probe!=='--oracle-only'){
    const actual=JSON.parse(execFileSync(path.resolve(probe),{input:JSON.stringify(requests),encoding:'utf8',maxBuffer:128*1024*1024,timeout:300000,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}}).trim().split(/\r?\n/).at(-1));fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);
    requests.forEach((r,i)=>{try{compare(actual[i],expected[i],r.case);}catch(error){failures.push({case:r.case,message:error.message});}});
}
const summary={cases:requests.length,expectedErrors:expected.filter(row=>row.error).length+5,failures};fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2));console.log(JSON.stringify({output,cases:requests.length,failures:failures.length}));assert.equal(failures.length,0,JSON.stringify(failures.slice(0,5)));
