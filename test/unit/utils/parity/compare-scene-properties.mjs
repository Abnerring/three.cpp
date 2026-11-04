import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const {backgroundBlurriness, backgroundIntensity, backgroundRotation} = await import(pathToFileURL(path.join(upstream, 'src/nodes/accessors/SceneProperties.js')));
const describe = node => ({
  type: node.type,
  nodeType: node.nodeType,
  valueType: node.value?.type ?? null,
  value: node.value?.elements ? {elements: Array.from(node.value.elements)} : node.value,
  updateType: node.updateType,
  group: node.groupNode?.name ?? null,
  hasUpdate: typeof node.update === 'function'
});
const expected = {
  backgroundBlurriness: describe(backgroundBlurriness),
  backgroundIntensity: describe(backgroundIntensity),
  backgroundRotation: describe(backgroundRotation)
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('SceneProperties native/module parity passed');

