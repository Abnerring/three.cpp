import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

const [upstream, probe, requestedOutput] = process.argv.slice(2);
assert(upstream && probe, 'Usage: compare-curve-runtime.mjs <pinned> <probe|--oracle-only> [fresh-directory]');
const sourceOnly = ['--oracle-only', '--source-only'].includes(probe);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url), 'utf8'));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
const load = relative => import(pathToFileURL(path.join(upstream, 'src', relative)).href);
const { Curve } = await load('extras/core/Curve.js');
const { Vector2 } = await load('math/Vector2.js');
const { Vector3 } = await load('math/Vector3.js');
const { getConsoleFunction, setConsoleFunction } = await load('utils.js');
const requests = JSON.parse(fs.readFileSync(new URL('./curve-runtime-fixtures.json', import.meta.url), 'utf8'));
function snapshot(root) {
    const seen = new Map();
    function visit(value) {
        if (value === undefined) return { $undefined: true };
        if (typeof value === 'number' && !Number.isFinite(value)) return { $number: Number.isNaN(value) ? 'NaN' : value < 0 ? '-Infinity' : 'Infinity' };
        if (typeof value === 'function') return { $function: true };
        if (value === null || typeof value !== 'object') return value;
        if (seen.has(value)) return { $ref: seen.get(value) };
        const output = { $id: seen.size + 1, $type: Array.isArray(value) ? 'Array' : 'Object' };
        seen.set(value, output.$id);
        if (Array.isArray(value)) output.items = Array.from({ length: value.length }, (_, i) => visit(value[i]));
        else for (const key of Object.keys(value).sort()) output[key] = visit(value[key]);
        return output;
    }
    return visit(root);
}
const getter = (target, key, get) => Object.defineProperty(target, key, { configurable: true, enumerable: true, get });
function sampledCurve(dim, events) {
    const curve = new Curve(); curve.arcLengthDivisions = 4;
    curve.getPoint = function (t, optionalTarget) {
        events.push(['point', t, optionalTarget !== undefined]);
        return (optionalTarget || new (dim === 2 ? Vector2 : Vector3)()).set(t*t, 2*t, t+1);
    };
    return curve;
}
function metadata() {
    const keys = Reflect.ownKeys(Curve.prototype), methods = [];
    for (const key of keys) if (key !== 'constructor') {
        const fn = Curve.prototype[key], d = Object.getOwnPropertyDescriptor(Curve.prototype, key);
        let constructible;
        try { Reflect.construct(function () {}, [], fn); constructible = true; } catch { constructible = false; }
        methods.push({ key, name: fn.name, length: fn.length, source: fn.toString(), flags: [d.writable, d.enumerable, d.configurable], constructible });
    }
    return { keys, methods };
}
function observable(request, events) {
    const variant = request.variant, proto = Curve.prototype, self = {}; let result;
    if (['cacheHit','cacheMiss','cacheShortCircuit','cacheThrow'].includes(variant)) {
        const cache = [0,1,2];
        getter(self, 'cacheArcLengths', () => { events.push('get:cache'); if (variant === 'cacheThrow') throw 'cache'; return cache; });
        getter(self, 'needsUpdate', () => { events.push('get:needsUpdate'); return false; });
        const divisions = { valueOf() { events.push('coerce:divisions'); return variant === 'cacheMiss' ? 3 : variant === 'cacheShortCircuit' ? '2' : 2; } };
        result = proto.getLengths.call(self, divisions);
    } else if (['stringLengths','pointGetThrow'].includes(variant)) {
        Object.assign(self, { cacheArcLengths: null, needsUpdate: true, arcLengthDivisions: 3 });
        getter(self, 'getPoint', () => {
            events.push('get:getPoint'); if (variant === 'pointGetThrow') throw 'point';
            return function (t) {
                events.push(['call:getPoint', t]); const point = {};
                getter(point, 'distanceTo', () => { events.push('get:distanceTo'); return function () { events.push('call:distanceTo'); return '2'; }; });
                return point;
            };
        }); result = proto.getLengths.call(self);
    } else if (['sampleDivisions','sampleReenter','sampleThrow'].includes(variant)) {
        const divisions = { valueOf() { events.push('coerce:divisions'); return 2; } };
        getter(self, 'getPoint', () => {
            events.push('get:getPoint'); return function (t) {
                events.push(['call:getPoint', t]); if (variant === 'sampleThrow' && t === .5) throw 'sample';
                if (variant === 'sampleReenter' && t === 0) events.push(proto.getPoints.call({ getPoint(t) { return t; } }, 1));
                return t;
            };
        }); result = proto.getPoints.call(self, divisions);
    } else if (['pointAt','pointAtThrow','tangentDispatch'].includes(variant)) {
        self.token = {};
        getter(self, 'getUtoTmapping', () => { events.push('get:mapping'); return function (u) { events.push(['call:mapping', this === self, u]); if (variant === 'pointAtThrow') throw 'mapping'; return this.token; }; });
        getter(self, variant === 'tangentDispatch' ? 'getTangent' : 'getPoint', () => {
            events.push('get:sample'); return function (t, target) { events.push(['call:sample', this === self, t === this.token, target === self]); return target; };
        });
        result = proto[variant === 'tangentDispatch' ? 'getTangentAt' : 'getPointAt'].call(self, .3, self);
        return [result === self, events];
    } else if (['mappingTruthy','mappingZero','mappingStringLength','mappingGetThrow'].includes(variant)) {
        const lengths = { 0: 0, 1: 1, 2: 4, 3: 9 };
        getter(lengths, 'length', () => { events.push('get:length'); return variant === 'mappingStringLength' ? '4' : 4; });
        for (const key of ['0','1','2','3']) { const value = lengths[key]; getter(lengths, key, () => { events.push('get:' + key); if (variant === 'mappingGetThrow') throw 'index'; return value; }); }
        self.getLengths = function () { events.push('call:getLengths'); return lengths; };
        const distance = { valueOf() { events.push('coerce:distance'); return 2; } };
        result = proto.getUtoTmapping.call(self, .5, variant === 'mappingZero' ? 0 : distance);
    } else if (variant === 'copyNull') {
        const source = {}; getter(source, 'arcLengthDivisions', () => { events.push('get:arcLengthDivisions'); return 17; });
        result = proto.copy.call(null, source);
    } else if (variant === 'cloneCustom') {
        getter(self, 'constructor', () => {
            events.push('get:constructor'); return function Replacement() {
                events.push('construct'); const out = {};
                getter(out, 'copy', () => { events.push('get:copy'); return function (source) { events.push(['call:copy', this === out, source === self]); return source; }; }); return out;
            };
        }); result = proto.clone.call(self); return result === self;
    } else if (['ctorDefault','ctorProxy','ctorPrimitive','ctorSetter','ctorThrow'].includes(variant)) {
        let newTarget = Curve, prototype;
        if (variant !== 'ctorDefault') {
            newTarget = function Replacement() {}; prototype = {};
            if (variant === 'ctorSetter') for (const key of ['type','arcLengthDivisions','needsUpdate','cacheArcLengths']) Object.defineProperty(prototype,key,{ configurable:true,set(value) { events.push(['set:'+key,value]); } });
            newTarget.prototype = variant === 'ctorPrimitive' ? 1 : prototype;
            newTarget = new Proxy(newTarget,{ get(target,key,receiver) { if (key === 'prototype') { events.push('get:newTarget:prototype'); if (variant === 'ctorThrow') throw 'prototype'; } return Reflect.get(target,key,receiver); } });
        }
        const instance = Reflect.construct(Curve,[],newTarget);
        result = { instance, prototype: Object.getPrototypeOf(instance) === (variant === 'ctorDefault' ? Curve.prototype : variant === 'ctorPrimitive' ? Object.prototype : prototype) };
    } else if (variant === 'numberConstants') {
        const names=['MAX_VALUE','MIN_VALUE','NaN','NEGATIVE_INFINITY','POSITIVE_INFINITY','MAX_SAFE_INTEGER','MIN_SAFE_INTEGER','EPSILON'];
        const entries=names.map(name=>{const d=Object.getOwnPropertyDescriptor(Number,name),row={name,value:Number[name],flags:[d.writable,d.enumerable,d.configurable]};
            try { Number[name]=1;row.write='success'; } catch(error) { row.write={name:error.name}; } row.delete=Reflect.deleteProperty(Number,name);row.final=Number[name];return row;
        });result={keys:Reflect.ownKeys(Number).filter(key=>names.includes(key)),entries};
    } else if (variant === 'warn') {
        const previous = getConsoleFunction(); setConsoleFunction((...args) => events.push(args));
        try { result = proto.getPoint.call(self, .5); } finally { setConsoleFunction(previous); }
    } else throw new Error('Unknown Curve observable variant ' + variant);
    return result;
}
function run(request) {
    if (request.op === 'metadata') return metadata();
    const events = [];
    try {
        let value;
        if (request.op === 'numeric') {
            const self = sampledCurve(request.dim, events), args = [...request.args]; let target;
            if (request.target) { target = new Vector3(); args.push(target); }
            if (request.method === 'copy') args.push({ arcLengthDivisions: 17 });
            const result = self[request.method](...args);
            value = { result, self: result === self, target: result === target, cache: result === self.cacheArcLengths };
        } else if (request.op === 'cache') {
            const self = sampledCurve(2, events), first = self.getLengths(); first[1] = 'edited'; const second = self.getLengths(); self.needsUpdate = true; const third = self.getLengths();
            value = { same: first === second, fresh: third !== first, first, third, needsUpdate: self.needsUpdate };
        } else value = observable(request, events);
        return { value: snapshot(value), events };
    } catch (error) { return { error: error !== null && ['object','function'].includes(typeof error) ? { name: error.name } : { name: 'ThrownValue', raw: error }, events }; }
}
const expected = requests.map(run);
const expectedError = { cacheMiss: ['TypeError'], cacheShortCircuit: ['TypeError'], cacheThrow: ['ThrownValue','cache'], pointGetThrow: ['ThrownValue','point'], sampleThrow: ['ThrownValue','sample'], pointAtThrow: ['ThrownValue','mapping'], mappingGetThrow: ['ThrownValue','index'], copyNull: ['TypeError'], ctorThrow: ['ThrownValue','prototype'] };
for (let i = 0; i < requests.length; ++i) {
    const e = requests[i].expectedError ?? expectedError[requests[i].case];
    assert.deepStrictEqual(expected[i].error, e && { name: e[0], ...(e.length > 1 ? { raw: e[1] } : {}) }, 'Unexpected pinned result ' + requests[i].case);
}
const output = requestedOutput ?? path.join('D:/Temp/surui', 'curve-runtime-parity-' + Date.now());
assert(!fs.existsSync(output), 'Output directory must be fresh'); fs.mkdirSync(output, { recursive: true });
for (const [name,value] of [['requests',requests],['expected',expected]]) fs.writeFileSync(path.join(output,name+'.json'), JSON.stringify(value,null,2));
function compare(a,b,where='') {
    if (typeof a === 'number' && typeof b === 'number') { assert(Math.abs(a-b) <= 1e-12 * Math.max(1,Math.abs(a),Math.abs(b)), where + ': ' + a + ' != ' + b); return; }
    if (a && b && typeof a === 'object' && typeof b === 'object') {
        assert.deepStrictEqual(Object.keys(a).sort(),Object.keys(b).sort(),where);
        for (const k of Object.keys(a)) compare(a[k],b[k],where+'/'+k); return;
    }
    assert.deepStrictEqual(a,b,where);
}
let actual, failures=[];
if (!sourceOnly) {
    actual=JSON.parse(execFileSync(path.resolve(probe),{input:JSON.stringify(requests),encoding:'utf8',maxBuffer:64*1024*1024,timeout:300000,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}}).trim().split(/\r?\n/).at(-1));
    fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);
    for(let i=0;i<requests.length;++i)try{
        const expectedJSON=JSON.parse(JSON.stringify(expected[i]));
        // In particular MIN_VALUE must remain 5e-324. A numerical tolerance
        // would incorrectly accept zero for the subnormal intrinsic.
        if(requests[i].variant==='numberConstants')assert.deepStrictEqual(actual[i],expectedJSON);
        else compare(actual[i],expectedJSON);
    }catch(error){failures.push({case:requests[i].case,difference:error.message});}
}
const summary={cases:requests.length,successes:expected.filter(v=>!v.error).length,expectedErrors:expected.filter(v=>v.error).length,sourceOnly,failures};
fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2));process.stdout.write(JSON.stringify({output,...summary,failures:failures.length})+'\n');
assert.equal(failures.length,0,failures.slice(0,3).map(row=>row.case+'\n'+row.difference).join('\n'));
