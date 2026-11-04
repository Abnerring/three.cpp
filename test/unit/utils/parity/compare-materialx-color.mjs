import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const [upstreamArg, probeArg] = process.argv.slice(2);
assert(upstreamArg && probeArg);
const upstream = path.resolve(upstreamArg);
const root = path.join(upstream, 'src/nodes/materialx');
const color = await import(pathToFileURL(path.join(root, 'MaterialXColor.js')));
const transform = await import(pathToFileURL(path.join(root, 'MaterialXColorTransform.js')));
const describe = fn => ({name: fn.name, isFn: fn.isFn, layout: fn.shaderNode.layout});
const expected = {
  hsvtorgb: describe(color.mx_hsvtorgb),
  rgbtohsv: describe(color.mx_rgbtohsv),
  srgb: describe(transform.mx_srgb_texture_to_lin_rec709)
};
const actual = JSON.parse(execFileSync(probeArg, ['--metadata'], {encoding: 'utf8'}));
for (const key of Object.keys(expected)) delete actual[key].source;
assert.deepEqual(actual, expected);
console.log('MaterialX color native/module parity passed');
