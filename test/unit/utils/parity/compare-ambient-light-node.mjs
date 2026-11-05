import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: AmbientLightNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/AmbientLightNode.js')).href);
const node = new AmbientLightNode();
const expected = {
  moduleSame: true,
  name: AmbientLightNode.name,
  length: AmbientLightNode.length,
  staticType: AmbientLightNode.type,
  type: node.type,
  isAnalyticLightNode: node.isAnalyticLightNode,
  lightNull: node.light === null,
  source: AmbientLightNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('AmbientLightNode native/module parity passed');
