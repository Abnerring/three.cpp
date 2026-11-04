import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const [upstreamArg, probeArg] = process.argv.slice(2);
assert(upstreamArg && probeArg);
const upstream = path.resolve(upstreamArg);
const {oscSine, oscSquare, oscTriangle, oscSawtooth} = await import(pathToFileURL(path.join(upstream, 'src/nodes/utils/Oscillators.js')));
const {float} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')));
const value = float(.25);
const describe = node => ({type: node.type, nodeType: node.nodeType ?? null, isNode: node.isNode});
const expected = {
  oscSine: describe(oscSine(value)),
  oscSquare: describe(oscSquare(value)),
  oscTriangle: describe(oscTriangle(value)),
  oscSawtooth: describe(oscSawtooth(value)),
  default: describe(oscSine())
};
const actual = JSON.parse(execFileSync(probeArg, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('Oscillators native/module parity passed');
