// Differential check for the native examples/jsm/utils/BufferGeometryUtils.js port.
//
// The scenario list below is fed to the probe on stdin and replayed here against the pinned
// module, so both sides always build the same attributes and geometries from the same spec.
// `result` and the captured console.error/console.warn lines are compared field by field:
// the message text of every failure path is part of the check.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-buffer-geometry-utils.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [,,sourceDir,probePath]=process.argv;
const url=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
// Both the module and the classes it uses come from the pinned checkout, so the spec builder
// below produces the same identities the module sees in a browser.
const THREE=await import(url('build/three.module.js'));
const BufferGeometryUtils=await import(url('examples/jsm/utils/BufferGeometryUtils.js'));
const {BufferAttribute,Float16BufferAttribute,InstancedBufferAttribute,InterleavedBuffer,
    InstancedInterleavedBuffer,InterleavedBufferAttribute,BufferGeometry,Mesh,MeshBasicMaterial}=THREE;

// ---------------------------------------------------------------------------
// Encoding. Non-finite numbers and -0 survive as strings, exactly like the probe.
// ---------------------------------------------------------------------------
const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const decode=value=>value==='NaN'?NaN:value==='-0'?-0:value==='Infinity'?Infinity:
    value==='-Infinity'?-Infinity:value;
const encodeValue=value=>value===undefined?'undefined':value===null?'null':
    typeof value==='boolean'?value:typeof value==='string'?value:
    typeof value==='number'?encode(value):String(value);

const arrayTypes=['Int8Array','Uint8Array','Int16Array','Uint16Array','Int32Array','Uint32Array',
    'Float32Array','Float64Array','Uint8ClampedArray'];
const makeArray=(type,values)=>{
    const constructor={Int8Array,Uint8Array,Int16Array,Uint16Array,Int32Array,Uint32Array,
        Float32Array,Float64Array,Uint8ClampedArray}[type];
    assert.ok(constructor,`unknown array type ${type}`);
    return new constructor(values.map(decode));
};

// Shared data identity of every attribute of one scenario, mirroring the probe.
const dataKeys=[],objectKeys=[];
function dataId(attribute){
    const key=attribute.isInterleavedBufferAttribute?attribute.data:attribute;
    const list=attribute.isInterleavedBufferAttribute?dataKeys:objectKeys;
    let index=list.indexOf(key);
    if(index<0){list.push(key);index=list.length-1;}
    return index;
}
function encodeAttribute(attribute){
    if(!attribute)return null;
    const result={kind:attribute.constructor.name,type:attribute.array.constructor.name,
        values:Array.from(attribute.array).map(encode),itemSize:encode(attribute.itemSize),
        count:encode(attribute.count),normalized:attribute.normalized,name:attribute.name,
        usage:encodeValue(attribute.usage),gpuType:encodeValue(attribute.gpuType),dataId:dataId(attribute)};
    if(attribute.isInterleavedBufferAttribute){
        result.offset=encode(attribute.offset);
        result.stride=encode(attribute.data.stride);
        result.dataCount=encode(attribute.data.count);
        result.dataUsage=encodeValue(attribute.data.usage);
        result.dataKind=attribute.data.isInstancedInterleavedBuffer?'InstancedInterleavedBuffer':'InterleavedBuffer';
    }
    if(attribute.isInstancedBufferAttribute)result.meshPerAttribute=encodeValue(attribute.meshPerAttribute);
    return result;
}
function encodeGeometry(geometry){
    if(!geometry)return null;
    const attributes={};
    for(const name of Object.keys(geometry.attributes))attributes[name]=encodeAttribute(geometry.attributes[name]);
    const morphAttributes={};
    for(const name of Object.keys(geometry.morphAttributes))
        morphAttributes[name]=geometry.morphAttributes[name].map(encodeAttribute);
    return {attributes,morphAttributes,morphTargetsRelative:geometry.morphTargetsRelative,
        index:geometry.index?encodeAttribute(geometry.index):null,
        groups:geometry.groups.map(group=>({start:encode(group.start),count:encode(group.count),
            materialIndex:encode(group.materialIndex)})),
        drawRange:{start:encode(geometry.drawRange.start),count:encode(geometry.drawRange.count)}};
}
// A failed interleave is the upstream `null`; the native port returns an empty attribute list.
const encodeAttributes=attributes=>attributes===null||attributes.length===0?null:attributes.map(encodeAttribute);

