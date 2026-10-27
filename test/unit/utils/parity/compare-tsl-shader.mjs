import { execFileSync, spawnSync } from 'node:child_process';
import { mkdirSync, writeFileSync } from 'node:fs';
import { pathToFileURL } from 'node:url';
import path from 'node:path';
import assert from 'node:assert/strict';

const [ upstream, executable, outputDirectory ] = process.argv.slice( 2 );
const isolated = process.argv.includes( '--isolated' );
if ( ! upstream || ! executable ) throw Error( 'compare-tsl-shader.mjs <pinned-three.js> <probe> [output-directory]' );
assert.equal( execFileSync( 'git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' } ).trim(), '11738a9d55e502fb484cefffcea3a66dc44e9fe9' );
const load = file => import( pathToFileURL( path.join( upstream, 'src', file ) ) );
const TSL = await load( 'nodes/tsl/TSLCore.js' );
const { default: Node } = await load( 'nodes/core/Node.js' );
let nodeBuilderModule = isolated ? undefined : await load( 'nodes/core/NodeBuilder.js' );
const { setConsoleFunction } = await load( 'utils.js' );
const requests = [], undef = { tag: 'undefined' };
const node = ( name, type = 'float' ) => ( { tag: 'node', name, type } );
for ( const layout of [ null, undef, 'float', 'void', '', false, 7, {}, { a: 'float', return: 'vec2' }, { inputs: [], type: 'bool', name: 'full' }, { inputs: null }, { inputs: false }, { inputs: 'not-object' } ] ) requests.push( { kind: 'metadata', layout } );
for ( const layout of [ null, 'float', 'void', { a: 'float', return: 'vec3' }, { inputs: [] } ] ) for ( const params of [ [], [ 1 ], [ true, 2 ], [ node( 'a' ) ], [ { a: 1, b: node( 'b' ) } ] ] ) requests.push( { kind: 'fn', layout, params } );
for ( const newLayout of [ {}, { return: 'float', b: 'vec3', a: 'int' }, { inputs: null }, { inputs: [] }, { inputs: [ { name: 'foo', type: 'vec2' } ] }, { inputs: false }, { inputs: 'x' } ] ) requests.push( { kind: 'metadata', layout: 'vec3', newLayout, once: true, subBuilds: [ 'a', 'b' ] } );
for ( const type of [ undef, null, 'float', 'void', 0, false ] ) for ( const params of [ null, undef, [], [ 1, 2 ], [ { color: 2 } ] ] ) requests.push( { kind: 'shader', type, params } );
for ( const arity of [ 0, 1, 2, 3 ] ) for ( const params of [ null, [], [ node( 'a' ), node( 'b' ) ], [ { x: 2, '0': 7, '1': node( 'n' ) } ] ] ) requests.push( { kind: 'call', arity, params, reads: params?.length ? [ 'x', 'missing', '0', 'other', 'length' ] : [], spread: true, subBuild: '', calls: 1 } );
for ( const cached of [ null, false, 0, '', 9, { existing: true } ] ) requests.push( { kind: 'call', params: [], arity: 1, cached, calls: 2, once: true, subBuild: 'sample' } );
for ( const once of [ false, true ] ) for ( const subBuild of [ '', 'x', null, 0 ] ) requests.push( { kind: 'call', params: [], arity: 1, calls: 2, once, subBuild } );
for ( const params of [ null, [], [ node( 'a' ), node( 'b' ), node( 'extra' ) ], [ { a: node( 'a' ), b: node( 'b' ), extra: node( 'unused' ) } ], [ { a: 1, b: true } ] ] ) for ( const once of [ false, true ] ) requests.push( { kind: 'layoutCall', params, layout: { name: 'test', inputs: [ { name: 'a', type: 'float' }, { name: 'b', type: 'float' } ] }, once, calls: 2, subBuild: '' } );
requests.push( { kind: 'stack' }, { kind: 'stack', nullStack: true } );
for ( const layout of [ null, 'float', 'void' ] ) for ( const once of [ false, true ] ) requests.push( { kind: 'integration', layout, once, subBuilds: null } );
for ( const params of [ null, [], [ node( 'a' ) ], [ { x: 3 } ] ] ) requests.push( { kind: 'call', params, arity: 2, throwBody: true, subBuild: 'failed' } );
for ( const builderGenerator of [ false, true ] ) requests.push( { kind: 'call', params: [ node( 'a' ), node( 'b' ) ], arity: 2, generator: true, builderGenerator } );
for ( const throwKey of [ null, 'x', '0' ] ) requests.push( { kind: 'call', params: [ { tag: 'getters', fields: { x: node( 'x' ), '0': node( 'zero' ) }, throwKey } ], arity: 2, reads: [ 'x', 'missing', '0', 'x' ] } );
for ( const name of [ 'Fn', 'ShaderNode', 'Stack' ] ) for ( const mode of [ 'object', 'null', 'newTarget' ] ) requests.push( { kind: 'exportConstructor', name, mode } );

