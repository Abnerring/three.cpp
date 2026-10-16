// Differential check for the native trigonometric code paths of the core math modules.
//
// The pinned classes are imported from the checkout, the same scenarios are replayed there,
// and every result is compared with assert.deepEqual (no tolerances). This covers the
// values that made Matrix4/Euler/Quaternion/Vector/Spherical/Cylindrical switch from
// std::sin/cos/asin/acos/atan/atan2 to the pinned V8 fdlibm implementations.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-math-core-trig.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const url=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
const THREE=await import(url('build/three.module.js'));
const {Quaternion,Euler,Matrix3,Matrix4,Vector2,Vector3,Vector4,Spherical,Cylindrical}=THREE;

const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>Array.from(values,encode);
const q4=value=>encodeArray([value.x,value.y,value.z,value.w]);
const v2=value=>encodeArray([value.x,value.y]);
const v3=value=>encodeArray([value.x,value.y,value.z]);
const m3=value=>encodeArray(value.elements);
const m4=value=>encodeArray(value.elements);

const scenarios=[];
const orders=['XYZ','YXZ','ZXY','ZYX','YZX','XZY'];
const angles=[0,-0,1e-8,-1e-8,0.1,-0.1,0.5,1,1.5707963267948966,-1.5707963267948966,2,3,-3,3.141592653589793,
    6.283185307179586,-6.283185307179586,10,-10,100,-100,1e6,1e-300];
const add=(line,expected)=>scenarios.push({line,expected});
const t=token;