// ---------------------------------------------------------------------------
// Spec builders, mirroring the probe one to one.
// ---------------------------------------------------------------------------
function buildAttribute(spec){
    const itemSize=spec.itemSize===undefined?1:spec.itemSize;
    const normalized=spec.normalized===undefined?false:spec.normalized;
    if(spec.kind==='interleaved'){
        const data=spec.data;
        const buffer=spec.data.instanced
            ? new InstancedInterleavedBuffer(makeArray(data.type,data.values),data.stride,data.meshPerAttribute)
            : new InterleavedBuffer(makeArray(data.type,data.values),data.stride);
        if(data.usage!==undefined)buffer.setUsage(data.usage);
        const attribute=new InterleavedBufferAttribute(buffer,itemSize,spec.offset,normalized);
        if(spec.name!==undefined)attribute.name=spec.name;
        return attribute;
    }
    const array=makeArray(spec.type,spec.values);
    let attribute;
    if(spec.kind==='instanced')attribute=new InstancedBufferAttribute(array,itemSize,normalized,spec.meshPerAttribute);
    else if(spec.kind==='float16')attribute=new Float16BufferAttribute(array,itemSize,normalized);
    else attribute=new BufferAttribute(array,itemSize,normalized);
    if(spec.name!==undefined)attribute.name=spec.name;
    if(spec.usage!==undefined)attribute.setUsage(spec.usage);
    if(spec.gpuType!==undefined)attribute.gpuType=spec.gpuType;
    return attribute;
}
// Scenario numbers may carry the same string encoding as the results, so both sides decode
// them before building the objects.
const numeric=(value,fallback)=>value===undefined?fallback:(typeof value==='string'?decode(value):value);
function buildGeometry(spec){
    const geometry=new BufferGeometry();
    if(spec.attributes)for(const name of Object.keys(spec.attributes))
        geometry.setAttribute(name,buildAttribute(spec.attributes[name]));
    if(spec.index!==undefined&&spec.index!==null)geometry.setIndex(buildAttribute(spec.index));
    if(spec.morphAttributes)for(const name of Object.keys(spec.morphAttributes))
        geometry.morphAttributes[name]=spec.morphAttributes[name].map(buildAttribute);
    geometry.morphTargetsRelative=spec.morphTargetsRelative===true;
    if(spec.groups)for(const group of spec.groups)
        geometry.addGroup(numeric(group.start,0),numeric(group.count,0),numeric(group.materialIndex,0));
    if(spec.drawRange)geometry.setDrawRange(numeric(spec.drawRange.start,0),numeric(spec.drawRange.count,Infinity));
    return geometry;
}

// ---------------------------------------------------------------------------
// Scenario dispatcher: the same kinds and the same work as the probe's run().
// ---------------------------------------------------------------------------
function runScenario(spec){
    if(spec.kind==='mergeGeometries')
        return encodeGeometry(BufferGeometryUtils.mergeGeometries(
            spec.geometries.map(buildGeometry),spec.useGroups===true));
    if(spec.kind==='mergeAttributes')
        return encodeAttribute(BufferGeometryUtils.mergeAttributes(spec.attributes.map(buildAttribute)));
    if(spec.kind==='deepCloneAttribute')
        return encodeAttribute(BufferGeometryUtils.deepCloneAttribute(buildAttribute(spec.attribute)));
    if(spec.kind==='interleaveAttributes')
        return encodeAttributes(BufferGeometryUtils.interleaveAttributes(spec.attributes.map(buildAttribute)));
    if(spec.kind==='deinterleaveAttribute')
        return encodeAttribute(BufferGeometryUtils.deinterleaveAttribute(buildAttribute(spec.attribute)));
    if(spec.kind==='deinterleaveGeometry'){
        const geometry=buildGeometry(spec.geometry);
        BufferGeometryUtils.deinterleaveGeometry(geometry);
        return encodeGeometry(geometry);
    }
    if(spec.kind==='estimateBytesUsed')
        return encode(BufferGeometryUtils.estimateBytesUsed(buildGeometry(spec.geometry)));
    if(spec.kind==='mergeVertices')
        return encodeGeometry(BufferGeometryUtils.mergeVertices(buildGeometry(spec.geometry),
            spec.tolerance===undefined?1e-4:spec.tolerance));
    if(spec.kind==='toTrianglesDrawMode'){
        const geometry=buildGeometry(spec.geometry);
        const result=BufferGeometryUtils.toTrianglesDrawMode(geometry,spec.drawMode);
        return {sameGeometry:result===geometry,geometry:encodeGeometry(result)};
    }
    if(spec.kind==='mergeGroups'){
        const geometry=buildGeometry(spec.geometry);
        const result=BufferGeometryUtils.mergeGroups(geometry);
        return {sameGeometry:result===geometry,geometry:encodeGeometry(result)};
    }
    if(spec.kind==='toCreasedNormals'){
        const geometry=buildGeometry(spec.geometry);
        const result=BufferGeometryUtils.toCreasedNormals(geometry,
            spec.creaseAngle===undefined?Math.PI/3:spec.creaseAngle);
        return {sameGeometry:result===geometry,geometry:encodeGeometry(result)};
    }
    if(spec.kind==='computeMorphedAttributes'){
        const geometry=buildGeometry(spec.geometry);
        const mesh=new Mesh(geometry,spec.materialArray===true
            ?[new MeshBasicMaterial(),new MeshBasicMaterial()]:new MeshBasicMaterial());
        if(spec.influences!==undefined)mesh.morphTargetInfluences=spec.influences.map(decode);
        const result=BufferGeometryUtils.computeMorphedAttributes(mesh);
        return {positionAttribute:encodeAttribute(result.positionAttribute),
            normalAttribute:encodeAttribute(result.normalAttribute),
            morphedPositionAttribute:encodeAttribute(result.morphedPositionAttribute),
            morphedNormalAttribute:encodeAttribute(result.morphedNormalAttribute)};
    }
    if(spec.kind==='remapAttribute'){
        const attribute=buildAttribute(spec.attribute);
        const indices=makeArray('Float64Array',spec.sourceIndices);
        return encodeAttribute(BufferGeometryUtils.remapAttribute(attribute,Array.from(indices),new Map()));
    }
    if(spec.kind==='remapAttributes'){
        const buffers=new Map();
        return spec.attributes.map((entry,index)=>encodeAttribute(BufferGeometryUtils.remapAttribute(
            buildAttribute(entry),Array.from(makeArray('Float64Array',spec.sourceIndices[index])),buffers)));
    }
    if(spec.kind==='computeMikkTSpaceTangents')
        return encodeGeometry(BufferGeometryUtils.computeMikkTSpaceTangents(buildGeometry(spec.geometry)));
    throw new Error(`unknown scenario kind: ${spec.kind}`);
}

