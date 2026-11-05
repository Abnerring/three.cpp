import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: ToonLightingModel } = await import(pathToFileURL(path.join(upstream, 'src/nodes/functions/ToonLightingModel.js')).href);
const { default: PhongLightingModel } = await import(pathToFileURL(path.join(upstream, 'src/nodes/functions/PhongLightingModel.js')).href);
const { vec3, float } = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')).href);

// The same counters and inputs the probe builds. The models are executed, so the differential compares
// how many times each assignment happens rather than only the shape of the classes.
const counter = () => {
  const value = { addAssignCount: 0, mulAssignCount: 0 };
  value.addAssign = () => { value.addAssignCount += 1; };
  value.mulAssign = () => { value.mulAssignCount += 1; };
  return value;
};
const reflected = () => {
  const value = { directDiffuse: counter(), directSpecular: counter(), indirectDiffuse: counter() };
  return value;
};
const directInputs = (target) => ({ lightDirection: vec3(0, 0, 1), lightColor: vec3(1, 1, 1), reflectedLight: target });
const builderWithContext = (target) => ({
  context: { reflectedLight: target, ambientOcclusion: float(1), irradiance: vec3(1, 1, 1) },
  material: {}
});

const toon = new ToonLightingModel();
const toonReflected = reflected();
toon.direct(directInputs(toonReflected), builderWithContext(toonReflected));
toon.indirect(builderWithContext(toonReflected));

const phong = new PhongLightingModel();
const phongReflected = reflected();
phong.direct(directInputs(phongReflected), builderWithContext(phongReflected));
phong.indirect(builderWithContext(phongReflected));

const phongPlain = new PhongLightingModel(false);
const plainReflected = reflected();
phongPlain.direct(directInputs(plainReflected), builderWithContext(plainReflected));

const expected = {
  toonName: ToonLightingModel.name,
  toonDirectLength: toon.direct.length,
  toonIndirectLength: toon.indirect.length,
  toonStartLength: toon.start.length,
  toonFinishLength: toon.finish.length,
  toonDirectDiffuse: toonReflected.directDiffuse.addAssignCount,
  toonIndirectAdd: toonReflected.indirectDiffuse.addAssignCount,
  toonIndirectMul: toonReflected.indirectDiffuse.mulAssignCount,
  phongName: PhongLightingModel.name,
  phongCtorLength: PhongLightingModel.length,
  phongSpecular: phong.specular,
  phongDirectLength: phong.direct.length,
  phongIndirectLength: phong.indirect.length,
  phongDirectDiffuse: phongReflected.directDiffuse.addAssignCount,
  phongDirectSpecular: phongReflected.directSpecular.addAssignCount,
  phongIndirectAdd: phongReflected.indirectDiffuse.addAssignCount,
  phongIndirectMul: phongReflected.indirectDiffuse.mulAssignCount,
  plainSpecular: phongPlain.specular,
  plainSpecularAssigns: plainReflected.directSpecular.addAssignCount,
  toonSource: ToonLightingModel.toString(),
  phongSource: PhongLightingModel.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('ToonLightingModel and PhongLightingModel parity passed');
