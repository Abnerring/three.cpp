// Differential check for the native examples/jsm/math ports:
// ImprovedNoise.js, ColorConverter.js, Capsule.js and OBB.js.
//
// The four pinned modules are imported from the checkout, the same scenarios are replayed
// there, and every probe line is compared with assert.deepEqual, so no tolerance is used
// anywhere. Scenario tokens are generated here, which keeps both sides on the same arguments
// and the same call order; the probe only echoes results.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-addon-math.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const moduleURL=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
const {ImprovedNoise}=await import(moduleURL('examples/jsm/math/ImprovedNoise.js'));
const {ColorConverter}=await import(moduleURL('examples/jsm/math/ColorConverter.js'));
const {Capsule}=await import(moduleURL('examples/jsm/math/Capsule.js'));
const {OBB}=await import(moduleURL('examples/jsm/math/OBB.js'));
const {Vector3}=await import(moduleURL('src/math/Vector3.js'));
const {Matrix3}=await import(moduleURL('src/math/Matrix3.js'));
const {Matrix4}=await import(moduleURL('src/math/Matrix4.js'));
const {Color}=await import(moduleURL('src/math/Color.js'));
const {Box3}=await import(moduleURL('src/math/Box3.js'));
const {Sphere}=await import(moduleURL('src/math/Sphere.js'));
const {Ray}=await import(moduleURL('src/math/Ray.js'));
const {Plane}=await import(moduleURL('src/math/Plane.js'));

const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>Array.from(values,encode);
const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const tokens=values=>values.map(token).join(' ');
const v3=value=>[value.x,value.y,value.z];
const box3Tokens=box=>`${tokens(v3(box.min))} ${tokens(v3(box.max))}`;
const rotationTokens=rotation=>tokens(Array.from(rotation.elements));
const obbTokens=(center,halfSize,rotation)=>`${tokens(v3(center))} ${tokens(v3(halfSize))} ${rotationTokens(rotation)}`;
const capsuleTokens=(start,end,radius)=>`${tokens(v3(start))} ${tokens(v3(end))} ${token(radius)}`;
const vector3Tokens=value=>tokens(v3(value));
// Upstream OBB stores the vectors it is handed, and applyMatrix4() mutates center and
// halfSize in place. Every construction here therefore copies its inputs, which also matches
// the value semantics of the native port, and keeps the generator's literals intact.
const makeOBB=(center,halfSize,rotation)=>new OBB(center.clone(),halfSize.clone(),rotation.clone());

const scenarios=[];
const add=(line,expected)=>scenarios.push({line,expected});

// ---------------------------------------------------------------- ImprovedNoise
const noise=new ImprovedNoise();
// A grid that covers zero, negative and fractional coordinates in every axis.
const noiseGrid=[-2.5,-1.75,-1,-0.5,-0.25,0,0.1,0.5,1,1.5,2.25,3];
for(const x of noiseGrid)
    for(const y of [-1.5,-0.75,-0.5,0,0.5,1,2.5])
        for(const z of [-3,-2,-0.5,0,0.75,1.5,4])
            add(`noise ${token(x)} ${token(y)} ${token(z)}`,{noise:encode(noise.noise(x,y,z))});
// Exact lattice points, including the wrapped table entries at and beyond 255.
for(const x of [-2,-1,0,1,2,255,256])
    for(const y of [-1,0,1,256])
        for(const z of [-1,0,1,255])
            add(`noise ${token(x)} ${token(y)} ${token(z)}`,{noise:encode(noise.noise(x,y,z))});
