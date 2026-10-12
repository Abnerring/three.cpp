import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
// Record actual upstream range ordering; the matching Vulkan test renders pixels.
if(process.argv.length!==3)throw new Error('Usage: node verify-webgl-attributes.mjs <three.js>');
const upstream=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {WebGLAttributes}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLAttributes.js')));
const {Float32BufferAttribute}=await import(pathToFileURL(path.join(upstream,'src/core/BufferAttribute.js')));
const calls=[];
const gl={FLOAT:5126,createBuffer(){return{};},bindBuffer(){},bufferData(){},
    bufferSubData(_target,destination,_data,source,count){calls.push([destination,source,count]);}};
const attributes=new WebGLAttributes(gl),attribute=new Float32BufferAttribute(new Float32Array(12),3);
attributes.update(attribute,1);
const handles=[2,NaN,1,7,NaN,0,9,4,6,3,NaN,8,5,10].map(start=>({start,count:1}));
attribute.updateRanges.push(...handles);
let callbackCounts,cleared;
attribute.onUpload(()=>{cleared=attribute.updateRanges.length===0;callbackCounts=handles.map(range=>range.count);});
attribute.needsUpdate=true;attributes.update(attribute,1);
assert.deepEqual(callbackCounts,[1,1,7,1,1,1,1,1,1,1,1,3,1,1]);
assert.equal(cleared,true);
assert.deepEqual(calls,[[8,2,1],[NaN,NaN,1],[4,1,7],[NaN,NaN,1],[0,0,1],[36,9,1],[NaN,NaN,1],[32,8,3]]);
console.log(`Pinned WebGLAttributes: NaN sort/merge identity and 8 upload calls passed; ${lock.commit}`);

// Execute the real cache with a byte-oriented native command recorder. Replacing
// an array must keep its first GPU format while uploading the new array's bytes.
function commandRecorder() {
    let bound;
    const uploads=[];
    return {
        FLOAT:5126, UNSIGNED_SHORT:5123, ARRAY_BUFFER:34962,
        uploads,
        createBuffer(){return {bytes:new Uint8Array()};},
        bindBuffer(_target,buffer){bound=buffer;},
        bufferData(_target,array){bound.bytes=new Uint8Array(array.buffer,array.byteOffset,array.byteLength).slice();},
        bufferSubData(_target,destination,array,source=0,count=0){
            const elements=count===0?array.length-source:count;
            bound.bytes.set(new Uint8Array(array.buffer,array.byteOffset+source*array.BYTES_PER_ELEMENT,
                elements*array.BYTES_PER_ELEMENT),destination);
            uploads.push([destination,source,count]);
        }
    };
}
const typedGL=commandRecorder(),typedCache=new WebGLAttributes(typedGL);
const colors=(r,g,b)=>new Float32Array([r,g,b,r,g,b,r,g,b,r,g,b]);
const byteView=array=>new Uint8Array(array.buffer,array.byteOffset,array.byteLength);
const changing=new Float32BufferAttribute(colors(1,0,0),3);
let changingUploads=0;
changing.onUpload(()=>changingUploads++);
typedCache.update(changing,typedGL.ARRAY_BUFFER);
const first=typedCache.get(changing),gpuValues=()=>[...new Float32Array(first.buffer.bytes.buffer)];
changing.array=byteView(colors(0,1,0));
typedCache.update(changing,typedGL.ARRAY_BUFFER);
assert.deepEqual(gpuValues(),[...colors(1,0,0)]);assert.equal(changingUploads,1);
changing.needsUpdate=true;typedCache.update(changing,typedGL.ARRAY_BUFFER);
assert.deepEqual(gpuValues(),[...colors(0,1,0)]);
assert.equal(typedCache.get(changing),first);assert.equal(first.type,5126);assert.equal(first.bytesPerElement,4);
changing.array=byteView(colors(0,0,1));
for(let row=0;row<4;row++)changing.addUpdateRange(row*12+4,8);
changing.needsUpdate=true;typedCache.update(changing,typedGL.ARRAY_BUFFER);
assert.deepEqual(gpuValues(),[...colors(0,0,1)]);assert.equal(changing.updateRanges.length,0);
const red=colors(1,0,0);changing.array=new Uint16Array(red.buffer);
for(let row=0;row<4;row++){changing.addUpdateRange(row*6+1,1);changing.addUpdateRange(row*6+5,1);}
typedGL.uploads.length=0;
changing.needsUpdate=true;typedCache.update(changing,typedGL.ARRAY_BUFFER);
assert.deepEqual(gpuValues(),[...red]);
assert.deepEqual(typedGL.uploads,[[2,1,1],[10,5,3],[22,11,3],[34,17,3],[46,23,1]]);
assert.equal(changingUploads,4);assert.equal(changing.updateRanges.length,0);
changing.array=byteView(colors(0,1,0));changing.addUpdateRange(0,0);
changing.needsUpdate=true;typedCache.update(changing,typedGL.ARRAY_BUFFER);
assert.deepEqual(gpuValues(),[...colors(0,1,0)]);assert.equal(changingUploads,5);
assert.equal(first.type,5126);assert.equal(first.bytesPerElement,4);

const {BufferAttribute}=await import(pathToFileURL(path.join(upstream,'src/core/BufferAttribute.js')));
const index=new BufferAttribute(new Uint16Array([0,2,1,2,3,1]),1);
typedCache.update(index,1);const initialIndex=typedCache.get(index),indices=index.array;
index.array=byteView(indices);index.needsUpdate=true;typedCache.update(index,1);
assert.deepEqual([...new Uint16Array(initialIndex.buffer.bytes.buffer)],[...indices]);
index.array=new Float32Array(3);index.needsUpdate=true;typedCache.update(index,1);
assert.deepEqual([...new Uint16Array(initialIndex.buffer.bytes.buffer)],[0,0,0,0,0,0]);
index.array=indices;index.needsUpdate=true;typedCache.update(index,1);
assert.deepEqual([...new Uint16Array(initialIndex.buffer.bytes.buffer)],[...indices]);
assert.equal(initialIndex.type,5123);assert.equal(initialIndex.bytesPerElement,2);

const {WebGLGeometries}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLGeometries.js')));
const geometryGL=commandRecorder(),geometryCache=new WebGLAttributes(geometryGL);
const geometries=new WebGLGeometries(geometryGL,geometryCache,{},{});
const uploaded=[],track=name=>new Float32BufferAttribute([1],1).onUpload(()=>uploaded.push(name));
const originalColor=track('old'),replacement=track('new'),removed=track('removed'),added=track('added');
const position=track('position'),geometry={attributes:{position,color:originalColor,removed}};
position.onUpload(()=>{
    uploaded.push('position');geometry.attributes.color=replacement;
    delete geometry.attributes.removed;geometry.attributes.added=added;
});
geometries.update(geometry);assert.deepEqual(uploaded,['position','new']);
geometries.update(geometry);assert.deepEqual(uploaded,['position','new','added']);
position.onUpload(()=>{geometry.attributes.color=null;});position.needsUpdate=true;
assert.throws(()=>geometries.update(geometry),TypeError);
geometry.attributes.color=replacement;geometries.update(geometry);
assert.deepEqual(uploaded,['position','new','added']);
console.log('Pinned WebGLAttributes/WebGLGeometries: retained formats, changed-type byte ranges, index bytes and callback enumeration passed');
