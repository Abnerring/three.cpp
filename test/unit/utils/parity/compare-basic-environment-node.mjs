import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: BasicEnvironmentNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/BasicEnvironmentNode.js')).href);
const node = new BasicEnvironmentNode();
const expected = {
  moduleSame: true,
  name: BasicEnvironmentNode.name,
  length: BasicEnvironmentNode.length,
  type: node.type,
  isLightingNode: node.isLightingNode,
  envNodeNull: node.envNode === null,
  source: BasicEnvironmentNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('BasicEnvironmentNode native/module parity passed');
