import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);

const {default: fn} = await import(pathToFileURL(path.join(upstream, 'src/nodes/functions/material/getRoughness.js')));
const {float} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')));
const result = fn({roughness: float(0.5)});
const expected = {
    name: fn.name,
    isFn: fn.isFn,
    source: fn.shaderNode.jsFunc.toString(),
    type: result.type,
    nodeType: result.node.type
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('getRoughness native/module parity passed');
