import { pathToFileURL } from 'node:url';
import path from 'node:path';
import { spawnSync, execFileSync } from 'node:child_process';
import assert from 'node:assert/strict';

const gcChild = process.argv[ 2 ] === '--gc-child';
const [ upstream, executable ] = process.argv.slice( gcChild ? 3 : 2 );
if ( ! upstream || ! executable ) throw new Error( 'Usage: node compare-node-utils.mjs THREEJS_SOURCE_DIR PROBE' );
assert.equal( execFileSync( 'git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' } ).trim(), '11738a9d55e502fb484cefffcea3a66dc44e9fe9' );
const U = await import( pathToFileURL( path.join( upstream, 'src/nodes/core/NodeUtils.js' ) ) );
const { default: StackTrace } = await import( pathToFileURL( path.join( upstream, 'src/nodes/core/StackTrace.js' ) ) );
if ( gcChild ) {
    const mode = process.argv[ 5 ], retained = [];
    const refs = ( () => {
        const key = {}, data = U.getDataFromObject( key );
        if ( mode === 'cycle' || mode === 'retain-data-cycle' ) data.key = key;
        if ( mode === 'retain-key' ) retained.push( key );
        if ( mode === 'retain-data' || mode === 'retain-data-cycle' ) retained.push( data );
        return [ new WeakRef( key ), new WeakRef( data ) ];
    } )();
    for ( let i = 0; i < 30; ++ i ) { await new Promise( resolve => setImmediate( resolve ) ); globalThis.gc(); }
    console.log( JSON.stringify( [ mode, ...refs.map( ref => ref.deref() !== undefined ) ] ) );
    assert.equal( retained.length, mode === 'cycle' ? 0 : 1 );
    process.exit( 0 );
}
const undef = { tag: 'undefined' };
const requests = [ { kind: 'metadata' }, { kind: 'cache' }, { kind: 'hashMutation' }, { kind: 'mapProtocol' }, { kind: 'weakMapProtocol' }, { kind: 'hashMethod' } ];
const add = ( method, ...args ) => requests.push( { kind: 'call', method, args } );
for ( const value of [ '', 'abc', '中文', '😀', 'a\ud800b', 'a\udc00', 'e\u0301', '\0x\uffff', '𐀀' ] ) {
    add( 'hashString', value ); add( 'hashArray', value );
}
for ( const values of [ [], [ 1, 2, 3 ], [ -5, .5, 1e9 ], [ null, undef, true, '42' ], [ { tag: 'nan' }, { tag: 'infinity' }, { tag: 'negative-zero' } ], [ 4294967297, -4294967297, 9007199254740991 ] ] ) {
    add( 'hashArray', values ); add( 'hash', ...values );
}
add( 'hashArray', [ { tag: 'coerce', name: 'element', values: [ 7, 8 ] } ] );
for ( const value of [ null, undef, true, 1, {}, { tag: 'typed', values: [ 1, 2 ] }, { length: 0 }, { length: -2 } ] ) add( 'hashString', value );
for ( const length of [ 0, 1, 2, 3, 4, 9, 16, '3', null, undef, { tag: 'nan' }, { tag: 'coerce', name: 'length', values: [ 3 ] } ] ) add( 'getTypeFromLength', length );
for ( const type of [ 'float', 'int', 'uint', 'bool', 'vec2', 'ivec3', 'uvec4', 'mat2', 'mat3', 'mat4', 'vec2junk', 'xxxmat3', 'floating', 'vec9', '', null, undef, { tag: 'symbol' } ] ) {
    for ( const method of [ 'getTypedArrayFromType' ] ) add( method, type );
    if ( typeof type === 'string' && /float|int|uint|bool|vec[234]|mat[234]/.test( type ) ) for ( const method of [ 'getLengthFromType', 'getMemoryLengthFromType', 'getAlignmentFromType' ] ) add( method, type );
}
for ( const method of [ 'getTypedArrayFromType', 'getLengthFromType', 'getMemoryLengthFromType', 'getAlignmentFromType' ] ) add( method, { tag: 'stringCoerce', name: 'type', values: [ 'nope', 'vec2' ] } );
for ( const value of [ null, undef, 1, true, 's', {}, { isNode: 1, isVector3: true }, { isNode: true, isVector3: true }, { isMatrix2: true }, { tag: 'buffer', values: [ 1, 2 ] }, { tag: 'typed', values: [ 1, 2 ] }, { tag: 'flags', values: { isNode: false, isVector2: false, isVector3: true } } ] ) add( 'getValueType', value );
for ( const value of [ null, undef, 1, true, 's', { tag: 'symbol' } ] ) add( 'getDataFromObject', value );
for ( const format of [ 1023, 1028, 1029, 1030, 1031, 1032, 1033, 1021, 1022, 1026, 1027, 9999, '1028' ] ) for ( const type of [ 1014, 1013, 1009 ] ) add( 'getTextureType', { format, type } );
add( 'getTextureType', { tag: 'texture', values: { isDepthTexture: true, format: 1, type: [ 1014, 1013 ] } } );
add( 'getTextureType', { tag: 'texture', values: { isDepthTexture: false, format: 1030, type: [ 0, 1013 ] } } );
for ( const type of [ 'bool', 'float', 'int', 'uint', 'string', null, '', 'unknown' ] ) for ( const value of [ undef, null, false, 0, { tag: 'negative-zero' }, { tag: 'nan' }, 'kept', 5, {} ] ) add( 'getValueFromType', type, value );
for ( const type of [ 'vec2', 'ivec3', 'uvec4' ] ) { add( 'getValueFromType', type ); add( 'getValueFromType', type, 5 ); add( 'getValueFromType', type, 1, 2, 3, 4 ); }
for ( const type of [ 'mat3', 'mat4' ] ) add( 'getValueFromType', type );
for ( const values of [ [], [ 0 ], [ 0, 1, 2 ], [ 255, 254, 253, 128, 0, 1 ], [ 256, -1, 42.5 ] ] ) {
    add( 'arrayBufferToBase64', { tag: 'buffer', values } ); add( 'arrayBufferToBase64', values );
}
for ( const text of [ '', 'AA==', 'Zg==', 'Zm8=', 'Zm9v', ' Z m\tf\n v\r', 'Zm8', 'Zg', 'AB==' ] ) { add( 'base64ToArrayBuffer', text ); add( 'getValueFromType', 'ArrayBuffer', text ); }
for ( const value of [ undef, null, {}, { isNode: 3 }, [], 1, true, 'x' ] ) add( 'isArrayAsParameter', [ value ] );
for ( const text of [ 'Error', 'Error\n    at myFn (http://host/src/main.js?v=3:12:7)\n    at ignored (http://host/Node.js:2:1)\nfoo@http://host/start.js:33:4\n    at http://host/plain.js:1:9', 'x@file:///d:/root/x.js:1234567891234:2', 'Error\n at internal (StackTrace.js:1:2)\n at tsl (TSLCore.js:1:2)\n at noise (FooNode.js:1:2)\n at bundle (three.webgpu.min.js:1:2)\n at run (entry.js:2:3)' ] ) requests.push( { kind: 'stack', text, message: 'failure' } );
requests.push( { kind: 'stackProtocol', frames: [ { tag: 'flags', values: { fn: 'run', file: 'custom.cpp', line: 7, column: 8 } } ], message: { tag: 'stringCoerce', name: 'message', values: [ 'dynamic' ] } } );
requests.push( { kind: 'stack', text: 'Error\n at\u00a0run\u2003(path/场景.js:7:8)', message: 'failure' } );
// Preserve the previous 302 requests; exercise the canonical Matrix2 constructor.
for ( const values of [ [], [ 1, 2, 3, 4 ], [ 1 ], [ 'a', false, null, undef ], [ undef, 8 ], [ null ], [ { tag: 'negative-zero' }, { tag: 'nan' }, 3, 4 ] ] ) add( 'getValueFromType', 'mat2', ...values );

