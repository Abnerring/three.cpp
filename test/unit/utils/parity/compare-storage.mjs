import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
const T = await import(pathToFileURL(path.join(upstream, 'src/Three.Core.js')));
const values = array => Array.from(array, number => Number.isNaN(number) ? 'NaN' :
    number === Infinity ? 'Infinity' : number === -Infinity ? '-Infinity' :
    Object.is(number, -0) ? '-0' : number);
const expected = {};
const array = new Float32Array([1, 2, 3, 4]), alias = array;
const view = array.subarray(1, -1), copy = view.slice();
view[0] = 9;
expected.identity = [alias === array, view === array, view.buffer === array.buffer, copy.buffer === array.buffer];
expected.metadata = [view.byteOffset, view.byteLength, view.length, Float32Array.BYTES_PER_ELEMENT];
expected.indexConversions = [new Float32Array(2.9).length, new Float32Array(NaN).length,
    new Uint8Array(array.buffer, 1.9, 2.9).byteOffset, new Uint8Array(array.buffer, 1.9, 2.9).length];
expected.source = values(array); expected.copy = values(copy);
expected.emptyRanges = [array.subarray(3, 1).length, array.subarray(-99, 99).length,
    array.subarray(NaN).length, array.subarray(Infinity).length];
expected.missing = [array[-1] === undefined, array[.5] === undefined, array[4] === undefined, array[-0]];
const overlap = new Uint8Array([1, 2, 3, 4, 5]); overlap.set(overlap.subarray(0, 4), 1);
expected.overlap = values(overlap);
expected.int8 = values(new Int8Array([257, -129, 127.9, -128.9, 3.9, -3.9, Infinity, NaN]));
expected.clamped = values(new Uint8ClampedArray([-.5, .5, 1.5, 2.5, 3.5, 254.5, 255.5, NaN, 300, -300]));
expected.float32 = values(new Float32Array([.1, -0, 1e100, -1e100, NaN]));
const largest = (2 - 2 ** -23) * 2 ** 127, overflow = (2 - 2 ** -24) * 2 ** 127;
expected.float32Overflow = values(new Float32Array([largest + 2 ** 100, overflow, -(largest + 2 ** 100), -overflow]));
const nanBits = new Uint32Array([0x7f800001, 0x80000000]), nanCopy = new Float32Array(2);
nanCopy.set(new Float32Array(nanBits.buffer)); expected.rawCopy = values(new Uint32Array(nanCopy.buffer));
expected.constructConversions = [values(new T.Float32BufferAttribute(new Uint8Array([1, 2, 3]), 3).array),
    values(new T.Uint8ClampedBufferAttribute([.5, 1.5, 300], 1).array)];
