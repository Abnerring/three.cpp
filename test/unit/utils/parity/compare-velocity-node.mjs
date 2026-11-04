import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const {default: VelocityNode, velocity} = await import(pathToFileURL(path.join(upstream, 'src/nodes/accessors/VelocityNode.js')));
const describe = node => {
  const uniforms = {};
  for (const key of ['previousModelWorldMatrix','currentProjectionMatrix','previousProjectionMatrix','previousCameraViewMatrix']) {
    const value = node[key];
    uniforms[key] = {type: value.type, group: value.groupNode.name};
  }
  return {type: node.type, nodeType: node.nodeType, projectionMatrix: node.projectionMatrix, updateType: node.updateType, updateAfterType: node.updateAfterType, uniforms};
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, {node: describe(new VelocityNode()), velocity: describe(velocity)});
console.log('VelocityNode native/module parity passed');
