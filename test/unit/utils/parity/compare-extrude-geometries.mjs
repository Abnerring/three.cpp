import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-extrude-geometries.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const T=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const extrudeSource=fs.readFileSync(path.join(upstream,'src/geometries/ExtrudeGeometry.js'),'utf8');
const worldUV=Function('Vector2',`return ${extrudeSource.match(/const WorldUVGenerator = ([\s\S]*?);\s*function toJSON/)[1]}`)(T.Vector2);
const serial=v=>JSON.parse(JSON.stringify(v));
const snapshot=g=>({type:g.type,attributes:Object.fromEntries(Object.entries(g.attributes).map(([k,v])=>[k,v.toJSON()])),
    groups:g.groups.map(g=>[g.start,g.count,g.materialIndex]),index:g.index?.toJSON()??null,json:serial(g.toJSON())});
function shape(points,uuid){const s=points.length?new T.Shape(points.map(p=>new T.Vector2(...p))):new T.Shape();s.uuid=uuid;return s;}
const rectangle=()=>shape([[0,0],[0,4],[6,4],[6,0]],'rectangle');
function holed(clockwise){const s=shape(clockwise?[[0,0],[0,8],[8,8],[8,0]]:[[0,0],[8,0],[8,8],[0,8]],'holed');
    s.holes.push(new T.Path([[2,2],[2,5],[5,5],[5,2]].map(p=>new T.Vector2(...p))));return s;}
const expected={};const defaults=new T.ExtrudeGeometry();defaults.parameters.shapes.uuid='default';expected.default=snapshot(defaults);
expected.variants=[{},{bevelEnabled:false},{depth:3,steps:3},{bevelThickness:.7},{bevelOffset:-.25,bevelSize:.4,bevelSegments:2},
    {steps:0,bevelEnabled:false},{steps:2.5,bevelEnabled:false},{bevelSegments:0},{bevelSegments:1.5},{bevelSegments:-1},
    {bevelSegments:NaN},{bevelSegments:null},{depth:null,steps:1},{steps:'2',bevelEnabled:false},
    {depth:-2,bevelThickness:-.2,bevelSize:-.3},{curveSegments:0,bevelEnabled:false}].map(o=>snapshot(new T.ExtrudeGeometry(rectangle(),o)));
expected.contours=[false,true].map(c=>snapshot(new T.ExtrudeGeometry(holed(c))));
for(const coordinates of [ [[0,0],[0,4],[3,2],[6,4],[6,0]], [[0,0],[0,2],[0,4],[6,4],[6,0]],
    [[0,0],[0,4],[6,4],[6,0],[6,1e-12],[0,0]], [[0,0],[0,4],[2,4],[0,4],[6,4],[6,0]],[],[[1,1]],[[0,0],[1,1]] ])
    expected.contours.push(snapshot(new T.ExtrudeGeometry(shape(coordinates,'special'))));
const curved=new T.Shape();curved.uuid='curve';curved.moveTo(0,0).quadraticCurveTo(3,7,6,0).lineTo(0,0);
expected.contours.push(snapshot(new T.ExtrudeGeometry(curved,{curveSegments:5,bevelSegments:2})));
const points=[[0,0,0],[3,2,1],[4,-1,3],[0,0,5]].map(p=>new T.Vector3(...p));
expected.paths=[];
for(const path of [new T.LineCurve3(new T.Vector3(),new T.Vector3(1,2,5)),
    new T.QuadraticBezierCurve3(new T.Vector3(),new T.Vector3(4,1,3),new T.Vector3(1,5,6)),
    new T.CatmullRomCurve3(points,false),new T.CatmullRomCurve3(points,true)]) {
    const g=new T.ExtrudeGeometry(rectangle(),{steps:4,bevelEnabled:true,bevelSegments:2,extrudePath:path});
    expected.paths.push(snapshot(g),snapshot(T.ExtrudeGeometry.fromJSON(g.toJSON(),{rectangle:g.parameters.shapes})));
}
const list=[rectangle(),holed(false)],options={steps:1,depth:1,bevelEnabled:false},calls=[];
options.UVGenerator={generateTopUV(g,v,a,b,c){calls.push(['top',g.groups.length,v.length,a,b,c]);
    if(calls.length===1){v[0]+=.25;options.depth=3;list.push(rectangle());}
    return [a,b,c].map(i=>new T.Vector2(v[i*3]+10,v[i*3+1]-4));},
    generateSideWallUV(g,v,a,b,c,d){calls.push(['side',g.groups.length,v.length,a,b,c,d]);
        const useX=Math.abs(v[a*3+1]-v[b*3+1])<Math.abs(v[a*3]-v[b*3]);
        return [a,b,c,d].map(i=>new T.Vector2(v[i*3+(useX?0:1)]*.5,(1-v[i*3+2])*.5));}};
