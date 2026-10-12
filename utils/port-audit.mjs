import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { auditCheckout, writeAudit } from './port-audit-support.mjs';

const root = fileURLToPath(new URL('../', import.meta.url));
const args = process.argv.slice(2);
const unsupported = args.filter(argument => argument.startsWith('--') && argument !== '--check');
if (unsupported.length) throw new Error(`Unknown audit option: ${unsupported.join(', ')}`);
const positional = args.filter(argument => argument !== '--check');
if (positional.length > 1) throw new Error('Usage: node utils/port-audit.mjs <three.js checkout> [--check]');
const upstream = path.resolve(positional[0] ?? '../three.js');
const checkOnly = args.includes('--check');
const audit = await auditCheckout({ root, upstream });
writeAudit(root, audit, { checkOnly });
console.log(`${audit.report.modules.length} upstream modules: ${JSON.stringify(audit.report.counts)}`);
console.log(`AST: ${audit.api.parser.name} ${audit.api.parser.version}; ${audit.api.counts.parseErrors} parse errors; completion is not inferred from syntax or file presence.`);
