import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const [upstreamArg, probeArg] = process.argv.slice(2);
assert(upstreamArg && probeArg);
const upstream = path.resolve(upstreamArg);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);

const {default: BasicLightMapNode} = await import(pathToFileURL(
  path.join(upstream, 'src/nodes/lighting/BasicLightMapNode.js')
));
const prototype = BasicLightMapNode.prototype;
const metadata = fn => ({name: fn.name, length: fn.length, source: fn.toString()});
const expected = {
  constructor: metadata(BasicLightMapNode),
  prototypeParent: Object.getPrototypeOf(prototype).constructor.name,
  ownStatic: Object.getOwnPropertyNames(BasicLightMapNode),
  ownPrototype: Object.getOwnPropertyNames(prototype),
  setup: metadata(prototype.setup)
};
const actual = JSON.parse(execFileSync(probeArg, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('BasicLightMapNode native/module parity passed');
