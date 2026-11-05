// Differential check for the native ShadowMesh port.
//
// The pinned module imports the bare "three" specifier, so its source is loaded with that import rewritten to the
// pinned build. The material, the flags and the projection matrix for two source world matrices are compared, the
// matrices element by element.
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
const sourcePath = path.join(upstream, 'examples/jsm/objects/ShadowMesh.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`);
const moduleUrl = `data:text/javascript;base64,${Buffer.from(rewritten, 'utf8').toString('base64')}`;
const { ShadowMesh } = await import(moduleUrl);

const geometry = new THREE.BoxGeometry();
const source = new THREE.Mesh(geometry, new THREE.MeshBasicMaterial());
const shadow = new ShadowMesh(source);

const plane = new THREE.Plane(new THREE.Vector3(0, 1, 0), -2);
const light = new THREE.Vector4(4, 5, 6, 1);
shadow.update(plane, light);
const identityMatrix = Array.from(shadow.matrix.elements);

source.matrixWorld.makeTranslation(1, 2, 3);
shadow.update(plane, light);
const translatedMatrix = Array.from(shadow.matrix.elements);

const expected = {
  sameGeometry: shadow.geometry === source.geometry,
  materialIsBasic: shadow.material.type === 'MeshBasicMaterial',
  materialColor: shadow.material.color.getHex(),
  transparent: shadow.material.transparent,
  opacity: shadow.material.opacity,
  depthWrite: shadow.material.depthWrite,
  stencilWrite: shadow.material.stencilWrite,
  stencilFunc: shadow.material.stencilFunc,
  stencilRef: shadow.material.stencilRef,
  stencilZPass: shadow.material.stencilZPass,
  isShadowMesh: shadow.isShadowMesh,
  frustumCulled: shadow.frustumCulled,
  matrixAutoUpdate: shadow.matrixAutoUpdate,
  referencesSourceMatrix: shadow.meshMatrix === source.matrixWorld,
  identityMatrix,
  translatedMatrix
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
console.log('ShadowMesh parity passed');
