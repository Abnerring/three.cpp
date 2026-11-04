import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const module = await import(pathToFileURL(path.join(upstream, 'src/nodes/gpgpu/BarrierNode.js')));

const describeNode = node => ({
  type: node.type,
  scope: node.scope,
  isBarrierNode: node.isBarrierNode,
  cacheable: node.isCacheable(),
  methods: Object.fromEntries(['isCacheable', 'setup', 'generate'].map(name => [name, node[name].length]))
});
const describeFunction = functionValue => ({
  length: functionValue.length,
  source: Function.prototype.toString.call(functionValue)
});
const expected = {
  constructorName: module.default.name,
  constructorLength: module.default.length,
  constructorType: module.default.type,
  workgroup: describeNode(new module.default('workgroup')),
  storage: describeNode(new module.default('storage')),
  texture: describeNode(new module.default('texture')),
  workgroupBarrier: describeFunction(module.workgroupBarrier),
  storageBarrier: describeFunction(module.storageBarrier),
  textureBarrier: describeFunction(module.textureBarrier)
};
const actual = JSON.parse(execFileSync(binary, { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('BarrierNode native/module parity passed');

