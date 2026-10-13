import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';
if (process.argv.length !== 4 || typeof global.gc !== 'function') throw Error('Usage: node --expose-gc compare-webgl-animation-retention.mjs <three.js> <probe>');
const upstream = path.resolve(process.argv[2]), file = 'src/renderers/webgl/WebGLAnimation.js';
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), '11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const source = execFileSync('git', ['-C', upstream, 'show', `HEAD:${file}`]);
assert.equal(createHash('sha256').update(source).digest('hex'), 'ad86bf7da307078fc5ea85573ff3db457b0f53a3df4196a05b2b1810a4afb5b7');
assert.equal(fs.readFileSync(path.join(upstream, file), 'utf8').replaceAll('\r\n', '\n'), source.toString());
const { WebGLAnimation } = await import(pathToFileURL(path.join(upstream, file)));
function prepare(method) {
    const animation = WebGLAnimation(), loop = function() {}, requestId = {}, context = {};
    const oldContext = { requestAnimationFrame() { return requestId; } };
    animation.setAnimationLoop(loop); animation.setContext(oldContext); animation.start(); animation.setContext(context);
    return { method, keep: animation[method], refs: [animation, oldContext, context, loop, requestId].map(v => new WeakRef(v)) };
}
const samples = ['start', 'stop', 'setAnimationLoop', 'setContext'].map(prepare);
async function collect() { for (let i = 0; i < 12; ++i) { await new Promise(setImmediate); global.gc(); } }
const alive = sample => sample.refs.map(ref => ref.deref() !== undefined);
await collect();
const expected = samples.map(sample => ({ method: sample.method, before: alive(sample) }));
for (const sample of samples) sample.keep = null;
await collect();
for (let i = 0; i < samples.length; ++i) expected[i].after = alive(samples[i]);
const actual = JSON.parse(execFileSync(process.argv[3], [], { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('WebGLAnimation retention: 4 actual pinned-source GC cases passed; replaced context, loop and opaque request id.');