// ---------------------------------------------------------------------------
// Scenarios.
// ---------------------------------------------------------------------------
const scenarios=[];
let capturing=null;
const originalError=console.error,originalWarn=console.warn;
console.error=(...args)=>{if(capturing)capturing.push(args.map(String).join(' '));else originalError(...args);};
console.warn=(...args)=>{if(capturing)capturing.push(args.map(String).join(' '));else originalWarn(...args);};
const errorResult=error=>({error:{name:error.constructor.name,
    ...(error.constructor.name==='TypeError'?{message:error.message}:{})}});
function scenario(spec){
    // Fresh identity tables per scenario, like the probe's AttributeEncoder.
    dataKeys.length=0;objectKeys.length=0;
    // The probe only ever sees the serialized spec, so this side replays the decoded line too.
    const line=JSON.stringify(spec);
    const decoded=JSON.parse(line);
    const messages=[];capturing=messages;
    let result;
    try{result=runScenario(decoded);}catch(error){result=errorResult(error);}
    capturing=null;
    scenarios.push({line,expected:{result,messages}});
}

const buffer=(type,itemSize,values,extra={})=>({kind:'buffer',type,itemSize,values,...extra});
const interleaved=(dataType,stride,dataValues,itemSize,offset,extra={},dataExtra={})=>
    ({kind:'interleaved',itemSize,offset,...extra,data:{type:dataType,stride,values:dataValues,...dataExtra}});
const triangle=(positions,extra={})=>({...extra,attributes:{...extra.attributes,
    position:buffer('Float32Array',3,positions)}});

// mergeGeometries: indexed and non-indexed, with and without groups.
const indexedA={attributes:{position:buffer('Float32Array',3,[0,0,0,1,0,0,0,1,0]),
    normal:buffer('Float32Array',3,[0,0,1,0,0,1,0,0,1])},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2]}};
const indexedB={attributes:{position:buffer('Float32Array',3,[2,0,0,3,0,0,2,1,0]),
    normal:buffer('Float32Array',3,[0,1,0,0,1,0,0,1,0])},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2]}};
const plainA={attributes:{position:buffer('Float32Array',3,[0,0,0,1,0,0,0,1,0])}};
const plainB={attributes:{position:buffer('Float32Array',3,[2,0,0,3,0,0,2,1,0])}};
scenario({kind:'mergeGeometries',geometries:[indexedA,indexedB]});
scenario({kind:'mergeGeometries',geometries:[indexedA,indexedB],useGroups:true});
scenario({kind:'mergeGeometries',geometries:[plainA,plainB]});
scenario({kind:'mergeGeometries',geometries:[plainA,plainB],useGroups:true});
scenario({kind:'mergeGeometries',geometries:[indexedA,indexedB,indexedA],useGroups:true});
// The merged index picks Uint32 storage once a value reaches the primitive restart value.
scenario({kind:'mergeGeometries',geometries:[
    {attributes:{position:buffer('Float32Array',3,[0,0,0,1,0,0,0,1,0])},
        index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2]}},
    {attributes:{position:buffer('Float32Array',3,[2,0,0,3,0,0,2,1,0])},
        index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[65534,1,2]}}]});
// Mismatched index, mismatched attribute names, mismatched attribute counts.
scenario({kind:'mergeGeometries',geometries:[indexedA,plainB]});
scenario({kind:'mergeGeometries',geometries:[indexedA,{attributes:{position:buffer('Float32Array',3,[2,0,0,3,0,0,2,1,0]),
    uv:buffer('Float32Array',2,[0,0,1,0,0,1])},index:indexedB.index}]});
scenario({kind:'mergeGeometries',geometries:[indexedA,{attributes:{position:indexedB.attributes.position},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2]}}]});
// Group counting needs an index or a position attribute.
scenario({kind:'mergeGeometries',geometries:[plainA,{attributes:{}}],useGroups:true});
// An indexed merge reads the position count of every geometry.
scenario({kind:'mergeGeometries',geometries:[indexedA,{attributes:{only:buffer('Float32Array',3,[1,2,3])},
    index:indexedB.index}]});
