import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const module = await import(pathToFileURL(path.join(upstream, 'src/nodes/math/PackFloatNode.js')));
const {vec2, vec4} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLCore.js')));
const describe = node => ({type: node.type, encoding: node.encoding, layout: node.layout, isPackFloatNode: node.isPackFloatNode, nodeType: node.nodeType});
const expected = {
    snorm2: describe(module.packSnorm2x16(vec2(.1, .2))),
    unorm4: describe(module.packUnorm4x8(vec4(.1, .2, .3, .4)))
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('PackFloatNode native/module parity passed');
