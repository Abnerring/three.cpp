import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const root = path.join(upstream, 'src');
const module = await import(pathToFileURL(path.join(root, 'nodes/accessors/Lights.js')));
const light = {};
const names = Object.keys(module).sort();
const functions = {};
for (const name of names) {
  const fn = module[name];
  functions[name] = {name: fn.name, length: fn.length, resultType: fn(light)?.type ?? null};
}
const expected = {keys: names, functions};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
actual.keys.sort();
assert.deepEqual(actual, expected);
console.log('Lights accessor native/module parity passed');
