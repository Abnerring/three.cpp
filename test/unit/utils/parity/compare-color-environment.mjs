// Differential check for the native ColorEnvironment port.
//
// The pinned module imports the bare "three" specifier, so its source is loaded with that import rewritten to
// the pinned build. The scene graph, the material parameters and the dispose events are compared as data.
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const threeUrl = pathToFileURL(path.join(upstream, 'build/three.module.js')).href;
const sourcePath = path.join(upstream, 'examples/jsm/environments/ColorEnvironment.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`);
const moduleUrl = `data:text/javascript;base64,${Buffer.from(rewritten, 'utf8').toString('base64')}`;
const { ColorEnvironment } = await import(moduleUrl);

const environment = new ColorEnvironment();
const child = environment.children[0];
let geometryEvents = 0;
let materialEvents = 0;
child.geometry.addEventListener('dispose', () => { geometryEvents += 1; });
child.material.addEventListener('dispose', () => { materialEvents += 1; });
environment.dispose();
environment.dispose();

const coloured = new ColorEnvironment(0x336699);
const colouredChild = coloured.children[0];

const expected = {
  name: environment.name,
  childrenCount: environment.children.length,
  childType: child.type,
  geometryType: child.geometry.type,
  radius: child.geometry.parameters.radius,
  widthSegments: child.geometry.parameters.widthSegments,
  heightSegments: child.geometry.parameters.heightSegments,
  materialType: child.material.type,
  materialSide: child.material.side,
  defaultColorHex: child.material.color.getHex(),
  customColorHex: colouredChild.material.color.getHex(),
  geometryDisposeEvents: geometryEvents,
  materialDisposeEvents: materialEvents,
  isScene: environment.isScene
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('ColorEnvironment parity passed');
