import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

if (process.argv.length !== 4) throw new Error('Usage: node compare-data-utils.mjs <three.js> <probe.exe>');
const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
const T = await import(pathToFileURL(path.join(upstream, 'src/Three.Core.js')));
const { toHalfFloat, fromHalfFloat } = await import(pathToFileURL(path.join(upstream, 'src/extras/DataUtils.js')));
const number = value => Number.isNaN(value) ? 'NaN' : value === Infinity ? 'Infinity' :
    value === -Infinity ? '-Infinity' : Object.is(value, -0) ? '-0' : value;
const values = array => Array.from(array, number);
const state = attribute => ({ array: values(attribute.array),
    readings: Array.from({ length: Math.ceil(attribute.count) }, (_, i) => values([
        attribute.getX(i), attribute.getY(i), attribute.getZ(i), attribute.getW(i), attribute.getComponent(i, 0)])),
    count: number(attribute.count), itemSize: number(attribute.itemSize), type: attribute.array.constructor.name,
    half: attribute.isFloat16BufferAttribute, normalized: attribute.normalized });
const errorName = action => { try { action(); return 'none'; } catch (error) { return error.name; } };
const expected = {};
const oldWarn = console.warn;
console.warn = () => {}; // Range saturation is deliberately exercised below.
expected.decoded = Array.from({ length: 65536 }, (_, bits) => number(fromHalfFloat(bits)));
expected.encodedBoundaries = [];
const float = new Float32Array(1), word = new Uint32Array(float.buffer);
for (let half = 0; half <= 0x7bff; ++half) {
    float[0] = fromHalfFloat(half);
    const bits = word[0];
    for (const delta of [-1, 0, 1]) {
        word[0] = Math.max(0, bits + delta);
        const value = float[0];
        expected.encodedBoundaries.push(toHalfFloat(value), toHalfFloat(-value));
    }
}
expected.converted = [0, -0, 1, -1, Math.PI, -Math.PI, 65504, -65504, 65519, -65519, 1e300, -1e300,
    Infinity, -Infinity, NaN, 2 ** -24, 2 ** -25, -(2 ** -25), 1 + 2 ** -11,
    1 + 2 ** -10 - 2 ** -25, 1 + 2 ** -10 - 2 ** -24, Number.MIN_VALUE].map(T.DataUtils.toHalfFloat);
const nanWords = new BigUint64Array([0x7ff0000000000001n, 0x7ff8000000000000n, 0x7ff8123456789abcn,
    0x7fffffffffffffffn, 0xfff0000000000001n, 0xfff8000000000000n, 0xfff8123456789abcn, 0xffffffffffffffffn]);
const nanSamples = new Float64Array(nanWords.buffer);
expected.nanPayloads = [];
for (let round = 0; round < 4; ++round) {
    expected.nanPayloads.push(Array.from(nanSamples, toHalfFloat));
    for (let i = 0; i < 100000; ++i) toHalfFloat(i % 2 ? nanSamples[i % nanSamples.length] : 1.25);
}
expected.invalid = [-0, -1, -1024, -65536, 65535.9, 65536, 15360.9, 2147483647, -2147483648,
    4294967296 + 15360, -4294967296 + 15360, 9007199254740991, -9007199254740991,
    Infinity, -Infinity, NaN].map(value => number(T.DataUtils.fromHalfFloat(value)));

const raw = new Uint16Array([0x3c00, 0x4000, 0x4200, 0x8000]);
const constructed = new T.Float16BufferAttribute(raw, 2);
expected.constructorAlias = constructed.array === raw;
raw[0] = 0x4400;
expected.constructed = state(constructed);
expected.rawConversions = state(new T.Float16BufferAttribute([1, -1, 65537, 1.9, NaN, Infinity], 2));
expected.typedConversions = state(new T.Float16BufferAttribute(new Float32Array([15360, 16384, 16896]), 3));
expected.lengths = [state(new T.Float16BufferAttribute(3.9, 2)), state(new T.Float16BufferAttribute(NaN, 2))];
expected.lengthErrors = [() => new T.Float16BufferAttribute(-1, 1),
    () => new T.Float16BufferAttribute(Infinity, 1)].map(errorName);
expected.bufferConstructors = [[T.Float16BufferAttribute, Uint16Array],
    [T.Int8BufferAttribute, Int8Array], [T.Uint8BufferAttribute, Uint8Array],
    [T.Int16BufferAttribute, Int16Array], [T.Uint16BufferAttribute, Uint16Array],
    [T.Int32BufferAttribute, Int32Array], [T.Uint32BufferAttribute, Uint32Array],
    [T.Float32BufferAttribute, Float32Array],
    [T.Uint8ClampedBufferAttribute, Uint8ClampedArray]].map(([Attribute, ArrayType]) => {
    const bytes = new ArrayBuffer(16), external = new ArrayType(bytes);
    const first = new Attribute(bytes, 2), second = new Attribute(bytes, 2);
    external[0] = 0x3c00;
    second.setY(0, 2.5);
    const cloned = first.clone();
    first.setX(0, 3.5);
    return {
        identity: [first.array.buffer === bytes, second.array.buffer === bytes,
            first.array === second.array, first.array === external, cloned.array.buffer === bytes],
        metadata: [first.array.byteOffset, first.array.byteLength, first.array.length, first.count],
        readings: values([first.getX(0), second.getX(0), cloned.getX(0), first.getY(0)]),
        raw: values(external),
        lengths: [new Attribute(3.9, 2).count, new Attribute(NaN, 1).count, new Attribute(new ArrayBuffer(0), 1).count],
        errors: [() => new Attribute(-1, 1), () => new Attribute(Infinity, 1),
            () => new Attribute(new ArrayBuffer(3), 1)].map(errorName)
    };
});
const storage = new Uint16Array([0, 0x3c00, 0x4000, 0]), view = storage.subarray(1, 3);
const assigned = new T.Float16BufferAttribute(2, 1);
assigned.array = view;
assigned.name = 'half'; assigned.usage = T.DynamicDrawUsage; assigned.gpuType = T.IntType;
assigned.needsUpdate = true; assigned.addUpdateRange(0, 1);
const clone = assigned.clone();
expected.cloneMetadata = [clone instanceof T.Float16BufferAttribute, clone.name, clone.usage, clone.gpuType,
    clone.version, clone.updateRanges.length, clone.array.buffer === storage.buffer];
