import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const [upstream, probe, output] = process.argv.slice(2);
assert.equal(spawnSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).stdout.trim(),
    '11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const oracle = path.join(path.dirname(fileURLToPath(import.meta.url)), 'native-callback-boundary.mjs');
const expected = spawnSync(process.execPath, [oracle, upstream], { encoding: 'utf8', timeout: 30000 });
const actual = spawnSync(probe, [], { encoding: 'utf8', timeout: 30000 });
assert.equal(expected.status, 0, expected.stderr);
assert.equal(actual.status, 0, actual.stderr);
fs.writeFileSync(output, actual.stdout);
fs.writeFileSync(output + '.expected.jsonl', expected.stdout);
const parse = text => text.trim().split(/\r?\n/).map(JSON.parse);
assert.deepEqual(parse(actual.stdout), parse(expected.stdout));
console.log('2 native callback registration/snapshot scenarios match actual three.js');