// Signed zero, non-finite coordinates and inputs past the ToInt32 range.
for(const point of [[-0,-0,-0],[0,-0,0],[-0,0,-0],[0.5,-0,-0.5],[NaN,0,0],[0,NaN,0],[0,0,NaN],
    [Infinity,0,0],[0,-Infinity,0.5],[-Infinity,-Infinity,-Infinity],[1e300,0.5,0.5],[-1e300,0.5,0.5],
    [4294967296.5,0.5,0.5],[-4294967296.5,0.5,0.5],[2147483647.25,-2147483648.75,1e21],
    [1e-300,-1e-300,5e-324],[0.9999999999999999,1.0000000000000002,-1.0000000000000002]]) {
    const [x,y,z]=point;
    add(`noise ${token(x)} ${token(y)} ${token(z)}`,{noise:encode(noise.noise(x,y,z))});
}

// ---------------------------------------------------------------- ColorConverter
const colorHSV=(h,s,v)=>{
    const color=new Color();
    ColorConverter.setHSV(color,h,s,v);
    return [color.r,color.g,color.b];
};
const colorHSL=(h,s,l)=>{
    const color=new Color();
    color.setHSL(h,s,l);
    return [color.r,color.g,color.b];
};
const hues=[-1.25,-0.5,0,0.1,0.25,1/3,0.5,0.75,1,1.25,2.5,0.999999];
const saturations=[-0.5,0,0.25,0.5,0.75,1,1.5];
const values=[-1,0,0.25,0.5,1,2];
for(const h of hues)
    for(const s of saturations)
        for(const v of values)
            add(`hsv ${token(h)} ${token(s)} ${token(v)}`,{rgb:encodeArray(colorHSV(h,s,v))});
// Round trips read the module scratch record back after the setter ran. The grid repeats the
// one used by ColorConverter.tests.cpp, where the upstream rounding of the HSL hop is
// visible (worst case one ulp on the saturation).
for(const h of [0,0.05,0.25,1/3,0.5,0.75,0.999999])
    for(const s of [0,0.25,0.5,0.9,1])
        for(const v of [0,0.125,0.5,0.875,1]) {
            const color=new Color();
            ColorConverter.setHSV(color,h,s,v);
            const target={};
            ColorConverter.getHSV(color,target);
            add(`hsvrt ${token(h)} ${token(s)} ${token(v)}`,
                {rgb:encodeArray([color.r,color.g,color.b]),hsv:encodeArray([target.h,target.s,target.v])});
        }
const rgbSamples=[[0,0,0],[1,1,1],[1,0,0],[0,1,0],[0,0,1],[0.5,0.5,0.5],[0.25,0.5,0.75],[0.1,0.2,0.3],
    [1,0.5,0],[-0.5,0.25,1.5],[0.999999,0.000001,0.5],[1e-300,0.5,1-1e-16],[0.2,0.2,0.20000000000000004]];
for(const [r,g,b] of rgbSamples) {
    const target={};
    ColorConverter.getHSV(new Color(r,g,b),target);
    add(`gethsv ${token(r)} ${token(g)} ${token(b)}`,{hsv:encodeArray([target.h,target.s,target.v])});
}
const hslHues=[-0.25,0,0.1,0.25,1/3,0.5,0.75,1,1.25,2.5];
const hslSaturations=[-0.5,0,0.25,0.5,1,1.5];
const lightness=[-0.5,0,0.25,0.5,0.75,1,1.5];
for(const h of hslHues)
    for(const s of hslSaturations)
        for(const l of lightness)
            add(`hsl ${token(h)} ${token(s)} ${token(l)}`,{rgb:encodeArray(colorHSL(h,s,l))});
// Color.getHSL writes the plain `h`, `s` and `l` fields of its target, which the native port
// models with the x, y and z components of the Vector3 target.
for(const [r,g,b] of rgbSamples) {
    const target={};
    new Color(r,g,b).getHSL(target);
    add(`gethsl ${token(r)} ${token(g)} ${token(b)}`,{hsl:encodeArray([target.h,target.s,target.l])});
}
for(const h of [-0.25,0,0.1,0.25,1/3,0.5,0.75,0.875,1,1.25,2.5])
    for(const s of [-0.5,0,0.25,0.5,0.9,1,1.5])
        for(const l of [-0.5,0,0.25,0.5,0.75,0.875,1,1.5]) {
            const color=new Color();
            color.setHSL(h,s,l);
            const target={};
            color.getHSL(target);
            add(`hslrt ${token(h)} ${token(s)} ${token(l)}`,
                {rgb:encodeArray([color.r,color.g,color.b]),hsl:encodeArray([target.h,target.s,target.l])});
        }

