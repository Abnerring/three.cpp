// Differential check for the native TranspilerUtils port.
//
// The pinned module has no imports, so it is imported straight from the checkout. The same batteries of statements,
// literals and type names are replayed there and every answer is compared, which pins the regular expression
// semantics of the port against the JavaScript ones.
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const { isExpression, isPrimitive, isBuiltinType, toFloatType } = await import(
  pathToFileURL(path.join(upstream, 'examples/jsm/transpiler/TranspilerUtils.js')).href
);

// isExpression statements, in the same order the probe uses.
const statements = [];
statements.push({});
statements.push({ isExpression: true });
for (const flag of [ 'isFunctionDeclaration', 'isFor', 'isWhile', 'isConditional', 'isSwitch',
  'isStructDefinition' ]) {
  statements.push({ [flag]: true });
}
statements.push({ isFor: 1 });

const values = [ 'true', 'false', '123', '-7', '.5', 'identifier', '', ' true', '0x10', '1e3',
  '-.25', 'falsy' ];
const types = [ 'void', 'bool', 'float', 'int', 'uint', 'mat2', 'mat3', 'mat4', 'mat3x2', 'mat4x4',
  'vec2', 'vec3', 'vec4', 'ivec2', 'uvec4', 'bvec3', 'vec5', 'mat5', 'float2', '', 'int2', 'vec2x2' ];

const expected = {
  statements: statements.map((input) => ({ input, result: isExpression(input) })),
  primitives: values.map((input) => ({ input, result: isPrimitive(input) })),
  builtins: types.map((input) => ({ input, result: isBuiltinType(input) })),
  floats: types.map((input) => ({ input, result: toFloatType(input) }))
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('TranspilerUtils parity passed');
