import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const {default: RotateNode, rotate} = await import(pathToFileURL(path.join(upstream, 'src/nodes/utils/RotateNode.js')));
const {vec2, vec3, float} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')));
const describe = node => ({
  type: node.type,
  nodeType: node.nodeType ?? null,
  positionType: node.positionNode?.type ?? null,
  rotationType: node.rotationNode?.type ?? null,
  order: node.getOrder(),
  cacheKey: node.customCacheKey()
});
const position2 = vec2(.25, .5);
const angle = float(.75);
const position3 = vec3(.25, .5, .75);
const rotation3 = vec3(.1, .2, .3);
const changed = rotate(position3, rotation3);
changed.setOrder('ZXY');
const expected = {
  constructor2: describe(new RotateNode(position2, angle)),
  factory2: describe(rotate(position2, angle)),
  constructor3: describe(new RotateNode(position3, rotation3, 'YZX')),
  changed: describe(changed)
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('RotateNode native/module parity passed');