// Morph attributes: consistent, inconsistent names, target count and morphTargetsRelative.
const morphedA={attributes:{position:buffer('Float32Array',3,[0,0,0,1,0,0,0,1,0])},
    morphAttributes:{position:[buffer('Float32Array',3,[0,0,1,0,0,2,0,0,3])]}};
const morphedB={attributes:{position:buffer('Float32Array',3,[2,0,0,3,0,0,2,1,0])},
    morphAttributes:{position:[buffer('Float32Array',3,[0,0,4,0,0,5,0,0,6])]}};
scenario({kind:'mergeGeometries',geometries:[morphedA,morphedB]});
scenario({kind:'mergeGeometries',geometries:[morphedA,morphedB],useGroups:true});
scenario({kind:'mergeGeometries',geometries:[{...morphedA,morphTargetsRelative:true},{...morphedB,morphTargetsRelative:true}]});
scenario({kind:'mergeGeometries',geometries:[{...morphedA,morphTargetsRelative:true},morphedB]});
scenario({kind:'mergeGeometries',geometries:[morphedA,{...morphedB,morphAttributes:{normal:[
    buffer('Float32Array',3,[0,0,1,0,0,1,0,0,1])]}}]});
scenario({kind:'mergeGeometries',geometries:[morphedA,{...morphedB,morphAttributes:{position:[
    buffer('Float32Array',3,[0,0,4,0,0,5,0,0,6]),buffer('Float32Array',3,[1,1,1,1,1,1,1,1,1])]}}]});
// mergeAttributes failures surface through the attribute merge messages.
scenario({kind:'mergeGeometries',geometries:[
    {attributes:{position:buffer('Float32Array',3,[0,0,0,1,0,0,0,1,0])}},
    {attributes:{position:buffer('Float64Array',3,[0,0,0,1,0,0,0,1,0])}}]});
scenario({kind:'mergeGeometries',geometries:[
    {attributes:{position:buffer('Float32Array',3,[0,0,0,1,0,0,0,1,0])}},
    {attributes:{position:buffer('Float32Array',2,[0,0,1,0])}}]});
scenario({kind:'mergeGeometries',geometries:[
    {attributes:{position:buffer('Float32Array',3,[0,0,0,1,0,0,0,1,0])}},
    {attributes:{position:buffer('Float32Array',3,[0,0,0,1,0,0,0,1,0],{normalized:true})}}]});
scenario({kind:'mergeGeometries',geometries:[
    {attributes:{position:buffer('Float32Array',3,[0,0,0,1,0,0,0,1,0],{gpuType:1016})}},
    {attributes:{position:buffer('Float32Array',3,[0,0,0,1,0,0,0,1,0])}}]});
// Interleaved attributes merge component by component.
const interleavedA={attributes:{position:interleaved('Float32Array',3,[0,0,0,1,0,0,0,1,0],3,0),
    normal:interleaved('Float32Array',3,[0,0,1,0,0,1,0,0,1],3,0)}};
scenario({kind:'mergeGeometries',geometries:[interleavedA,{attributes:{
    position:interleaved('Float32Array',3,[2,0,0,3,0,0,2,1,0],3,0),
    normal:interleaved('Float32Array',3,[0,1,0,0,1,0,0,1,0],3,0)}}]});
// Three attributes of mixed array types, names and usage.
scenario({kind:'mergeGeometries',geometries:[
    {attributes:{position:buffer('Float32Array',3,[0,0,0,'-0',1,2],{name:'position',usage:35048}),
        normal:buffer('Int8Array',3,[0,0,127,0,0,-128],{normalized:true,name:'normal'}),
        uv:buffer('Uint16Array',2,[0,65535,1,2])}},
    {attributes:{position:buffer('Float32Array',3,[3,4,5,6,7,8],{name:'position',usage:35048}),
        normal:buffer('Int8Array',3,[1,2,3,4,5,6],{normalized:true,name:'normal'}),
        uv:buffer('Uint16Array',2,[3,4,5,6])}}],useGroups:true});
scenario({kind:'mergeGeometries',geometries:[]});

// mergeAttributes.
scenario({kind:'mergeAttributes',attributes:[buffer('Float32Array',3,[1,2,3,4,5,6]),
    buffer('Float32Array',3,[7,8,9])]});
scenario({kind:'mergeAttributes',attributes:[buffer('Int16Array',2,[1,2,3,4],{normalized:true,gpuType:1016})]});
scenario({kind:'mergeAttributes',attributes:[buffer('Float32Array',3,[1,2,3]),buffer('Float64Array',3,[4,5,6])]});
scenario({kind:'mergeAttributes',attributes:[buffer('Float32Array',3,[1,2,3]),buffer('Float32Array',2,[4,5])]});
scenario({kind:'mergeAttributes',attributes:[buffer('Float32Array',3,[1,2,3]),buffer('Float32Array',3,[4,5,6],{normalized:true})]});
scenario({kind:'mergeAttributes',attributes:[buffer('Float32Array',3,[1,2,3],{gpuType:1015}),buffer('Float32Array',3,[4,5,6],{gpuType:1016})]});
scenario({kind:'mergeAttributes',attributes:[interleaved('Float32Array',4,[1,2,3,4,5,6,7,8],3,0),
    interleaved('Float32Array',4,[9,10,11,12,13,14,15,16],1,3)]});
