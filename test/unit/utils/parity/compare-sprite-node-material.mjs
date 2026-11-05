import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: SpriteNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/SpriteNodeMaterial.js')).href);

const builder = (perspective, object) => ({ object, camera: { isPerspectiveCamera: perspective } });

const material = new SpriteNodeMaterial();
const versionBefore = material.version;
material.sizeAttenuation = true;
const versionAfterSame = material.version;
material.sizeAttenuation = false;
const versionAfterChange = material.version;

const freshMaterial = new SpriteNodeMaterial();
let simpleBuilt = true;
let simpleResult;
try {
  simpleResult = freshMaterial.setupPositionView(builder(false, {}));
} catch {
  simpleBuilt = false;
}

const perspectiveMaterial = new SpriteNodeMaterial();
perspectiveMaterial.sizeAttenuation = false;
let perspectiveBuilt = true;
try {
  perspectiveMaterial.setupPositionView(builder(true, {}));
} catch {
  perspectiveBuilt = false;
}

const expected = {
  name: SpriteNodeMaterial.name,
  length: SpriteNodeMaterial.length,
  staticType: SpriteNodeMaterial.type,
  type: material.type,
  isSpriteNodeMaterial: material.isSpriteNodeMaterial,
  isNodeMaterial: material.isNodeMaterial,
  useSizeAttenuation: material._useSizeAttenuation,
  positionNodeIsNull: material.positionNode === null,
  rotationNodeIsNull: material.rotationNode === null,
  scaleNodeIsNull: material.scaleNode === null,
  transparent: material.transparent,
  setupPositionViewLength: material.setupPositionView.length,
  versionBefore,
  versionAfterSame,
  versionAfterChange,
  sizeAttenuationAfterChange: material.sizeAttenuation,
  simpleBuilt,
  simpleResultIsNode: simpleBuilt ? simpleResult.isNode === true : false,
  perspectiveBuilt,
  source: SpriteNodeMaterial.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('SpriteNodeMaterial parity passed');
