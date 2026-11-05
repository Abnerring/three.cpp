import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: PointsNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/PointsNodeMaterial.js')).href);

const material = new PointsNodeMaterial();
const versionBefore = material.version;
material.alphaToCoverage = false;
const versionAfterFirst = material.version;
material.alphaToCoverage = false;
const versionAfterSame = material.version;
material.alphaToCoverage = true;
const versionAfterChange = material.version;

let positionBuilt = true;
let position;
try {
  position = material.setupPositionView();
} catch {
  positionBuilt = false;
}

// Both setupVertex branches are only recorded as built or raised: the base implementation needs a full
// builder, so neither side is expected to run it against a stub.
let pointsBranchBuilt = true;
try {
  material.setupVertex({ object: { isPoints: true }, material: {} });
} catch {
  pointsBranchBuilt = false;
}

let spriteBranchBuilt = true;
try {
  material.setupVertex({ object: { isPoints: false }, material: {} });
} catch {
  spriteBranchBuilt = false;
}

const expected = {
  name: PointsNodeMaterial.name,
  length: PointsNodeMaterial.length,
  staticType: PointsNodeMaterial.type,
  type: material.type,
  isPointsNodeMaterial: material.isPointsNodeMaterial,
  isSpriteNodeMaterial: material.isSpriteNodeMaterial,
  isNodeMaterial: material.isNodeMaterial,
  sizeNodeIsNull: material.sizeNode === null,
  positionNodeIsNull: material.positionNode === null,
  size: material.size,
  sizeAttenuation: material.sizeAttenuation,
  setupPositionViewLength: material.setupPositionView.length,
  setupVertexLength: material.setupVertex.length,
  setupVertexSpriteLength: material.setupVertexSprite.length,
  versionBefore,
  versionAfterFirst,
  versionAfterSame,
  versionAfterChange,
  alphaToCoverageFlag: material._useAlphaToCoverage,
  positionBuilt,
  positionIsNode: positionBuilt ? position.isNode === true : false,
  pointsBranchBuilt,
  spriteBranchBuilt,
  source: PointsNodeMaterial.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('PointsNodeMaterial parity passed');
