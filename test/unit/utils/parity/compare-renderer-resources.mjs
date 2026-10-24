import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';
import path from 'node:path';
import { mkdirSync, writeFileSync } from 'node:fs';
const [ upstream, executable, output ] = process.argv.slice( 2 );
assert.equal( execFileSync( 'git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' } ).trim(), '11738a9d55e502fb484cefffcea3a66dc44e9fe9' );
const load = relative => import( pathToFileURL( path.join( upstream, 'src', relative ) ) );
const [ U, N, { default: Binding }, { default: Buffer }, { default: UniformBuffer }, Utils, { Matrix2 } ] = await Promise.all( [ load( 'renderers/common/Uniform.js' ), load( 'renderers/common/nodes/NodeUniform.js' ), load( 'renderers/common/Binding.js' ), load( 'renderers/common/Buffer.js' ), load( 'renderers/common/UniformBuffer.js' ), load( 'renderers/common/BufferUtils.js' ), load( 'math/Matrix2.js' ) ] );
const { uniform } = await load( 'nodes/core/UniformNode.js' );
function normal( value ) {
    if ( value === undefined ) return '#undefined';
    if ( typeof value === 'number' ) { if ( Number.isNaN( value ) ) return '#NaN';if ( value === Infinity ) return '#Infinity';if ( value === - Infinity ) return '#-Infinity';if ( Object.is( value, - 0 ) ) return '#-0';return value; }
    if ( value === null || typeof value !== 'object' ) return value;
    for ( const type of [ 'Vector2', 'Vector3', 'Vector4', 'Color', 'Matrix2', 'Matrix3', 'Matrix4' ] ) if ( value[ `is${ type }` ] === true ) return { math: type, values: ( type.startsWith( 'Matrix' ) ? value.elements : type === 'Color' ? [ value.r, value.g, value.b ] : type === 'Vector2' ? [ value.x, value.y ] : type === 'Vector3' ? [ value.x, value.y, value.z ] : [ value.x, value.y, value.z, value.w ] ).map( normal ) };
    if ( Array.isArray( value ) ) return Array.from( value, normal );
    return Object.fromEntries( Object.keys( value ).map( key => [ key, normal( value[ key ] ) ] ) );
}
const names = Object.getOwnPropertyNames;
const snapshot = value => Object.fromEntries( [ [ 'keys', names( value ) ], ...names( value ).map( key => [ key, normal( value[ key ] ) ] ) ] );
const expected = [], row = ( kind, value ) => expected.push( { kind, value } );
for ( const type of [ 'Number', 'Vector2', 'Vector3', 'Vector4', 'Color', 'Matrix2', 'Matrix3', 'Matrix4' ] ) {
    const ctor = U[ `${ type }Uniform` ], u = new ctor();row( `${ type } defaults`, snapshot( u ) );
    const another = new ctor();row( `${ type } independent`, u.value !== null && typeof u.value === 'object' ? u.value !== another.value : true );
    row( `${ type } constructor`, [ ctor.name, ctor.length, Object.getPrototypeOf( ctor ).name, names( ctor.prototype ), names( Object.getPrototypeOf( ctor ).prototype ) ] );
    const value = { v: 1 };row( `${ type } setter return`, normal( u.setValue( value ) ) );row( `${ type } setter identity`, u.getValue() === value );row( `${ type } null`, snapshot( new ctor( 'n', null ) ) );
    const node = { name: 'node', value, type: 'test' }, n = new N[ `${ type }NodeUniform` ]( node );
    row( `${ type } node initial`, [ n.name, n.boundary, n.itemSize, n.value === value, n.getValue() === value, n.getType() ] );
    node.value = 42;node.type = 'changed';n.setValue( 3 );row( `${ type } node live`, [ n.value, n.getValue(), n.getType(), n.nodeUniform === node ] );
}
for ( const [ name, ctor ] of [ [ 'Binding', Binding ], [ 'Buffer', Buffer ], [ 'UniformBuffer', UniformBuffer ] ] ) {
    const instance = new ctor();row( `${ name } defaults`, snapshot( instance ) );const clone = instance.clone();row( `${ name } clone`, [ snapshot( clone ), clone !== instance, Object.getPrototypeOf( clone ) === Object.getPrototypeOf( instance ) ] );
    if ( name !== 'Binding' ) { const buffer = new Float32Array( 3 ), b = new ctor( 'test', buffer );b.addUpdateRange( 1, 2 );b.addUpdateRange( 3, 4 );const c = b.clone();row( `${ name } buffer`, [ b.byteLength, b.bytesPerElement, normal( b.updateRanges ), c._buffer === buffer, c.updateRanges === b.updateRanges, b.update() ] );b.clearUpdateRanges();b.release();row( `${ name } released`, [ b.buffer, normal( b.updateRanges ), c.buffer === buffer ] ); }
}
for ( const value of [ undefined, null, false, true, 0, - 0, - 1, 1, 3.5, 15, 16, 17, 4294967299, '3', '', 'bad', NaN, Infinity, - Infinity ] ) {
    const b = new Binding();b.setVisibility( value );b.setVisibility( 8 );row( 'visibility', normal( b.visibility ) );
    for ( const method of [ 'getFloatLength', 'getStrideLength', 'getVectorLength' ] ) row( method, normal( Utils[ method ]( value ) ) );
}
for ( const method of [ 'getFloatLength', 'getStrideLength', 'getVectorLength' ] ) { const state = { events: [], count: 0 }, object = { [ Symbol.toPrimitive ]( hint ) { const n = state.count ++;state.events.push( [ hint, n ] );return n === 0 ? 5 : '7'; } };row( `${ method } coercion`, [ normal( Utils[ method ]( object, object ) ), state.events ] ); }
for ( const args of [ [], [ undefined, 4 ], [ null ], [ 1, 2, 3, 4 ], [ 'a', false, null ] ] ) row( 'Matrix2 constructor', normal( new Matrix2( ...args ) ) );
for ( const offset of [ undefined, 0, 1, '1', - 1, 0.5, null ] ) { const m = new Matrix2(), a = {};for ( let i = 0; i < 15; i ++ ) a[ i ] = i + 10;m.fromArray( a, offset );row( 'Matrix2 fromArray', normal( m ) ); }
const m = new Matrix2( 1, 2, 3, 4 );row( 'Matrix2 identity', [ m.identity() === m, normal( m ) ] );row( 'Matrix2 metadata', [ Matrix2.length, names( Matrix2.prototype ), Matrix2.prototype.fromArray.length ] );
Object.defineProperty( Object.prototype, 'start', { configurable: true, set() { throw new TypeError( 'inherited range setter' ); } } );
try { const b = new Buffer();b.addUpdateRange( 3, 5 );row( 'range literal', normal( b.updateRanges ) ); } finally { delete Object.prototype.start; }
const attempt = fn => { try { return normal( fn() ); } catch ( error ) { return error.name; } };
for ( const value of [ undefined, null, false, 4, 'a😀', [ 1, undefined, 3 ] ] ) row( 'Array.from', attempt( () => Array.from( value ) ) );
for ( let mode = 0; mode < 8; mode ++ ) {
    const state = { events: [], count: 0 }, iterator = { get next() { state.events.push( 'get next' );return () => { const n = state.count ++;state.events.push( [ 'next', n ] );if ( mode === 4 ) throw new TypeError( 'step' );return { get done() { state.events.push( 'done' );return n >= 2; }, get value() { state.events.push( 'value' );if ( mode === 5 ) throw new TypeError( 'value' );return n; } }; }; }, get return() { state.events.push( 'get return' );return () => { state.events.push( 'return' );if ( mode === 3 ) throw new RangeError( 'close' );return {}; }; } };
    const source = { get [ Symbol.iterator ]() { state.events.push( 'get iterator' );if ( mode === 6 ) return null;return () => { state.events.push( 'iterator' );return iterator; }; }, get length() { state.events.push( 'length' );return 3; }, get 0() { state.events.push( 'get 0' );return 4; } };
    const mapper = function ( value, index ) { state.events.push( [ 'map', normal( value ), index, this === state ] );if ( mode >= 2 && mode <= 3 ) throw new TypeError( 'map' );return value; };
    class Target { constructor( ...args ) { state.events.push( [ 'construct', args ] ); } }
    const result = attempt( () => Array.from.call( mode === 0 ? Array : Target, source, mode === 7 ? 2 : mapper, state ) );row( 'Array.from protocol', [ result, state.events ] );
}
for ( const value of [ undefined, null, false, 3, 'a😀', [ 1, 2 ] ] ) row( 'Object.values', attempt( () => Object.values( value ) ) );
const object = { a: 1, get b() { delete this.c;this.d = 4;return 2; }, c: 3, [ Symbol( 'skip' ) ]: 5 };row( 'Object.values mutation', [ Object.values( object ), names( object ) ] );
for ( const value of [ undefined, null, 0, true, 2.5, 'float', 'bool', 'vec2', 'mat2', 'mat3', 'mat4' ] ) for ( const type of [ undefined, false, 'uint' ] ) { const u = uniform( value, type );row( 'uniform factory', [ normal( u.value ), normal( u.nodeType ), u.groupNode.name ] ); }
for ( let mode = 0; mode < 4; mode ++ ) {
    const events = [], value = { isNode: true, get value() { events.push( 'value' );return 3; }, get traverse() { events.push( 'get traverse' );return callback => { events.push( 'traverse' );if ( mode === 3 ) throw new TypeError( 'traverse' );for ( const flag of [ false, 1, true ] ) callback( { isConstNode: flag, value: 7 } ); }; } };
    if ( mode === 1 ) value.nodeType = 'vec3';if ( mode === 2 ) value.convertTo = 'int';const result = attempt( () => { const u = uniform( value );return [ normal( u.value ), normal( u.nodeType ) ]; } );row( 'uniform factory traversal', [ result, events ] );
}
for ( const Type of [ Int8Array, Uint8Array, Int16Array, Uint16Array, Int32Array, Uint32Array, Float32Array, Float64Array, Uint8ClampedArray ] ) for ( const method of [ 'keys', 'values', 'entries' ] ) for ( const shadow of [ false, true ] ) {
    const typed = new Type( [ 1, 2, 3 ] );if ( shadow ) Object.defineProperty( typed, 'length', { configurable: true, enumerable: true, get() { throw new TypeError( 'length shadow must not be observed' ); } } );
    const iterator = typed[ method ](), result = [];for ( let i = 0; i < 5; i ++ ) { const step = iterator.next();result.push( normal( step ) );if ( step.done ) break; }row( 'TypedArray iterator', result );
    row( 'TypedArray method metadata', [ typed[ method ].name, typed[ method ].length, ! Object.hasOwn( typed[ method ], 'prototype' ), typed.values === typed[ Symbol.iterator ] ] );
}
const typed = new Uint8Array( [ 1, 2, 3 ] );row( 'TypedArray receiver brand', attempt( () => typed.values.call( {} ) ) );Object.defineProperty( typed, 'length', { configurable: true, enumerable: true, get() { throw new TypeError( 'length shadow' ); } } );
const borrowed = Array.prototype.values.call( typed );row( 'Array iterator typed length', [ normal( borrowed.next() ), attempt( () => Array.from( typed ) ) ] );
const text = execFileSync( executable, [], { encoding: 'utf8', maxBuffer: 16 * 1024 * 1024 } );
const actual = text.trim().split( /\r?\n/ ).map( line => JSON.parse( line ) );
if ( output ) { mkdirSync( output, { recursive: true } );writeFileSync( path.join( output, 'renderer-resources-upstream.json' ), JSON.stringify( expected, null, 2 ) );writeFileSync( path.join( output, 'renderer-resources-native.json' ), JSON.stringify( actual, null, 2 ) ); }
assert.equal( actual.length, expected.length );for ( let i = 0; i < expected.length; i ++ ) assert.deepStrictEqual( actual[ i ], expected[ i ], `row ${ i }: ${ expected[ i ].kind }` );
console.log( `Renderer resources: ${ expected.length } direct upstream rows passed` );