const custom=new T.ExtrudeGeometry(list,options);expected.customUV=snapshot(custom);expected.uvCalls=calls;
const clone=custom.clone();expected.identities=[clone.parameters.options===options,clone.parameters.shapes===list,
    clone.attributes.position!==custom.attributes.position,list.length,custom.groups.length];
expected.clone=snapshot(clone);custom.translate(1,2,3);expected.transformed=snapshot(custom);
const early=new T.ExtrudeGeometry();early.parameters.shapes.uuid='default';early.setAttribute('position',null);early.morphAttributes.position=[null];
expected.earlyJSON=early.toJSON();expected.emptyArray=snapshot(new T.ExtrudeGeometry([]));
expected.inheritedOptions=snapshot(new T.ExtrudeGeometry(rectangle(),Object.create({depth:3,bevelEnabled:false})));
const regressions={};let popped=false;
const popUV={generateTopUV(g,v,a,b,c){if(!popped){popped=true;v.pop();}return worldUV.generateTopUV(g,v,a,b,c);},
    generateSideWallUV(...args){return worldUV.generateSideWallUV(...args);}};
regressions.poppedCoordinate=snapshot(new T.ExtrudeGeometry(rectangle(),{bevelEnabled:false,UVGenerator:popUV}));
class FramePath extends T.LineCurve3 {
    calls=[];
    constructor(){super(new T.Vector3(),new T.Vector3(0,0,5));}
    computeFrenetFrames(segments,closed){this.calls.push([segments,closed]);const result=super.computeFrenetFrames(segments,closed);
        result.normals.forEach(n=>n.set(1,0,0));result.binormals.forEach(b=>b.set(0,1,0));return result;}
}
const framePath=new FramePath();regressions.customFrames=snapshot(new T.ExtrudeGeometry(rectangle(),{steps:2,extrudePath:framePath}));
regressions.frameCalls=framePath.calls;
const trace=[],vertices=[];for(let i=0;i<12;++i)Object.defineProperty(vertices,i,{enumerable:true,get(){trace.push(i);return i;}});
const empty=new T.ExtrudeGeometry([]);
regressions.topUV=worldUV.generateTopUV(empty,vertices,0,1,2).map(v=>v.toArray());regressions.topGetters=[...trace];trace.length=0;
regressions.sideUV=worldUV.generateSideWallUV(empty,vertices,0,1,2,3).map(v=>v.toArray());regressions.sideGetters=[...trace];
regressions.conversionOrder=[];
for(const name of ['steps','bevelSegments']){const events=[];class TracedShape extends T.Shape {
    extractPoints(divisions){events.push('extract');return super.extractPoints(divisions);}}
    let threw=false;try{new T.ExtrudeGeometry(new TracedShape(),{[name]:{valueOf(){events.push(name);throw new TypeError('conversion');}}});}
    catch(error){if(!(error instanceof TypeError))throw error;threw=true;}
    regressions.conversionOrder.push([events,threw]);}
const assignmentTrace=[],prototype={get b(){throw new TypeError('deleted own property must not read its prototype');}};
const assigned=Object.create(prototype);
Object.defineProperty(assigned,'a',{enumerable:true,get(){assignmentTrace.push('a');delete this.b;Object.defineProperty(this,'c',{enumerable:true});return 1;}});
Object.defineProperty(assigned,'b',{value:2,enumerable:true,configurable:true});
Object.defineProperty(assigned,'c',{value:3,enumerable:false,configurable:true});
regressions.assignedOptions=new T.ExtrudeGeometry([],assigned).toJSON().options;regressions.assignmentGetters=assignmentTrace;
expected.regressions=regressions;
function normalize(v){if(typeof v==='number')return Number.isNaN(v)?'NaN':v===Infinity?'Infinity':v===-Infinity?'-Infinity':Object.is(v,-0)?'-0':v;
    if(Array.isArray(v))return v.map(normalize);if(v&&typeof v==='object')return Object.fromEntries(Object.entries(v).map(([k,v])=>[k,k==='uuid'?'uuid':normalize(v)]));return v;}
const actual=JSON.parse(execFileSync(path.resolve(process.argv[3]),[],{encoding:'utf8',maxBuffer:32*1024*1024}));
let comparisons=0;function compare(a,b,where='$'){++comparisons;
    if(typeof a==='number'&&typeof b==='number')assert.ok(Math.abs(a-b)<=2e-12*Math.max(1,Math.abs(a),Math.abs(b)),`${where}: ${a} != ${b}`);
    else if(b&&typeof b==='object'){assert.ok(a&&typeof a==='object',`${where}: type`);assert.deepEqual(Object.keys(a),Object.keys(b),`${where}: keys`);for(const k of Object.keys(b))compare(a[k],b[k],`${where}.${k}`);}
    else assert.ok(Object.is(a,b),`${where}: ${JSON.stringify(a)} != ${JSON.stringify(b)}`);}
compare(actual,normalize(expected));console.log(`Extrude geometry parity: ${comparisons} values/identities; fixed upstream ${lock.commit}`);
