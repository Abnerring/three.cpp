// Differential check for the native OBJExporter mesh path.
//
// The pinned module imports the bare "three" specifier, so its source is loaded with that import rewritten to the
// pinned build. The whole obj document is compared byte for byte, which also pins the number formatting and the
// face index packing.
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
const sourcePath = path.join(upstream, 'examples/jsm/exporters/OBJExporter.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`);
const moduleUrl = `data:text/javascript;base64,${Buffer.from(rewritten, 'utf8').toString('base64')}`;
const { OBJExporter } = await import(moduleUrl);

const geometry = new THREE.BoxGeometry();
const material = new THREE.MeshBasicMaterial();
material.name = 'mat';
const mesh = new THREE.Mesh(geometry, material);
mesh.name = 'box';
mesh.matrixWorld.makeTranslation(1, 2, 3);
const scene = new THREE.Scene();
scene.add(mesh);

const obj = new OBJExporter().parse(scene);
const expected = { obj, lineCount: (obj.match(/\n/g) ?? []).length };
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('OBJExporter parity passed');
