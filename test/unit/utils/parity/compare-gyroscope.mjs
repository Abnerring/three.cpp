// Differential check for the native Gyroscope port.
//
// The pinned example imports the bare "three" specifier, so the module source is loaded with that import
// rewritten to the built module of the pinned checkout, which is the same convention the other addon
// comparisons use. Every matrix element is compared with a tight tolerance because both sides run the same
// double precision matrix maths.
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

// Rewrite the bare specifier into the pinned build and import the result.
const sourcePath = path.join(upstream, 'examples/jsm/misc/Gyroscope.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8')
  .replace(/from 'three'/g, `from '${threeUrl}'`);
const moduleUrl = `data:text/javascript;base64,${Buffer.from(rewritten, 'utf8').toString('base64')}`;
const { Gyroscope } = await import(moduleUrl);

const elements = (matrix) => Array.from(matrix.elements);
const compareMatrix = (actual, expected, label) => {
  assert.equal(actual.length, expected.length, `${label} length`);
  for (let i = 0; i < expected.length; i += 1) {
    assert.ok(Math.abs(actual[i] - expected[i]) < 1e-12, `${label}[${i}] ${actual[i]} vs ${expected[i]}`);
  }
};

// Scenario one: no parent.
const standalone = new Gyroscope();
standalone.position.set(1, 2, 3);
standalone.quaternion.setFromEuler(new THREE.Euler(0.3, 0.5, 0.7));
standalone.scale.set(2, 3, 4);
standalone.updateMatrixWorld();

// Scenario two: attached under a rotated, translated parent.
const parent = new THREE.Object3D();
parent.position.set(10, 20, 30);
parent.quaternion.setFromEuler(new THREE.Euler(0.1, 0.2, 0.3));
const gyroscope = new Gyroscope();
gyroscope.position.set(1, 2, 3);
gyroscope.quaternion.setFromEuler(new THREE.Euler(0.4, 0.5, 0.6));
gyroscope.scale.set(1.5, 1.5, 1.5);
parent.add(gyroscope);
parent.updateMatrixWorld(true);

// Scenario three: the forced update reaches the children.
const host = new Gyroscope();
host.position.set(0, 5, 0);
const child = new THREE.Object3D();
child.position.set(0, 0, 2);
host.add(child);
host.updateMatrixWorld(true);

const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
compareMatrix(actual.standaloneMatrix, elements(standalone.matrix), 'standaloneMatrix');
compareMatrix(actual.standaloneWorld, elements(standalone.matrixWorld), 'standaloneWorld');
compareMatrix(actual.parentWorld, elements(parent.matrixWorld), 'parentWorld');
compareMatrix(actual.attachedWorld, elements(gyroscope.matrixWorld), 'attachedWorld');
compareMatrix(actual.childWorld, elements(child.matrixWorld), 'childWorld');
assert.equal(actual.childNeedsUpdate, child.matrixWorldNeedsUpdate);
console.log('Gyroscope parity passed');
