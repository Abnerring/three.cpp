import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const module = await import(pathToFileURL(path.join(upstream, 'src/nodes/geometry/RangeNode.js')));
const {float} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')));
const describe = value => ({
    type: value.type,
    salt: value._salt,
    isCacheable: value.isCacheable(),
    minType: value.minNode.type,
    maxType: value.maxNode.type
});
const expected = {
    default: describe(module.range()),
    explicit: describe(module.range(float(1), float(2)))
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('RangeNode native/module parity passed');