scenario({kind:'mergeAttributes',attributes:[]});
// A non-divisible array length keeps the fractional count of the source.
scenario({kind:'mergeAttributes',attributes:[buffer('Float32Array',3,[1,2,3,4,5,6,7]),
    buffer('Float32Array',3,[8,9,10,11,12])]});

// deepCloneAttribute.
scenario({kind:'deepCloneAttribute',attribute:buffer('Float32Array',3,[1,2,3,4,5,6],{name:'position'})});
scenario({kind:'deepCloneAttribute',attribute:buffer('Uint8ClampedArray',4,[1,2,3,4],{normalized:true})});
scenario({kind:'deepCloneAttribute',attribute:buffer('Int32Array',1,[7,-8],{usage:35048,gpuType:1016})});
scenario({kind:'deepCloneAttribute',attribute:{kind:'instanced',type:'Float32Array',itemSize:2,
    values:[1,2,3,4],meshPerAttribute:3,name:'instance'}});
scenario({kind:'deepCloneAttribute',attribute:{kind:'float16',type:'Uint16Array',itemSize:2,
    values:[15360,16384,0,32768]}});
scenario({kind:'deepCloneAttribute',attribute:interleaved('Float32Array',5,[1,2,3,4,5,6,7,8,9,10],3,1,
    {name:'position',normalized:true})});

// interleaveAttributes.
scenario({kind:'interleaveAttributes',attributes:[buffer('Float32Array',3,[1,2,3,4,5,6]),
    buffer('Float32Array',2,[7,8,9,10])]});
scenario({kind:'interleaveAttributes',attributes:[{kind:'float16',type:'Uint16Array',itemSize:2,
    values:[15360,16384,0,32768]},buffer('Uint16Array',1,[5,6],{normalized:true})]});
scenario({kind:'interleaveAttributes',attributes:[buffer('Float32Array',3,[1,2,3]),buffer('Int16Array',1,[4])]});
scenario({kind:'interleaveAttributes',attributes:[]});
scenario({kind:'interleaveAttributes',attributes:[buffer('Float32Array',4,[1,2,3,4,5,6,7,8])]});

// deinterleaveAttribute / deinterleaveGeometry.
scenario({kind:'deinterleaveAttribute',attribute:interleaved('Float32Array',5,[1,2,3,4,5,6,7,8,9,10],3,1,
    {normalized:false})});
scenario({kind:'deinterleaveAttribute',attribute:interleaved('Int16Array',4,[1,2,3,4,5,6,7,8],2,2,
    {normalized:true,name:'uv'})});
scenario({kind:'deinterleaveAttribute',attribute:interleaved('Uint8Array',6,[1,2,3,4,5,6,7,8,9,10,11,12],4,0)});
scenario({kind:'deinterleaveGeometry',geometry:{attributes:{
    position:interleaved('Float32Array',6,[0,0,0,1,1,1,1,0,0,0,0,1,0,1,0,1,1,0],3,0,{name:'position'}),
    normal:interleaved('Float32Array',6,[0,0,0,1,1,1,1,0,0,0,0,1,0,1,0,1,1,0],3,3,{name:'normal'}),
    uv:buffer('Float32Array',2,[0,0,1,0,0,1])},
    morphAttributes:{position:[interleaved('Float32Array',6,[1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6],3,0)],
        normal:[buffer('Float32Array',3,[0,0,1,0,0,1,0,0,1])]}}});

// estimateBytesUsed.
scenario({kind:'estimateBytesUsed',geometry:{attributes:{
    position:buffer('Float32Array',3,[0,0,0,1,0,0,0,1,0]),normal:buffer('Int8Array',3,[0,0,127,0,0,127,0,0,127])},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2]}}});
scenario({kind:'estimateBytesUsed',geometry:{attributes:{
    position:interleaved('Float32Array',6,[0,0,0,1,1,1,1,0,0,0,0,1,0,1,0,1,1,0],3,0),
    normal:interleaved('Float32Array',6,[0,0,0,1,1,1,1,0,0,0,0,1,0,1,0,1,1,0],3,3)}}});
scenario({kind:'estimateBytesUsed',geometry:{attributes:{}}});