// ---------------------------------------------------------------- Capsule
add('capsule-default',(()=>{
    const capsule=new Capsule();
    return {start:encodeArray(v3(capsule.start)),end:encodeArray(v3(capsule.end)),
        radius:encode(capsule.radius)};
})());
const capsuleShapes=[
    [new Vector3(0,0,0),new Vector3(0,1,0),1],
    [new Vector3(0,0,0),new Vector3(0,0,0),0],
    [new Vector3(1,2,3),new Vector3(4,6,8),0.5],
    [new Vector3(-1,-2,-3),new Vector3(1,2,3),2],
    [new Vector3(0,0,0),new Vector3(1,0,0),0.25],
    [new Vector3(0,0,0),new Vector3(0,0,10),0.0000001],
    [new Vector3(2.5,-0.5,1.25),new Vector3(-2.5,0.5,-1.25),1e-9],
    [new Vector3(1e10,1e10,1e10),new Vector3(-1e10,-1e10,-1e10),0.5],
    [new Vector3(0.1,0.2,0.3),new Vector3(0.1,0.2,0.3),3],
    [new Vector3(-0.5,0,0),new Vector3(0.5,0,0),-1],
    [new Vector3(0,1,0),new Vector3(0,0,1),1.5],
    [new Vector3(0.125,0.25,0.5),new Vector3(2,4,8),0.75]];
const capsuleOffsets=[new Vector3(1,2,3),new Vector3(-0.5,0.25,-2),new Vector3(0,0,0)];
for(const [start,end,radius] of capsuleShapes)
    for(const offset of capsuleOffsets) {
        const capsule=new Capsule(start.clone(),end.clone(),radius);
        const setReturnsThis=capsule.set(start,end,radius)===capsule;
        const copy=new Capsule();
        copy.copy(capsule);
        const clone=capsule.clone();
        const center=new Vector3();
        const centerReturnsTarget=capsule.getCenter(center)===center;
        const translateReturnsThis=clone.translate(offset)===clone;
        const translatedCenter=clone.getCenter(new Vector3());
        add(`capsule ${capsuleTokens(start,end,radius)} ${vector3Tokens(offset)}`,
            {start:encodeArray(v3(capsule.start)),end:encodeArray(v3(capsule.end)),radius:encode(capsule.radius),
                center:encodeArray(v3(center)),copyStart:encodeArray(v3(copy.start)),
                copyEnd:encodeArray(v3(copy.end)),copyRadius:encode(copy.radius),
                cloneStart:encodeArray(v3(clone.start)),cloneEnd:encodeArray(v3(clone.end)),
                cloneRadius:encode(clone.radius),translatedCenter:encodeArray(v3(translatedCenter)),
                sourceStart:encodeArray(v3(capsule.start)),sourceEnd:encodeArray(v3(capsule.end)),
                sourceRadius:encode(capsule.radius),setReturnsThis,centerReturnsTarget,translateReturnsThis});
    }
