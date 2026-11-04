import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);

const {time, deltaTime, frameId} = await import(pathToFileURL(path.join(upstream, 'src/nodes/utils/Timer.js')));
const describe = node => ({
    type: node.type,
    isNode: node.isNode,
    isUniformNode: node.isUniformNode,
    nodeType: node.nodeType,
    value: node.value,
    groupType: node.groupNode.type,
    groupName: node.groupNode.name,
    updateType: node.updateType
});
const expected = {time: describe(time), deltaTime: describe(deltaTime), frameId: describe(frameId)};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('Timer native/module parity passed');
