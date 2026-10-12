import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync,spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: node compare-core-camera.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {Clock,Timer,InstancedBufferGeometry,BufferGeometry,Float32BufferAttribute,ArrayCamera,StereoCamera,Camera,PerspectiveCamera,OrthographicCamera,FrustumArray,Mesh,Sprite,Sphere,Box3,Vector3,Object3D,Plane,WebGLCoordinateSystem,WebGPUCoordinateSystem}=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const {ClippingGroup}=await import(pathToFileURL(path.join(upstream,'src/objects/ClippingGroup.js')));
const {setConsoleFunction}=await import(pathToFileURL(path.join(upstream,'src/utils.js')));
const value=v=>v===undefined?'undefined':typeof v==='number'?(Number.isNaN(v)?'NaN':!Number.isFinite(v)?v<0?'-Infinity':'Infinity':Object.is(v,-0)?'-0':v):v;
const matrix=m=>m.elements.map(value);
let now=0,trace=[];
globalThis.performance={now(){trace.push(['now',value(now)]);return now;}};
setConsoleFunction((level,message)=>trace.push([level,message]));
const clockState=(c,result)=>[value(result),value(c.autoStart),value(c.startTime),value(c.oldTime),value(c.elapsedTime),value(c.running)];
function clockCase(mode){
    now=100;trace=[];const starts=[true,false,0,1,'','false',null,undefined],c=new Clock(starts[mode%starts.length]),frames=[];
    frames.push(clockState(c,c.getDelta()));now=116;frames.push(clockState(c,c.getElapsedTime()));
    if(mode>=8)c.elapsedTime='elapsed';now=216;frames.push(clockState(c,c.getDelta()));now=250;c.stop();frames.push(clockState(c,undefined));
    now=1000;frames.push(clockState(c,c.getElapsedTime()));c.start();frames.push(clockState(c,undefined));
    now=900;frames.push(clockState(c,c.getDelta()));now=1001;frames.push(clockState(c,c.getElapsedTime()));
    return {kind:'clock',mode,frames,trace};
}
class Visibility {
    state=false;listeners=[];
    constructor(name){this.name=name;}
    get hidden(){trace.push(['hidden',this.name,value(this.state)]);return this.state;}
    addEventListener(type,listener,capture){assert.equal(type,'visibilitychange');trace.push(['add',this.name,capture]);this.listeners.push(listener);}
    removeEventListener(type,listener){assert.equal(type,'visibilitychange');trace.push(['remove',this.name,this.listeners.includes(listener)]);this.listeners=this.listeners.filter(v=>v!==listener);}
    dispatch(){for(const listener of [...this.listeners])listener();}
}
const timerState=t=>[value(t._previousTime),value(t._currentTime),value(t._startTime),value(t._delta),value(t._elapsed),value(t.getTimescale()),value(t.getDelta()),value(t.getElapsed()),!!t._pageVisibilityHandler,!!t._document];
function timerCase(mode){
    now=100;trace=[];const t=new Timer(),frames=[],scales=[1,0,-1,2,.5,NaN,Infinity,'2',null,false,undefined,'bad'];
    t.setTimescale(scales[mode%scales.length]);frames.push(timerState(t));
    now=116;t.update();frames.push(timerState(t));t.update(150);frames.push(timerState(t));
    now=500;t.reset();frames.push(timerState(t));t.update('550');frames.push(timerState(t));
    t.update(null);frames.push(timerState(t));if(mode>=12)t._elapsed='total';t.update(100);frames.push(timerState(t));t.dispose();frames.push(timerState(t));
    return {kind:'timer',mode,frames,trace};
}
function visibilityCase(mode){
    now=100;trace=[];const t=new Timer(),a=new Visibility('a'),b=new Visibility('b'),frames=[],values=[false,true,undefined,'true',0,null];
    a.state=values[mode%values.length];t.connect(a);now=200;t.update();frames.push(timerState(t));
    a.state=true;now=1000;a.dispatch();t.update(1100);frames.push(timerState(t));
    a.state=false;now=1200;a.dispatch();now=1216;t.update();frames.push(timerState(t));
    if(mode>=6){if(mode>=12)b.state=undefined;t.connect(b);now=2000;a.dispatch();frames.push(timerState(t));}
    t.disconnect();frames.push(timerState(t));now=2100;t.update();frames.push(timerState(t));t.dispose();
    return {kind:'visibility',mode,frames,trace,listeners:[a.listeners.length,b.listeners.length]};
}
function runtimeTimerCase(mode){
    now=100;trace=[];const t=new Timer(),coercions=[],object={valueOf(){coercions.push('coerce');
        if(mode===0){t._startTime=999;return 200;}if(mode===1){t._currentTime=999;return 25;}
        if(mode===2){t._delta=99;t._elapsed='x';return 2;}if(mode===3){t._delta=777;return 10;}
        if(mode===4){t._timescale=9;return 100;}throw new TypeError('coercion');
    }};
    if(mode===1)t._currentTime=object;if(mode===2)t._timescale=object;if(mode===3)t._elapsed=object;if(mode===4)t._startTime=object;
    let error='';try{t.update(mode===0||mode===5?object:200);}catch(e){error=e.name;}
    const state=timerState(t);return {kind:'runtimeTimer',mode,state,trace,coercions,error};
}
const stereoState=s=>({left:matrix(s.cameraL.projectionMatrix),right:matrix(s.cameraR.projectionMatrix),matrixL:matrix(s.cameraL.matrix),matrixR:matrix(s.cameraR.matrix),worldL:matrix(s.cameraL.matrixWorld),inverseL:matrix(s.cameraL.projectionMatrixInverse),dirty:[s.cameraL.matrixWorldNeedsUpdate,s.cameraR.matrixWorldNeedsUpdate],cache:[s._cache.focus,s._cache.fov,s._cache.aspect,s._cache.near,s._cache.far,s._cache.zoom,s._cache.eyeSep].map(value)});
function stereoCase(mode){
    const c=new PerspectiveCamera(35+mode,.5+(mode%5)*.4,.01+(mode%3)*.1,10+mode*100);
    c.focus=mode===16?0:2+mode;c.zoom=mode===17?0:mode===18?-1:1+(mode%3)*.5;
    c.coordinateSystem=mode%2?WebGPUCoordinateSystem:WebGLCoordinateSystem;c._reversedDepth=mode%4===3;
    c.filmOffset=(mode%3)-1;if(mode%3===1)c.setViewOffset(1200,800,200,100,600,400);c.updateProjectionMatrix();
    c.position.set(mode-5,mode%3,2);c.rotation.set(.1,.2,.3);c.scale.set(2,1,3);c.updateMatrixWorld();
    const s=new StereoCamera(),frames=[];s.aspect=mode%2?.5:1;s.eyeSep=mode===19?-.5:.064;
    s.update(c);frames.push(stereoState(s));c.projectionMatrix.elements[5]=123;c.matrixWorld.elements[12]+=10;
    s.update(c);frames.push(stereoState(s));s.eyeSep=.2;s.update(c);frames.push(stereoState(s));
    return {kind:'stereo',mode,frames};
}
function stereoShared(){const c=new PerspectiveCamera(),a=new StereoCamera(),b=new StereoCamera();a.eyeSep=.2;b.eyeSep=.8;a.update(c);b.update(c);a.update(c);return {kind:'stereoShared',state:stereoState(a)};}
const frustumState=f=>({count:f._count,coordinateSystem:f.coordinateSystem,planes:f._frustums.map(frustum=>frustum.planes.map(p=>[p.normal.x,p.normal.y,p.normal.z,p.constant].map(value)))});
function frustumCase(mode){
    const a=new PerspectiveCamera(60,1,.1,50),b=new OrthographicCamera(-3,3,3,-3,.2,30);
    a.coordinateSystem=b.coordinateSystem=mode%2?WebGPUCoordinateSystem:WebGLCoordinateSystem;
    a._reversedDepth=b._reversedDepth=mode%4>=2;a.position.set(-5,0,1);b.position.set(5,0,2);
    a.rotation.y=Math.floor(mode/4)*.1;b.rotation.y=-Math.floor(mode/4)*.1;a.updateProjectionMatrix();b.updateProjectionMatrix();a.updateMatrixWorld();b.updateMatrixWorld();
    const cameras=new ArrayCamera([a,b]),f=new FrustumArray(),queries=[];f.coordinateSystem=9000;f.setFromArrayCamera(cameras);
    const geometry=new BufferGeometry().setAttribute('position',new Float32BufferAttribute([-1,-1,0,1,1,0,0,0,1],3)),mesh=new Mesh(geometry),sprite=new Sprite();
    for(const x of [-10,-5,0,5,10])for(const y of [-4,0,4])for(const z of [2,0,-5,-20,-60]){
        const p=new Vector3(x,y,z),sphere=new Sphere(p,.5),box=new Box3(new Vector3(x-.5,y-.5,z-.5),new Vector3(x+.5,y+.5,z+.5));
        mesh.matrixWorld.makeTranslation(x,y,z);sprite.matrixWorld.makeTranslation(x,y,z);sprite.center.set(.2,.8);
        queries.push([f.containsPoint(p),f.intersectsSphere(sphere),f.intersectsBox(box),f.intersectsObject(mesh),f.intersectsSprite(sprite)]);
    }
    const states=[frustumState(f)],original=f._frustums[1];cameras.cameras=[a];f.setFromArrayCamera(cameras);states.push(frustumState(f));
    const clone=f.clone();states.push(frustumState(clone));cameras.cameras=[];f.setFromArrayCamera(cameras);states.push(frustumState(f));
    const empty=f.intersectsSphere(new Sphere(new Vector3(),100));f.copy(clone);states.push(frustumState(f));
    return {kind:'frustum',mode,queries,states,empty,poolIdentity:f._frustums[1]===original,copyIdentity:f._frustums[0]===clone._frustums[0]};
}
function geometryCase(mode){
    const g=new InstancedBufferGeometry();g.uuid='geometry';g.name='instanced';g.setAttribute('position',new Float32BufferAttribute([1,2,3,4,5,6],3));g.setIndex([0,1]);g.addGroup(0,2,3);g.computeBoundingSphere();
    g.instanceCount=[Infinity,3,1.5,0,-1,'7',null,undefined,NaN,false][mode];const copy=g.clone();copy.uuid='copy';const before=copy.toJSON();copy.copy(new BufferGeometry());
    return {kind:'geometry',mode,count:value(g.instanceCount),json:g.toJSON(),clone:before,plain:copy.toJSON(),undefined:copy.instanceCount===undefined};
}
function stamp(o,name){o.uuid=name;o.children.forEach((c,i)=>stamp(c,name+'/'+i));}
function containersCase(mode){
    const c=new PerspectiveCamera(),array=[c],a=new ArrayCamera(array);a.isMultiViewCamera=true;a.name='array';a.fov=42;array.push(c);a.add(new Object3D());
    const ac=a.clone(mode%2===0),target=new ArrayCamera([c]);target.copy(a,false);stamp(a,'array');stamp(ac,'arrayClone');
    const g=new ClippingGroup();g.enabled=false;g.clipIntersection=true;g.clipShadows=true;g.clippingPlanes.push(new Plane(new Vector3(1,2,3),4));g.add(new Object3D());
    const gc=g.clone(mode%2===0),gt=new ClippingGroup();gt.enabled='manual';gt.copy(g,false);stamp(g,'group');stamp(gc,'groupClone');
    return {kind:'containers',mode,array:[a.type,a.isArrayCamera,a.isMultiViewCamera,a.cameras.length,a.cameras===array,ac.type,ac.isArrayCamera,ac.isMultiViewCamera,ac.cameras.length,target.cameras.length],group:[g.type,g.isGroup,g.isClippingGroup,g.clippingPlanes.length,gc.type,gc.isClippingGroup,gc.clippingPlanes.length,value(gc.enabled),value(gc.clipIntersection),value(gc.clipShadows),value(gt.enabled)],json:[a.toJSON(),ac.toJSON(),g.toJSON(),gc.toJSON()]};
}
function cameraCloneCase(kind){
    const camera=kind===0?new Camera():kind===1?new PerspectiveCamera():new OrthographicCamera();camera.name='camera';camera.add(new Object3D().add(new Object3D()));
    const clone=camera.clone(false);stamp(clone,'clone');const shallow=camera.clone();shallow.clear();shallow.copy(camera,false);
    return {kind:'cameraClone',camera:kind,children:clone.children.length,nested:clone.children[0].children.length,shallow:shallow.children.length,json:clone.toJSON()};
}
const expected=[];
for(let i=0;i<16;++i)expected.push(clockCase(i));for(let i=0;i<24;++i)expected.push(timerCase(i));for(let i=0;i<18;++i)expected.push(visibilityCase(i));for(let i=0;i<20;++i)expected.push(stereoCase(i));expected.push(stereoShared());for(let i=0;i<8;++i)expected.push(frustumCase(i));for(let i=0;i<10;++i)expected.push(geometryCase(i));for(let i=0;i<2;++i)expected.push(containersCase(i));
for(let i=0;i<6;++i)expected.push(runtimeTimerCase(i));
for(let i=0;i<3;++i)expected.push(cameraCloneCase(i));
const run=spawnSync(process.argv[3],[],{encoding:'utf8',maxBuffer:32*1024*1024});assert.equal(run.status,0,run.stderr||String(run.error));assert.equal(run.stderr,'');
const actual=JSON.parse(run.stdout),normalized=JSON.parse(JSON.stringify(expected));assert.equal(actual.length,normalized.length);
let comparisons=0;const failures=[];
function compare(a,b,where){
    if(b!==null&&typeof b==='object'){
        assert.equal(Array.isArray(a),Array.isArray(b),where);assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),where);
        for(const key of Object.keys(b))compare(a[key],b[key],where+'.'+key);
    }else{comparisons++;if(typeof a==='number'&&typeof b==='number')assert.ok(Math.abs(a-b)<=1e-11*Math.max(1,Math.abs(a),Math.abs(b)),`${where}: ${a} != ${b}`);else assert.deepEqual(a,b,where);}
}
for(let i=0;i<actual.length;++i){try{compare(actual[i],normalized[i],`${i}:${expected[i].kind}`);}catch(error){failures.push(error.message);}}
assert.equal(failures.length,0,failures.join('\n'));console.log(`Core/camera parity: ${actual.length} scenarios / ${comparisons} comparisons; ${lock.commit}`);
