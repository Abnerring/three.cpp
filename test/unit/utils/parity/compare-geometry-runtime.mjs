import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync, spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
const [upstreamArg, probeArg, outputArg, filter = ''] = process.argv.slice(2);
const upstream = path.resolve(upstreamArg), probe = path.resolve(probeArg);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding:'utf8'}).trim(), lock.commit);
const load = rel => import(pathToFileURL(path.join(upstream, 'src', rel)).href);
const [geometries, {Shape}, curves, {BufferGeometry}] = await Promise.all([load('geometries/Geometries.js'), load('extras/core/Shape.js'), load('extras/curves/Curves.js'), load('core/BufferGeometry.js')]);
const output = outputArg || path.join(process.env.TEMP, `threecpp-geometry-runtime-parity-${Date.now()}`);
assert(!fs.existsSync(output),'Output must be a fresh directory: '+output);
fs.mkdirSync(output, {recursive:true});
const hash = file => createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const guards = {probe:hash(probe), sources:Object.fromEntries(Object.keys(geometries).map(name => [name,
    hash(path.join(upstream,'src/geometries',`${name}.js`))]))};
let trace;
function decode(value) {
    if (value === null || typeof value !== 'object') return value;
    if ('$undefined' in value) return undefined;
    if ('$number' in value) return value.$number === '-0' ? -0 : Number(value.$number);
    if ('$geometry' in value) return new geometries[value.$geometry.name](...decode(value.$geometry.args));
    if ('$shape' in value) return new Shape().fromJSON(decode(value.$shape));
    if ('$curve' in value) return new curves[value.$curve.type]().fromJSON(decode(value.$curve));
    if ('$coerce' in value) {
        const object = {};
        Object.defineProperty(object, 'valueOf', {configurable:true, get() {
            trace.push(['get:valueOf', value.$coerce, this === object]);
            return function valueOf() { trace.push(['valueOf', value.$coerce, this === object]); return decode(value.value); };
        }});
        return object;
    }
    return Array.isArray(value) ? value.map(decode)
        : Object.fromEntries(Object.entries(value).map(([key,v]) => [key,decode(v)]));
}
function encode(value) {
    if (value === undefined) return ['undefined'];
    if (typeof value === 'number') {
        if (Number.isNaN(value)) return ['number','NaN'];
        if (!Number.isFinite(value)) return ['number',String(value)];
        if (Object.is(value,-0)) return ['number','-0'];
    }
    if (value === null || typeof value !== 'object') return value;
    if (Array.isArray(value)) return value.map(encode);
    return Object.fromEntries(Object.keys(value).map(k => [k,encode(value[k])]));
}
function snapshot(geometry) {
    const attributes = {}, parameters = {};
    for (const name of Object.keys(geometry.attributes).sort()) {
        const a = geometry.attributes[name];
        attributes[name] = {itemSize:a.itemSize, normalized:a.normalized, array:Array.from(a.array, encode)};
    }
    for (const key of Object.keys(geometry.parameters)) {
        const v = geometry.parameters[key];
        if (v === null || typeof v !== 'object' || Array.isArray(v)) parameters[key] = encode(v);
    }
    return {type:geometry.type, index:geometry.index === null ? null : Array.from(geometry.index.array, encode),
        attributes, groups:encode(geometry.groups), parameters};
}
function parameterAlias(value, parameters) {
    return value !== null && typeof value === 'object' ? Object.keys(parameters).find(key => parameters[key] === value) : undefined;
}
function encodeJSON(value, parameters) {
    const alias = parameterAlias(value,parameters);
    if (alias !== undefined) return ['parameter',alias];
    if (value === null || typeof value !== 'object') return encode(value);
    return Array.isArray(value) ? value.map(v => encodeJSON(v,parameters)) : Object.fromEntries(Object.keys(value).map(k => [k,encodeJSON(value[k],parameters)]));
}
function jsonKeyOrder(value, parameters, path = '', out = []) {
    if (value === null || typeof value !== 'object') return out;
    if (parameterAlias(value,parameters) !== undefined) return out;
    out.push([path,Object.keys(value)]);
    for (const key of Object.keys(value)) jsonKeyOrder(value[key],parameters,path+'/'+key,out);
    return out;
}
function run(request) {
    const ctor = geometries[request.name];
    if (request.mode === 'copyNull') {
        trace = [];
        const original = Object.getOwnPropertyDescriptor(BufferGeometry.prototype,'copy');
        BufferGeometry.prototype.copy = function() {trace.push(['base',this === null]);return this;};
        const parameters = {get width() {trace.push('get:width');return 2;}};
        const source = {get parameters() {trace.push(['get:parameters',this === source]);return parameters;}};
        try {
            let error = null;
            try {ctor.prototype.copy.call(null,source);} catch(e) {error = e.name;}
            return {error,trace};
        } finally {Object.defineProperty(BufferGeometry.prototype,'copy',original);}
    }
    if (request.mode === 'metadata') {
        const proto = ctor.prototype, methods = {};
        for (const name of Object.getOwnPropertyNames(proto).filter(k => k !== 'constructor')) {
            const d = Object.getOwnPropertyDescriptor(proto,name), f = d.value;
            methods[name] = {name:f.name, length:f.length, source:f.toString(), flags:[d.writable,d.enumerable,d.configurable]};
        }
        const out = {name:ctor.name, length:ctor.length, source:ctor.toString(), prototypeNames:Object.getOwnPropertyNames(proto), methods};
        if (Object.hasOwn(ctor,'fromJSON')) { const f = ctor.fromJSON; out.fromJSON = {name:f.name,length:f.length,source:f.toString()}; }
        return out;
    }
    trace = [];
    const args = decode(request.args ?? []);
    const data = decode(request.data);
    if (request.mode === 'fromJSONTrace') for (const key of Object.keys(data)) {
        const value = data[key]; Object.defineProperty(data,key,{configurable:true,enumerable:true,get() {trace.push([key,this === data]);return value;}});
    }
    const result = request.mode === 'fromJSON' || request.mode === 'fromJSONTrace' ? ctor.fromJSON(data,decode(request.shapes ?? {})) : new ctor(...args);
    if (request.mode === 'toJSON') {
        result.uuid = 'geometry-under-test'; const json = result.toJSON();
        return {json:encodeJSON(json,result.parameters),keyOrder:jsonKeyOrder(json,result.parameters)};
    }
    if (request.mode === 'copy') {
        const target = new ctor();
        const returnsTarget = target.copy(result) === target;
        return {returnsTarget,geometry:snapshot(target),parametersDistinct:target.parameters !== result.parameters,
            parameterAliases:Object.fromEntries(Object.keys(result.parameters).map(key => [key,target.parameters[key] === result.parameters[key]]))};
    }
    const geometry = snapshot(result);
    if (request.mode === 'fromJSONTrace') return {geometry,trace};
    if (request.mode === 'trace') return {geometry,trace,identity:Object.fromEntries(request.parameterKeys.map((k,i) => [k,result.parameters[k] === args[i]]))};
    return geometry;
}
const requests = [];
const add = (name, args = [], extra = {}) => requests.push({name,args,...extra});
for (const name of Object.keys(geometries)) { add(name,[],{mode:'metadata'}); add(name); }
for (const name of ['PlaneGeometry','ConeGeometry','TubeGeometry','ShapeGeometry','ExtrudeGeometry']) add(name,[],{mode:'copyNull'});
const configs = {
    BoxGeometry:[[2,3,4,2,3,2],['2','3','4',1.9,2.1,2.9],[null,null,null,1,1,1]],
    CapsuleGeometry:[[1,2,4,6],[2,0,2,3]], CircleGeometry:[[2,7,0.3,4],[0,2.4,0,0]],
    ConeGeometry:[[2,3,7,2,true,0.5,4],[0,1,3,1,false]],
    CylinderGeometry:[[1,2,3,7,2,false,0.3,4],[0,1,2,3,1,true]],
    DodecahedronGeometry:[[2,0],[1,1]], IcosahedronGeometry:[[2,0],[1,1]], OctahedronGeometry:[[2,0],[1,1]], TetrahedronGeometry:[[2,0],[1,1]],
    LatheGeometry:[[[{x:0,y:-1},{x:2,y:0},{x:0,y:2}],7,0.4,4], [[],3]],
    PlaneGeometry:[[2,3,2,3],['2','3',2.9,3.1],[0,0,1,1]],
    PolyhedronGeometry:[[[1,1,1,-1,-1,1,-1,1,-1,1,-1,-1],[2,1,0,0,3,2,1,3,0,2,3,1],2,1]],
    RingGeometry:[[1,3,7,2,0.2,4],[0,1,2.5,0.1]],
    SphereGeometry:[[2,7,5,0.2,5,0.3,2],[0,2.1,1.9,0,0,0,0]],
    TorusGeometry:[[2,0.4,5,7,4],[1,0,2.5,3.9]],
    TorusKnotGeometry:[[2,0.3,12,5,2,3],[1,0.1,6,3,3,2]],
    TubeGeometry:[[{ $undefined:true },8,0.4,5,false],[{ $undefined:true },5,0.2,3,true]],
    EdgesGeometry:[[{ $geometry:{name:'BoxGeometry',args:[2,3,4,2,1,1]} },1],[{ $geometry:{name:'IcosahedronGeometry',args:[1,0]} },40]],
    WireframeGeometry:[[{ $geometry:{name:'BoxGeometry',args:[2,3,4]} }],[{ $geometry:{name:'SphereGeometry',args:[1,4,3]} }]]
};
for (const [name, rows] of Object.entries(configs)) for (const args of rows) add(name,args);
const shape = {metadata:{version:4.7,type:'Curve'},arcLengthDivisions:200,type:'Shape',autoClose:false,
    curves:[[[0,0],[3,0]],[[3,0],[3,2]],[[3,2],[0,2]],[[0,2],[0,0]]].map(([v1,v2]) => ({metadata:{version:4.7,type:'Curve'},arcLengthDivisions:200,type:'LineCurve',v1,v2})),currentPoint:[0,0],uuid:'test-shape',holes:[]};
