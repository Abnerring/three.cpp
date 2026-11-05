// Differential check for the native RectAreaLightHelper port.
//
// The pinned module imports the bare "three" specifier, so its source is loaded with that import rewritten to the
// pinned build. The outline and fill geometries, the material flags, both colour paths and the world matrix after
// an update are compared.
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const threeUrl = pathToFileURL(path.join(upstream, 'build/three.module.js')).href;
const THREE = await import(threeUrl);
const sourcePath = path.join(upstream, 'examples/jsm/helpers/RectAreaLightHelper.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`);
const moduleUrl = `data:text/javascript;base64,${Buffer.from(rewritten, 'utf8').toString('base64')}`;
const { RectAreaLightHelper } = await import(moduleUrl);

const light = new THREE.RectAreaLight(0xff0000, 2, 4, 6);
light.matrixWorld.makeTranslation(1, 2, 3);
const helper = new RectAreaLightHelper(light);
helper.updateMatrixWorld();

const explicitLight = new THREE.RectAreaLight(0xff0000, 2, 4, 6);
const explicitHelper = new RectAreaLightHelper(explicitLight, 0x00ff00);
explicitHelper.updateMatrixWorld();

const attributes = (geometry) => Array.from(geometry.getAttribute('position').array);

const expected = {
  type: helper.type,
  outlinePositions: attributes(helper.geometry),
  fillPositions: attributes(helper.children[0].geometry),
  outlineFog: helper.material.fog,
  fillSide: helper.children[0].material.side,
  fillFog: helper.children[0].material.fog,
  scale: [ helper.scale.x, helper.scale.y, helper.scale.z ],
  outlineColorHex: helper.material.color.getHex(),
  fillColorHex: helper.children[0].material.color.getHex(),
  explicitOutlineColorHex: explicitHelper.material.color.getHex(),
  explicitFillColorHex: explicitHelper.children[0].material.color.getHex(),
  matrixWorld: Array.from(helper.matrixWorld.elements),
  fillMatrixEqualsHelper: helper.children[0].matrixWorld.equals(helper.matrixWorld)
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
for (const key of Object.keys(expected)) {
  const e = expected[key];
  const a = actual[key];
  if (typeof e === 'number') {
    assert.ok(Math.abs(a - e) < 1e-12, `${key}: ${a} vs ${e}`);
  } else if (Array.isArray(e)) {
    assert.equal(a.length, e.length, `${key} length`);
    for (let i = 0; i < e.length; i += 1) {
      assert.ok(Math.abs(a[i] - e[i]) < 1e-12, `${key}[${i}] ${a[i]} vs ${e[i]}`);
    }
  } else {
    assert.deepEqual(a, e, key);
  }
}
console.log('RectAreaLightHelper parity passed');
