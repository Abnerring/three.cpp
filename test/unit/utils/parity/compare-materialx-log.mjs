// Differential check for the native MaterialXLog port.
//
// The pinned module has no imports at all, so it is imported straight from the checkout. The code table and the
// collected entries, including the exact filtered arrays for errors and warnings, are compared as data.
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const { MaterialXLogCodes, MaterialXLog } = await import(
  pathToFileURL(path.join(upstream, 'examples/jsm/loaders/materialx/MaterialXLog.js')).href
);

const log = new MaterialXLog();
const emptyErrors = log.errors.length;
const emptyWarnings = log.warnings.length;

log.add(MaterialXLogCodes.UNSUPPORTED_NODE, 'node is unsupported');
log.add(MaterialXLogCodes.IGNORED_SURFACE_INPUT, 'surface input ignored', 'surfaceShader');
log.add(MaterialXLogCodes.TYPE_MISMATCH, 'type mismatch');
log.add(MaterialXLogCodes.MISSING_REFERENCE, 'reference missing');
log.add(MaterialXLogCodes.TEXTURE_LOAD_FAILED, 'texture failed', 'imageNode');

const entryToValue = (entry) => {
  const value = {
    code: entry.code,
    severity: entry.severity,
    message: entry.message,
    hasNodeName: entry.nodeName !== undefined && entry.nodeName !== null
  };
  if (value.hasNodeName) value.nodeName = entry.nodeName;
  return value;
};

const expected = {
  emptyErrors,
  emptyWarnings,
  codes: Object.fromEntries(
    Object.entries(MaterialXLogCodes).map(([ name, code ]) => [ name, { label: code.label, severity: code.severity } ])
  ),
  entryCount: log.entries.length,
  entries: log.entries.map(entryToValue),
  errors: log.errors.map(entryToValue),
  warnings: log.warnings.map(entryToValue)
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('MaterialXLog parity passed');
