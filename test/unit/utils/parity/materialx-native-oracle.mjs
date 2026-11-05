import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const unstableKeys = new Set(['id', 'uuid', 'stackTrace', '_cacheKey', '_cacheKeyVersion']);
const modules = ['MaterialXNoise', 'MaterialXNodes'];

// The appended export exposes lexical bindings for independent helper checks.
// The bodies, defaults, and Function#toString text are the pinned source itself.
export async function createMaterialXOracle(upstream, fixtures) {
    const load = name => import(pathToFileURL(path.join(upstream, 'src', name + '.js')));
    const [tsl, { default: StackNode }, { default: NodeBuilder }, { expression }] = await Promise.all([
        load('nodes/tsl/TSLBase'), load('nodes/core/StackNode'),
        load('nodes/core/NodeBuilder'), load('nodes/code/ExpressionNode')
    ]);
    const namespaces = {}, declarations = fixtures.declarations, sourceURLs = {};
    for (const name of modules) {
        const filename = path.join(upstream, 'src/nodes/materialx', name + '.js');
        const original = fs.readFileSync(filename, 'utf8');
        const imports = [];
        for (const match of original.matchAll(/import\s*\{([^}]+)\}\s*from\s*'[^']+'/g)) {
            for (const part of match[1].split(',')) {
                const alias = part.trim().split(/\s+as\s+/).at(-1);
                if (alias) imports.push(alias);
            }
        }
        const names = [...new Set([...declarations[name].map(d => d.name), ...imports])];
        let source = original.replace(/from '([^']+)'/g, (_, specifier) => {
            const url = specifier === './MaterialXNoise.js' && sourceURLs.MaterialXNoise
                ? sourceURLs.MaterialXNoise
                : pathToFileURL(path.resolve(path.dirname(filename), specifier)).href;
            return `from '${url}'`;
        });
        source += '\nexport const __private = { ' + names.join(', ') + ' };\n';
        sourceURLs[name] = 'data:text/javascript;base64,' + Buffer.from(source).toString('base64');
        namespaces[name] = await import(sourceURLs[name]);
    }
    const tags = new Map();
    for (const module of modules) {
        for (const declaration of declarations[module]) {
            const name = declaration.name, value = namespaces[module].__private[name];
            if (value?.isFn && !tags.has(value.shaderNode)) tags.set(value.shaderNode, ['shader', module, name]);
        }
    }
    for (const name of ['mx_hsvtorgb', 'mx_rgbtohsv', 'mx_srgb_texture_to_lin_rec709']) {
        const value = namespaces.MaterialXNodes[name];
        if (!tags.has(value.shaderNode)) tags.set(value.shaderNode, ['shader', 'MaterialXNodes', name]);
    }
    function graph(root) {
        const seen = new Map(), objects = [];
        function encode(value) {
            if (value === undefined) return ['undefined'];
            if (typeof value === 'function') return ['function'];
            if (typeof value === 'symbol') return ['symbol'];
            if (typeof value === 'number') {
                if (Number.isNaN(value)) return ['number', 'NaN'];
                if (!Number.isFinite(value)) return ['number', value > 0 ? 'Infinity' : '-Infinity'];
                if (Object.is(value, -0)) return ['number', '-0'];
            }
            if (value === null || typeof value !== 'object') return value;
            if (tags.has(value)) return tags.get(value);
            if (seen.has(value)) return ['ref', seen.get(value)];
            const index = objects.length;
            seen.set(value, index);
            const record = { kind: Array.isArray(value) ? 'array' : 'object', properties: [] };
            objects.push(record);
            if (Array.isArray(value)) record.length = value.length;
            if (value.isNode === true) record.type = value.type;
            for (const key of Object.keys(value).sort()) {
                if (!unstableKeys.has(key)) record.properties.push([key, encode(value[key])]);
            }
            return ['ref', index];
        }
        return { root: encode(root), objects };
    }
    function decode(value) {
        if (value === null || typeof value !== 'object') return value;
        if (Object.hasOwn(value, '$undefined')) return undefined;
        if (Object.hasOwn(value, '$number')) {
            return { NaN, Infinity, '-Infinity': -Infinity, '-0': -0 }[value.$number];
        }
        if (Object.hasOwn(value, 'node')) {
            const result = tsl[value.node](...(value.args ?? []).map(decode));
            return value.variable ? result.toVar(value.variable) : result;
        }
        if (Array.isArray(value)) return value.map(decode);
        return Object.fromEntries(Object.entries(value).map(([key, v]) => [key, decode(v)]));
    }
    function withStack(fn) {
        const previous = tsl.getCurrentStack(), stack = new StackNode();
        tsl.setCurrentStack(stack);
        try { return fn(stack); } finally { tsl.setCurrentStack(previous); }
    }
    function builder() {
        const result = new NodeBuilder(null, { coordinateSystem: 2001, debug: { diagnostics: { keywords: false } } }, null);
        result.shaderStage = 'fragment';
        return result;
    }
    function controls(root, b) {
        const records = [], seen = new Set();
        function walk(value) {
            if (value === null || typeof value !== 'object' || seen.has(value) || tags.has(value)) return;
            seen.add(value);
            let record;
            if (value.type === 'LoopNode') {
                const params = value.params;
                const candidate = params.at(-1);
                const body = candidate.isFn ? candidate.shaderNode.jsFunc : candidate;
                const inputs = {};
                for (let i = 0; i < params.length - 1; ++i) {
                    const param = params[i], name = (param.isNode !== true && param.name) || String.fromCharCode(105 + i);
                    const type = (param.isNode !== true && param.type) || 'int';
                    inputs[name] = expression(name, type);
                }
                record = withStack(stack => ({ kind: 'Loop', source: body.toString(), inputs, result: body.call(undefined, inputs, b), stack: stack.nodes }));
            } else if (typeof value.jsFunc === 'function') {
                const inputs = [];
                record = withStack(stack => ({ kind: 'Shader', source: value.jsFunc.toString(), inputs, result: value.jsFunc.call(undefined, inputs, b), stack: stack.nodes }));
            }
            if (record) records.push(record);
            for (const key of Object.keys(value).sort()) if (!unstableKeys.has(key)) walk(value[key]);
            if (record) walk(record);
        }
        walk(root);
        return records;
    }
    const functionMetadata = value => ({ name: value.name, length: value.length, source: value.toString() });
    function fullFunctionMetadata(value) {
        const data = functionMetadata(value);
        if (value.isFn) {
            const shader = value.shaderNode, callback = shader.jsFunc;
            Object.assign(data, { isFn: value.isFn, nodeType: shader.nodeType, layout: shader.layout,
                callbackName: callback.name, callbackLength: callback.length, callbackSource: callback.toString() });
        }
        return data;
    }
    function metadata(module) {
        const entries = declarations[module].map(declaration => {
            const value = namespaces[module].__private[declaration.name];
            const data = functionMetadata(value);
            if (declaration.kind === 'Fn') {
                const shader = value.shaderNode, callback = shader.jsFunc;
                Object.assign(data, { isFn: value.isFn, nodeType: shader.nodeType, layout: shader.layout,
                    callbackName: callback.name, callbackLength: callback.length, callbackSource: callback.toString() });
            }
            return { name: declaration.name, kind: declaration.kind, public: declaration.exported, metadata: data };
        });
        const root = { exports: Object.keys(namespaces[module]).filter(key => key !== '__private'), entries };
        if (module === 'MaterialXNodes') {
            root.reexports = ['mx_rotate2d', 'mx_rotate3d', 'mx_hsvtorgb', 'mx_rgbtohsv', 'mx_srgb_texture_to_lin_rec709']
                .map(name => ({ name, same: namespaces[module][name] === namespaces[module].__private[name], metadata: fullFunctionMetadata(namespaces[module][name]) }));
        }
        return graph(root);
    }
    function globalOrder(variant) {
        return withStack(stack => {
            const events = [], originalMath = globalThis.Math, originalNumber = globalThis.Number;
            const wrappedMath = Object.create(originalMath), wrappedNumber = Object.create(originalNumber);
            const coercing = ['coerce', 'leftCoerceThrow', 'rightCoerceThrow'].includes(variant);
            function coercion(side, value) {
                const result = {};
                Object.defineProperty(result, 'valueOf', { configurable: true, get() {
                    events.push('get:' + side + ':valueOf');
                    return function () {
                        events.push('call:' + side + ':valueOf');
                        if (variant === side + 'CoerceThrow') throw side + ':valueOf';
                        return value;
                    };
                } });
                return result;
            }
            Object.defineProperty(wrappedMath, 'abs', { configurable: true, get() {
                events.push(['get:abs', this === wrappedMath]);
                if (variant === 'getAbsThrow') throw 'get:abs';
                return function (...args) {
                    events.push(['call:abs', this === wrappedMath, args[0]]);
                    if (variant === 'callAbsThrow') throw 'call:abs';
                    if (variant === 'reenter') {
                        namespaces.MaterialXNodes.mx_add(tsl.float(.2), tsl.float(.3));
                        events.push('reenter');
                    }
                    return coercing ? coercion('left', 1) : originalMath.abs.call(this, args[0]);
                };
            } });
            Object.defineProperty(wrappedNumber, 'EPSILON', { configurable: true, get() {
                events.push(['get:EPSILON', this === wrappedNumber]);
                if (variant === 'epsilonThrow') throw 'get:EPSILON';
                return coercing ? coercion('right', .5) : originalNumber.EPSILON;
            } });
            let result, value;
            globalThis.Math = wrappedMath;
            globalThis.Number = wrappedNumber;
            try {
                value = namespaces.MaterialXNodes.mx_place2d(tsl.vec2(.2, .3), tsl.vec2(.1, .2),
                    tsl.vec2(2, 3), tsl.float(.5), tsl.vec2(.4, .5), 1);
                result = 'success';
            } catch (error) {
                result = ['throw', error];
            } finally {
                globalThis.Math = originalMath;
                globalThis.Number = originalNumber;
            }
            const out = { events, result };
            if (result === 'success') out.graph = graph({ result: value, stack: stack.nodes });
            return out;
        });
    }
    function run(request) {
        try {
            const module = request.module;
            if (request.mode === 'metadata') return metadata(module);
            if (request.mode === 'globalOrder') return globalOrder(request.variant);
            return withStack(stack => {
                const fn = namespaces[module].__private[request.fn], args = (request.args ?? []).map(decode), b = builder();
                if (request.mode === 'overload') {
                    const node = fn(...args), candidate = node.getCandidateFn(b);
                    const candidateName = declarations[module].find(d => namespaces[module].__private[d.name] === candidate)?.name;
                    return graph({ inputs: args, candidate: candidateName, returnType: node.getNodeType(b), setup: node.setup(b), cached: node._candidateFn === candidate });
                }
                const result = request.kind === 'Fn' ? fn.shaderNode.jsFunc(args, b) : fn(...args);
                const root = { inputs: args, result, stack: stack.nodes };
                root.controls = controls(root, b);
                if (request.build) {
                    const steps = [];
                    for (const stage of ['setup', 'analyze', 'generate']) {
                        b.buildStage = stage;
                        steps.push(result.build(b, request.output ?? 'float'));
                    }
                    root.build = { steps, flow: b.flow };
                }
                return graph(root);
            });
        } catch (error) {
            if (!(error instanceof Error)) return { thrown: error };
            return { error: { name: error.name } };
        }
    }
    return { run, graph, namespaces };
}

if (process.argv[1] && import.meta.url === pathToFileURL(path.resolve(process.argv[1])).href) {
    const [upstream, requestArg] = process.argv.slice(2);
    const fixtures = JSON.parse(fs.readFileSync(new URL('./materialx-native-fixtures.json', import.meta.url)));
    const oracle = await createMaterialXOracle(path.resolve(upstream), fixtures);
    const request = JSON.parse(requestArg ?? fs.readFileSync(0, 'utf8'));
    process.stdout.write(JSON.stringify(oracle.run(request)));
}
