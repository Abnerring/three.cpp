import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const module = await import(pathToFileURL(path.join(upstream, 'src/nodes/math/Packed4x8IntegerNode.js')));
const {uint, uvec4, ivec4} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLCore.js')));
const uintNode = uint(7);
const uvec = uvec4(1, 2, 3, 4);
const ivec = ivec4(-1, 2, -3, 4);
const describe = node => ({type: node.type, method: node.method, isPacked4x8IntegerNode: node.isPacked4x8IntegerNode, inputType: node.getInputType(), nodeType: node.generateNodeType()});
const expected = {
    dotU: describe(module.dot4U8Packed(uintNode, uintNode)),
    dotI: describe(module.dot4I8Packed(uintNode, uintNode)),
    packI: describe(module.pack4xI8(ivec)),
    packU: describe(module.pack4xU8(uvec)),
    packIC: describe(module.pack4xI8Clamp(ivec)),
    packUC: describe(module.pack4xU8Clamp(uvec)),
    unpackI: describe(module.unpack4xI8(uintNode)),
    unpackU: describe(module.unpack4xU8(uintNode))
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('Packed4x8IntegerNode native/module parity passed');
