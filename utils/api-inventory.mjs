// Static API candidates, not a proof of the full JavaScript runtime API.
export const API_LIMITATIONS = [
    'AST parsing establishes syntax only. No API is marked implemented or verified by this inventory.',
    'Inherited members are linked by superclass/import names, not flattened or evaluated.',
    'Star reexports record dependency edges, not a recursively resolved public export namespace.',
    'Properties assigned to this are write sites, not proven unconditional constructor defaults.',
    'Aliases of this and callback binding can mutate instances outside the directly recognized write sites.',
    'Factory calls, proxies, eval, computed members and runtime object mutation require manual or runtime review.',
    'Object member lists describe literal syntax only; spreads, aliases and generated node/TSL interfaces may add members.',
    'Underscore names remain candidates; ECMAScript private names and explicit JSDoc @private are labelled separately.',
    'CommonJS/UMD dependency exports are not resolved as an ESM public interface.'
];

export function walkAst(node, visit, parent = null) {
    if (!node || typeof node !== 'object') return;
    if (typeof node.type === 'string') visit(node, parent);
    for (const [key, value] of Object.entries(node)) {
        if (key === 'loc' || key === 'start' || key === 'end') continue;
        if (Array.isArray(value)) {
            for (const child of value) if (child?.type) walkAst(child, visit, node);
        } else if (value?.type) walkAst(value, visit, node);
    }
}

function expressionName(node) {
    if (!node) return null;
    if (node.type === 'Identifier' || node.type === 'PrivateIdentifier') return node.name;
    if (node.type === 'Literal') return String(node.value);
    if (node.type === 'ThisExpression') return 'this';
    if (node.type === 'MemberExpression') {
        const object = expressionName(node.object);
        const member = !node.computed || node.property.type === 'Literal' ? expressionName(node.property) : null;
        return object && member ? `${object}.${member}` : null;
    }
    return null;
}

function keyInfo(node, computed, source) {
    if (node.type === 'PrivateIdentifier') return { name: `#${node.name}`, resolved: true, private: true };
    if (!computed || node.type === 'Literal') return { name: expressionName(node), resolved: true, private: false };
    if (node.type === 'MemberExpression' && node.object.type === 'Identifier' && node.object.name === 'Symbol' && expressionName(node)) {
        return { name: `[${expressionName(node)}]`, resolved: true, private: false };
    }
    return { name: `[${source.slice(node.start, node.end)}]`, resolved: false, private: false };
}

function bindingNames(pattern) {
    if (!pattern) return [];
    if (pattern.type === 'Identifier') return [pattern.name];
    if (pattern.type === 'RestElement') return bindingNames(pattern.argument);
    if (pattern.type === 'AssignmentPattern') return bindingNames(pattern.left);
    if (pattern.type === 'ArrayPattern') return pattern.elements.flatMap(bindingNames);
    if (pattern.type === 'ObjectPattern') return pattern.properties.flatMap(p => bindingNames(p.type === 'RestElement' ? p.argument : p.value));
    return [];
}