function run( request ) {
    const state = { events: [] }, out = {};
    TSL.setCurrentStack( null );
    const normal = ( value, depth = 0 ) => {
        if ( value === undefined ) return '#undefined';
        if ( typeof value === 'function' ) return '#function';
        if ( typeof value === 'symbol' ) return '#symbol';
        if ( typeof value === 'number' ) { if ( Number.isNaN( value ) ) return '#NaN'; if ( ! Number.isFinite( value ) ) return value > 0 ? '#Infinity' : '#-Infinity'; if ( Object.is( value, -0 ) ) return '#-0'; }
        if ( value === null || typeof value !== 'object' ) return value;
        for ( const key of [ 'builder', 'output', 'shader', 'call', 'function', 'stack' ] ) if ( value === state[ key ] ) return `#${ key }`;
        if ( depth > 5 ) return '#depth';
        const result = Array.isArray( value ) ? new Array( value.length ) : {};
        if ( value.isNode === true ) {
            result.kind = value.constructor.name;
            for ( const key of [ 'nodeType', 'value', 'rawInputs', 'isFn', 'isShaderCallNodeInternal', 'layout', 'once', 'subBuilds' ] ) if ( Object.hasOwn( value, key ) ) result[ key ] = normal( value[ key ], depth + 1 );
            return result;
        }
        for ( const key of Object.keys( value ) ) result[ key ] = normal( value[ key ], depth + 1 );
        return result;
    };
    const event = ( ...values ) => state.events.push( values.map( value => normal( value ) ) );
    const decode = spec => {
        if ( spec === null || typeof spec !== 'object' ) return spec;
        if ( Array.isArray( spec ) ) return spec.map( decode );
        if ( spec.tag === 'undefined' ) return undefined;
        if ( spec.tag === 'node' ) { const node = new Node( spec.type ); node.fixture = spec.name; node.build = function ( ...args ) { event( 'inputBuild', this.fixture, args ); return this.fixture; }; return node; }
        if ( spec.tag === 'getters' ) {
            const object = {}; for ( const [ key, raw ] of Object.entries( spec.fields ) ) { const value = decode( raw ); Object.defineProperty( object, key, { enumerable: true, configurable: true, get() { event( 'getter', key, this === object ); if ( spec.throwKey === key ) throw TypeError( 'getter' ); return value; } } ); } return object;
        }
        return Object.fromEntries( Object.entries( spec ).map( ( [ key, value ] ) => [ key, decode( value ) ] ) );
    };
    setConsoleFunction( level => event( 'console', level ) );
    try {
        const body = ( ...args ) => {
            event( 'body', args.length );
            const inputs = args[ 0 ], builder = args[ args.length > 1 ? 1 : 0 ];
            for ( const key of request.reads ?? [] ) event( 'read', key, inputs[ key ] );
            if ( request.spread ) event( 'builderIterable', [ ...builder ] );
            if ( request.generator ) {
                const iterable = request.builderGenerator ? builder : inputs, func = iterable[ Symbol.iterator ], iterator = func.call( iterable );
                event( 'generatorMeta', func.name, func.length, [] ); event( 'generatorKeys', Object.getOwnPropertyNames( iterator ) );
                const next = iterator.next; try { next.call( {} ); event( 'borrowed', 'ok' ); } catch ( error ) { event( 'borrowed', error.name ); }
                event( 'next', next.call( iterator ) ); event( 'return', iterator.return( 23 ) ); event( 'after', next.call( iterator ) );
            }
            if ( request.throwBody ) throw TypeError( 'body' );
            return state.output;
        };
        Object.defineProperty( body, 'length', { value: request.arity ?? 0 } );
        state.output = new Node( 'float' );
        const layout = Object.hasOwn( request, 'layout' ) ? decode( request.layout ) : null;
        if ( request.kind === 'exportConstructor' ) {
            const fn = TSL[ request.name ], original = fn.prototype;
            const descriptor = Object.getOwnPropertyDescriptor( fn, 'prototype' );
            out.descriptor = { writable: descriptor.writable, enumerable: descriptor.enumerable, configurable: descriptor.configurable };
            out.originalConstructor = original.constructor === fn;
            const replacement = request.mode === 'null' ? null : { fixture: 'replacement' };
            try {
                fn.prototype = replacement; out.replaced = fn.prototype === replacement;
                let newTarget = fn;
                if ( request.mode === 'newTarget' ) newTarget = new Proxy( function () {}, { get( target, key, receiver ) { if ( key === 'prototype' ) { event( 'newTarget.prototype' ); return replacement; } return Reflect.get( target, key, receiver ); } } );
                const value = Reflect.construct( fn, request.name === 'Stack' ? [ 7 ] : [ body ], newTarget );
                out.callable = typeof value === 'function'; out.constructed = typeof value === 'object' || typeof value === 'function';
                out.usesReplacement = Object.getPrototypeOf( value ) === replacement;
                out.usesObjectPrototype = Object.getPrototypeOf( value ) === Object.prototype;
                out.isNode = value.isNode === true;
            } finally { fn.prototype = original; }
        } else if ( request.kind === 'metadata' || request.kind === 'fn' ) {
            const fn = state.function = TSL.Fn( body, layout ), shader = state.shader = fn.shaderNode;
            out.callable = typeof fn === 'function'; out.isNode = fn.isNode; out.isFn = fn.isFn;
            out.keys = Object.getOwnPropertyNames( fn ); out.nodeType = normal( shader.nodeType ); out.layout = normal( shader.layout );
            out.cacheable = fn.isCacheable(); out.shaderCacheable = shader.isCacheable();
            if ( Object.hasOwn( request, 'newLayout' ) ) { out.setLayoutSelf = fn.setLayout( decode( request.newLayout ) ) === fn; out.newLayout = normal( shader.layout ); }
            if ( request.once ) { out.onceSelf = fn.once( decode( request.subBuilds ) ) === fn; out.once = shader.once; out.subBuilds = normal( shader.subBuilds ); }
            if ( request.kind === 'fn' ) { const call = state.call = fn( ...request.params.map( decode ) ); out.rawInputs = normal( call.rawInputs ); out.sameShader = call.shaderNode === shader; out.brand = call.isShaderCallNodeInternal; }
        } else if ( [ 'shader', 'call', 'layoutCall' ].includes( request.kind ) ) {
            const shader = state.shader = TSL.ShaderNode( body, Object.hasOwn( request, 'type' ) ? decode( request.type ) : undefined );
            out.nodeType = normal( shader.nodeType ); out.global = shader.global; out.once = shader.once; out.layout = shader.layout;
            const raw = Object.hasOwn( request, 'params' ) ? decode( request.params ) : null;
            const call = state.call = shader.call( raw );
            out.rawSame = call.rawInputs === raw; out.sameShader = call.shaderNode === shader;
            if ( request.kind !== 'shader' ) {
                const properties = {}, builder = state.builder = { subBuildFn: 'oldSub', fnCall: 'oldCall', fixture: 7 };
                builder.getNodeProperties = node => { event( 'getNodeProperties', node ); return properties; };
                builder.getClosestSubBuild = arg => { event( 'getClosestSubBuild', arg ); return request.subBuild; };
                if ( Object.hasOwn( request, 'cached' ) ) properties[ request.subBuild || 'default' ] = decode( request.cached );
                if ( request.once ) shader.once = true;
                if ( request.kind === 'layoutCall' ) {
                    shader.setLayout( layout );
                    const func = state.function = { call( ...args ) { event( 'functionCall', args ); return state.output; } };
                    builder.buildFunctionNode = node => { event( 'buildFunctionNode', node ); return func; };
                    builder.addInclude = node => { event( 'addInclude', node ); };
                }
                out.results = []; for ( let i = 0; i < ( request.calls ?? 1 ); ++ i ) out.results.push( normal( call.call( builder ) ) );
                out.subBuildFn = builder.subBuildFn; out.fnCall = normal( builder.fnCall ); out.properties = normal( properties );
            }
        } else if ( request.kind === 'integration' ) {
            state.output = TSL.float( 7 );
            const { default: NodeBuilder } = nodeBuilderModule;
            const builder = state.builder = new NodeBuilder( null, { debug: { diagnostics: { keywords: false } } }, null ); builder.shaderStage = 'fragment';
            const fn = TSL.Fn( body, layout ); if ( request.once ) fn.once( request.subBuilds );
            const node = state.call = fn(); out.values = [];
            for ( const stage of [ 'setup', 'analyze', 'generate' ] ) { builder.buildStage = stage; out.values.push( normal( node.build( builder, 'float' ) ) ); }
            out.flow = normal( builder.flow ); out.stackRestored = TSL.getCurrentStack() === null;
        } else if ( request.kind === 'stack' ) {
            const stack = state.stack = {}; for ( const key of [ 'addToStack', 'If', 'Switch' ] ) stack[ key ] = function ( ...args ) { event( key, this, args ); return 'result'; };
            TSL.setCurrentStack( request.nullStack ? null : stack ); out.sameStack = TSL.getCurrentStack() === stack;
            for ( const key of [ 'Stack', 'If', 'Switch' ] ) out[ key ] = normal( TSL[ key ]( state.output, 7 ) );
        }
    } catch ( error ) { out.error = error.name; }
    finally { TSL.setCurrentStack( null ); setConsoleFunction( null ); }
    if ( Object.hasOwn( out, 'error' ) && state.builder ) { out.partialSubBuildFn = normal( state.builder.subBuildFn ); out.partialFnCall = normal( state.builder.fnCall ); }
    out.events = state.events;
    return out;
}