assigned.setX(0, 3); clone.setX(1, 4);
expected.sharedView = [values(storage), state(assigned), state(clone)];
const setters = new T.Float16BufferAttribute(8, 4);
expected.setterReturn = setters.setXYZW(0, 1, 2, 3, 4) === setters;
setters.setXY(1, 5, 6).setZ(1, 7).setW(1, 8);
setters.setX(0, -0).setY(0, Infinity).setZ(0, -Infinity).setW(0, NaN);
expected.setters = state(setters);
const normalized = new T.Float16BufferAttribute(8, 4, true);
normalized.setXYZW(0, .5, 1, -0, -.5).setXYZ(1, .25, 0, -1).setW(1, NaN);
expected.normalized = state(normalized);
normalized.setComponent(0, 0, 0x3c00 / 65535);
expected.genericNormalized = state(normalized);
normalized.copyArray(new Uint16Array([0x4000, 0x4200])); normalized.copyAt(1, normalized, 0);
expected.rawCopy = state(normalized);
expected.numericIndices = [0, -1, 2, 2.5].map(itemSize => {
    const attribute = new T.Float16BufferAttribute([0x3c00, 0x4000, 0x4200], itemSize);
    const readings = [number(attribute.count)];
    for (const index of [-1, -0, .25, .5, 1, Infinity, NaN])
        readings.push(number(attribute.getX(index)), number(attribute.getY(index)), number(Number(attribute.getComponent(index, 0))));
    attribute.setX(.25, 9).setY(1, 9).setX(-1, 9);
    return [readings, values(attribute.array)];
});
expected.transforms = Array.from({ length: 5 }, (_, operation) => {
    const attribute = new T.Float16BufferAttribute([0x3c00, 0x4000, 0x4200, 0x4400], operation === 4 ? 2 : 3);
    if (operation === 0) attribute.applyMatrix3(new T.Matrix3().set(2, 1, 3, -1, 3, 2, 0, 2, 4));
    if (operation === 1) attribute.applyMatrix4(new T.Matrix4().makeTranslation(4, -2, 1));
    if (operation === 2) attribute.applyNormalMatrix(new T.Matrix3().set(2, 1, 0, 0, 3, 0, 1, 0, 4));
    if (operation === 3) attribute.transformDirection(new T.Matrix4().makeScale(2, 3, -4));
    if (operation === 4) attribute.applyMatrix3(new T.Matrix3().set(2, 1, 3, -1, 3, 2, 0, 2, 4));
    return state(attribute);
});
const copied = new T.Float16BufferAttribute(0, 1);
copied.copy(new T.Float32BufferAttribute([15360, 16384, 16896], 3, true));
copied.setXYZ(0, 1, 2, 3);
expected.copyOtherType = state(copied);
const capture = action => { try { return action(); } catch (error) { return error.name; } };
expected.reboundTypes = [Int8Array, Uint8Array, Int16Array, Uint16Array, Int32Array, Uint32Array,
    Float32Array, Float64Array, Uint8ClampedArray].flatMap(ArrayType => [false, true].map(normalized => {
    const source = new T.BufferAttribute(new ArrayType([0x3c00, 0x4000, 0x4200, 0x4400]), 4, normalized);
    const destination = new T.Float16BufferAttribute(0, 4);
    destination.copy(source);
    const before = capture(() => state(destination));
    const write = errorName(() => destination.setXYZW(0, .5, -.5, -0, NaN));
    return [before, write, capture(() => state(destination)), values(destination.array)];
}));
expected.serialized = constructed.toJSON();
console.warn = oldWarn;

const actual = JSON.parse(execFileSync(path.resolve(process.argv[3]), [], { encoding: 'utf8', maxBuffer: 16 * 1024 * 1024 }));
let comparisons = 0;
function compare(actual, expected, location = '$') {
    if (expected && typeof expected === 'object') {
        assert.equal(typeof actual, 'object', `${location}: type`);
        assert.deepEqual(Object.keys(actual), Object.keys(expected), `${location}: keys`);
        for (const key of Object.keys(expected)) compare(actual[key], expected[key], `${location}.${key}`);
    } else {
        assert.ok(Object.is(actual, expected), `${location}: ${JSON.stringify(actual)} !== ${JSON.stringify(expected)}`);
        ++comparisons;
    }
}
compare(actual, expected);
console.log(`DataUtils parity: ${Object.keys(expected).length} scenarios, ${comparisons} comparisons (all 65536 half encodings)`);
