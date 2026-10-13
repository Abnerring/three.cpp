import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
assert.equal(process.argv.length,4,'Usage: node compare-vulkan-clipping-camera-identity.mjs <lifetime executable> <three.js checkout>');
const checkout=resolve(process.argv[3]);
const revision=spawnSync('git',['-C',checkout,'rev-parse','HEAD'],{encoding:'utf8'});
assert.equal(revision.status,0,revision.stderr);assert.equal(revision.stdout.trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const source=name=>import(pathToFileURL(resolve(checkout,name)).href);
const {WebGLClipping}=await source('src/renderers/webgl/WebGLClipping.js');
const {WebGLProperties}=await source('src/renderers/webgl/WebGLProperties.js');
const {MeshBasicMaterial}=await source('src/materials/MeshBasicMaterial.js');
const {OrthographicCamera}=await source('src/cameras/OrthographicCamera.js');
const {Plane}=await source('src/math/Plane.js');
const {Vector3}=await source('src/math/Vector3.js');
const renderer=readFileSync(resolve(checkout,'src/renderers/WebGLRenderer.js'),'utf8');
const start=renderer.indexOf('if ( _clippingEnabled === true ) {',renderer.indexOf('function setProgram('));
assert.ok(start>=0);let end=start,depth=0,opened=false;
for(;end<renderer.length;end++){
    if(renderer[end]==='{'){depth++;opened=true;}
    else if(renderer[end]==='}'&&opened&&--depth===0){end++;break;}
}
const selectState=new Function('clipping','material','camera','_currentCamera','_currentMaterialId','_clippingEnabled','_localClippingEnabled',renderer.slice(start,end));
const clipping=new WebGLClipping(WebGLProperties()),material=new MeshBasicMaterial();
material.clippingPlanes=[new Plane(new Vector3(1,0,0),0)];
const first=new OrthographicCamera(-1,1,1,-1,.1,20),second=new OrthographicCamera(-1,1,1,-1,.1,20);
first.position.z=5;second.position.set(.5,0,5);
const projected=[],original=Plane.prototype.applyMatrix4;
Plane.prototype.applyMatrix4=function(...args){const result=original.apply(this,args);projected.push(this.constant);return result;};
try{
    for(const [camera,previous,id] of [[first,null,-1],[second,first,material.id]]){
        camera.updateMatrixWorld();const enabled=clipping.init([],true);
        if(enabled===true){clipping.beginShadows();clipping.endShadows();clipping.setGlobalState([],camera);}
        // The previous camera/material remain selected after the first draw's exception.
        selectState(clipping,material,camera,previous,id,enabled,true);
    }
}finally{Plane.prototype.applyMatrix4=original;}
assert.notEqual(first.id,second.id);assert.deepEqual(projected,[-0,.5]);
const shader=(await source('src/renderers/shaders/ShaderChunk/clipping_planes_fragment.glsl.js')).default;
assert.ok(shader.includes('dot( vClipPosition, plane.xyz ) > plane.w'));
const planes=clipping.uniform.value;
const vClip=[-(2*(8+.5)/32-1),-(1-2*(16+.5)/32),5];
const discard=vClip[0]*planes[0]+vClip[1]*planes[1]+vClip[2]*planes[2]>planes[3];
const expected={projected,pixel:discard?[0,0,0,255]:[255,255,255,255]};
const run=spawnSync(resolve(process.argv[2]),['reconstructed'],{encoding:'utf8'});
assert.equal(run.status,0,run.stdout+'\n'+run.stderr);
const record=run.stdout.split(/\r?\n/).find(line=>line.startsWith('{'));
assert.ok(record,'Missing GPU camera reconstruction record');assert.deepEqual(JSON.parse(record),expected);
console.log('1 same-address native camera reconstruction matches pinned distinct-Camera projection and analytic fragment sample');
