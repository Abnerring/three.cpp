import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

const [upstream, probe, requestedOutput] = process.argv.slice(2);
assert(upstream && probe, 'Usage: compare-object-loader-shapes-json.mjs <pinned> <probe|--oracle-only> [fresh-directory]');
const sourceOnly = ['--oracle-only', '--source-only'].includes(probe);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url), 'utf8'));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
const load = relative => import(pathToFileURL(path.join(upstream, 'src', relative)).href);
const names = ['Curve', 'CurvePath', 'Path', 'Shape', 'ArcCurve', 'CatmullRomCurve3', 'CubicBezierCurve', 'CubicBezierCurve3', 'EllipseCurve', 'LineCurve', 'LineCurve3', 'QuadraticBezierCurve', 'QuadraticBezierCurve3', 'SplineCurve'];
const classes = {};
for (const name of names) classes[name] = (await load(`extras/${['Curve', 'CurvePath', 'Path', 'Shape'].includes(name) ? 'core' : 'curves'}/${name}.js`))[name];
const requests = JSON.parse(fs.readFileSync(new URL('./object-loader-shapes-json-fixtures.json', import.meta.url), 'utf8'));

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
function metadata() {
    return { entries: names.map(name => {
        const prototype = classes[name].prototype, method = prototype.toJSON;
        let owner = prototype;
        while (!Object.hasOwn(owner, 'toJSON')) owner = Object.getPrototypeOf(owner);
        const descriptor = Object.getOwnPropertyDescriptor(owner, 'toJSON');
        let constructible;
        try { Reflect.construct(function () {}, [], method); constructible = true; } catch { constructible = false; }
        return { name, own: Object.hasOwn(prototype, 'toJSON'), length: method.length, source: method.toString(), flags: [descriptor.writable, descriptor.enumerable, descriptor.configurable], constructible };
    }) };
}
const getter = (target, key, get) => Object.defineProperty(target, key, { configurable: true, enumerable: true, get });
function observable(request, events) {
    const variant = request.variant, { Curve, LineCurve } = classes;
    let result, token;
    if (['superAlias', 'homeObject', 'constructorIgnored'].includes(variant)) {
        const self = new LineCurve(), prototype = LineCurve.prototype, originalMethod = prototype.toJSON;
        if (variant === 'constructorIgnored') {
            const saved = Object.getOwnPropertyDescriptor(prototype, 'constructor');
            const alternate = {};
            getter(alternate, 'toJSON', function () { throw 'wrong-super'; });
            prototype.constructor = { prototype: Object.create(alternate) };
            try { result = originalMethod.call(self); } finally { Object.defineProperty(prototype, 'constructor', saved); }
        } else {
            token = { marker: 1 };
            const alternate = {};
            getter(alternate, 'toJSON', function () {
                events.push(['get:super', this === self]);
                return function () { events.push(['call:super', this === self]); return token; };
            });
            if (variant === 'homeObject') {
                const saved = Object.getPrototypeOf(prototype);
                Object.setPrototypeOf(prototype, alternate);
                try { result = originalMethod.call(self); } finally { Object.setPrototypeOf(prototype, saved); }
            } else {
                const saved = Object.getOwnPropertyDescriptor(Curve.prototype, 'toJSON');
                Object.defineProperty(Curve.prototype, 'toJSON', Object.getOwnPropertyDescriptor(alternate, 'toJSON'));
                try { result = originalMethod.call(self); } finally { Object.defineProperty(Curve.prototype, 'toJSON', saved); }
            }
        }
    } else if (['superNull', 'superUndefined'].includes(variant)) {
        const self = {}, point = {}, originalMethod = LineCurve.prototype.toJSON;
        const saved = Object.getOwnPropertyDescriptor(Curve.prototype, 'toJSON');
        getter(self, 'v1', () => { events.push('get:v1'); return point; });
        getter(point, 'toArray', () => {
            events.push('get:v1:toArray');
            return function () { events.push('call:v1:toArray'); return [1, 2]; };
        });
        Object.defineProperty(Curve.prototype, 'toJSON', { configurable: true, writable: true, value() { return variant === 'superNull' ? null : undefined; } });
        try { result = originalMethod.call(self); } finally { Object.defineProperty(Curve.prototype, 'toJSON', saved); }
    } else if (['rawGetters', 'getterThrow'].includes(variant)) {
        const self = {};
        if (variant === 'rawGetters') token = { raw: 1 };
        getter(self, 'arcLengthDivisions', () => { events.push('get:arcLengthDivisions'); return variant === 'rawGetters' ? token : 17; });
        getter(self, 'type', () => { events.push('get:type'); if (variant === 'getterThrow') throw 'type'; return token; });
        result = Curve.prototype.toJSON.call(self);
    } else if (['vectorOrder', 'vectorReenter', 'vectorGetThrow'].includes(variant)) {
        const self = { arcLengthDivisions: 7, type: 'LineCurve' };
        for (const [name, array] of [['v1', [1, 2]], ['v2', [3, 4]]]) {
            const point = {};
            getter(self, name, () => { events.push('get:' + name); return point; });
            getter(point, 'toArray', () => {
                events.push('get:' + name + ':toArray');
                if (variant === 'vectorGetThrow' && name === 'v2') throw 'toArray:v2';
                return function () {
                    events.push('call:' + name + ':toArray');
                    if (variant === 'vectorReenter' && name === 'v1') {
                        const inner = Curve.prototype.toJSON.call({ arcLengthDivisions: 9, type: 'inner' });
                        events.push(['reenter', inner.type, inner.arcLengthDivisions]);
                    }
                    return array;
                };
            });
        }
        result = LineCurve.prototype.toJSON.call(self);
    } else if (['pushPoints', 'pushCurves', 'pushHoles', 'pushGetThrow', 'pushReenter', 'lengthThrow', 'childGetThrow'].includes(variant)) {
        const name = variant === 'pushCurves' ? 'CurvePath' : variant === 'pushHoles' ? 'Shape' : 'SplineCurve';
        const key = name === 'CurvePath' ? 'curves' : name === 'Shape' ? 'holes' : 'points';
        const method = name === 'SplineCurve' ? 'toArray' : 'toJSON';
        const constructor = classes[name], parent = Object.getPrototypeOf(constructor.prototype), originalMethod = constructor.prototype.toJSON;
        const originalCurveToJSON = Curve.prototype.toJSON;
        const saved = Object.getOwnPropertyDescriptor(parent, 'toJSON');
        const self = { uuid: 'shape' }; if (name === 'CurvePath') self.autoClose = false;
        token = { marker: 1 };
        Object.defineProperty(token, key, {
            configurable: true,
            get() { events.push('get:result:' + key); return token.saved; },
            set(array) {
                events.push('set:result:' + key);
                Object.defineProperty(array, 'push', { configurable: true, get() {
                    events.push('get:push');
                    if (variant === 'pushGetThrow') throw 'push';
                    return function (...args) { events.push('call:push'); return Array.prototype.push.call(this, ...args); };
                } });
                token.saved = array;
            }
        });
        const child = {};
        getter(child, method, () => {
            events.push('get:child:' + method);
            if (variant === 'childGetThrow') throw 'child';
            return function () {
                events.push('call:child:' + method);
                if (variant === 'pushReenter') {
                    const inner = originalCurveToJSON.call({ arcLengthDivisions: 9, type: 'inner' });
                    events.push(['reenter', inner.type, inner.arcLengthDivisions]);
                }
                return ['child'];
            };
        });
        const list = { 0: child }, length = {};
        Object.defineProperty(length, 'valueOf', { value() { events.push('coerce:length'); if (variant === 'lengthThrow') throw 'length'; return 1; } });
        getter(list, 'length', () => { events.push('get:length'); return length; });
        getter(self, key, () => { events.push('get:input:' + key); return list; });
        Object.defineProperty(parent, 'toJSON', { configurable: true, writable: true, value() { return token; } });
        try { result = originalMethod.call(self); } finally { Object.defineProperty(parent, 'toJSON', saved); }
    } else throw new Error('Unknown observable variant ' + variant);
    return { result, token };
}
function run(request) {
    if (request.op === 'metadata') return metadata();
    const events = [];
    try {
        let value;
        if (request.op === 'serialize') {
            const instance = new classes[request.curve]();
            if (request.json) instance.fromJSON(structuredClone(request.json));
            if (request.curve === 'Shape') instance.uuid = 'shape';
            value = instance.toJSON();
        } else if (request.op === 'observable') value = observable(request, events);
        else throw new Error('Unknown operation ' + request.op);
        return { value: snapshot(value), events };
    } catch (error) {
        return { error: error !== null && ['object', 'function'].includes(typeof error) ? { name: error.name } : { name: 'ThrownValue', raw: error }, events };
    }
}
const expected = requests.map(run);
for (let i = 0; i < requests.length; ++i) assert.deepStrictEqual(expected[i].error, requests[i].expectedError, 'Unexpected pinned result ' + requests[i].case);
const output = requestedOutput ?? path.join('D:/Temp/surui', 'object-loader-shapes-json-' + Date.now());
assert(!fs.existsSync(output), 'Output directory must be fresh'); fs.mkdirSync(output, { recursive: true });
for (const [name, value] of [['requests', requests], ['expected', expected]]) fs.writeFileSync(path.join(output, name + '.json'), JSON.stringify(value, null, 2));
let actual, failures = [];
if (!sourceOnly) {
    actual = JSON.parse(execFileSync(path.resolve(probe), { input: JSON.stringify(requests), encoding: 'utf8', maxBuffer: 64 * 1024 * 1024, timeout: 300000, env: { ...process.env, TEMP: 'D:/Temp/surui', TMP: 'D:/Temp/surui' } }).trim().split(/\r?\n/).at(-1));
    fs.writeFileSync(path.join(output, 'actual.json'), JSON.stringify(actual, null, 2)); assert.equal(actual.length, expected.length);
    for (let i = 0; i < requests.length; ++i) try { assert.deepStrictEqual(actual[i], JSON.parse(JSON.stringify(expected[i]))); } catch (error) { failures.push({ case: requests[i].case, difference: error.message }); }
}
const summary = { cases: requests.length, successes: expected.filter(value => !value.error).length, expectedErrors: expected.filter(value => value.error).length, sourceOnly, failures };
fs.writeFileSync(path.join(output, 'summary.json'), JSON.stringify(summary, null, 2));
process.stdout.write(JSON.stringify({ output, ...summary, failures: failures.length }) + '\n');
assert.equal(failures.length, 0, failures.slice(0, 3).map(row => row.case + '\n' + row.difference).join('\n'));