// Quaternion.setFromEuler over every order and a wide angle grid.
for(const order of orders){
    for(const x of angles){
        for(const y of [0,0.3,-0.7,1.2]){
            const z=angles[(scenarios.length*7)%angles.length];
            const q=new Quaternion().setFromEuler(new Euler(x,y,z,order));
            add(`Q ${order} ${t(x)} ${t(y)} ${t(z)}`,{value:q4(q)});
        }
    }
}
// Quaternion.setFromAxisAngle and slerpQuaternions.
for(const axis of [[1,0,0],[0,1,0],[0,0,1],[1,2,3],[-1,0.5,0.25],[0,0,0]]){
    for(const angle of angles){
        const q=new Quaternion().setFromAxisAngle(new Vector3(...axis),angle);
        add(`QA ${axis.map(t).join(' ')} ${t(angle)}`,{value:q4(q)});
    }
}
for(const [a,b,ratio] of [
    [[0,0,0,1],[0,0,0,1],0.5],
    [[0,0,0,1],[0.7071067811865476,0,0,0.7071067811865476],0.25],
    [[0.1,0.2,0.3,0.9],[0.4,-0.2,0.1,0.86],0.5],
    [[0.1,0.2,0.3,0.9],[0.4,-0.2,0.1,0.86],0],
    [[0.1,0.2,0.3,0.9],[0.4,-0.2,0.1,0.86],1],
    [[1,0,0,0],[-1,0,0,0],0.5],
    [[0,0,0,0],[0,0,0,1],0.5]]){
    const q=new Quaternion().slerpQuaternions(new Quaternion(...a),new Quaternion(...b),ratio);
    add(`QS ${a.map(t).join(' ')} ${b.map(t).join(' ')} ${t(ratio)}`,{value:q4(q)});
}
// Quaternion.setFromRotationMatrix and Euler.setFromRotationMatrix, fed with matrices built
// from the pinned trig so only the reconstruction is compared.
for(const order of orders){
    for(const [x,y,z] of [[0.3,0.2,0.1],[-1.2,0.7,-0.4],[0,0,0],[1.5707963267948966,0.25,-0.5],
        [-3,2.5,1.5],[0.5,-0.5,0.5]]){
        const matrix=new Matrix4().makeRotationFromEuler(new Euler(x,y,z,order));
        const elements=Array.from(matrix.elements);
        add(`QM ${elements.map(t).join(' ')}`,{value:q4(new Quaternion().setFromRotationMatrix(matrix))});
        add(`E ${elements.map(t).join(' ')} ${order}`,
            {value:v3(new Euler(0,0,0,order).setFromRotationMatrix(matrix))});
    }
}
// Matrix4/Matrix3 rotations.
for(const angle of angles){
    add(`M4 0 ${t(angle)}`,{value:m4(new Matrix4().makeRotationX(angle))});
    add(`M4 1 ${t(angle)}`,{value:m4(new Matrix4().makeRotationY(angle))});
    add(`M4 2 ${t(angle)}`,{value:m4(new Matrix4().makeRotationZ(angle))});
    add(`M4 3 ${t(angle)}`,{value:m4(new Matrix4().makeRotationAxis(new Vector3(1,2,3).normalize(),angle))});
    add(`M3 ${t(angle)}`,{value:m3(new Matrix3().makeRotation(angle))});
}
for(const order of orders){
    for(const [x,y,z] of [[0,0,0],[0.3,0.2,0.1],[-1.2,0.7,-0.4],[2.5,-1.5,0.75]]){
        add(`ME ${t(x)} ${t(y)} ${t(z)} ${order}`,
            {value:m4(new Matrix4().makeRotationFromEuler(new Euler(x,y,z,order)))});
    }
}
// Spherical, cylindrical and angle helpers.
for(const [radius,phi,theta] of [[1,0,0],[2,0.5,-0.75],[0.5,-1.2,2.5],[0,1,1],[3,3.141592653589793,0],
    [1e6,1e-8,-1e-8]]){
    add(`V3S ${t(radius)} ${t(phi)} ${t(theta)}`,
        {value:v3(new Vector3().setFromSphericalCoords(radius,phi,theta))});
}
for(const [radius,theta,y] of [[1,0,0],[2,-0.7,1.5],[0.5,2.5,-0.25],[0,1,1]]){
    add(`V3C ${t(radius)} ${t(theta)} ${t(y)}`,
        {value:v3(new Vector3().setFromCylindricalCoords(radius,theta,y))});
}
for(const [a,b] of [
    [[1,0,0],[0,1,0]],[[1,2,3],[3,2,1]],[[-1,0,0],[0,-1,0]],[[1,0,0],[0,0,0]],[[0,0,0],[0,0,0]],
    [[1e-8,0,0],[0,1e-8,0]],[[1,2,3],[1,2,3]],[[1,2,3],[-1,-2,-3]]]){
    add(`V3A ${a.map(t).join(' ')} ${b.map(t).join(' ')}`,
        {value:encode(new Vector3(...a).angleTo(new Vector3(...b)))});
}
for(const [x,y] of [[1,0],[0,1],[-1,0],[0,-1],[1,1],[-1,-1],[1e-8,-1e-8],[0,0],[-0,-0]]){
    add(`V2G ${t(x)} ${t(y)}`,{value:encode(new Vector2(x,y).angle())});
}
for(const [x,y,z] of [[1,0,0],[0,1,0],[0,0,1],[1,2,3],[-1,-2,-3],[0,0,0],[1e-8,1e-8,1e-8],[1e6,-1e6,1e6]]){
    const spherical=new Spherical().setFromVector3(new Vector3(x,y,z));
    add(`SP ${t(x)} ${t(y)} ${t(z)}`,{value:encodeArray([spherical.radius,spherical.phi,spherical.theta])});
    const cylindrical=new Cylindrical().setFromVector3(new Vector3(x,y,z));
    add(`CY ${t(x)} ${t(y)} ${t(z)}`,{value:encodeArray([cylindrical.radius,cylindrical.theta,cylindrical.y])});
}
for(const [x,y,z,w] of [[0,0,0,1],[0.1,0.2,0.3,0.9],[-0.5,0.5,-0.5,0.5],[0,0,0,-1],[1,0,0,0]]){
    const v=new Vector4().setAxisAngleFromQuaternion(new Quaternion(x,y,z,w));
    add(`V4Q ${[x,y,z,w].map(t).join(' ')}`,{value:encodeArray([v.x,v.y,v.z,v.w])});
}

const input=scenarios.map(scenario=>scenario.line).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:512*1024*1024})
    .trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
for(let index=0;index<scenarios.length;++index){
    const {line,expected}=scenarios[index];
    assert.deepEqual(JSON.parse(output[index]),expected,`scenario ${index}: ${line.slice(0,140)}`);
}
console.log(`Core math trig parity: ${scenarios.length} scenarios across Euler, Quaternion, Matrix3/Matrix4,`
    +` Vector2/3/4, Spherical and Cylindrical match the pinned checkout bit for bit; ${process.version}`);
