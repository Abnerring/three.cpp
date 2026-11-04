import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);

const {default: PointUVNode, pointUV} = await import(pathToFileURL(path.join(upstream, 'src/nodes/accessors/PointUVNode.js')));
const instance = new PointUVNode();
const expected = {
    exports: [PointUVNode.name, pointUV.isNode],
    constructorName: PointUVNode.name,
    constructorLength: PointUVNode.length,
    type: PointUVNode.type,
    nodeType: instance.nodeType,
    isPointUVNode: instance.isPointUVNode,
    isCacheable: instance.isCacheable(),
    generate: instance.generate(),
    pointUVType: pointUV.nodeType,
    pointUVIsPointUVNode: pointUV.isPointUVNode,
    pointUVCacheable: pointUV.isCacheable(),
    pointUVGenerate: pointUV.generate()
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('PointUVNode native/module parity passed');
