import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if (process.argv.length !== 4) throw Error('Usage: node compare-lights.mjs <three.js> <probe.exe>');
const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding:'utf8'}).trim(), lock.commit);
const T = await import(pathToFileURL(path.join(upstream, 'src/Three.Core.js')));
const {default: IESSpotLight} = await import(pathToFileURL(path.join(upstream,'src/lights/webgpu/IESSpotLight.js')));
const {default: ProjectorLight} = await import(pathToFileURL(path.join(upstream,'src/lights/webgpu/ProjectorLight.js')));
const {SpotLightShadow} = await import(pathToFileURL(path.join(upstream,'src/lights/SpotLightShadow.js')));
const types = [T.Light,T.AmbientLight,T.DirectionalLight,T.HemisphereLight,T.PointLight,T.SpotLight,T.RectAreaLight,T.LightProbe,IESSpotLight,ProjectorLight];
function create(type,intensity=1) { return type===3 ? new T.HemisphereLight(undefined,undefined,intensity) : new types[type](undefined,intensity); }
function sanitize(value) { if(Array.isArray(value)) return value.map(sanitize); if(value && typeof value === 'object') return Object.fromEntries(Object.entries(value).filter(([key])=>key!=='uuid').map(([key,child])=>[key,key==='target'?'<target>':sanitize(child)])); return value; }
function shadowState(shadow) { return {json:sanitize(shadow.toJSON()),matrix:shadow.matrix.elements,planes:shadow._frustum.planes.map(p=>[...p.normal.toArray(),p.constant]),viewports:shadow._viewports.map(v=>v.toArray()),
    frameExtents:shadow._frameExtents.toArray(),count:shadow.getViewportCount(),cameraSame:shadow.getCamera(37)===shadow.camera,frustumSame:shadow.getFrustum()===shadow._frustum,viewportSame:shadow.getViewport(0)===shadow._viewports[0],extentsSame:shadow.getFrameExtents()===shadow._frameExtents,
    mapType:shadow.mapType,autoUpdate:shadow.autoUpdate,needsUpdate:shadow.needsUpdate,mapNull:shadow.map===null,mapPassNull:shadow.mapPass===null,position:shadow.camera.position.toArray(),world:shadow.camera.matrixWorld.elements,projection:shadow.camera.projectionMatrix.elements}; }