export function extractApi(source, parse) {
    const comments = [];
    let ast;
    let sourceType = 'module';
    let moduleParseError = null;
    try {
        ast = parse(source, { ecmaVersion: 'latest', sourceType, locations: true, onComment: comments, allowHashBang: true });
    } catch (error) {
        moduleParseError = { message: error.message, line: error.loc?.line ?? null, column: error.loc?.column ?? null };
        comments.length = 0;
        sourceType = 'script';
        try {
            ast = parse(source, { ecmaVersion: 'latest', sourceType, locations: true, onComment: comments, allowHashBang: true });
        } catch (scriptError) {
            return {
                extraction: { status: 'parse-error', apiCoverageComplete: false, sourceType: null, errors: [moduleParseError, { message: scriptError.message, line: scriptError.loc?.line ?? null, column: scriptError.loc?.column ?? null }] },
                exports: [], imports: [], classes: [], functions: [], objects: [], assets: [], reviewRequired: []
            };
        }
    }
    const location = node => ({ line: node.loc.start.line, column: node.loc.start.column });
    const text = node => {
        const value = source.slice(node.start, node.end);
        return value.length <= 240 ? value : `${value.slice(0, 240)}… [truncated; review upstream source]`;
    };
    const commentBefore = node => {
        const comment = comments.findLast(c => c.end <= node.start);
        return comment && source.slice(comment.end, node.start).trim() === '' ? comment.value : '';
    };
    const visibility = (node, key = null) => key?.private || /@private\b/.test(commentBefore(node)) ? 'private' : /@protected\b/.test(commentBefore(node)) ? 'protected' : 'public-candidate';
    const reviewRequired = [];
    const reviewKeys = new Set();
    const review = (node, reason) => {
        const key = `${node.start}:${reason}`;
        if (!reviewKeys.has(key)) {
            reviewKeys.add(key);
            reviewRequired.push({ reason, ...location(node) });
        }
    };
    const exports = [];
    const imports = [];
    const assets = [];
    const bindings = new Map();
    for (const statement of ast.body) {
        const declaration = statement.type === 'ExportNamedDeclaration' || statement.type === 'ExportDefaultDeclaration' ? statement.declaration : statement;
        if (!declaration) continue;
        if (declaration.type === 'VariableDeclaration') {
            for (const item of declaration.declarations) {
                for (const name of bindingNames(item.id)) bindings.set(name, { node: item.init, declaration: item, kind: item.init?.type ?? 'uninitialized' });
            }
        } else if (declaration.type === 'ClassDeclaration' || declaration.type === 'FunctionDeclaration') {
            if (declaration.id) bindings.set(declaration.id.name, { node: declaration, declaration, kind: declaration.type });
        }
    }
    for (const statement of ast.body) {
        if (statement.type === 'ImportDeclaration') {
            imports.push({ kind: 'import', source: statement.source.value, bindings: statement.specifiers.map(specifier => ({ local: specifier.local.name, imported: specifier.type === 'ImportDefaultSpecifier' ? 'default' : specifier.type === 'ImportNamespaceSpecifier' ? '*' : expressionName(specifier.imported) })), ...location(statement) });
        } else if (statement.type === 'ExportAllDeclaration') {
            exports.push({ name: statement.exported ? expressionName(statement.exported) : '*', kind: statement.exported ? 'namespace-reexport' : 'star-reexport', source: statement.source.value, local: null, verificationStatus: 'unverified', ...location(statement) });
            imports.push({ kind: 'reexport', source: statement.source.value, bindings: [], ...location(statement) });
        } else if (statement.type === 'ExportNamedDeclaration') {
            const declaration = statement.declaration;
            if (declaration) {
                const names = declaration.type === 'VariableDeclaration' ? declaration.declarations.flatMap(d => bindingNames(d.id)) : declaration.id ? [declaration.id.name] : [];
                for (const name of names) exports.push({ name, local: name, kind: bindings.get(name)?.kind ?? declaration.type, source: null, verificationStatus: 'unverified', ...location(declaration) });
            }
            for (const specifier of statement.specifiers) exports.push({ name: expressionName(specifier.exported), local: expressionName(specifier.local), kind: statement.source ? 'named-reexport' : bindings.get(expressionName(specifier.local))?.kind ?? 'local-reference', source: statement.source?.value ?? null, verificationStatus: 'unverified', ...location(specifier) });
            if (statement.source) imports.push({ kind: 'reexport', source: statement.source.value, bindings: [], ...location(statement) });
        } else if (statement.type === 'ExportDefaultDeclaration') {
            const declaration = statement.declaration;
            const name = declaration.id?.name ?? (declaration.type === 'Identifier' ? declaration.name : null);
            exports.push({ name: 'default', local: name, kind: declaration.type === 'Identifier' ? bindings.get(name)?.kind ?? 'local-reference' : declaration.type, source: null, verificationStatus: 'unverified', ...location(statement) });
        }
    }
    const exportNames = name => exports.filter(e => e.source === null && e.local === name).map(e => e.name);
    const functions = [];
    const objects = [];
    const classes = [];
    const functionInfo = (node, name, owner = null) => ({ name, exportedAs: owner ? [] : exportNames(name), parameters: node.params.map(text), async: node.async, generator: node.generator ?? false, verificationStatus: 'unverified', ...location(node) });
    const objectMembers = node => node.properties.map(property => {
        if (property.type === 'SpreadElement') {
            review(property, 'object-spread-requires-runtime-member-review');
            return { name: null, kind: 'spread', expression: text(property.argument), verificationStatus: 'unverified', ...location(property) };
        }
        const key = keyInfo(property.key, property.computed, source);
        if (!key.resolved) review(property, 'computed-object-member');
        const callable = property.value.type === 'FunctionExpression' || property.value.type === 'ArrowFunctionExpression';
        return { name: key.name, kind: property.kind !== 'init' ? property.kind : callable ? 'method' : 'property', visibility: visibility(property, key), ...(callable ? { parameters: property.value.params.map(text) } : { initializer: text(property.value) }), verificationStatus: 'unverified', ...location(property) };
    });
    for (const [name, binding] of bindings) {
        const node = binding.node;
        if (!node) continue;
        if (node.type === 'FunctionDeclaration' || node.type === 'FunctionExpression' || node.type === 'ArrowFunctionExpression') functions.push({ ...functionInfo(node, name), methods: [], properties: [], thisBinding: 'call-site-dependent' });
        if (node.type === 'ObjectExpression') objects.push({ name, exportedAs: exportNames(name), members: objectMembers(node), verificationStatus: 'unverified', ...location(node) });
        if (exportNames(name).length && (node.type === 'CallExpression' || node.type === 'NewExpression')) review(node, 'exported-factory-or-runtime-object');
    }
    for (const statement of ast.body) {
        if (statement.type !== 'ExportDefaultDeclaration' || statement.declaration.id || statement.declaration.type === 'Identifier') continue;
        const node = statement.declaration;
        if (['FunctionDeclaration', 'FunctionExpression', 'ArrowFunctionExpression'].includes(node.type)) functions.push({ ...functionInfo(node, '<default>'), exportedAs: ['default'], methods: [], properties: [], thisBinding: 'call-site-dependent' });
        if (node.type === 'ObjectExpression') objects.push({ name: '<default>', exportedAs: ['default'], members: objectMembers(node), verificationStatus: 'unverified', ...location(node) });
    }
    const className = (node, parent) => node.id?.name ?? (parent?.type === 'VariableDeclarator' && parent.id.type === 'Identifier' ? parent.id.name : parent?.type === 'ExportDefaultDeclaration' ? '<default>' : `<anonymous@${node.loc.start.line}:${node.loc.start.column}>`);
    const addProperty = (record, node, name, initializer, options = {}) => {
        let property = record.properties.find(p => p.name === name && p.static === (options.static ?? false));
        if (!property) {
            property = { name, static: options.static ?? false, visibility: options.visibility ?? visibility(node), writeSites: [], verificationStatus: 'unverified' };
            record.properties.push(property);
        }
        property.writeSites.push({ kind: options.kind ?? 'assignment', context: options.context ?? 'external-class-mutation', initializer: initializer ? text(initializer) : null, ...location(node) });
        if (initializer && ['FunctionExpression', 'ArrowFunctionExpression'].includes(initializer.type)) {
            record.methods.push({ ...functionInfo(initializer, name, record.name), kind: 'instance-assignment', static: options.static ?? false, visibility: options.visibility ?? visibility(node), ...location(node) });
        }
    };
    const scanThis = (node, record, context, allowRootFunction = false, staticContext = false) => {
        if (!node?.type) return;
        if (node.type === 'ClassDeclaration' || node.type === 'ClassExpression') return;
        if (!allowRootFunction && (node.type === 'FunctionExpression' || node.type === 'FunctionDeclaration')) return;
        if (node.type === 'AssignmentExpression' && node.left.type === 'MemberExpression' && node.left.object.type === 'ThisExpression') {
            const key = keyInfo(node.left.property, node.left.computed, source);
            if (!key.resolved) review(node, 'computed-instance-write');
            addProperty(record, node, key.name, node.right, { context, static: staticContext, visibility: visibility(node, key) });
            if (['FunctionExpression', 'ArrowFunctionExpression'].includes(node.right.type)) scanThis(node.right, record, `assigned-method:${key.name}`, true, staticContext);
        }
        if (node.type === 'CallExpression') {
            const callName = expressionName(node.callee);
            if ((callName === 'Object.defineProperty' || callName === 'Object.defineProperties') && node.arguments[0]?.type === 'ThisExpression') {
                if (callName === 'Object.defineProperty' && node.arguments[1]?.type === 'Literal') addProperty(record, node, String(node.arguments[1].value), node.arguments[2], { kind: 'descriptor', context, static: staticContext });
                else if (callName === 'Object.defineProperties' && node.arguments[1]?.type === 'ObjectExpression') {
                    for (const property of node.arguments[1].properties) {
                        if (property.type === 'SpreadElement') { review(property, 'dynamic-property-descriptor'); continue; }
                        const key = keyInfo(property.key, property.computed, source);
                        if (!key.resolved) review(property, 'dynamic-property-descriptor');
                        addProperty(record, property, key.name, property.value, { kind: 'descriptor', context, static: staticContext, visibility: visibility(property, key) });
                    }
                } else review(node, 'dynamic-property-descriptor');
            }
            if (callName === 'Object.assign' && node.arguments[0]?.type === 'ThisExpression') {
                for (const argument of node.arguments.slice(1)) {
                    if (argument.type !== 'ObjectExpression') { review(argument, 'dynamic-instance-object-assignment'); continue; }
                    for (const property of argument.properties) {
                        if (property.type === 'SpreadElement') { review(property, 'dynamic-instance-object-assignment'); continue; }
                        const key = keyInfo(property.key, property.computed, source);
                        if (!key.resolved) review(property, 'computed-instance-write');
                        addProperty(record, property, key.name, property.value, { kind: 'object-assign', context, static: staticContext, visibility: visibility(property, key) });
                    }
                }
            }
        }
        for (const [key, value] of Object.entries(node)) {
            if (key === 'loc' || key === 'start' || key === 'end') continue;
            if (Array.isArray(value)) {
                for (const child of value) if (child?.type) scanThis(child, record, context, false, staticContext);
            } else if (value?.type) scanThis(value, record, context, false, staticContext);
        }
    };
    for (const record of functions) {
        const binding = bindings.get(record.name)?.node ?? ast.body.find(statement => statement.type === 'ExportDefaultDeclaration')?.declaration;
        scanThis(binding, record, 'function-body', true);
        if (record.properties.length) review(binding, 'function-this-binding-requires-runtime-review');
    }
    walkAst(ast, (node, parent) => {
        if (node.type !== 'ClassDeclaration' && node.type !== 'ClassExpression') return;
        const name = className(node, parent);
        const isModuleBinding = bindings.get(name)?.node === node || parent?.type === 'ExportDefaultDeclaration';
        const record = { name, exportedAs: name === '<default>' ? ['default'] : isModuleBinding ? exportNames(name) : [], declarationScope: isModuleBinding ? 'module' : 'nested', superclass: node.superClass ? text(node.superClass) : null, methods: [], properties: [], verificationStatus: 'unverified', ...location(node) };
        classes.push(record);
        for (const member of node.body.body) {
            if (member.type === 'StaticBlock') { review(member, 'class-static-block'); continue; }
            const key = keyInfo(member.key, member.computed, source);
            if (!key.resolved) review(member, 'computed-class-member');
            if (member.type === 'MethodDefinition') {
                record.methods.push({ ...functionInfo(member.value, key.name, name), kind: member.kind, static: member.static, visibility: visibility(member, key), ...location(member) });
                scanThis(member.value, record, member.kind === 'constructor' ? 'constructor' : `method:${key.name}`, true, member.static);
            } else if (member.type === 'PropertyDefinition') addProperty(record, member, key.name, member.value, { kind: 'class-field', context: 'class-body', static: member.static, visibility: visibility(member, key) });
            else review(member, 'unhandled-class-element');
        }
    });
    const findClass = name => classes.find(c => c.name === name && c.declarationScope === 'module') ?? functions.find(f => f.name === name);
    walkAst(ast, node => {
        if (node.type === 'ImportExpression') {
            if (node.source.type === 'Literal') imports.push({ kind: 'dynamic-import', source: node.source.value, bindings: [], ...location(node) });
            else review(node, 'dynamic-import-target');
        }
        if (node.type === 'NewExpression' && expressionName(node.callee) === 'URL' && node.arguments[0]?.type === 'Literal') assets.push({ source: node.arguments[0].value, ...location(node) });
        if ((node.type === 'NewExpression' || node.type === 'CallExpression') && ['Proxy', 'Function', 'eval'].includes(expressionName(node.callee))) review(node, 'runtime-generated-interface');
        if (node.type === 'CallExpression' && ['Object.assign', 'Object.defineProperty', 'Object.defineProperties', 'Object.setPrototypeOf', 'Reflect.defineProperty'].includes(expressionName(node.callee))) review(node, 'object-mutation-requires-runtime-review');
        if (node.type !== 'AssignmentExpression' || node.left.type !== 'MemberExpression') return;
        const assignedName = expressionName(node.left);
        if (assignedName === 'module.exports' || assignedName?.startsWith('exports.')) review(node, 'commonjs-export-interface-not-resolved');
        const target = node.left.object;
        let record = null;
        let isStatic = false;
        if (target.type === 'Identifier') { record = findClass(target.name); isStatic = true; }
        else if (target.type === 'MemberExpression' && expressionName(target.property) === 'prototype' && target.object.type === 'Identifier') record = findClass(target.object.name);
        if (!record) return;
        const key = keyInfo(node.left.property, node.left.computed, source);
        if (!key.resolved) review(node, 'computed-external-class-mutation');
        const callable = ['FunctionExpression', 'ArrowFunctionExpression'].includes(node.right.type);
        if (callable) record.methods.push({ ...functionInfo(node.right, key.name, record.name), kind: 'external-assignment', static: isStatic, visibility: visibility(node, key), ...location(node) });
        else addProperty(record, node, key.name, node.right, { static: isStatic, visibility: visibility(node, key) });
    });
    if (sourceType === 'script') review(ast, 'script-or-commonjs-export-interface-not-resolved');
    return {
        extraction: { status: reviewRequired.length ? 'partial-static' : 'static-only', apiCoverageComplete: false, sourceType, ...(moduleParseError ? { moduleParseError } : {}) },
        exports, imports, classes, functions, objects, assets, reviewRequired
    };
}