// mergeVertices.
const weldSource={attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 0,1,0, 0,0,0, 1,0,0, 0,1,0])}};
scenario({kind:'mergeVertices',geometry:weldSource});
scenario({kind:'mergeVertices',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 0,1,0, 0,0,0, 1,0,0, 0,1,0]),uv:buffer('Float32Array',2,[0,0,1,0,0,1,0,0,1,0,0,1])},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2,3,4,5]}}});
scenario({kind:'mergeVertices',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 0.00004,0,0, 0,1,0])}},tolerance:1e-5});
// The tolerance floor is Math.max( tolerance, Number.EPSILON ) upstream, so a negative zero has
// to behave like the epsilon and a NaN tolerance has to propagate: with NaN the hash comparison
// makes every vertex look equal and the whole geometry welds into one vertex.
scenario({kind:'mergeVertices',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 0.00004,0,0, 0,1,0])}},tolerance:-0});
scenario({kind:'mergeVertices',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 0.00004,0,0, 0,1,0])}},tolerance:NaN});
scenario({kind:'mergeVertices',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 0,1,0, 0,0,0, 1,0,0, 0,1,0]),normal:buffer('Int8Array',3,
    [0,0,127,0,0,127,0,0,127,0,0,127,0,0,127,0,0,127],{normalized:true,name:'normal',usage:35048})},
    morphAttributes:{position:[buffer('Float32Array',3,[1,1,1,2,2,2,3,3,3,1,1,1,2,2,2,3,3,3])]}}});
scenario({kind:'mergeVertices',geometry:{attributes:{
    position:interleaved('Float32Array',6,[0,0,0,1,1,1,1,0,0,0,0,1,0,1,0,1,1,0, 0,0,0,1,1,1],
        3,0,{name:'position'}),
    normal:interleaved('Float32Array',6,[0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1, 0,0,1,0,0,1],3,3)},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2,3,4,5]}}});
scenario({kind:'mergeVertices',geometry:{attributes:{}}});
scenario({kind:'mergeVertices',geometry:{attributes:{normal:buffer('Float32Array',3,[0,0,1,0,0,1,0,0,1])}}});
scenario({kind:'mergeVertices',geometry:{attributes:{position:buffer('Int8Array',3,[0,0,0,1,0,0,0,1,0,0,0,0],{normalized:true})}}});

// toTrianglesDrawMode.
const fanGeometry={attributes:{position:buffer('Float32Array',3,[0,0,0, 1,0,0, 1,1,0, 0,1,0, -1,1,0])},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2,3,4]},
    groups:[{start:0,count:3,materialIndex:0},{start:3,count:3,materialIndex:1}]};
scenario({kind:'toTrianglesDrawMode',geometry:fanGeometry,drawMode:2});
scenario({kind:'toTrianglesDrawMode',geometry:{...fanGeometry,index:{kind:'buffer',type:'Uint16Array',
    itemSize:1,values:[0,1,2,3,4,5]}},drawMode:1});
scenario({kind:'toTrianglesDrawMode',geometry:fanGeometry,drawMode:0});
scenario({kind:'toTrianglesDrawMode',geometry:fanGeometry,drawMode:5});
scenario({kind:'toTrianglesDrawMode',geometry:{attributes:{}},drawMode:2});
scenario({kind:'toTrianglesDrawMode',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 1,1,0, 0,1,0, -1,1,0])}},drawMode:2});
scenario({kind:'toTrianglesDrawMode',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 1,1,0, 0,1,0, -1,1,0])}},drawMode:1});

// mergeGroups.
scenario({kind:'mergeGroups',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0,1,0,0,0,1,0, 2,0,0,3,0,0,2,1,0, 4,0,0,5,0,0,4,1,0])},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2,3,4,5,6,7,8]},
    groups:[{start:6,count:3,materialIndex:2},{start:0,count:3,materialIndex:0},{start:3,count:3,materialIndex:0}]}});
scenario({kind:'mergeGroups',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0,1,0,0,0,1,0, 2,0,0,3,0,0,2,1,0])},
    groups:[{start:0,count:3,materialIndex:1},{start:3,count:3,materialIndex:1}]}});
scenario({kind:'mergeGroups',geometry:{attributes:{position:buffer('Float32Array',3,[0,0,0,1,0,0,0,1,0])},
    groups:[]}});
scenario({kind:'mergeGroups',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0,1,0,0,0,1,0, 2,0,0,3,0,0,2,1,0])},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2,3,4,5]},
    groups:[{start:0,count:3,materialIndex:3},{start:3,count:3,materialIndex:1}]}});
// Equal sort keys keep the upstream order.
scenario({kind:'mergeGroups',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0,1,0,0,0,1,0, 2,0,0,3,0,0,2,1,0])},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2,3,4,5]},
    groups:[{start:0,count:3,materialIndex:0},{start:0,count:3,materialIndex:0}]}});
// A NaN material index never compares equal, so those groups are never merged.
scenario({kind:'mergeGroups',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0,1,0,0,0,1,0, 2,0,0,3,0,0,2,1,0])},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2,3,4,5]},
    groups:[{start:0,count:3,materialIndex:'NaN'},{start:3,count:3,materialIndex:0}]}});
scenario({kind:'mergeGroups',geometry:{attributes:{},groups:[{start:0,count:3,materialIndex:0}]}});

// toCreasedNormals: a creased quad, indexed and non-indexed, plus the flatten path.
const creasedQuad={attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 1,1,0, 0,0,0, 1,1,0, 0,1,0])}};
scenario({kind:'toCreasedNormals',geometry:creasedQuad});
scenario({kind:'toCreasedNormals',geometry:creasedQuad,creaseAngle:2.5});
scenario({kind:'toCreasedNormals',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 1,1,0, 0,0,0, 1,1,0, 0,1,0]),
    normal:buffer('Float32Array',3,[0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1])},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2,3,4,5]}}});
