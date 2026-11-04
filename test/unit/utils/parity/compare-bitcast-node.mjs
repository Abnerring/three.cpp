import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);

const module = await import(pathToFileURL(path.join(upstream, 'src/nodes/math/BitcastNode.js')));
const {float} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')));
const value = float(1.5);
const direct = module.floatBitsToInt(value);
const wrapped = module.bitcast(value, 'uint');
const expectedExports = {};
for (const name of ['bitcast', 'floatBitsToInt', 'floatBitsToUint', 'intBitsToFloat', 'uintBitsToFloat'])
    expectedExports[name] = {name: module[name].name, length: module[name].length};
const expected = {
    ctorName: module.default.name,
    ctorLength: module.default.length,
    ctorType: module.default.type,
    exports: expectedExports,
    bitcast: {
        type: wrapped.type,
        nodeType: wrapped.node.type,
        nodeConversion: wrapped.node.conversionType,
        nodeInput: wrapped.node.inputType,
        nodeFlag: wrapped.node.isBitcastNode
    },
    direct: {
        type: direct.type,
        conversion: direct.conversionType,
        input: direct.inputType,
        flag: direct.isBitcastNode,
        valueType: direct.valueNode.type
    }
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('BitcastNode native/module parity passed');
