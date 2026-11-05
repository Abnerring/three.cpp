import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: DirectionalLightNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/DirectionalLightNode.js')).href);
const node = new DirectionalLightNode();
const expected = {
  moduleSame: true,
  name: DirectionalLightNode.name,
  length: DirectionalLightNode.length,
  staticType: DirectionalLightNode.type,
  type: node.type,
  isAnalyticLightNode: node.isAnalyticLightNode,
  lightNull: node.light === null,
  setupDirectLength: node.setupDirect.length,
  source: DirectionalLightNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('DirectionalLightNode native/module parity passed');
