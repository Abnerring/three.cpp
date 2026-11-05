import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: VolumetricLightingModel } = await import(pathToFileURL(path.join(upstream, 'src/nodes/functions/VolumetricLightingModel.js')).href);

const model = new VolumetricLightingModel();

// direct bails when the light node is not analytic, and when it is analytic but has no distance.
const plainResult = model.direct({ lightNode: { isAnalyticLightNode: false }, lightColor: {} }, {});
const analyticResult = model.direct(
  { lightNode: { isAnalyticLightNode: true, light: {} }, lightColor: {} },
  {}
);

// finish assigns the outgoing ray light once.
const outgoing = { count: 0 };
outgoing.assign = () => { outgoing.count += 1; };
const builder = { context: { outgoingLight: outgoing }, material: {} };
model.finish(builder);

// start builds a ray marching loop that needs a full builder, so it is called under a try on both sides and
// only whether it raised is compared. The port reports the missing loop by raising.
let startThrew = false;
try {
  model.start(builder);
} catch {
  startThrew = true;
}

const expected = {
  name: VolumetricLightingModel.name,
  ctorLength: VolumetricLightingModel.length,
  startLength: model.start.length,
  scatteringLightLength: model.scatteringLight.length,
  directLength: model.direct.length,
  directRectAreaLength: model.directRectArea.length,
  finishLength: model.finish.length,
  indirectLength: model.indirect.length,
  directPlainIsUndefined: plainResult === undefined,
  directAnalyticIsUndefined: analyticResult === undefined,
  finishAssigns: outgoing.count,
  startThrows: startThrew,
  source: VolumetricLightingModel.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);

console.log('VolumetricLightingModel parity passed for the ported surface');