function decode( spec, state ) {
    if ( ! spec || typeof spec !== 'object' ) return spec;
    if ( Array.isArray( spec ) ) return spec.map( x => decode( x, state ) );
    if ( spec.tag === 'undefined' ) return undefined;
    if ( spec.tag === 'nan' ) return NaN;
    if ( spec.tag === 'infinity' ) return Infinity;
    if ( spec.tag === 'negative-zero' ) return -0;
    if ( spec.tag === 'symbol' ) return Symbol( 'fixture' );
    if ( spec.tag === 'buffer' || spec.tag === 'typed' ) { const typed = new Uint8Array( decode( spec.values, state ) ); return spec.tag === 'buffer' ? typed.buffer : typed; }
    if ( spec.tag === 'coerce' || spec.tag === 'stringCoerce' ) {
        let n = 0; const key = spec.tag === 'coerce' ? 'valueOf' : 'toString';
        return Object.defineProperty( {}, key, { enumerable: true, configurable: true, get() {
            state.events.push( `get:${ spec.name }` ); return () => { const i = n ++; state.events.push( `call:${ spec.name }:${ i }` ); return decode( spec.values[ Math.min( i, spec.values.length - 1 ) ], state ); };
        } } );
    }
    if ( spec.tag === 'flags' || spec.tag === 'texture' ) {
        let n = 0; const out = {};
        for ( const key of Object.keys( spec.values ) ) Object.defineProperty( out, key, { enumerable: true, configurable: true, get() {
            state.events.push( `get:${ key }` ); const value = spec.values[ key ]; return decode( key === 'type' && Array.isArray( value ) ? value[ Math.min( n ++, value.length - 1 ) ] : value, state );
        } } ); return out;
    }
    return Object.fromEntries( Object.entries( spec ).map( ( [ k, v ] ) => [ k, decode( v, state ) ] ) );
}
function encode( value ) {
    if ( value === undefined ) return '#undefined'; if ( typeof value === 'symbol' ) return '#symbol';
    if ( typeof value === 'function' ) return value.name;
    if ( typeof value === 'number' ) { if ( Number.isNaN( value ) ) return '#NaN'; if ( value === Infinity ) return '#Infinity'; if ( value === -Infinity ) return '#-Infinity'; if ( Object.is( value, -0 ) ) return '#-0'; }
    if ( value instanceof ArrayBuffer ) return [ ...new Uint8Array( value ) ];
    if ( value && typeof value === 'object' ) { const type = U.getValueType( value );
        if ( [ 'vec2', 'vec3', 'vec4', 'color' ].includes( type ) ) { const keys = type === 'color' ? 'rgb' : type === 'vec2' ? 'xy' : type === 'vec3' ? 'xyz' : 'xyzw'; return { type, values: [ ...keys ].map( k => encode( value[ k ] ) ) }; }
        if ( type === 'mat2' || type === 'mat3' || type === 'mat4' ) return { type, values: value.elements.map( encode ) }; return '#object';
    } return value;
}
function run( request ) {
    const state = { events: [] };
    if ( request.kind === 'metadata' ) return Object.keys( U ).map( key => { const fn = U[ key ]; let constructible; try { Reflect.construct( String, [], fn ); constructible = true; } catch { constructible = false; } return { name: fn.name, length: fn.length, prototype: Object.hasOwn( fn, 'prototype' ), constructible }; } ).sort( ( a, b ) => a.name.localeCompare( b.name ) );
    if ( request.kind === 'cache' ) { const key = {}, other = {}, data = U.getDataFromObject( key ); const rows = [ data === U.getDataFromObject( key ), data !== U.getDataFromObject( other ), Object.getOwnPropertyNames( data ).length === 0 ]; data.cycle = key; rows.push( false, false ); return rows; }
    if ( request.kind === 'mapProtocol' ) {
        const original = Object.getOwnPropertyDescriptor( Map.prototype, 'get' );Object.defineProperty( Map.prototype, 'get', { configurable: true, get() { state.events.push( 'get:get' );return function ( key ) { state.events.push( `call:get:${ key }` );return 'custom'; }; } } );
        try { return { value: [ U.getTypeFromLength( 3 ), U.getTextureType( { format: 0, type: 0 } ) ], events: state.events }; } finally { Object.defineProperty( Map.prototype, 'get', original ); }
    }
    if ( request.kind === 'weakMapProtocol' ) {
        const get = Object.getOwnPropertyDescriptor( WeakMap.prototype, 'get' ), set = Object.getOwnPropertyDescriptor( WeakMap.prototype, 'set' );let n = 0;
        Object.defineProperty( WeakMap.prototype, 'get', { configurable: true, get() { state.events.push( 'get:get' );return function () { state.events.push( 'call:get' );return n ++ === 0 ? undefined : false; }; } } );
        Object.defineProperty( WeakMap.prototype, 'set', { configurable: true, get() { state.events.push( 'get:set' );return function ( ...args ) { state.events.push( 'call:set' );return set.value.apply( this, args ); }; } } );
        try { const key = {};return { value: [ encode( U.getDataFromObject( key ) ), encode( U.getDataFromObject( key ) ) ], events: state.events }; } finally { Object.defineProperty( WeakMap.prototype, 'get', get );Object.defineProperty( WeakMap.prototype, 'set', set ); }
    }
    if ( request.kind === 'hashMethod' ) {
        const original = Object.getOwnPropertyDescriptor( String.prototype, 'charCodeAt' ), number = decode( { tag: 'coerce', name: 'char', values: [ 7, 8, 9, 10, 11, 12 ] }, state );
        Object.defineProperty( String.prototype, 'charCodeAt', { ...original, value( index ) { state.events.push( `char:${ index }` );return number; } } );
        try { return { value: U.hashString( 'A😀' ), events: state.events }; } finally { Object.defineProperty( String.prototype, 'charCodeAt', original ); }
    }
    if ( request.kind === 'hashMutation' ) { const values = [ 0, 99 ]; let n = 0; values[ 0 ] = { valueOf() { const i = n ++; state.events.push( `valueOf:${ i }` ); if ( i === 0 ) values.length = 1; return i + 7; } }; return { value: U.hashArray( values ), events: state.events }; }
    if ( request.kind === 'stack' ) { const object = new StackTrace( request.text ); return { stack: object.stack, location: object.getLocation(), error: encode( object.getError( decode( request.message, state ) ) ) }; }
    if ( request.kind === 'stackProtocol' ) { const object = new StackTrace( 'Error' ); object.stack = decode( request.frames, state ); return { location: object.getLocation(), error: encode( object.getError( decode( request.message, state ) ) ), events: state.events }; }
    const args = request.args.map( x => decode( x, state ) ); try { return { value: encode( U[ request.method ]( ...args ) ), events: state.events }; } catch ( error ) { return { error: error.name, events: state.events }; }
}
const native = spawnSync( executable, [], { input: JSON.stringify( requests ), encoding: 'utf8', maxBuffer: 16 * 1024 * 1024, env: { ...process.env, TEMP: 'D:\\Temp\\surui', TMP: 'D:\\Temp\\surui' } } );
if ( native.status !== 0 ) throw new Error( native.stderr || `probe exit ${ native.status }` );
const actual = JSON.parse( native.stdout ), expected = requests.map( run );actual[ 0 ].sort( ( a, b ) => a.name.localeCompare( b.name ) );
for ( let i = 0; i < requests.length; ++ i ) assert.deepStrictEqual( actual[ i ], expected[ i ], `case ${ i }: ${ JSON.stringify( requests[ i ] ) }` );
const nativeGC = spawnSync( executable, [ 'gc' ], { encoding: 'utf8' } );
assert.equal( nativeGC.status, 0, nativeGC.stderr );
const upstreamGC = [ 'cycle', 'retain-key', 'retain-data', 'retain-data-cycle' ].map( mode => {
    const child = spawnSync( process.execPath, [ '--expose-gc', process.argv[ 1 ], '--gc-child', upstream, executable, mode ], { encoding: 'utf8' } );
    assert.equal( child.status, 0, child.stderr );return JSON.parse( child.stdout );
} );
assert.deepStrictEqual( JSON.parse( nativeGC.stdout ), upstreamGC );
const stable = spawnSync( executable, [ 'array-buffer-intrinsic' ], { encoding: 'utf8' } );assert.equal( stable.status, 0, stable.stderr );
const originalBufferConstructor = Object.getOwnPropertyDescriptor( ArrayBuffer.prototype, 'constructor' );Object.defineProperty( ArrayBuffer.prototype, 'constructor', { ...originalBufferConstructor, value: false } );
try { assert.deepStrictEqual( JSON.parse( stable.stdout ), [ U.getValueType( new ArrayBuffer( 0 ) ), U.getValueType( Object.create( ArrayBuffer.prototype ) ) ] ); } finally { Object.defineProperty( ArrayBuffer.prototype, 'constructor', originalBufferConstructor ); }
console.log( `NodeUtils + StackTrace: ${ requests.length } direct upstream cases + 4 GC cases + stable ArrayBuffer binding passed` );
