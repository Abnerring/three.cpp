import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: AnalyticLightNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/AnalyticLightNode.js')).href);
const getLightVector = AnalyticLightNode.prototype.getLightVector;
// Only the members this port traces are compared here. The class source itself is a reduced model
// in this port, so comparing the whole class text would be a claim the port does not make.
const expected = {
  moduleSame: true,
  name: AnalyticLightNode.name,
  length: AnalyticLightNode.length,
  staticType: AnalyticLightNode.type,
  hasGetLightVector: typeof getLightVector === 'function',
  getLightVectorLength: getLightVector.length,
  getLightVectorName: getLightVector.name,
  getLightVectorSource: getLightVector.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('AnalyticLightNode getLightVector parity passed');