const boxes=[
    new Box3(new Vector3(-0.5,-0.5,-0.5),new Vector3(0.5,0.5,0.5)),
    new Box3(new Vector3(-1,-1,-1),new Vector3(1,1,1)),
    new Box3(new Vector3(-4,0,-4),new Vector3(-3,1,-3)),
    new Box3(new Vector3(0.5,0,0.5),new Vector3(1.5,1,1.5)),
    new Box3(new Vector3(1,0,0),new Vector3(2,1,1)),
    new Box3(new Vector3(0.9999999,0,0),new Vector3(2,1,1)),
    new Box3(new Vector3(-2,0,0),new Vector3(-1,1,1)),
    new Box3(new Vector3(-1,-2,-1),new Vector3(1,-1,1)),
    new Box3(new Vector3(0,0,10),new Vector3(1,1,11)),
    new Box3(new Vector3(0,10,0),new Vector3(1,11,1)),
    new Box3(new Vector3(5,5,5),new Vector3(6,6,6)),
    new Box3(new Vector3(0.25,-1,-1),new Vector3(0.75,1,1)),
    new Box3(new Vector3(1,1,1),new Vector3(3,3,3)),
    new Box3(new Vector3(-1,-1,4),new Vector3(1,1,6)),
    new Box3(new Vector3(-2,-2,-2),new Vector3(-1.5,-1.5,-1.5)),
    new Box3(new Vector3(2,0,0),new Vector3(2.5,1,1)),
    new Box3(new Vector3(-0.25,-0.25,-0.25),new Vector3(0.25,0.25,0.25)),
    new Box3(),
    new Box3(new Vector3(0,0,0),new Vector3(0,0,0))];
const boxCapsules=[
    [new Vector3(0,0,0),new Vector3(0,1,0),1],
    [new Vector3(0,0,0),new Vector3(1,0,0),0],
    [new Vector3(0,0,0),new Vector3(0,0,10),0.5],
    [new Vector3(0,0,0),new Vector3(2,2,2),0.5],
    [new Vector3(-1,-1,-1),new Vector3(1,1,1),0.75],
    [new Vector3(0.5,0.5,0.5),new Vector3(0.5,0.5,0.5),0]];
for(const [start,end,radius] of boxCapsules)
    for(const box of boxes) {
        const capsule=new Capsule(start,end,radius);
        add(`capsule-box ${capsuleTokens(start,end,radius)} ${box3Tokens(box)}`,
            {intersects:capsule.intersectsBox(box)});
    }

// ---------------------------------------------------------------- OBB
add('obb-default',(()=>{
    const obb=new OBB();
    return {center:encodeArray(v3(obb.center)),halfSize:encodeArray(v3(obb.halfSize)),
        rotation:encodeArray(Array.from(obb.rotation.elements))};
})());
const rotX=angle=>{const c=Math.cos(angle),s=Math.sin(angle);return new Matrix3().set(1,0,0,0,c,-s,0,s,c);};
const rotY=angle=>{const c=Math.cos(angle),s=Math.sin(angle);return new Matrix3().set(c,0,s,0,1,0,-s,0,c);};
const rotZ=angle=>{const c=Math.cos(angle),s=Math.sin(angle);return new Matrix3().set(c,-s,0,s,c,0,0,0,1);};
const identity=()=>new Matrix3();
const rotations=[identity(),rotX(0),rotZ(Math.PI/2),rotZ(Math.PI/4),rotX(0.5),rotY(-1.2),
    rotZ(2.1).multiply(rotX(0.35)),rotX(Math.PI/2).multiply(rotY(Math.PI/3)),
    new Matrix3().set(1,0.25,0,0,1,0.5,0,0,1),new Matrix3().set(2,0,0,0,0.5,0,0,0,-1)];
const centers=[new Vector3(),new Vector3(1,2,3),new Vector3(-0.5,0.25,-2),new Vector3(1e6,-1e6,0.5)];
const halfSizes=[new Vector3(),new Vector3(1,1,1),new Vector3(1,2,3),new Vector3(0.5,0.25,0.125),
    new Vector3(-1,-2,-3),new Vector3(1e-8,1e8,1)];
const obbShapes=[];
for(const [index,center] of centers.entries())
    for(const [other,halfSize] of halfSizes.entries())
        for(const [rotationIndex,rotation] of rotations.entries()) {
            if((index+other+rotationIndex)%3!==0)continue;
            obbShapes.push([center,halfSize,rotation]);
        }