scenario({kind:'toCreasedNormals',geometry:{attributes:{position:buffer('Float32Array',2,
    [0,0, 1,0, 1,1, 0,0, 1,1, 0,1])}}});
scenario({kind:'toCreasedNormals',geometry:{attributes:{
    position:buffer('Float32Array',3,[0,0,0, 1,0,0, 1,1,0, 0,0,0, 1,1,0, 0,1,0],{normalized:true})}}});
scenario({kind:'toCreasedNormals',geometry:{attributes:{position:interleaved('Float32Array',3,
    [0,0,0, 1,0,0, 1,1,0, 0,0,0, 1,1,0, 0,1,0],3,0)}}});
scenario({kind:'toCreasedNormals',geometry:{attributes:{position:buffer('Float32Array',3,
    ['NaN',0,0, 1,0,0, 1,1,0, 0,0,0, 1,1,0, 0,1,0])}}});
scenario({kind:'toCreasedNormals',geometry:{attributes:{position:buffer('Float64Array',3,
    [0,0,0, 1,0,0, 1,1,0, 0,0,0, 1,1,0, 0,1,0])}}});

// computeMorphedAttributes.
const morphGeometry={attributes:{position:buffer('Float32Array',3,[0,0,0, 1,0,0, 0,1,0]),
    normal:buffer('Float32Array',3,[0,0,1, 0,0,1, 0,0,1])}};
scenario({kind:'computeMorphedAttributes',geometry:morphGeometry});
scenario({kind:'computeMorphedAttributes',geometry:{...morphGeometry,
    morphAttributes:{position:[buffer('Float32Array',3,[0,0,1, 0,0,1, 0,0,1]),
        buffer('Float32Array',3,[0,0,2, 0,0,2, 0,0,2])],
        normal:[buffer('Float32Array',3,[1,0,0, 1,0,0, 1,0,0])]}},
    influences:[0.5,0.25]});
scenario({kind:'computeMorphedAttributes',geometry:{...morphGeometry,morphTargetsRelative:true,
    morphAttributes:{position:[buffer('Float32Array',3,[0,0,1, 0,0,1, 0,0,1]),
        buffer('Float32Array',3,[0,0,2, 0,0,2, 0,0,2])]}},influences:[1,0.5]});
scenario({kind:'computeMorphedAttributes',geometry:{...morphGeometry,
    morphAttributes:{position:[buffer('Float32Array',3,[0,0,1, 0,0,1, 0,0,1])]}},influences:[0]});
// A geometry without a normal attribute cannot be morphed.
scenario({kind:'computeMorphedAttributes',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 0,1,0])}}});
scenario({kind:'computeMorphedAttributes',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 0,1,0]),normal:buffer('Float32Array',3,[0,0,1,0,0,1,0,0,1])},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2]}},materialArray:true,
    groups:[{start:0,count:3,materialIndex:0}]});
scenario({kind:'computeMorphedAttributes',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 0,1,0, 1,1,0]),normal:buffer('Float32Array',3,[0,0,1,0,0,1,0,0,1,0,0,1])},
    drawRange:{start:0,count:3}}});

// remapAttribute: plain, interleaved, shared buffer, out of range source index.
scenario({kind:'remapAttribute',attribute:buffer('Float32Array',3,[1,2,3,4,5,6,7,8,9],
    {name:'position',usage:35048,gpuType:1016}),sourceIndices:[2,0]});
scenario({kind:'remapAttribute',attribute:{kind:'instanced',type:'Float32Array',itemSize:2,
    values:[1,2,3,4,5,6],meshPerAttribute:4,name:'instance'},sourceIndices:[1,0,2]});
scenario({kind:'remapAttribute',attribute:{kind:'float16',type:'Uint16Array',itemSize:2,
    values:[15360,16384,0,32768],normalized:true},sourceIndices:[1,0]});
scenario({kind:'remapAttribute',attribute:interleaved('Float32Array',5,[1,2,3,4,5,6,7,8,9,10],3,1,
    {name:'position'}),sourceIndices:[0,1]});
scenario({kind:'remapAttribute',attribute:interleaved('Float32Array',5,[1,2,3,4,5,6,7,8,9,10],1,3,
    {name:'uv'}),sourceIndices:[3,1]});
scenario({kind:'remapAttribute',attribute:buffer('Float32Array',3,[1,2,3]),sourceIndices:[5]});
scenario({kind:'remapAttribute',attribute:interleaved('Float32Array',3,[1,2,3,4,5,6],3,0,
    {},{instanced:true,meshPerAttribute:5}),sourceIndices:[1,0]});
