import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/utils/SpriteUtils.js')).href);
const { billboarding } = mod;

const build = (options) => {
  try {
    return { built: true, result: billboarding(options) };
  } catch (error) {
    return { built: false, failure: String(error && error.message ? error.message : error) };
  }
};

const defaultAttempt = build({});
const verticalAttempt = build({ horizontal: false, vertical: true });
const rotationAttempt = build({ horizontalRotation: true });
// A position override goes through nodeObject, so it is only compared as built or raised.
const positionAttempt = build({ position: { isNode: true } });

const expected = {
  callable: typeof billboarding === 'function',
  isCallable: typeof billboarding === 'function',

  defaultBuilt: defaultAttempt.built,
  defaultIsNode: defaultAttempt.built ? defaultAttempt.result.isNode === true : false,
  defaultType: defaultAttempt.built ? defaultAttempt.result.type : '',
  verticalBuilt: verticalAttempt.built,
  verticalIsNode: verticalAttempt.built ? verticalAttempt.result.isNode === true : false,
  rotationBuilt: rotationAttempt.built,
  rotationIsNode: rotationAttempt.built ? rotationAttempt.result.isNode === true : false,
  positionBuilt: positionAttempt.built
};
// The runtime prints TSL warnings about assigns happening outside a Fn stack, exactly like the pinned code
// does, so the probe output can carry those lines before the JSON payload. Only the last line is the result.
const stdout = execFileSync(path.resolve(probe), { encoding: 'utf8' }).trim();
const actual = JSON.parse(stdout.split('\n').at(-1));
assert.deepEqual(actual, expected);
console.log('SpriteUtils parity passed');