function state(light) { const result={json:sanitize(light.toJSON()),position:light.position.toArray(),matrix:light.matrix.elements,color:light.color.toArray(),isLight:light.isLight}; if(light.shadow) result.shadow=shadowState(light.shadow); if('power' in light) result.power=light.power; if(light instanceof IESSpotLight) result.iesMapNull=light.iesMap===null; if(light instanceof ProjectorLight) result.aspect=light.aspect; return result; }
function copyCase(type,recursive) {
    const source=create(type,2.75); source.color.setHex(0x339977);source.position.set(3,-2,7);source.name='sample';source.add(new T.Object3D());
    if(source.isHemisphereLight) source.groundColor.setHex(0xaaff55);
    if(source.isPointLight){source.distance=7;source.decay=-3;}
    if(source.isSpotLight){source.distance=23;source.angle=.3;source.decay=0;source.penumbra=.4;source.map=new T.Texture();source.map.uuid='map-id';}
    if(source.isRectAreaLight){source.width=-3;source.height=0;}
    if(source.isLightProbe)source.sh.coefficients[3].set(2,5,-1);
    if(source instanceof IESSpotLight)source.iesMap=new T.Texture();
    if(source instanceof ProjectorLight)source.aspect=1.7;
    if(source.target){source.target.name='target';source.target.position.set(-1,4,9);source.target.add(new T.Object3D());}
    const shadow=source.shadow;
    if(shadow){shadow.intensity=.7;shadow.bias=-.1;shadow.normalBias=.2;shadow.radius=4;shadow.blurSamples=13;shadow.autoUpdate=false;shadow.needsUpdate=true;shadow.mapSize.set(256,1024);shadow.mapType=T.FloatType;
        shadow.biasNode={};shadow.map=new T.RenderTarget();shadow.mapPass=new T.RenderTarget();shadow.matrix.makeTranslation(9,8,7);shadow._frameExtents.set(4,2);shadow.camera.position.set(5,6,7);if(shadow.isSpotLightShadow){shadow.focus=.7;shadow.aspect=1.8;}}
    const clone=source.clone(recursive),result={state:state(clone),children:clone.children.length,uuidDifferent:source.uuid!==clone.uuid};
    if(shadow){result.shadowDifferent=clone.shadow!==shadow;result.cameraDifferent=clone.shadow.camera!==shadow.camera;result.biasNodeSame=clone.shadow.biasNode===shadow.biasNode;}
    if(source.target){result.targetDifferent=source.target!==clone.target;result.targetChildren=clone.target.children.length;result.targetPosition=clone.target.position.toArray();}
    if(source.isSpotLight)result.mapSame=clone.map===source.map;
    if(source.isLightProbe){result.shDifferent=clone.sh!==source.sh;result.shEqual=clone.sh.equals(source.sh);}
    if(source instanceof IESSpotLight)result.iesMapSame=clone.iesMap===source.iesMap;
    return result;
}
function matrixCase(type,coordinate,reversed,scenario){
    const light=create(type),shadow=light.shadow,target=light.target,parent=new T.Object3D(),targetParent=new T.Object3D();parent.add(light);targetParent.add(target);
    light.position.set(3-scenario,4+scenario*.5,-2);target.position.set(-1,2,3+scenario);
    if(scenario&1){parent.position.set(9,-4,3);parent.rotation.set(.2,-.3,.5);parent.scale.set(2,1.5,.7);targetParent.position.set(-2,1,-5);targetParent.rotateY(.3);}
    if(scenario===2){light.position.set(0,0,0);target.position.set(0,0,0);}
    parent.updateMatrixWorld();targetParent.updateMatrixWorld();shadow.camera.coordinateSystem=coordinate;shadow.camera._reversedDepth=reversed;shadow.camera.updateProjectionMatrix();
    if(light.isSpotLight){light.angle=.3+scenario*.1;light.distance=scenario===3?-2:scenario===4?NaN:scenario*8;shadow.focus=.8;shadow.aspect=1.3;shadow.mapSize.set(1024,256);}
    shadow.updateMatrices(light);const result=structuredClone(shadowState(shadow));shadow._frameExtents.set(4,2);const matrix=new T.Matrix4();shadow._updateMatrix(shadow.camera,matrix,new T.Frustum(),new T.Vector4(2,1,1,1));result.atlas=matrix.elements;return result;
}
function disposeCase(type,scenario){const light=create(type),shadow=light.shadow,first=new T.RenderTarget(),second=new T.RenderTarget(),replacement=new T.RenderTarget(),trace=[];shadow.map=first;shadow.mapPass=scenario===1?first:second;
    first.addEventListener('dispose',()=>{trace.push('map');if(scenario===2)shadow.mapPass=replacement;if(scenario===5)throw Error('stop map');});second.addEventListener('dispose',()=>trace.push('mapPass'));replacement.addEventListener('dispose',()=>trace.push('replacement'));
    light.addEventListener('dispose',()=>{trace.push('light');if(scenario===3)throw Error('stop');if(scenario===4){const next=new SpotLightShadow();next.map=replacement;light.shadow=next;}});
    try{light.dispose();if(scenario===0)light.dispose();}catch{trace.push('error');}
    return {trace,mapRetained:shadow.map===first,mapPassRetained:shadow.mapPass===(scenario===1?first:scenario===2?replacement:second)};
}
function harmonics(index){const a=new T.SphericalHarmonics3(),b=new T.SphericalHarmonics3(),input=[123,456];for(let i=0;i<27;i++)input.push(((index+3)*(i+7)%97-43)*.125);
    a.fromArray(input,2);b.copy(a).scale(-.75);const normal=new T.Vector3(index%5-2,index*3%7-3,.1+index%11).normalize(),radiance=new T.Vector3(),irradiance=new T.Vector3();
    a.getAt(normal,radiance);a.getIrradianceAt(normal,irradiance);const basis=Array(12).fill(-9);T.SphericalHarmonics3.getBasisAt(normal,basis);const clone=a.clone();a.add(b).addScaledSH(b,.3).lerp(clone,.6);const output=[4,5];a.toArray(output,2);
    const result={radiance:radiance.toArray(),irradiance:irradiance.toArray(),basis,values:output,equalClone:clone.equals(clone),equalOther:clone.equals(b)};
    const alias=clone.clone();alias.getAt(normal,alias.coefficients[index%9]);result.coefficientAlias=alias.toArray();alias.copy(clone).getIrradianceAt(normal,alias.coefficients[(index+2)%9]);result.irradianceAlias=alias.toArray();
    alias.copy(clone).add(alias).addScaledSH(alias,.2).lerp(alias,1.7);result.self=alias.toArray();normal.set(1,2,3);clone.getAt(normal,normal);result.normalAlias=normal.toArray();clone.zero();result.zero=clone.toArray();return result;
}
const expected={};
for(let type=0;type<10;type++){expected[`default:${type}`]=state(create(type));for(const recursive of [false,true])expected[`copy:${type}:${+recursive}`]=copyCase(type,recursive);}
for(const type of [2,5])for(const coordinate of [T.WebGLCoordinateSystem,T.WebGPUCoordinateSystem])for(const reversed of [false,true])for(let scenario=0;scenario<5;scenario++)expected[`matrix:${type}:${coordinate}:${+reversed}:${scenario}`]=matrixCase(type,coordinate,reversed,scenario);
for(const type of [2,4,5])for(let scenario=0;scenario<(type===5?6:4);scenario++)expected[`dispose:${type}:${scenario}`]=disposeCase(type,scenario);
for(let i=0;i<128;i++)expected[`sh:${i}`]=harmonics(i);
for(const type of [4,5,6])for(const value of [-50,-0,.25,100,1e30]){const light=create(type);if(light.isRectAreaLight){light.width=-3;light.height=7;}light.power=value;expected[`power:${type}:${value}`]={intensity:light.intensity,state:state(light)};}
const sh=new T.SphericalHarmonics3(),probe=new T.LightProbe(sh);sh.coefficients[0].x=9;expected.shIdentity={same:probe.sh===sh,value:probe.sh.coefficients[0].x};
const point=new T.PointLight();point.position.set(2,3,4);point.updateMatrixWorld();let pointError=false;try{point.shadow.updateMatrices(point)}catch(error){pointError=error instanceof TypeError;}expected.pointUpdate={error:pointError,position:point.shadow.camera.position.toArray()};
const inheritedNode={},ownNode={},ies=new IESSpotLight();T.Light.registerNode(inheritedNode);expected.nodeBase=ies._lightNode===inheritedNode;T.SpotLight.registerNode(ownNode);expected.nodeInherited=ies._lightNode===ownNode;IESSpotLight.registerNode(null);expected.nodeOwnNull=ies._lightNode===null;
const parent=new T.Object3D(),first=new T.SpotLight(),second=new T.SpotLight();first.map=second.map=new T.Texture();first.map.uuid='shared-map';first.map.source.uuid='shared-image';parent.add(first,second);expected.nestedJSON=sanitize(parent.toJSON());
const meta={geometries:{},materials:{},textures:{},images:{},shapes:{},skeletons:{},animations:{},nodes:{}};first.toJSON(meta);second.toJSON(meta);expected.sharedMetaCount=Object.keys(meta.textures).length;
const plain=new T.LightShadow(new T.Camera()),cloned=plain.clone();expected.plainShadow={state:shadowState(cloned),cameraDifferent:cloned.camera!==plain.camera};
const actual=JSON.parse(execFileSync(path.resolve(process.argv[3]),[],{encoding:'utf8',maxBuffer:16*1024*1024}));
let comparisons=0;
function compare(a,b,location='root'){comparisons++;if(typeof a==='number'&&typeof b==='number'){assert.ok(Math.abs(a-b)<=2e-11*Math.max(1,Math.abs(a),Math.abs(b)),`${location}: ${a} vs ${b}`);return;}
    if(a&&b&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),`${location}: keys`);for(const key of Object.keys(b))compare(a[key],b[key],`${location}.${key}`);return;}assert.deepEqual(a,b,location);}
compare(actual,JSON.parse(JSON.stringify(expected)));
console.log(`Lights parity: ${Object.keys(expected).length} scenarios, ${comparisons} comparisons; defaults, copies, resources, shadows, disposal and SH match ${lock.commit}`);