const expected = [];
for ( const request of requests ) {
    // Keep module-only TSL behavior separate from NodeBuilder's wider import
    // graph. The eager mode remains available as a distinct fidelity check.
    if ( request.kind === 'integration' && ! nodeBuilderModule ) nodeBuilderModule = await load( 'nodes/core/NodeBuilder.js' );
    expected.push( run( request ) );
}
if ( executable === '--oracle-only' ) {
    if ( ! outputDirectory ) throw Error( 'oracle-only requires an output directory' );
    mkdirSync( outputDirectory, { recursive: true } );
    writeFileSync( path.join( outputDirectory, 'requests.json' ), JSON.stringify( requests, null, 2 ) + '\n' );
    writeFileSync( path.join( outputDirectory, 'expected.json' ), JSON.stringify( expected, null, 2 ) + '\n' );
    console.log( `${ requests.length } pinned upstream TSL shader oracle rows saved` ); process.exit( 0 );
}
const native = spawnSync( executable, isolated ? [] : [ '--eager-builder-import' ], { input: JSON.stringify( requests ), encoding: 'utf8', maxBuffer: 64 * 1024 * 1024, env: { ...process.env, TEMP: 'D:\\Temp\\surui', TMP: 'D:\\Temp\\surui' } } );
assert.equal( native.status, 0, native.stderr );
const actual = JSON.parse( native.stdout );
// Both snapshots pass through JSON. Own properties holding undefined disappear
// during that transport in the same way as the native probe's JSON output.
const comparableExpected = JSON.parse( JSON.stringify( expected ) );
const differences = [];
for ( let i = 0; i < requests.length; ++ i ) { try { assert.deepStrictEqual( actual[ i ], comparableExpected[ i ] ); } catch { differences.push( { index: i, request: requests[ i ], expected: expected[ i ], actual: actual[ i ] } ); } }
if ( outputDirectory ) { mkdirSync( outputDirectory, { recursive: true } ); for ( const [ name, value ] of Object.entries( { requests, expected, actual, differences } ) ) writeFileSync( path.join( outputDirectory, `${ name }.json` ), JSON.stringify( value, null, 2 ) + '\n' ); }
assert.equal( differences.length, 0, JSON.stringify( differences.slice( 0, 3 ), null, 2 ) );
console.log( `ShaderNode/Fn/ShaderCall/stack (${ isolated ? 'isolated TSL then real builder' : 'eager builder imports' }): ${ requests.length } direct pinned-upstream cases passed` );
