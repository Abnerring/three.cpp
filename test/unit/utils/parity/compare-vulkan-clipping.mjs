import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
assert.equal(process.argv.length,4,'Usage: node compare-vulkan-clipping.mjs <probe executable> <three.js checkout>');
const checkout=resolve(process.argv[3]);
const revision=spawnSync('git',['-C',checkout,'rev-parse','HEAD'],{encoding:'utf8'});
assert.equal(revision.status,0,revision.stderr);assert.equal(revision.stdout.trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const source=relative=>import(pathToFileURL(resolve(checkout,relative)).href);
const {WebGLClipping}=await source('src/renderers/webgl/WebGLClipping.js');
const {WebGLProperties}=await source('src/renderers/webgl/WebGLProperties.js');
const {Plane}=await source('src/math/Plane.js');const {Vector3}=await source('src/math/Vector3.js');
const {OrthographicCamera}=await source('src/cameras/OrthographicCamera.js');
const {MeshBasicMaterial}=await source('src/materials/MeshBasicMaterial.js');
const rendererSource=readFileSync(resolve(checkout,'src/renderers/WebGLRenderer.js'),'utf8');
const start=rendererSource.indexOf('if ( _clippingEnabled === true ) {',rendererSource.indexOf('function setProgram('));
assert.ok(start>=0);let end=start,depth=0,opened=false;
for(;end<rendererSource.length;++end){const c=rendererSource[end];if(c==='{'){depth++;opened=true;}else if(c==='}'&&--depth===0&&opened){end++;break;}}
const selectState=new Function('clipping','material','camera','_currentCamera','_currentMaterialId','_clippingEnabled','_localClippingEnabled',rendererSource.slice(start,end));
const shader=(await source('src/renderers/shaders/ShaderChunk/clipping_planes_fragment.glsl.js')).default;
assert.ok(shader.includes('dot( vClipPosition, plane.xyz ) > plane.w'));
assert.ok(shader.includes('clipped = ( dot( vClipPosition, plane.xyz ) > plane.w ) && clipped;'));
function expected(seed){
    const properties=WebGLProperties(),clipping=new WebGLClipping(properties),projected=[];
    const original=Plane.prototype.applyMatrix4;
    Plane.prototype.applyMatrix4=function(...args){const result=original.apply(this,args);projected.push([...this.normal.toArray(),this.constant].map(Math.fround));return result;};
    try{
        const camera=new OrthographicCamera(-1,1,1,-1,.1,20);camera.position.z=5;
        const material=new MeshBasicMaterial();let global=[],enable=false,objectZ=0;
        const x=new Plane(new Vector3(1,0,0),0),y=new Plane(new Vector3(0,1,0),0);
        const local=(values,enabled=true)=>{material.clippingPlanes=values;enable=enabled;};
        const frame=()=>{
            camera.updateMatrixWorld();const enabled=clipping.init(global,enable);
            if(enabled===true)clipping.beginShadows();if(enabled===true)clipping.endShadows();
            if(enabled===true)clipping.setGlobalState(global,camera);
            selectState(clipping,material,camera,null,-1,enabled,enable);
            if(material.transparent&&material.side===2)selectState(clipping,material,camera,camera,material.id,enabled,enable);
        };
        switch(seed){
        case 1:global=[x];break;case 2:global=[y];break;case 3:global=[x,y];break;
        case 4:local([x],false);break;case 5:local([x]);break;case 6:local([x,y]);break;
        case 7:local([x,y]);material.clipIntersection=true;break;
        case 8:global=[x];local([new Plane(new Vector3(-1,0,0),0),y]);material.clipIntersection=true;break;
        case 9:global=[x];camera.position.x=.4;break;
        case 10:x.constant=.2;global=[x];camera.scale.set(2,.5,1);break;
        case 11:global=[x];camera.rotation.z=Math.PI/2;break;
        case 12:global=[x];break;case 13:x.constant=-.2;global=[x];break;
        case 14:local([x]);material.transparent=true;material.side=2;break;
        case 15:local([x]);enable='enabled';break;
        case 16:global=[x];local([y]);enable='enabled';break;
        case 17:global=[x];frame();global=[];break;
        case 18:global=[new Plane(new Vector3(0,0,1),-.1)];break;
        case 19:global=[new Plane(new Vector3(0,0,1),.1)];break;
        case 20:global=[new Plane(new Vector3(0,0,1),-1)];objectZ=2;break;
        case 21:x.constant=.3;global=[x];camera.rotation.z=.6;camera.scale.set(1.2,.8,1);break;
        }
        frame();const pixels=[],n=clipping.numPlanes,union=n-clipping.numIntersection,planes=clipping.uniform.value;
        for(const yPixel of [16,32,48])for(const xPixel of [16,32,48]){
            // Analytic sample of the pinned non-alphaToCoverage fragment inequalities.
            // The probe's large plane covers these samples; all cameras are orthographic.
            const vClip=[-(2*(xPixel+.5)/64-1),-(1-2*(yPixel+.5)/64),5-objectZ];
            const outside=i=>vClip[0]*planes[i*4]+vClip[1]*planes[i*4+1]+vClip[2]*planes[i*4+2]>planes[i*4+3];
            let discard=false;for(let i=0;i<union;i++)discard=outside(i)||discard;
            if(union<n){let intersection=true;for(let i=union;i<n;i++)intersection=outside(i)&&intersection;discard=discard||intersection;}
            pixels.push(discard?[0,0,0,255]:[255,255,255,255]);
        }
        const calls=seed===14?2:1;return {seed,projected,pixels,calls,triangles:calls*2};
    }finally{Plane.prototype.applyMatrix4=original;}
}
const run=spawnSync(resolve(process.argv[2]),[],{encoding:'utf8',maxBuffer:10*1024*1024});assert.equal(run.status,0,run.stderr);
const actual=run.stdout.trim().split(/\r?\n/).map(JSON.parse);assert.equal(actual.length,22);
for(let seed=0;seed<22;++seed)assert.deepEqual(actual[seed],JSON.parse(JSON.stringify(expected(seed))),`Vulkan clipping scenario ${seed}`);
console.log('22 actual Vulkan clipping scenarios match pinned projection, renderer selection, and fragment inequalities');

// Execute the pinned disposal functions and new-program listener block directly.
// This isolates listener/WeakMap reentry; it does not claim a WebGL driver run.
function sourceBlock(marker) {
    const begin=rendererSource.indexOf(marker);assert.ok(begin>=0,marker);
    let depth=0,opened=false;
    for(let i=begin;i<rendererSource.length;i++){
        if(rendererSource[i]==='{'){depth++;opened=true;}
        else if(rendererSource[i]==='}'&&opened&&--depth===0)return rendererSource.slice(begin,i+1);
    }
    throw new Error(`Unterminated pinned source block: ${marker}`);
}
const listenerBlock=sourceBlock('if ( programs === undefined ) {');
const disposeFunctions=['function onMaterialDispose(', 'function deallocateMaterial(',
    'function releaseMaterialProgramReferences('].map(sourceBlock).join('\n');
function lifecycle(mode){
    const material=new MeshBasicMaterial(),properties=WebGLProperties();
    const create=new Function('properties','programCache',disposeFunctions+`
        return function renderMaterial(material){
            const materialProperties=properties.get(material);
            let programs=materialProperties.programs;
            ${listenerBlock}
        };
    `);
    const render=create(properties,{releaseProgram(){},releaseShaderCache(){}});
    render(material);
    const originalDelete=WeakMap.prototype.delete;let deletes=0,reenter=true;
    WeakMap.prototype.delete=function(key){
        if(key!==material)return originalDelete.call(this,key);
        deletes++;
        if(mode==='throw')throw new Error('source-before-delete');
        const result=originalDelete.call(this,key);
        if(reenter){reenter=false;render(material);}
        return result;
    };
    try{
        if(mode==='throw'){
            assert.throws(()=>material.dispose(),/source-before-delete/);
            material.dispose();render(material);material.dispose();
            assert.equal(deletes,1,'source preserves removed listener when delete throws before mutation');
        }else{
            material.dispose();material.dispose();
            assert.equal(deletes,2,'source re-registers the listener for properties created by disposal reentry');
        }
    }finally{WeakMap.prototype.delete=originalDelete;}
}
lifecycle('reentry');lifecycle('throw');
console.log('2 pinned renderer material-disposal cases match Vulkan lifetime regression expectations');
