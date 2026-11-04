import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const [upstreamArg, probeArg] = process.argv.slice(2);
assert(upstreamArg && probeArg);
const root = path.resolve(upstreamArg);
const {attributeArray, instancedArray} = await import(pathToFileURL(path.join(root, 'src/nodes/accessors/Arrays.js')));
const describe = node => {
    const value = node.value;
    return {
        type: node.type,
        isStorageBufferNode: node.isStorageBufferNode,
        inputType: node.getInputType(),
        bufferCount: node.bufferCount,
        nodeType: node.nodeType,
        valueStorage: value.isStorageBufferAttribute === true ? 'storage' : 'instanced',
        itemSize: value.itemSize,
        count: value.count,
        arrayType: value.array.constructor.name,
        arrayLength: value.array.length
    };
};
const expected = {
    float: describe(attributeArray(5, 'float')),
    vec3: describe(attributeArray(2, 'vec3')),
    instanced: describe(instancedArray(3, 'vec4'))
};
const actual = JSON.parse(execFileSync(probeArg, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('Arrays native/module parity passed');

