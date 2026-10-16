// Differential check for the native examples/jsm/curves/CurveExtras.js port.
//
// The pinned module is imported, the same scale construction and parameter list is replayed
// there, and the probe output is compared field by field. The Math.pow/Math.sin/Math.cos
// scenarios cover the JSNumberMath functions the formulas depend on, including the
// y === 2 and y === 0.5 shortcuts of the pinned Node release.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-curve-extras.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const extras=await import(pathToFileURL(path.join(sourceDir,'examples/jsm/curves/CurveExtras.js')).href);

const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>values.map(encode);
const point3=value=>[value.x,value.y,value.z];

const scenarios=[];
// Math function coverage: the curve formulas call sin/cos many times and pow for
// sin(t)**3, t**3, t**4 and t**5, so the grid mixes those shapes with the boundary values.
const powCases=[];
for(const x of [0,-0,1,-1,2,-2,0.5,-0.5,3,-3,1.000093339211477,0.9999999999999999,1e300,1e-300,
    Number.MAX_VALUE,Number.MIN_VALUE,-2.5,7.25,1/3,4/3,Math.PI]){
    for(const y of [0,1,2,3,4,5,-1,-2,0.5,-0.5,1/3,7,1024,-1074]) powCases.push([x,y]);
}
for(let i=0;i<120;++i){
    const value=Math.sin(i*0.37)*Math.cos(i*1.11);
    powCases.push([Math.abs(value),3],[1+value*1e-3,2],[-Math.abs(value),4],[1+value*1e-9,5]);
// Extreme but finite operands for the special cases of Math.pow: a subnormal base, huge and
// tiny bases, bases just around 1, negative bases with integer and non-integer exponents, and
// exponents that overflow or underflow the result.
for(const x of [5e-324,1e-320,1e-308,0.9999999999999999,1.0000000000000002,1.0000000001,1,-1,2,-2,123.456,1e300,-1e300,1e308])
    for(const y of [-1074,-1024,-3,-2,-1.5,-1,-0.5,0,0.5,1,1.5,2,3,1024,1074,1e6])
        powCases.push([x,y]);
}
for(const [x,y] of powCases) scenarios.push({line:`W ${token(x)} ${token(y)}`,expected:{value:encode(Math.pow(x,y))}});
for(let i=0;i<200;++i){
    const value=(i-100)*0.37;
    scenarios.push({line:`N ${token(value)}`,expected:{value:encode(Math.sin(value))}});
    scenarios.push({line:`O ${token(value)}`,expected:{value:encode(Math.cos(value))}});
}
for(const value of [0,-0,1,-1,1e-8,-1e-8,1e8,Math.PI,-Math.PI,1e300,9007199254740993]){
    scenarios.push({line:`N ${token(value)}`,expected:{value:encode(Math.sin(value))}});
    scenarios.push({line:`O ${token(value)}`,expected:{value:encode(Math.cos(value))}});
}

// Curve coverage: every exported class, default construction plus an explicit scale, and a
// parameter list that spans the domain and beyond it.
const classes=[extras.GrannyKnot,extras.HeartCurve,extras.VivianiCurve,extras.KnotCurve,extras.HelixCurve,
    extras.TrefoilKnot,extras.TorusKnot,extras.CinquefoilKnot,extras.TrefoilPolynomialKnot,
    extras.FigureEightPolynomialKnot,extras.DecoratedTorusKnot4a,extras.DecoratedTorusKnot4b,
    extras.DecoratedTorusKnot5a,extras.DecoratedTorusKnot5c];
const times=[0,0.05,0.125,0.25,0.5,0.75,0.9,1,-0.25,1.5];
let curveScenarios=0;
for(let index=0;index<classes.length;++index){
    for(const scale of [undefined,2.5]){
        const curve=scale===undefined?new classes[index]():new classes[index](scale);
        const points=[],tangents=[];
        for(const t of times){
            points.push(encodeArray(point3(curve.getPoint(t))));
            tangents.push(encodeArray(point3(curve.getTangent(t))));
        }
        curve.arcLengthDivisions=8;
        const length8=curve.getLength();
        curve.arcLengthDivisions=200;
        const length200=curve.getLength();
        scenarios.push({
            line:`C ${index} ${scale===undefined?0:1} ${token(scale===undefined?0:scale)} ${times.length}`
                +` ${times.map(token).join(' ')}`,
            expected:{points,tangents,length8:encode(length8),length200:encode(length200),type:curve.type}
        });
        curveScenarios+=1;
    }
}

const input=scenarios.map(scenario=>scenario.line).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:256*1024*1024})
    .trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
let compared=0;
for(let index=0;index<scenarios.length;++index){
    const {line,expected}=scenarios[index];
    assert.deepEqual(JSON.parse(output[index]),expected,`scenario ${index}: ${line.slice(0,140)}`);
    compared+=1;
}
console.log(`CurveExtras parity: ${compared} scenarios (${curveScenarios} curve constructions, `
    +`${powCases.length} Math.pow, 212 Math.sin/Math.cos inputs) from the pinned checkout; ${process.version}`);