for (const segments of [1,3]) add('ShapeGeometry',[{$shape:shape},segments]);
for (const bevelEnabled of [false,true]) add('ExtrudeGeometry',[{$shape:shape},{steps:2,depth:1,bevelEnabled,bevelSegments:1,curveSegments:2}]);
const holed = structuredClone(shape);
holed.holes = [{...structuredClone(shape),type:'Path',curves:[[[1,0.5],[1,1.5]],[[1,1.5],[2,1.5]],[[2,1.5],[2,0.5]],[[2,0.5],[1,0.5]]].map(([v1,v2]) => ({arcLengthDivisions:200,type:'LineCurve',v1,v2})),currentPoint:[1,0.5]}];
for (const item of [shape,holed]) {
    add('ShapeGeometry',[{$shape:item},2],{mode:'copy'});
    add('ShapeGeometry',[{$shape:item},2],{mode:'toJSON'});
    add('ExtrudeGeometry',[{$shape:item},{steps:2,depth:1,bevelEnabled:false}],{mode:'copy'});
    add('ExtrudeGeometry',[{$shape:item},{steps:2,depth:1,bevelEnabled:false}],{mode:'toJSON'});
    add('ShapeGeometry',[],{mode:'fromJSON',data:{shapes:['test-shape'],curveSegments:2},shapes:{'test-shape':{$shape:item}}});
    add('ExtrudeGeometry',[],{mode:'fromJSON',data:{shapes:['test-shape'],options:{steps:1,depth:2,bevelEnabled:false}},shapes:{'test-shape':{$shape:item}}});
}
add('ShapeGeometry',[[{$shape:shape},{$shape:holed}],2]);
add('ExtrudeGeometry',[[{$shape:shape},{$shape:holed}],{steps:1,depth:1,bevelEnabled:false}]);
const fromJSON = {
    BoxGeometry:{width:2,height:3,depth:4,widthSegments:2,heightSegments:1,depthSegments:2},
    CapsuleGeometry:{radius:1,height:2,length:999,capSegments:3,radialSegments:4,heightSegments:2},
    CircleGeometry:{radius:2,segments:5,thetaStart:0.2,thetaLength:3},
    ConeGeometry:{radius:2,height:3,radialSegments:5,heightSegments:2,openEnded:true,thetaStart:0.2,thetaLength:3},
    CylinderGeometry:{radiusTop:1,radiusBottom:2,height:3,radialSegments:5,heightSegments:2,openEnded:false,thetaStart:0.2,thetaLength:3},
    PlaneGeometry:{width:2,height:3,widthSegments:2,heightSegments:3},
    RingGeometry:{innerRadius:1,outerRadius:2,thetaSegments:5,phiSegments:2,thetaStart:0.2,thetaLength:3},
    SphereGeometry:{radius:2,widthSegments:5,heightSegments:3,phiStart:0.2,phiLength:3,thetaStart:0.3,thetaLength:2},
    TorusGeometry:{radius:2,tube:0.5,radialSegments:4,tubularSegments:5,arc:3},
    TorusKnotGeometry:{radius:2,tube:0.4,tubularSegments:8,radialSegments:4,p:2,q:3},
    LatheGeometry:{points:[{x:0,y:-1},{x:1,y:0},{x:0,y:1}],segments:5,phiStart:0.1,phiLength:4},
};
for (const name of ['DodecahedronGeometry','IcosahedronGeometry','OctahedronGeometry','TetrahedronGeometry']) fromJSON[name] = {radius:2,detail:0};
for (const [name,data] of Object.entries(fromJSON)) {
    add(name,[],{mode:'fromJSON',data}); add(name,[],{mode:'fromJSONTrace',data});
}
for (const [name,rows] of Object.entries(configs)) add(name,rows[0],{mode:'copy'});
for (const [name,rows] of Object.entries(configs)) add(name,rows[0],{mode:'toJSON'});
for (const [name,args,parameterKeys] of [
    ['PlaneGeometry',[2,3,2.9,3.1],['width','height','widthSegments','heightSegments']],
    ['CircleGeometry',[2,5.9,0.3,3],['radius','segments','thetaStart','thetaLength']],
    ['CylinderGeometry',[1,2,3,4.9,2.1,false,0.2,4],['radiusTop','radiusBottom','height','radialSegments','heightSegments','openEnded','thetaStart','thetaLength']],
    ['SphereGeometry',[2,5.9,3.1,0.2,4,0.1,2],['radius','widthSegments','heightSegments','phiStart','phiLength','thetaStart','thetaLength']],
    ['TorusGeometry',[2,0.5,4.9,5.1,4],['radius','tube','radialSegments','tubularSegments','arc']],
]) add(name,args.map((value,i) => typeof value === 'number' ? {$coerce:parameterKeys[i],value} : value),{mode:'trace',parameterKeys});
const selected = requests.filter(r => !filter || r.name.includes(filter));
fs.writeFileSync(path.join(output,'input.json'),JSON.stringify(selected));
const expected = [], actual = [], failures = [];
for (let i = 0; i < selected.length; i++) {
    const request = selected[i];
    try { expected.push(run(request)); }
    catch(e) { throw Error(`Unexpected source failure for ${JSON.stringify(request)}`,{cause:e}); }
    const result = spawnSync(probe, {input:JSON.stringify(request),encoding:'utf8',maxBuffer:32*1024*1024,timeout:120000});
    try { actual.push(JSON.parse(result.stdout.trim().split('\n').at(-1))); }
    catch { actual.push({processError:result.error?.message ?? result.status,stdout:result.stdout,stderr:result.stderr}); }
    try { assert.deepEqual(actual[i],expected[i]); }
    catch(e) { failures.push({index:i,request,message:e.message.slice(0,1400)}); }
}
assert.equal(hash(probe),guards.probe,'Probe changed during differential');
for (const [name,expectedHash] of Object.entries(guards.sources)) assert.equal(hash(path.join(upstream,'src/geometries',`${name}.js`)),expectedHash,'Upstream source changed: '+name);
for (const [name,value] of Object.entries({expected,actual,failures,guards,summary:{upstreamCommit:lock.commit,cases:selected.length,failures:failures.length}})) fs.writeFileSync(path.join(output,`${name}.json`),JSON.stringify(value));
if (failures.length) { console.error(JSON.stringify(failures.slice(0,8),null,2)); throw Error(`Geometry runtime parity: ${failures.length}/${selected.length} mismatches; ${output}`); }
console.log(`Geometry runtime parity passed: ${selected.length} cases; ${output}`);