const bytes = new ArrayBuffer(8), words = new Uint32Array(bytes), floats = new Float32Array(bytes);
words[0] = 0x3f800000; floats[1] = -2; expected.reinterpret = [floats[0], words[1]];
const direct = new T.BufferAttribute(view, 1), typed = new T.Float32BufferAttribute(view, 1), cloned = direct.clone();
expected.attributeIdentity = [direct.array === view, typed.array === view, cloned.array === view];
view[0] = 12; expected.attributeValues = [direct.getX(0), typed.getX(0), cloned.getX(0)];
const geometry = new T.BufferGeometry();
geometry.setAttribute('position', new T.Float32BufferAttribute([1, 2, 3], 3));
geometry.setIndex(new T.Uint16BufferAttribute([0, 2], 1)); geometry.index.count = 1;
expected.nonIndexed = values(geometry.toNonIndexed().getAttribute('position').array);
geometry.setAttribute('position', new T.Uint8ClampedBufferAttribute([1, 2, 3], 3));
const expandedClamped = geometry.toNonIndexed().getAttribute('position');
expected.nonIndexedClamped = [expandedClamped.array.constructor.name, values(expandedClamped.array)];
expected.numericAttributes = [0, -0, -1, 2.5, 3, Infinity, -Infinity, NaN].map(itemSize => {
    const attribute = new T.Float32BufferAttribute([1, 2, 3, 4], itemSize);
    const readings = [itemSize, attribute.count];
    for (const index of [-1, -0, .5, 1 / 3, 1, Infinity, NaN])
        readings.push(Number(attribute.getX(index)), Number(attribute.getComponent(index, 1.5)));
    attribute.setX(.5, 21).setComponent(-1, 4, 22).setY(-0, 23);
    return [values(readings), values(attribute.array)];
});
expected.partialTransforms = ['applyMatrix3', 'applyMatrix4', 'applyNormalMatrix', 'transformDirection'].map((name, i) => {
    const attribute = new T.Float32BufferAttribute([1, 2, 3, 4], 3);
    attribute[name](i % 2 ? new T.Matrix4() : new T.Matrix3()); return values(attribute.array);
});
const copyAt = new T.Float32BufferAttribute([1, 2, 3, 4, 5, 6], 2.5);
copyAt.copyAt(.4, copyAt, 0); expected.fractionalCopyAt = values(copyAt.array);
geometry.setAttribute('position', new T.Float32BufferAttribute([1, 2, 3, 4, 5], 2.5));
geometry.setIndex(new T.Uint16BufferAttribute([0, 1], 1));
expected.fractionalExpansion = values(geometry.toNonIndexed().getAttribute('position').array);
expected.partialNormals = [false, true].map(indexed => {
    const normals = new T.BufferGeometry();
    normals.setAttribute('position', new T.Float32BufferAttribute([0,0,0, 1,0,0, 0,1,0, 4], 3));
    if (indexed) normals.setIndex(new T.Uint16BufferAttribute([0,1,2,3], 1));
    normals.computeVertexNormals(); return values(normals.getAttribute('normal').array);
});
class CallbackAttribute extends T.Float32BufferAttribute {
    getX(i) { this.onRead?.('x', i); return super.getX(i); }
    getY(i) { this.onRead?.('y', i); return super.getY(i); }
    getZ(i) { this.onRead?.('z', i); return super.getZ(i); }
    getW(i) { this.onRead?.('w', i); return super.getW(i); }
    setX() { throw new Error('Unexpected delegated setX'); }
}
expected.transformCallbacks = ['applyMatrix3', 'applyMatrix4', 'applyNormalMatrix', 'transformDirection'].map((name, i) => {
    const attribute = new CallbackAttribute([1, 2, 3, 4, 5, 6], 3);
    const matrix = i % 2 ? new T.Matrix4() : new T.Matrix3(), trace = [];
    attribute.onRead = (component, index) => {
        trace.push(component + index);
        if (component === 'x') { attribute.count = 0; matrix.elements[0] = 2; }
    };
    attribute[name](matrix);
    return [trace, values(attribute.array), attribute.count];
});
const callbackVector = new CallbackAttribute([1, 2, 3, 4], 1), vectorTrace = [];
callbackVector.onRead = component => vectorTrace.push(component);
const vector2 = new T.Vector2().fromBufferAttribute(callbackVector, 1);
const vector3 = new T.Vector3().fromBufferAttribute(callbackVector, 0);
const vector4 = new T.Vector4().fromBufferAttribute(callbackVector, 0);
const color = new T.Color().fromBufferAttribute(callbackVector, 0);
expected.attributeReadOrder = [vectorTrace, values(vector2.toArray()), values(vector3.toArray()), values(vector4.toArray()), values(color.toArray())];
const outerTransform = new CallbackAttribute([1, 2, 3], 3), innerTransform = new T.Float32BufferAttribute([10, 20, 30], 3);
outerTransform.onRead = component => {
    if (component === 'y') innerTransform.applyMatrix4(new T.Matrix4().makeTranslation(1, 0, 0));
};
outerTransform.applyMatrix4(new T.Matrix4());
expected.transformReentry = [values(outerTransform.array), values(innerTransform.array)];
const lifecycle = new T.Float32BufferAttribute([1, 2, 3], 1);
expected.defaultUploadIdentity = lifecycle.onUploadCallback === direct.onUploadCallback;
expected.needsUpdateStrictTrue = [undefined, null, false, 0, 1, 'true', {}, true, true].map(value => {
    lifecycle.needsUpdate = value; return lifecycle.version;
});
expected.numberVersions = values([.25, -2, 2 ** 53, Infinity, -Infinity, NaN].map(version => {
    lifecycle.version = version; lifecycle.needsUpdate = true; return lifecycle.version;
}));
const rangeArray = lifecycle.updateRanges;
lifecycle.addUpdateRange(.5, -2.25); lifecycle.addUpdateRange(NaN, Infinity);
const rangeObject = rangeArray[0];
expected.numericRanges = values([rangeObject.start, rangeObject.count, rangeArray[1].start, rangeArray[1].count]);
const rangeClone = lifecycle.clone(); lifecycle.clearUpdateRanges(); rangeObject.count = 8;
expected.rangeLifecycle = [rangeArray === lifecycle.updateRanges, rangeArray.length === 0,
    rangeClone.updateRanges !== rangeArray, rangeClone.updateRanges.length === 0, rangeObject.count];
const uploadStates = [];
lifecycle.onUpload((() => { let state = 0; return () => uploadStates.push(++state); })());
const uploadAlias = lifecycle.onUploadCallback;
uploadAlias(); lifecycle.onUploadCallback(); uploadAlias();
expected.uploadIdentity = [uploadAlias === lifecycle.onUploadCallback, uploadStates];
const numericRanges = new T.BufferGeometry();
const defaultRange = values([numericRanges.drawRange.start, numericRanges.drawRange.count]);
numericRanges.addGroup(.5, 11.75, -.25); numericRanges.addGroup(NaN, Infinity, -Infinity);
numericRanges.setDrawRange(-.75, Infinity);
const copiedRanges = numericRanges.clone();
expected.geometryRanges = [defaultRange, values([numericRanges.groups[0].start, numericRanges.groups[0].count,
    numericRanges.groups[0].materialIndex, numericRanges.groups[1].start, numericRanges.groups[1].count,
    numericRanges.groups[1].materialIndex, copiedRanges.drawRange.start, copiedRanges.drawRange.count,
    copiedRanges.groups[0].start, copiedRanges.groups[0].count])];
const errorName = action => { try { action(); return 'none'; } catch (error) { return error.name; } };
expected.errors = [() => new Float32Array(bytes, 1), () => new Float32Array(bytes, 4, 2),
    () => new Float32Array(new ArrayBuffer(3)), () => array.set([1], -1), () => array.set([1, 2], 3),
    () => array.set([], Infinity), () => new ArrayBuffer(-1), () => new Float32Array(-1),
    () => new Float32Array(Infinity)].map(errorName);
const actual = JSON.parse(execFileSync(path.resolve(process.argv[3]), [], { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log(`Storage parity: ${Object.keys(expected).length} scenarios`);
