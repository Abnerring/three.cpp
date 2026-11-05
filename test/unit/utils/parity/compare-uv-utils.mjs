import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const { replaceDefaultUV, rotateUV, spherizeUV } = await import(pathToFileURL(path.join(upstream, 'src/nodes/utils/UVUtils.js')).href);
const { vec2, float } = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')).href);
const describe = value => ({
  type: value?.type ?? null,
  isFn: !!value?.isFn,
  isNode: !!value?.isNode,
  isContextNode: !!value?.isContextNode,
  source: value?.shaderNode?.jsFunc?.toString?.() ?? ''
});
const uv = vec2(0.25, 0.75);
const rotation = float(0.5);
const expected = {
  moduleKeys: ['replaceDefaultUV', 'rotateUV', 'spherizeUV'],
  replace: describe(replaceDefaultUV),
  rotate: describe(rotateUV),
  spherize: describe(spherizeUV),
  rotateCall: describe(rotateUV(uv, rotation)),
  spherizeCall: describe(spherizeUV(uv, 1.0))
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('UVUtils native/module parity passed');
