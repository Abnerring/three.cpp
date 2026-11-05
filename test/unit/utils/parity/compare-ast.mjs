// Differential check for the native transpiler AST port.
//
// The pinned module imports a sibling utility by relative path, so the rewritten source is written beside it as a
// temporary file, imported and removed again.
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const dir = path.join(upstream, 'examples/jsm/transpiler');
const tempPath = path.join(dir, '__parity_ast.mjs');
fs.writeFileSync(tempPath, fs.readFileSync(path.join(dir, 'AST.js'), 'utf8'));
let AST;
try {
  AST = await import(pathToFileURL(tempPath).href + '?v=' + Date.now());
} finally {
  fs.rmSync(tempPath, { force: true });
}
const { Number: NumberNode, String: StringNode, Uniform, Operator, Unary, VariableDeclaration, FunctionCall,
  Accessor, SwitchCase, Program } = AST;

const floatOne = new NumberNode(1.0);
const intTwo = new NumberNode(2.0, 'int');
const text = new StringNode('hello');
const vec3Uniform = new Uniform('vec3', 'uColor');
const intUniform = new Uniform('int', 'uCount');

const operators = {};
for (const op of [ '+', '+=', '>>>=', '<' ]) {
  const candidate = new Operator(op, floatOne, intTwo);
  operators[op] = {
    isAssignment: candidate.isAssignment,
    isNumeric: candidate.isNumericExpression,
    type: candidate.getType() ?? null
  };
}
const stringConcat = new Operator('+', floatOne, text);
const typeMismatch = new Operator('+', floatOne, vec3Uniform);

const unaries = {};
for (const op of [ '++', '--', '-' ]) {
  const candidate = new Unary(op, floatOne);
  unaries[op] = {
    isAssignment: candidate.isAssignment,
    isNumeric: candidate.isNumericExpression
  };
}

const bare = new VariableDeclaration('float', 'x');
const withValue = new VariableDeclaration('float', 'y', floatOne);
const chained = new VariableDeclaration('float', 'z', null, bare);

const builtinCall = new FunctionCall('float', [ intTwo ]);
const referencedCall = new FunctionCall('myVec3');
referencedCall.linker.reference = vec3Uniform;
const plainCall = new FunctionCall('myOther');

const accessor = new Accessor('x');
accessor.linker.reference = intUniform;
const plainAccessor = new Accessor('y');

const defaultCase = new SwitchCase([], null);
const conditionalCase = new SwitchCase([], intTwo);

const programStatement = new Uniform('float', 'uA');
const program = new Program([ programStatement ]);

const expected = {
  operators,
  stringConcatNumeric: stringConcat.isNumericExpression,
  typeMismatch: typeMismatch.getType() ?? null,
  unaries,
  bareDeclarationAssignment: bare.isAssignment,
  valueDeclarationAssignment: withValue.isAssignment,
  chainedNextAssignment: chained.isAssignment,
  childHasAssignment: withValue.hasAssignment,
  chainedChildHasAssignment: chained.hasAssignment,
  builtinCallType: builtinCall.getType() ?? null,
  referencedCallType: referencedCall.getType() ?? null,
  plainCallType: plainCall.getType() ?? null,
  accessorType: accessor.getType() ?? null,
  plainAccessorType: plainAccessor.getType() ?? null,
  defaultCaseIsDefault: defaultCase.isDefault,
  conditionalCaseIsDefault: conditionalCase.isDefault,
  programChildIsProgram: programStatement.getProgram() === program,
  numberNumeric: floatOne.isNumericExpression,
  numberType: floatOne.getType() ?? null,
  stringNodeType: text.getType() ?? null
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('AST parity passed');