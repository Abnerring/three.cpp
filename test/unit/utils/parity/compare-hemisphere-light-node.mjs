import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: HemisphereLightNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/HemisphereLightNode.js')).href);
// The accessors key a weak map on the light, so a plain object stands in for a real light here.
const light = {};
const node = new HemisphereLightNode(light);
const expected = {
  moduleSame: true,
  name: HemisphereLightNode.name,
  length: HemisphereLightNode.length,
  staticType: HemisphereLightNode.type,
  type: node.type,
  isAnalyticLightNode: node.isAnalyticLightNode,
  lightIsGiven: node.light === light,
  positionNodeType: node.lightPositionNode.type,
  positionNodeGroup: node.lightPositionNode.groupNode.name,
  directionIsNode: node.lightDirectionNode.isNode,
  groundNodeType: node.groundColorNode.type,
  groundNodeGroup: node.groundColorNode.groupNode.name,
  source: HemisphereLightNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('HemisphereLightNode native/module parity passed');
