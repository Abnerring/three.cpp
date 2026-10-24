import { execFileSync, spawnSync } from 'node:child_process';
import { mkdirSync, writeFileSync } from 'node:fs';
import { pathToFileURL } from 'node:url';
import path from 'node:path';
import assert from 'node:assert/strict';
const [ upstream, executable, outputDirectory ] = process.argv.slice( 2 );
if ( ! upstream || ! executable ) throw Error( 'compare-float16-buffer-attribute.mjs <pinned-three.js> <probe> [output-directory]' );
assert.equal( execFileSync( 'git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' } ).trim(), '11738a9d55e502fb484cefffcea3a66dc44e9fe9' );
const load = file => import( pathToFileURL( path.join( upstream, 'src', file ) ) );
const { BufferAttribute, Float16BufferAttribute, Uint16BufferAttribute } = await load( 'core/BufferAttribute.js' );
const { InterleavedBufferAttribute } = await load( 'core/InterleavedBufferAttribute.js' );
const { default: NodeBuilder } = await load( 'nodes/core/NodeBuilder.js' );
const { setConsoleFunction } = await load( 'utils.js' );
setConsoleFunction( () => {} );
const expected = [];
const normal = value => {
    if ( value === undefined ) return '#undefined';
    if ( typeof value === 'number' ) { if ( Number.isNaN( value ) ) return '#NaN';if ( ! Number.isFinite( value ) ) return value > 0 ? '#Infinity' : '#-Infinity';if ( Object.is( value, -0 ) ) return '#-0'; }
    if ( ! value || typeof value !== 'object' ) return value;
    return Object.fromEntries( Object.keys( value ).map( key => [ key, normal( value[ key ] ) ] ) );
};
// Preserve actual arrays in the serialized oracle, including nested arrays.
const normalize = value => Array.isArray( value ) ? value.map( normalize ) : value && typeof value === 'object' ? Object.fromEntries( Object.keys( value ).map( key => [ key, normalize( value[ key ] ) ] ) ) : normal( value );
const attempt = callback => { try { return normalize( callback() ); } catch ( error ) { return `#${ error.name }`; } };
const row = ( name, value ) => expected.push( { name, value: normalize( value ) } );
const getter = ( object, key, get ) => Object.defineProperty( object, key, { enumerable: true, configurable: true, get } );
const coerce = ( events, name, sequence ) => { let index = 0;return { [ Symbol.toPrimitive ]( hint ) { const i = index ++;events.push( [ 'coerce', name, hint, i ] );return sequence[ Math.min( i, sequence.length - 1 ) ]; } }; };
const methods = Object.getOwnPropertyNames( Float16BufferAttribute.prototype ).map( key => {
    const fn = Float16BufferAttribute.prototype[ key ], d = Object.getOwnPropertyDescriptor( Float16BufferAttribute.prototype, key );
    return [ key, fn.name, fn.length, key === 'constructor', d.enumerable, d.configurable, d.writable ];
} );
row( 'metadata', [ Float16BufferAttribute.name, Float16BufferAttribute.length, Object.getPrototypeOf( Float16BufferAttribute ) === BufferAttribute, Object.getPrototypeOf( Float16BufferAttribute.prototype ) === BufferAttribute.prototype, methods ] );
{
    const value = new Float16BufferAttribute( [ 15360, 49152 ], 1 ), ordinary = new Uint16BufferAttribute( [ 15360 ], 1 );ordinary.isFloat16BufferAttribute = true;
    row( 'identity', [ value instanceof Float16BufferAttribute, ordinary instanceof Float16BufferAttribute, Object.create( Float16BufferAttribute.prototype ) instanceof Float16BufferAttribute, Float16BufferAttribute.prototype instanceof Float16BufferAttribute, null instanceof Float16BufferAttribute ] );
    value.isFloat16BufferAttribute = false;row( 'flag independent', [ value instanceof Float16BufferAttribute, value.getX( 0 ), value.getX( 1 ) ] );
    Object.setPrototypeOf( value, BufferAttribute.prototype );row( 'changed instance prototype', value instanceof Float16BufferAttribute );
    Float16BufferAttribute.prototype.constructor = null;row( 'stable binding', Float16BufferAttribute === Float16BufferAttribute );Float16BufferAttribute.prototype.constructor = Float16BufferAttribute;
    for ( let mode = 0; mode < 5; ++ mode ) {
        const events = [];
        if ( mode === 0 || mode === 1 ) getter( Float16BufferAttribute, Symbol.hasInstance, () => { events.push( [ 'get' ] );return function ( arg ) { events.push( [ 'call', this === Float16BufferAttribute, arg === value ] );return mode === 1; }; } );
        else Object.defineProperty( Float16BufferAttribute, Symbol.hasInstance, { value: mode === 2 ? 0 : mode === 3 ? null : undefined, configurable: true } );
        row( 'hasInstance', [ mode, attempt( () => value instanceof Float16BufferAttribute ), events ] );delete Float16BufferAttribute[ Symbol.hasInstance ];
    }
}
for ( const input of [ undefined, null, 0, 3, 2.5, -1, NaN, [ 15360, 49152, -1, 65537 ], Symbol( 'input' ) ] ) row( 'construction', attempt( () => {
    const value = new Float16BufferAttribute( input, '2', 1 );return [ [ ...value.array ], value.itemSize, value.count, value.normalized, value.isFloat16BufferAttribute, value instanceof Float16BufferAttribute ];
} ) );
{
    const input = new Uint16Array( [ 15360, 49152 ] ), value = new Float16BufferAttribute( input, 1 );value.setX( 0, .5 );const clone = value.clone();
    row( 'copied backing and clone', [ [ ...input ], [ ...value.array ], clone instanceof Float16BufferAttribute, clone.getX( 0 ), value.array === clone.array ] );
    class Derived extends Float16BufferAttribute {}const derived = Reflect.construct( Float16BufferAttribute, [ [ 15360 ], 1 ], Derived );
    row( 'newTarget', [ Object.getPrototypeOf( derived ) === Derived.prototype, derived instanceof Float16BufferAttribute, derived.getX( 0 ) ] );
    const events = [];class Alternate { constructor( ...args ) { events.push( [ 'super', [ ...args[ 0 ] ], args[ 1 ], args[ 2 ], Object.getPrototypeOf( this ) === Float16BufferAttribute.prototype ] ); } }
    const parent = Object.getPrototypeOf( Float16BufferAttribute );Object.setPrototypeOf( Float16BufferAttribute, Alternate );const alternateValue = new Float16BufferAttribute( [ 65537 ], 'item', false );Object.setPrototypeOf( Float16BufferAttribute, parent );
    row( 'dynamic super', [ alternateValue.isFloat16BufferAttribute, events ] );
    const mutationEvents = [];class ChangedSuper { constructor( array ) { mutationEvents.push( [ 'changedSuper', [ ...array ] ] ); } }
    const mutationInput = { [ Symbol.iterator ]() { mutationEvents.push( [ 'iterator' ] );Object.setPrototypeOf( Float16BufferAttribute, ChangedSuper );return [ 65537 ][ Symbol.iterator ](); } };
    const changedValue = new Float16BufferAttribute( mutationInput, 1 );Object.setPrototypeOf( Float16BufferAttribute, parent );row( 'super mutated by argument', [ changedValue.isFloat16BufferAttribute, mutationEvents ] );
    const coercions = [], size = coerce( coercions, 'itemSize', [ 2, 7 ] ), raw = new Float16BufferAttribute( [ 1, 2, 3, 4 ], size, undefined );
    row( 'constructor coercion', [ raw.itemSize === size, raw.count, raw.normalized, coercions ] );row( 'constructor class call', attempt( () => Float16BufferAttribute() ) );
}
for ( const method of [ 'getX', 'getY', 'getZ', 'getW' ] ) for ( const index of [ 0, 1, -1, .5 ] ) {
    const value = new Float16BufferAttribute( [ 15360, 49152, 14336, 32768, 31744, 32256, 0, 16384 ], 4 );row( 'half getter', [ method, index, attempt( () => value[ method ]( index ) ) ] );
}
for ( const method of [ 'setX', 'setY', 'setZ', 'setW', 'setXY', 'setXYZ', 'setXYZW' ] ) for ( const normalized of [ false, true ] ) {
    const value = new Float16BufferAttribute( [ 0, 0, 0, 0, 0, 0, 0, 0 ], 4, normalized );row( 'half setter', [ method, normalized, attempt( () => value[ method ]( 1, .25, -.5, .125, .75 ) === value ), [ ...value.array ] ] );
}
for ( const method of [ 'getX', 'getY', 'getZ', 'getW', 'setX', 'setY', 'setZ', 'setW', 'setXY', 'setXYZ', 'setXYZW' ] ) for ( const normalized of [ false, true ] ) {
    const events = [], backing = [ 15360, 15360, 15360, 15360 ], self = {}, size = coerce( events, 'size', [ 4 ] );
    for ( const key of [ 'array', 'itemSize', 'normalized' ] ) getter( self, key, () => { events.push( [ 'get', key ] );return key === 'array' ? backing : key === 'itemSize' ? size : normalized; } );
    const result = attempt( () => Float16BufferAttribute.prototype[ method ].call( self, coerce( events, 'index', [ 0 ] ), coerce( events, 'x', [ .5, .25 ] ), .5, .125, .75 ) );
    row( 'generic half protocol', [ method, normalized, result && typeof result === 'object' ? '#self' : result, backing, events ] );
}
for ( const method of [ 'getX', 'setX' ] ) {
    const events = [], backing = { 0: coerce( events, 'read', [ 15360, 0 ] ) }, self = { itemSize: 1, normalized: false, array: backing };
    const result = attempt( () => Float16BufferAttribute.prototype[ method ].call( self, 0, coerce( events, 'write', [ 1, .5 ] ) ) );
    row( 'half repeated coercion', [ method, result && typeof result === 'object' ? '#self' : result, backing[ 0 ] && typeof backing[ 0 ] === 'object' ? '#object' : backing[ 0 ], events ] );
}
const constructors = [ Int8Array, Uint8Array, Int16Array, Uint16Array, Int32Array, Uint32Array, Float32Array, Float64Array, Uint8ClampedArray ];
function iterableInput( events, mode ) {
    if ( mode === 15 ) { const input = new Uint16Array( [ 1, 2 ] );for ( const key of [ 'length', Symbol.iterator ] ) getter( input, key, () => { throw new TypeError( 'typed internal branch' ); } );return input; }
    const input = {};let index = 0;getter( input, 'length', () => { events.push( [ 'length' ] );return 2; } );
    for ( let i = 0; i < 2; ++ i ) getter( input, i, () => { events.push( [ 'index', i ] );return coerce( events, 'element', [ i + 1 ] ); } );
    getter( input, Symbol.iterator, function () {
        events.push( [ 'iterator:get', this === input ] );if ( mode === 12 ) throw new TypeError( 'iterator getter' );if ( mode === 1 ) return null;if ( mode === 13 ) return undefined;if ( mode === 2 ) return 7;
        return function () {
            events.push( [ 'iterator:call', this === input ] );if ( mode === 3 ) throw new TypeError( 'iterator call' );if ( mode === 4 ) return 1;
            const iterator = {};getter( iterator, 'next', () => {
                events.push( [ 'next:get' ] );if ( mode === 10 ) throw new TypeError( 'next getter' );if ( mode === 5 ) return 1;
                return function () {
                    const i = index ++;events.push( [ 'next', i ] );if ( mode === 6 && i === 1 ) throw new TypeError( 'next call' );if ( mode === 11 && i === 1 ) return 1;
                    const step = {};getter( step, 'done', () => { events.push( [ 'done', i ] );if ( mode === 7 && i === 1 ) throw new TypeError( 'done getter' );return i === 2; } );
                    getter( step, 'value', () => { events.push( [ 'value', i ] );if ( mode === 8 && i === 1 ) throw new TypeError( 'value getter' );return { [ Symbol.toPrimitive ]( hint ) { events.push( [ 'convert', i, hint ] );if ( mode === 9 && i === 0 ) throw new TypeError( 'element conversion' );return i + 1; } }; } );return step;
                };
            } );getter( iterator, 'return', () => { events.push( [ 'return:get' ] );return () => { events.push( [ 'return:call' ] );return {}; }; } );return iterator;
        };
    } );return input;
}
for ( let kind = 0; kind < 10; ++ kind ) for ( let mode = 0; mode < 16; ++ mode ) {
    const events = [], input = iterableInput( events, mode );
    row( 'iterable construction', [ kind, mode, attempt( () => { const value = kind === 9 ? new Float16BufferAttribute( input, 1 ).array : new constructors[ kind ]( input );return Array.from( value ); } ), events ] );
}
const builder = () => new NodeBuilder( null, { debug: { diagnostics: { keywords: false } } }, null );
const b = builder(), sizes = [ 1, 2, 3, 4, 9, 16, '3' ], normalizations = [ false, true, 0, 1, 'true', null, undefined ];
for ( let kind = 0; kind < 9; ++ kind ) for ( const size of sizes ) for ( const normalized of normalizations ) {
    const attribute = new BufferAttribute( new constructors[ kind ]( 4 ), size, normalized );row( 'builder actual attribute', [ kind, size, normalized, attempt( () => b.getTypeFromAttribute( attribute ) ) ] );
}
for ( const size of sizes ) for ( const normalized of normalizations ) {
    const attribute = new Float16BufferAttribute( [ 0, 0, 0, 0 ], size, normalized );attribute.isFloat16BufferAttribute = false;
    row( 'builder actual half attribute', [ size, normalized, attempt( () => b.getTypeFromAttribute( attribute ) ) ] );
}
for ( let kind = 0; kind < 9; ++ kind ) for ( const normalized of normalizations ) {
    const attribute = new InterleavedBufferAttribute( { array: new constructors[ kind ]( 4 ), stride: 4, count: 1 }, 3, 0, normalized );
    row( 'builder actual interleaved', [ kind, normalized, attempt( () => b.getTypeFromAttribute( attribute ) ) ] );
}
for ( let mode = 0; mode < 32; ++ mode ) {
    const events = [], self = builder(), attribute = Object.create( mode & 2 ? Float16BufferAttribute.prototype : {} ), data = {}, typed = new Int16Array( 4 );
    for ( const key of [ 'isInterleavedBufferAttribute', 'data', 'array', 'itemSize', 'normalized', 'isFloat16BufferAttribute' ] ) {
        const result = key === 'isInterleavedBufferAttribute' ? Boolean( mode & 1 ) : key === 'data' ? data : key === 'array' ? typed : key === 'itemSize' ? 2 : key === 'normalized' ? mode & 4 ? true : 1 : undefined;
        getter( attribute, key, () => { events.push( [ 'get', key ] );if ( key === 'isFloat16BufferAttribute' ) throw new TypeError( 'flag must not be read' );return result; } );
    }
    getter( data, 'array', () => { events.push( [ 'get', 'data.array' ] );return typed; } );getter( typed, 'constructor', () => { events.push( [ 'get', 'array.constructor' ] );return Int16Array; } );
    for ( const method of [ 'getTypeFromArray', 'getTypeFromLength' ] ) getter( self, method, () => {
        events.push( [ 'get', method ] );return function ( ...args ) {
            if ( method === 'getTypeFromArray' ) { events.push( [ 'call', method, args[ 0 ] === typed ] );if ( mode & 16 ) throw new TypeError( 'array type' ); }
            else events.push( [ 'call', method, args[ 0 ], args[ 1 ] ] );return NodeBuilder.prototype[ method ].apply( this, args );
        };
    } );
    if ( mode & 8 ) getter( Float16BufferAttribute, Symbol.hasInstance, () => { events.push( [ 'get', 'hasInstance' ] );return function ( value ) { events.push( [ 'call', 'hasInstance', this === Float16BufferAttribute, value === attribute ] );return false; }; } );
    const result = attempt( () => self.getTypeFromAttribute( attribute ) );if ( mode & 8 ) delete Float16BufferAttribute[ Symbol.hasInstance ];
    row( 'builder attribute protocol', [ mode, result, events ] );
}
for ( let kind = 0; kind < 9; ++ kind ) for ( let mode = 0; mode < 11; ++ mode ) {
    const events = [], old = {}, fresh = {};function Alternate() {}Alternate.prototype = old;
    const phase = ( name, index ) => {
        events.push( [ name, index ] );
        if ( ( mode === 0 && name === 'iterator:get' ) || ( mode === 1 && name === 'iterator:call' ) || ( mode === 2 && name === 'next:get' ) ||
            ( mode === 3 && name === 'next' && index === 0 ) || ( mode === 4 && name === 'element' && index === 0 ) || ( mode === 5 && name === 'length' ) ||
            ( mode === 6 && name === 'index' && index === 0 ) || ( mode === 8 && name === 'offset' ) || ( mode === 9 && name === 'length:convert' ) ) Alternate.prototype = fresh;
        if ( mode === 10 && name === 'iterator:get' ) Alternate.prototype = null;
    };
    let input = {}, args;
    if ( mode === 7 ) { input = new Uint16Array( [ 1, 2 ] );getter( input, Symbol.iterator, () => { throw new TypeError( 'ignored typed iterator' ); } );args = [ input ]; }
    else if ( mode === 8 ) { const bytes = new constructors[ kind ]( 2 ).buffer;args = [ bytes, { [ Symbol.toPrimitive ]() { phase( 'offset' );return 0; } } ]; }
    else if ( mode === 5 || mode === 6 || mode === 9 ) {
        getter( input, 'length', () => { phase( 'length' );if ( mode !== 9 ) return 2;return { [ Symbol.toPrimitive ]() { phase( 'length:convert' );return 2; } }; } );
        for ( let i = 0; i < 2; ++ i ) getter( input, i, () => { phase( 'index', i );return i + 1; } );args = [ input ];
    } else {
        let index = 0;getter( input, Symbol.iterator, () => {
            phase( 'iterator:get' );return () => { phase( 'iterator:call' );const iterator = {};getter( iterator, 'next', () => {
                phase( 'next:get' );return () => { const i = index ++;phase( 'next', i );return { done: i === 2, value: { [ Symbol.toPrimitive ]() { phase( 'element', i );return i + 1; } } }; };
            } );return iterator; };
        } );args = [ input ];
    }
    row( 'typed allocation timing', [ kind, mode, attempt( () => {
        const result = Reflect.construct( constructors[ kind ], args, Alternate );return [ Object.getPrototypeOf( result ) === old, Object.getPrototypeOf( result ) === fresh, Object.getPrototypeOf( result ) === constructors[ kind ].prototype, [ ...Uint8Array.prototype.values.call( result ) ] ];
    } ), events ] );
}
const native = spawnSync( executable, [], { encoding: 'utf8', maxBuffer: 32 * 1024 * 1024, env: { ...process.env, TEMP: 'D:\\Temp\\surui', TMP: 'D:\\Temp\\surui' } } );
assert.equal( native.status, 0, native.stderr );const actual = native.stdout.trim().split( /\r?\n/ ).map( line => JSON.parse( line ) );
if ( outputDirectory ) { mkdirSync( outputDirectory, { recursive: true } );for ( const [ name, value ] of Object.entries( { 'native.json': actual, 'upstream.json': expected } ) ) writeFileSync( path.join( outputDirectory, name ), JSON.stringify( value, null, 2 ) + '\n' ); }
assert.equal( actual.length, expected.length );for ( let i = 0; i < expected.length; ++ i ) assert.deepStrictEqual( actual[ i ], expected[ i ], `case ${ i }: ${ expected[ i ].name }` );
console.log( `Float16BufferAttribute identity, native half methods and TypedArray construction: ${ expected.length } direct pinned-upstream cases passed` );
