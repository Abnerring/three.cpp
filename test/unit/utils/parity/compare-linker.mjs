// Differential check for the native transpiler Linker port.
//
// Both pinned modules live in the transpiler directory and AST.js imports a sibling utility by relative path, so the
// two files are copied into temporary modules beside them, imported and removed again.
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const dir = path.join(upstream, 'examples/jsm/transpiler');
const astTemp = path.join(dir, '__parity_linker_ast.mjs');
const linkerTemp = path.join(dir, '__parity_linker_impl.mjs');
fs.writeFileSync(astTemp, fs.readFileSync(path.join(dir, 'AST.js'), 'utf8'));
fs.writeFileSync(linkerTemp, fs.readFileSync(path.join(dir, 'Linker.js'), 'utf8'));
let AST;
let Linker;
try {
  AST = await import(pathToFileURL(astTemp).href + '?v=' + Date.now());
  ({ default: Linker } = await import(pathToFileURL(linkerTemp).href + '?v=' + Date.now()));
} finally {
  fs.rmSync(astTemp, { force: true });
  fs.rmSync(linkerTemp, { force: true });
}
const { Uniform, Accessor, Operator, VariableDeclaration, FunctionParameter, FunctionDeclaration,
  Program } = AST;

const uniform = new Uniform('float', 'uTime');
const read = new Accessor('uTime');
const write = new Accessor('uTime');
const assignment = new Operator('=', write, read);
const declaration = new VariableDeclaration('float', 'localValue', read);
const program = new Program([ uniform, declaration, assignment ]);

const linker = new Linker();
linker.process(program);

const parameter = new FunctionParameter('float', 'p');
const parameterAccess = new Accessor('p');
const functionNode = new FunctionDeclaration('void', 'main', [ parameter ], [ parameterAccess ]);
const functionProgram = new Program([ functionNode ]);
const functionLinker = new Linker();
functionLinker.process(functionProgram);

const expected = {
  uniformAccesses: uniform.linker.accesses.length,
  uniformAssignments: uniform.linker.assignments.length,
  readReferenceIsUniform: read.linker.reference === uniform,
  writeReferenceIsUniform: write.linker.reference === uniform,
  declarationReferenceIsUniform: declaration.linker.reference === null,
  parameterReferenceIsParameter: parameterAccess.linker.reference === parameter,
  parameterAccesses: parameter.linker.accesses.length,
  blockIsNullAfterProcess: linker.block === null,
  functionBlockIsNullAfterProcess: functionLinker.block === null
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('Linker parity passed');