for(const [center,halfSize,rotation] of obbShapes) {
    const obb=new OBB(center,halfSize,rotation);
    const size=new Vector3();
    const sizeReturnsTarget=obb.getSize(size)===size;
    const clone=obb.clone();
    const cloneEquals=clone.equals(obb);
    const assigned=new OBB();
    const setReturnsThis=assigned.set(center,halfSize,rotation)===assigned;
    const setEquals=assigned.equals(obb);
    clone.center.set(123,456,789);
    const cloneIndependent=!clone.equals(obb)&&obb.center.x===center.x&&obb.center.y===center.y
        &&obb.center.z===center.z;
    add(`obb ${obbTokens(center,halfSize,rotation)}`,
        {center:encodeArray(v3(obb.center)),halfSize:encodeArray(v3(obb.halfSize)),
            rotation:encodeArray(Array.from(obb.rotation.elements)),size:encodeArray(v3(size)),
            sizeReturnsTarget,cloneEquals,cloneIndependent,setReturnsThis,setEquals});
}
const matrices=[
    new Matrix4(),
    new Matrix4().makeTranslation(1,2,3),
    new Matrix4().makeScale(2,3,4),
    new Matrix4().makeScale(2,3,4).setPosition(1,2,3),
    new Matrix4().makeRotationX(0.7),
    new Matrix4().makeRotationY(-0.4).setPosition(-1,0.5,2),
    new Matrix4().makeRotationZ(1.1),
    new Matrix4().makeRotationAxis(new Vector3(1,2,3).normalize(),0.9),
    new Matrix4().makeScale(-2,1,1),
    new Matrix4().makeScale(1,-1,1),
    new Matrix4().makeScale(0,1,1),
    new Matrix4().makeScale(1,1e-300,1),
    new Matrix4().set(1,0.5,0,0,0,1,0,0,0,0,1,0,0,0,0,1),
    new Matrix4().set(1,0,0,0.25,0,1,0,0,0,0,1,0,0.5,0.5,0.5,1),
    new Matrix4().makeBasis(new Vector3(0,1,0),new Vector3(-1,0,0),new Vector3(0,0,1)).setPosition(1,2,3),
    new Matrix4().makeRotationZ(Math.PI/2).multiply(new Matrix4().makeScale(2,2,2)).setPosition(4,-2,1)];
for(const matrix of matrices)
    for(const [index,[center,halfSize,rotation]] of obbShapes.entries()) {
        if(index%4!==0)continue;
        const obb=makeOBB(center,halfSize,rotation);
        const returnsThis=obb.applyMatrix4(matrix)===obb;
        add(`obb-apply ${obbTokens(center,halfSize,rotation)} ${tokens(Array.from(matrix.elements))}`,
            {center:encodeArray(v3(obb.center)),halfSize:encodeArray(v3(obb.halfSize)),
                rotation:encodeArray(Array.from(obb.rotation.elements)),returnsThis});
    }
const points=[new Vector3(),new Vector3(1,1,1),new Vector3(1,2,3),new Vector3(1,2,3.0000001),
    new Vector3(-1,-2,-3),new Vector3(0.5,0.5,0.5),new Vector3(2,0,0),new Vector3(0,3,0),
    new Vector3(-5,5,-5),new Vector3(1e6,1e6,1e6),new Vector3(-0,0,-0),new Vector3(0.9999999,0,0)];
for(const [index,[center,halfSize,rotation]] of obbShapes.entries())
    for(const [pointIndex,point] of points.entries()) {
        if((index+pointIndex)%3!==0)continue;
        const obb=makeOBB(center,halfSize,rotation);
        add(`obb-contains ${obbTokens(center,halfSize,rotation)} ${vector3Tokens(point)}`,
            {contains:obb.containsPoint(point)});
        const target=new Vector3();
        const targetReturned=obb.clampPoint(point,target)===target;
        add(`obb-clamp ${obbTokens(center,halfSize,rotation)} ${vector3Tokens(point)}`,
            {point:encodeArray(v3(target)),targetReturned});
    }
