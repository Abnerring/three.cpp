import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const module = await import(pathToFileURL(path.join(upstream, 'src/nodes/math/UnpackFloatNode.js')));
const {uint} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLCore.js')));
const describe = node => ({type: node.type, encoding: node.encoding, layout: node.layout, isUnpackFloatNode: node.isUnpackFloatNode, nodeType: node.nodeType});
const value = uint(42);
const expected = {
    snorm2: describe(module.unpackSnorm2x16(value)),
    unorm4: describe(module.unpackUnorm4x8(value))
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('UnpackFloatNode native/module parity passed');
