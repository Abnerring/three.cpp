// Differential check for the native src/loaders/ObjectLoader.js port.
//
// Both sides parse the same documents and the resulting trees are compared as structural summaries: type,
// name, uuid, transform, flags, layers and children. The documents cover every branch the port implements,
// including the two that resolve later - a light target and a level of detail level - and the shared
// epilogue fields.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-object-loader.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {ObjectLoader}=await import(pathToFileURL(path.join(sourceDir,'src/loaders/ObjectLoader.js')).href);
const geometry={type:'BufferGeometry',uuid:'g',data:{attributes:{}}};
const material={type:'MeshBasicMaterial',uuid:'m',color:255};
const documents=[
    {object:{type:'Object3D',uuid:'root'}},
    {object:{type:'Scene',uuid:'scene',background:255,children:[
        {type:'Group',uuid:'group',name:'grouped',position:[1,2,3],scale:[2,2,2],visible:false,renderOrder:3,layers:5,
         children:[{type:'Mesh',uuid:'mesh',material:'m',geometry:'g'}]}
    ]},geometries:[geometry],materials:[material]},
    {object:{type:'Scene',uuid:'scene',children:[
        {type:'Mesh',uuid:'matrixMesh',material:'m',geometry:'g',matrix:[1,0,0,0, 0,1,0,0, 0,0,1,0, 4,5,6,1]},
        {type:'Mesh',uuid:'plainMesh',material:'m',geometry:'g',quaternion:[0,0,0,1]}
    ]},geometries:[geometry],materials:[material]},
    {object:{type:'Scene',uuid:'scene',children:[
        {type:'Line',uuid:'line',material:'m',geometry:'g'},
        {type:'LineLoop',uuid:'loop',material:'m',geometry:'g'},
        {type:'LineSegments',uuid:'segments',material:'m',geometry:'g'},
        {type:'Points',uuid:'points',material:'m',geometry:'g'},
        {type:'Sprite',uuid:'sprite',material:'m'}
    ]},geometries:[geometry],materials:[material]},
    {object:{type:'Scene',uuid:'scene',children:[
        {type:'AmbientLight',uuid:'ambient',color:255,intensity:2},
        {type:'DirectionalLight',uuid:'sun',color:16777215,intensity:1,target:'aim'},
        {type:'PointLight',uuid:'point',color:65280,intensity:3,distance:10,decay:1},
        {type:'SpotLight',uuid:'spot',color:255,intensity:1,angle:0.5,penumbra:0.2},
        {type:'HemisphereLight',uuid:'hemi',color:255,groundColor:65280,intensity:4},
        {type:'Object3D',uuid:'aim'}
    ]}},
    {object:{type:'Scene',uuid:'scene',children:[
        {type:'PerspectiveCamera',uuid:'pcam',fov:60,aspect:2,near:0.5,far:500,zoom:2},
        {type:'OrthographicCamera',uuid:'ocam',left:-2,right:2,top:3,bottom:-3,zoom:4}
    ]}},
    {object:{type:'LOD',uuid:'lod',autoUpdate:false,
        levels:[{object:'near',distance:10},{object:'far',distance:20}],
        children:[
            {type:'Mesh',uuid:'near',material:'m',geometry:'g'},
            {type:'Mesh',uuid:'far',material:'m',geometry:'g'}
        ]},geometries:[geometry],materials:[material]},
    {object:{type:'Scene',uuid:'scene',children:[
        {type:'Object3D',uuid:'frozen',matrix:[1,0,0,0, 0,1,0,0, 0,0,1,0, 7,8,9,1],matrixAutoUpdate:false},
        {type:'Object3D',uuid:'auto',matrix:[1,0,0,0, 0,1,0,0, 0,0,1,0, 1,2,3,1]}
    ]}},    {object:{type:'Scene',uuid:'scene',children:[
        {type:'RectAreaLight',uuid:'area',color:255,intensity:2,width:4,height:6},
        {type:'LightProbe',uuid:'probe',intensity:3,
         sh:[1,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0]}
    ]}},
    {object:{type:'BatchedMesh',uuid:'batched',geometry:'g',material:'m',
        maxInstanceCount:4,maxVertexCount:8,maxIndexCount:16,
        perObjectFrustumCulled:false,sortObjects:false,
        geometryInitialized:true,geometryCount:1,
        nextVertexStart:3,nextIndexStart:3,
        matricesTexture:{uuid:'mt'},indirectTexture:{uuid:'it'},
        geometryInfo:[{vertexStart:0,vertexCount:3,reservedVertexCount:3,indexStart:0,indexCount:3,
                       reservedIndexCount:3,start:0,count:3,active:true}],
        instanceInfo:[{geometryIndex:0,active:true,visible:false}],
        availableInstanceIds:[],availableGeometryIds:[]},
     geometries:[geometry],materials:[material],
     images:[{uuid:'img1',url:{data:[0,0,0,255],width:1,height:1,type:'Uint8Array'}},
             {uuid:'img2',url:{data:[0,0,0,255],width:1,height:1,type:'Uint8Array'}}],
     textures:[{uuid:'mt',image:'img1'},{uuid:'it',image:'img2'}]},    {object:{type:'Mesh',uuid:'multi',geometry:'g',material:['m','m2']},
     geometries:[geometry],
     materials:[material,{type:'MeshBasicMaterial',uuid:'m2',color:65280}]},    {object:{type:'Scene',uuid:'scene',backgroundBlurriness:0.5,backgroundIntensity:2,
        fog:{type:'Fog',color:16777215,near:1,far:100}}}
];
const encode=value=>{
    if(Object.is(value,-0))return '-0';
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const triple=v=>({x:encode(v.x),y:encode(v.y),z:encode(v.z)});
const materialSummary=(object)=>{
    const material=object.material;
    if(!material)return {present:false};
    if(Array.isArray(material))return {present:true,array:true,count:material.length,types:material.map(entry=>entry?entry.type:'')};
    return {present:true,array:false,type:material.type};
};const summary=(object,depth=0)=>{
    if(depth>6)return {cut:true};
    return {
        type:object.type,name:object.name??'',uuid:object.uuid??'',
        position:triple(object.position),quaternion:triple(object.quaternion),scale:triple(object.scale),
        visible:object.visible,renderOrder:encode(object.renderOrder),layers:encode(object.layers.mask),
        material:materialSummary(object),
        children:(object.children??[]).map(child=>summary(child,depth+1))
    };
};
const raw=execFileSync(path.resolve(probePath),[],{input:documents.map(d=>JSON.stringify(d)).join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,documents.length,'one probe line per document');
for(let index=0;index<documents.length;++index){
    const object=new ObjectLoader().parse(JSON.parse(JSON.stringify(documents[index])));
    const expected=summary(object);
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH document',index);
        console.log('  probe :',JSON.stringify(actual).slice(0,400));
        console.log('  oracle:',JSON.stringify(expected).slice(0,400));
    }
    assert.deepEqual(actual,expected,`document ${index}`);
}
console.log(`ObjectLoader parity: ${documents.length} documents`);