for(const [index,[center,halfSize,rotation]] of obbShapes.entries())
    for(const [boxIndex,box] of boxes.entries()) {
        if((index+boxIndex)%4!==0)continue;
        const obb=makeOBB(center,halfSize,rotation);
        add(`obb-box3 ${obbTokens(center,halfSize,rotation)} ${box3Tokens(box)}`,
            {intersects:obb.intersectsBox3(box)});
    }
const spheres=[new Sphere(new Vector3(),0),new Sphere(new Vector3(),1),new Sphere(new Vector3(2,0,0),1),
    new Sphere(new Vector3(2,0,0),0.5),new Sphere(new Vector3(0,3,0),0.25),new Sphere(new Vector3(-5,-5,-5),4),
    new Sphere(new Vector3(1,2,3),1e-9),new Sphere(new Vector3(1e6,0,0),1),new Sphere(new Vector3(0.5,0.5,0.5),2)];
for(const [index,[center,halfSize,rotation]] of obbShapes.entries())
    for(const [sphereIndex,sphere] of spheres.entries()) {
        if((index+sphereIndex)%3!==0)continue;
        const obb=makeOBB(center,halfSize,rotation);
        add(`obb-sphere ${obbTokens(center,halfSize,rotation)} ${vector3Tokens(sphere.center)} ${token(sphere.radius)}`,
            {intersects:obb.intersectsSphere(sphere)});
    }
const rays=[new Ray(new Vector3(0,0,-5),new Vector3(0,0,1)),new Ray(new Vector3(0,0,-5),new Vector3(0,0,-1)),
    new Ray(new Vector3(4,0,0),new Vector3(-1,0,0)),new Ray(new Vector3(0,4,0),new Vector3(0,-1,0)),
    new Ray(new Vector3(0,4,0),new Vector3(0,1,0)),new Ray(new Vector3(1,1,1),new Vector3(1,1,1)),
    new Ray(new Vector3(1,1,1),new Vector3(-1,-1,-1)),new Ray(new Vector3(5,5,5),new Vector3(0,0,0)),
    new Ray(new Vector3(-0,0,-0),new Vector3(1,1e-16,0)),new Ray(new Vector3(2,2,2),new Vector3(1,0,0))];
for(const [index,[center,halfSize,rotation]] of obbShapes.entries())
    for(const [rayIndex,ray] of rays.entries()) {
        if((index+rayIndex)%4!==0)continue;
        const obb=makeOBB(center,halfSize,rotation);
        const target=new Vector3();
        const hit=obb.intersectRay(ray,target);
        add(`obb-ray ${obbTokens(center,halfSize,rotation)} ${vector3Tokens(ray.origin)} ${vector3Tokens(ray.direction)}`,
            {hit:hit!==null,point:hit===null?null:encodeArray(v3(target)),intersects:obb.intersectsRay(ray)});
    }
