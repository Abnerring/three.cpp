// Differential check for the native ColorUtils port.
//
// The pinned module imports the bare "three" specifier, so its source is loaded with that import rewritten to
// the pinned build. Every sample compares the three colour channels with a tiny tolerance, because both sides run
// the same formula but the transcendental functions come from different standard libraries, and the exact hex
// value as well.
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
const sourcePath = path.join(upstream, 'examples/jsm/utils/ColorUtils.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`);
const moduleUrl = `data:text/javascript;base64,${Buffer.from(rewritten, 'utf8').toString('base64')}`;
const { setKelvin } = await import(moduleUrl);

const temperatures = [ 500, 1000, 1500, 1900, 2000, 3000, 6500, 6600, 10000, 20000, 40000, 50000 ];
const expected = temperatures.map((kelvin) => {
  const color = new THREE.Color();
  setKelvin(color, kelvin);
  return { kelvin, r: color.r, g: color.g, b: color.b, hex: color.getHex() };
});

const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' })).samples;
assert.equal(actual.length, expected.length, 'sample count');
for (let i = 0; i < expected.length; i += 1) {
  const a = actual[i];
  const e = expected[i];
  assert.equal(a.kelvin, e.kelvin, `kelvin ${i}`);
  for (const channel of [ 'r', 'g', 'b' ]) {
    assert.ok(Math.abs(a[channel] - e[channel]) < 1e-12, `channel ${channel} at ${e.kelvin}: ${a[channel]} vs ${e[channel]}`);
  }
  assert.equal(a.hex, e.hex, `hex at ${e.kelvin}`);
}
console.log('ColorUtils parity passed');
