import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {readFileSync} from 'node:fs';
assert.equal(process.argv.length,3,'Usage: node trace-vulkan-instanced-index-bounds.mjs <three.js checkout>');
const checkout=resolve(process.argv[2]);
const revision=spawnSync('git',['-C',checkout,'rev-parse','HEAD'],{encoding:'utf8'});
assert.equal(revision.status,0,revision.stderr);
assert.equal(revision.stdout.trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const source=relative=>import(pathToFileURL(resolve(checkout,relative)).href);
const {WebGLAttributes}=await source('src/renderers/webgl/WebGLAttributes.js');
const {WebGLBindingStates}=await source('src/renderers/webgl/WebGLBindingStates.js');
const {WebGLGeometries}=await source('src/renderers/webgl/WebGLGeometries.js');
const {WebGLIndexedBufferRenderer}=await source('src/renderers/webgl/WebGLIndexedBufferRenderer.js');
const {WebGLInfo}=await source('src/renderers/webgl/WebGLInfo.js');
const {InstancedBufferGeometry}=await source('src/core/InstancedBufferGeometry.js');
const {InstancedBufferAttribute}=await source('src/core/InstancedBufferAttribute.js');
const {Float32BufferAttribute,Uint16BufferAttribute}=await source('src/core/BufferAttribute.js');
const rendererSource=readFileSync(resolve(checkout,'src/renderers/WebGLRenderer.js'),'utf8');
const instanceBody=rendererSource.match(/const maxInstanceCount = geometry\._maxInstanceCount[^;]+;\s*const instanceCount = Math\.min\([^;]+;\s*renderer\.renderInstances\([^;]+;/)?.[0];
assert.ok(instanceBody,'Pinned renderer instance-count branch not found');
const submitInstances=new Function('geometry','renderer','drawStart','drawCount',instanceBody);
for(const requested of [3,0,9]) {
    let next=0,uploads=0,boundIndex;
    const bound=new Map(),draws=[];
    const gl={MAX_VERTEX_ATTRIBS:34921,ARRAY_BUFFER:34962,ELEMENT_ARRAY_BUFFER:34963,
        FLOAT:5126,INT:5124,UNSIGNED_INT:5125,UNSIGNED_SHORT:5123,TRIANGLES:4,LINES:1,LINE_STRIP:3,LINE_LOOP:2,POINTS:0,
        getParameter(){return 16},createBuffer(){return {id:++next}},createVertexArray(){return {id:++next}},
        bindVertexArray(){},deleteVertexArray(){},deleteBuffer(){},
        bindBuffer(target,buffer){bound.set(target,buffer);if(target===this.ELEMENT_ARRAY_BUFFER)boundIndex=buffer;},
        bufferData(target,array){bound.get(target).bytes=array.byteLength;},
        bufferSubData(){throw Error('Unchanged version must not upload again');},
        enableVertexAttribArray(){},disableVertexAttribArray(){},vertexAttribDivisor(){},vertexAttribPointer(){},vertexAttribIPointer(){},
        drawElementsInstanced(mode,count,type,offset,instances){
            // Command protocol oracle only: a driver must reject this index byte range.
            draws.push({mode,count,type,offset,instances,outOfBounds:offset+count*2>boundIndex.bytes});
        }};
    const attributes=WebGLAttributes(gl),bindings=WebGLBindingStates(gl,attributes),info=WebGLInfo(gl);
    const geometries=WebGLGeometries(gl,attributes,info,bindings);
    const renderer=new WebGLIndexedBufferRenderer(gl,{},info);renderer.setMode(gl.TRIANGLES);
    const geometry=new InstancedBufferGeometry();
    geometry.setAttribute('position',new Float32BufferAttribute([-.8,-.8,0,.8,-.8,0,0,.8,0],3));
    geometry.setAttribute('color',new InstancedBufferAttribute(new Float32Array([0,1,0,0,1,0,0,1,0,0,1,0]),3));
    const index=new Uint16BufferAttribute([0,1,2],1);
    index.onUpload(()=>{uploads++;index.count=6;});geometry.setIndex(index);geometry.instanceCount=3;
    const object={id:1},material={wireframe:false},program={id:1,getAttributes(){return {
        position:{location:0,locationSize:1},color:{location:1,locationSize:1}}}};
    const frame=()=>{
        info.reset();geometries.get(object,geometry);geometries.update(geometry);
        // These range/capacity reads follow WebGLRenderer.renderBufferDirect.
        const start=geometry.drawRange.start,count=Math.min(index.count,geometry.drawRange.count);
        bindings.setup(object,material,program,geometry,index);
        renderer.setIndex(attributes.get(index));
        submitInstances(geometry,renderer,start,count);
        return {calls:info.render.calls,triangles:info.render.triangles};
    };
    assert.deepEqual(frame(),{calls:1,triangles:3});assert.equal(uploads,1);assert.equal(index.count,6);
    assert.equal(geometry._maxInstanceCount,4);assert.equal(draws[0].outOfBounds,false);
    geometry.instanceCount=requested;const actual=frame();
    assert.deepEqual(actual,{calls:requested===0?0:1,triangles:2*Math.min(requested,4)});
    assert.equal(draws.length,requested===0?1:2);
    if(requested!==0)assert.equal(draws[1].outOfBounds,true);
    assert.equal(uploads,1);
    console.log(JSON.stringify({requested,actual,draws}));
    index.count=3;geometry.instanceCount=3;assert.deepEqual(frame(),{calls:1,triangles:3});
    assert.equal(draws.at(-1).outOfBounds,false);assert.equal(uploads,1);
    geometry.dispose();bindings.dispose();geometries.dispose();
}
console.log('3 pinned upstream instanced indexed bounds scenarios passed');
for(const requested of [3,0]) {
    const draws=[];
    const gl={TRIANGLES:4,drawElementsInstanced(...args){draws.push(args);}};
    const info=WebGLInfo(gl),renderer=new WebGLIndexedBufferRenderer(gl,{},info);
    renderer.setMode(4);renderer.setIndex({type:5123,bytesPerElement:2});
    const geometry=new InstancedBufferGeometry();geometry.instanceCount=requested;
    submitInstances(geometry,renderer,0,3);
    assert.equal(geometry._maxInstanceCount,undefined);
    assert.equal(info.render.calls,requested===0?0:1);assert.equal(info.render.triangles,requested);
    assert.equal(draws.length,requested===0?0:1);
    console.log(JSON.stringify({missingMaximum:requested,calls:info.render.calls,triangles:info.render.triangles}));
}
for(const absent of [false,true]) {
    const log=[];
    const maximum={valueOf(){log.push('maximum.number');return 4;}};
    const requested={valueOf(){log.push('requested.number');return 3;}};
    const geometry={get _maxInstanceCount(){log.push('maximum.get');return absent?undefined:maximum;},
                    get instanceCount(){log.push('requested.get');return requested;}};
    let actual;
    submitInstances(geometry,{renderInstances(start,count,instances){actual={start,count,instances};}},0,6);
    assert.deepEqual(actual,{start:0,count:6,instances:3});
    assert.deepEqual(log,absent?['maximum.get','requested.get','requested.number']:
        ['maximum.get','maximum.get','requested.get','requested.number','maximum.number']);
    console.log(JSON.stringify({absent,log}));
}
console.log('2 missing-capacity and 2 getter/coercion source scenarios passed');