const obbPairs=[];
for(const [index,shape] of obbShapes.entries()) {
    const [center,halfSize,rotation]=shape;
    const other=obbShapes[(index*5+3)%obbShapes.length];
    obbPairs.push([shape,other]);
    obbPairs.push([shape,[new Vector3(center.x+1.5,center.y,center.z),halfSize,rotation]]);
    obbPairs.push([shape,[new Vector3(center.x-2.5,center.y+0.5,center.z),halfSize,rotation]]);
    obbPairs.push([shape,[new Vector3(center.x,center.y,center.z+2),halfSize,rotZ(0.5)]]);
    obbPairs.push([shape,[new Vector3(center.x+2,center.y,center.z),halfSize,identity()]]);
}
const epsilons=[undefined,0,1e-9,Number.EPSILON,1e-3];
for(const [index,[a,b]] of obbPairs.entries()) {
    const epsilon=epsilons[index%epsilons.length];
    const obbA=makeOBB(a[0],a[1],a[2]);
    const obbB=makeOBB(b[0],b[1],b[2]);
    const intersects=epsilon===undefined?obbA.intersectsOBB(obbB):obbA.intersectsOBB(obbB,epsilon);
    const line=`obb-obb ${obbTokens(a[0],a[1],a[2])} ${obbTokens(b[0],b[1],b[2])}`
        +(epsilon===undefined?'':` ${token(epsilon)}`);
    add(line,{intersects});
}
const planes=[new Plane(new Vector3(1,0,0),0),new Plane(new Vector3(1,0,0),1),
    new Plane(new Vector3(1,0,0),1.0000001),new Plane(new Vector3(0,1,0),-2),
    new Plane(new Vector3(0,0,1),-3),new Plane(new Vector3(0,0,1),-3.5),
    new Plane(new Vector3(1,1,1).normalize(),0),new Plane(new Vector3(-1,-1,-1).normalize(),0.75),
    new Plane(new Vector3(0,0,1),0),new Plane(new Vector3(1,0,0),-1e6)];
for(const [index,[center,halfSize,rotation]] of obbShapes.entries())
    for(const [planeIndex,plane] of planes.entries()) {
        if((index+planeIndex)%3!==0)continue;
        const obb=makeOBB(center,halfSize,rotation);
        add(`obb-plane ${obbTokens(center,halfSize,rotation)} ${vector3Tokens(plane.normal)} ${token(plane.constant)}`,
            {intersects:obb.intersectsPlane(plane)});
    }
for(const box of boxes) {
    const obb=new OBB(new Vector3(9,9,9),new Vector3(9,9,9),rotZ(0.5));
    const returnsThis=obb.fromBox3(box)===obb;
    add(`obb-frombox ${box3Tokens(box)}`,
        {center:encodeArray(v3(obb.center)),halfSize:encodeArray(v3(obb.halfSize)),
            rotation:encodeArray(Array.from(obb.rotation.elements)),returnsThis});
}
for(const [index,a] of obbShapes.entries()) {
    const b=obbShapes[(index+1)%obbShapes.length];
    const same=index%2===0?a:b;
    const obbA=makeOBB(a[0],a[1],a[2]);
    const obbB=makeOBB(same[0],same[1],same[2]);
    add(`obb-equals ${obbTokens(a[0],a[1],a[2])} ${obbTokens(same[0],same[1],same[2])}`,
        {equals:obbA.equals(obbB),reversed:obbB.equals(obbA)});
    const obbC=makeOBB(b[0],b[1],b[2]);
    add(`obb-equals ${obbTokens(a[0],a[1],a[2])} ${obbTokens(b[0],b[1],b[2])}`,
        {equals:obbA.equals(obbC),reversed:obbC.equals(obbA)});
}

// ---------------------------------------------------------------- run the probe
const input=scenarios.map(scenario=>scenario.line).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:512*1024*1024})
    .trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
let compared=0;
for(let index=0;index<scenarios.length;++index){
    const {line,expected}=scenarios[index];
    assert.deepEqual(JSON.parse(output[index]),expected,`scenario ${index}: ${line.slice(0,160)}`);
    compared+=1;
}
const counts={noise:0,color:0,capsule:0,obb:0};
for(const {line} of scenarios){
    const kind=line.slice(0,line.indexOf(' ')===-1?line.length:line.indexOf(' '));
    if(kind==='noise')counts.noise+=1;
    else if(['hsv','gethsv','hsvrt','hsl','gethsl','hslrt'].includes(kind))counts.color+=1;
    else if(kind.startsWith('capsule'))counts.capsule+=1;
    else counts.obb+=1;
}
console.log(`Addon math parity: ${compared} scenarios (${counts.noise} ImprovedNoise, ${counts.color} ColorConverter/Color, `
    +`${counts.capsule} Capsule, ${counts.obb} OBB) match the pinned checkout bit for bit; ${process.version}`);
