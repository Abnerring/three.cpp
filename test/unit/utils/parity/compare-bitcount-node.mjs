import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const module = await import(pathToFileURL(path.join(upstream, 'src/nodes/math/BitcountNode.js')));
const {uint} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLCore.js')));
const value = uint(7);
const describe = node => ({
    type: node.type,
    method: node.method,
    isBitcountNode: node.isBitcountNode
});
const expected = {
    trailing: describe(module.countTrailingZeros(value)),
    leading: describe(module.countLeadingZeros(value)),
    one: describe(module.countOneBits(value))
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('BitcountNode native/module parity passed');