// One pass over two attributes that view the same interleaved buffer keeps one copied buffer.
const sharedInterleaved=interleaved('Float32Array',5,[1,2,3,4,5,6,7,8,9,10],3,1,{name:'position'});
const sharedNormal=interleaved('Float32Array',5,[1,2,3,4,5,6,7,8,9,10],3,3,{name:'normal'});
scenario({kind:'remapAttributes',attributes:[sharedInterleaved,sharedNormal],
    sourceIndices:[[0,1],[1,0]]});
scenario({kind:'remapAttributes',attributes:[buffer('Float32Array',3,[1,2,3,4,5,6]),buffer('Float32Array',3,[7,8,9])],
    sourceIndices:[[1,0],[2]]});
// The MikkTSpace entry point reports the missing library on both sides.
scenario({kind:'computeMikkTSpaceTangents',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0,1,0,0,0,1,0]),normal:buffer('Float32Array',3,[0,0,1,0,0,1,0,0,1]),
    uv:buffer('Float32Array',2,[0,0,1,0,0,1])}}});

// ---------------------------------------------------------------------------
// Boundary scenarios: out of range reads, fractional counts and NaN input.
// ---------------------------------------------------------------------------
// A vertex count that is not a multiple of three reads past the position buffer.
scenario({kind:'toCreasedNormals',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 1,1,0, 0,0,0, 1,1,0, 0,1,0, 2,2,0])}}});
// Position data whose length is not a multiple of the item size keeps the fractional count.
scenario({kind:'mergeVertices',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 2,2,2, 3,3])}}});
scenario({kind:'mergeVertices',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 0,1,0, 'NaN','Infinity','-Infinity'])}}});
scenario({kind:'mergeVertices',geometry:{attributes:{position:buffer('Float64Array',3,
    [0,0,0, 1e-9,0,0, 0,1,0, 1e-9,0,0])},tolerance:1e-8}});
// Group ranges past the end of the index read undefined entries.
scenario({kind:'mergeGroups',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0,1,0,0,0,1,0])},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2]},
    groups:[{start:0,count:5,materialIndex:0}]}});
// Out of range, negative and fractional source indices.
scenario({kind:'remapAttribute',attribute:buffer('Float32Array',3,[1,2,3,4,5,6]),sourceIndices:[3,-1,0.5,1]});
scenario({kind:'remapAttribute',attribute:interleaved('Float32Array',3,[1,2,3,4,5,6],3,0),
    sourceIndices:[7,1]});
// Interleaved input to interleaveAttributes reads the whole backing array.
scenario({kind:'interleaveAttributes',attributes:[interleaved('Float32Array',4,[1,2,3,4,5,6,7,8],3,1),
    buffer('Float32Array',2,[9,10,11,12])]});
// A stride that does not divide the backing array leaves a fractional count.
scenario({kind:'deinterleaveAttribute',attribute:interleaved('Float32Array',6,[1,2,3,4,5,6,7],4,0)});
// Fewer influences than morph targets contribute undefined, which is NaN in the addition.
scenario({kind:'computeMorphedAttributes',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 0,1,0]),normal:buffer('Float32Array',3,[0,0,1, 0,0,1, 0,0,1])},
    morphAttributes:{position:[buffer('Float32Array',3,[0,0,1, 0,0,1, 0,0,1]),
        buffer('Float32Array',3,[0,0,2, 0,0,2, 0,0,2])]}},influences:[0.5]});
// Indexed morphing with a material array and a partial draw range.
scenario({kind:'computeMorphedAttributes',geometry:{attributes:{position:buffer('Float32Array',3,
    [0,0,0, 1,0,0, 0,1,0, 1,1,0]),normal:buffer('Float32Array',3,[0,0,1,0,0,1,0,0,1,0,0,1]),
    morphAttributes:undefined},
    index:{kind:'buffer',type:'Uint16Array',itemSize:1,values:[0,1,2,1,3,2]},
    groups:[{start:0,count:6,materialIndex:0}],drawRange:{start:1,count:4}},
    materialArray:true,influences:[]});
// Float16 and instanced attributes in the byte estimate.
scenario({kind:'estimateBytesUsed',geometry:{attributes:{
    position:{kind:'float16',type:'Uint16Array',itemSize:3,values:[15360,0,0,16384,0,0,0,15360,0]},
    color:{kind:'instanced',type:'Uint8Array',itemSize:4,values:[1,2,3,4],meshPerAttribute:2}},
    index:{kind:'buffer',type:'Uint32Array',itemSize:1,values:[0,1,2]}}});

// ---------------------------------------------------------------------------
// Run the probe and compare every scenario.
// ---------------------------------------------------------------------------
const input=scenarios.map(entry=>entry.line).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:512*1024*1024})
    .trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
let compared=0;
for(let index=0;index<scenarios.length;++index){
    const {line,expected}=scenarios[index];
    assert.deepEqual(JSON.parse(output[index]),expected,`scenario ${index}: ${line.slice(0,160)}`);
    compared+=1;
}
const kinds=new Set(scenarios.map(entry=>JSON.parse(entry.line).kind));
console.log(`BufferGeometryUtils parity: ${compared} scenarios across ${kinds.size} operations `
    +`(${[...kinds].sort().join(', ')}) match the pinned checkout; ${process.version}`);
