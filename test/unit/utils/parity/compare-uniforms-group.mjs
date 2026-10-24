import { execFileSync, spawnSync } from 'node:child_process';
import { mkdirSync, writeFileSync } from 'node:fs';
import { pathToFileURL } from 'node:url';
import path from 'node:path';
import assert from 'node:assert/strict';
const [ upstream, executable, outputDirectory ] = process.argv.slice( 2 );
if ( ! upstream || ! executable ) throw Error( 'compare-uniforms-group.mjs <pinned-three.js> <probe> [output-directory]' );
assert.equal( execFileSync( 'git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' } ).trim(), '11738a9d55e502fb484cefffcea3a66dc44e9fe9' );
const load = file => import( pathToFileURL( path.join( upstream, 'src', file ) ) );
const UniformsGroup = ( await load( 'renderers/common/UniformsGroup.js' ) ).default;
const NodeUniformsGroup = ( await load( 'renderers/common/nodes/NodeUniformsGroup.js' ) ).default;
const uniforms = await load( 'renderers/common/nodes/NodeUniform.js' );
const { setConsoleFunction } = await load( 'utils.js' );
const undef = { tag: 'undefined' }, nan = { tag: 'nan' }, negzero = { tag: 'negative-zero' }, infinity = { tag: 'infinity' };
const number = ( value = 0, type = 'float' ) => ( { kind: 'Number', type, value } );
const examples = [ number( 1.5 ), { kind: 'Vector2', type: 'vec2', value: { x: 1.2, y: -2.3 } }, { kind: 'Vector3', type: 'vec3', value: { x: 1, y: 2, z: 3 } }, { kind: 'Vector4', type: 'vec4', value: { x: 1, y: 2, z: 3, w: 4 } }, { kind: 'Color', type: 'color', value: { r: 0.1, g: 0.2, b: 0.3 } }, { kind: 'Matrix2', type: 'mat2', value: { elements: [ 1, 2, 3, 4 ] } }, { kind: 'Matrix3', type: 'mat3', value: { elements: [ 1, 2, 3, 4, 5, 6, 7, 8, 9 ] } }, { kind: 'Matrix4', type: 'mat4', value: { elements: Array.from( { length: 16 }, ( _, i ) => i + 1 ) } } ];
const requests = [];
for ( const nodeGroup of [ false, true ] ) requests.push( { kind: 'metadata', nodeGroup } );
requests.push( { kind: 'layout', uniforms: [] } );
for ( const uniform of examples ) requests.push( { kind: 'layout', uniforms: [ uniform ] } );
for ( const index of [ 1, 2, 4, 5, 6, 7 ] ) for ( const count of [ 1, 2, 3 ] ) requests.push( { kind: 'layout', uniforms: [ ...Array.from( { length: count }, () => number() ), examples[ index ], number() ] } );
requests.push( { kind: 'layout', nodeGroup: true, uniforms: examples } );
for ( const type of [ 'float', 'int', 'uint', 'bool', 'other' ] ) for ( const value of [ 0, 1.2, -1.2, 4294967297, negzero, nan, infinity ] ) requests.push( { kind: 'updates', uniforms: [ number( value, type ) ], steps: [ {}, {}, { clear: true }, { values: [ 7.75 ] } ] } );
for ( const example of examples ) requests.push( { kind: 'updates', nodeGroup: true, uniforms: [ number(), example ], steps: [ {}, {}, { clear: true }, { values: [ 7, example.value ] } ] } );
for ( const [ kind, count ] of [ [ 'Vector2', 2 ], [ 'Vector3', 3 ], [ 'Vector4', 4 ] ] ) for ( const prefix of [ 'i', 'u' ] ) requests.push( { kind: 'updates', uniforms: [ { kind, type: `${ prefix }vec${ count }`, value: Object.fromEntries( [ 'x', 'y', 'z', 'w' ].slice( 0, count ).map( ( c, i ) => [ c, i - 2.7 ] ) ) } ], steps: [ {}, {}, { clear: true } ] } );
requests.push( { kind: 'ranges', uniforms: [ number( 5 ) ] } );
requests.push( { kind: 'addRemove', uniforms: [ number( 5 ), examples[ 1 ] ] } );
for ( const result of [ true, false, 1, undef ] ) requests.push( { kind: 'iterator', uniforms: [ number() ], result } );
requests.push( { kind: 'iterator', uniforms: [ number() ], throw: true } );
for ( const example of examples.filter( value => value.kind !== 'Matrix2' ) ) {
    requests.push( { kind: 'protocol', uniforms: [ example ], method: `update${ example.kind }` } );
    requests.push( { kind: 'protocol', uniforms: [ example ], method: `update${ example.kind }`, reenter: true } );
}
requests.push( { kind: 'protocol', uniforms: [ number( 5 ) ], method: 'updateNumber', replaceValues: true } );
requests.push( { kind: 'protocol', uniforms: [ number( 5 ) ], method: 'updateNumber', throwType: true } );
for ( const example of [ examples[ 0 ], examples[ 1 ], examples[ 6 ] ] ) requests.push( { kind: 'protocol', uniforms: [ example ], method: `update${ example.kind }`, offsetObject: true } );
for ( const method of [ 'release', 'clearUpdateRanges' ] ) requests.push( { kind: 'super', method } );

function run( request ) {
    const state = { events: [] }, result = {};
    const normal = ( value, depth = 0 ) => {
        if ( value === undefined ) return '#undefined';if ( typeof value === 'function' ) return '#function';if ( typeof value === 'symbol' ) return '#symbol';
        if ( typeof value === 'number' ) { if ( Number.isNaN( value ) ) return '#NaN';if ( value === Infinity ) return '#Infinity';if ( value === -Infinity ) return '#-Infinity';if ( Object.is( value, -0 ) ) return '#-0'; }
        if ( ! value || typeof value !== 'object' ) return value;for ( const key of [ 'group', 'uniform', 'value', 'values', 'buffer' ] ) if ( value === state[ key ] ) return `#${ key }`;
        if ( value instanceof Map || value instanceof Set ) return [];if ( depth > 8 ) return '#depth';const out = Array.isArray( value ) ? new Array( value.length ) : {};for ( const key of Object.keys( value ) ) out[ key ] = normal( value[ key ], depth + 1 );return out;
    };
    const decode = value => {
        if ( ! value || typeof value !== 'object' ) return value;if ( value.tag === 'undefined' ) return undefined;if ( value.tag === 'nan' ) return NaN;if ( value.tag === 'negative-zero' ) return -0;if ( value.tag === 'infinity' ) return Infinity;
        return Array.isArray( value ) ? value.map( decode ) : Object.fromEntries( Object.entries( value ).map( ( [ k, v ] ) => [ k, decode( v ) ] ) );
    };
    const event = ( ...values ) => state.events.push( values.map( value => normal( value ) ) );
    const getter = ( self, key, get ) => Object.defineProperty( self, key, { configurable: true, enumerable: true, get } );
    const Constructor = request.nodeGroup ? NodeUniformsGroup : UniformsGroup, groupNode = { name: 'fixture-node' }, group = state.group = new Constructor( 'fixture', groupNode );
    for ( const spec of request.uniforms ?? [] ) group.addUniform( new uniforms[ `${ spec.kind }NodeUniform` ]( { name: 'fixture', type: decode( spec.type ), value: decode( spec.value ) } ) );
    const proto = UniformsGroup.prototype, oldParent = Object.getPrototypeOf( proto ), oldCtorParent = Object.getPrototypeOf( Constructor );
    setConsoleFunction( ( level, message ) => event( 'console', level, message.startsWith( 'THREE.WebGPUUniformsGroup:' ) ) );
    const snapshot = ( plain = false ) => {
        const convert = plain ? value => {
            const ownState = {};for ( const key of [ 'group', 'uniform', 'value', 'values', 'buffer' ] ) { ownState[ key ] = state[ key ];delete state[ key ]; }
            try { return normal( value ); } finally { Object.assign( state, ownState ); }
        } : value => normal( value );
        return { values: [ ...group.values ].map( convert ), bytes: [ ...new Uint8Array( group.buffer.buffer ) ].map( convert ), ranges: convert( group.updateRanges ), added: [ ...group._addedIndices ].map( convert ), cacheSize: group._updateRangeCache.size, offsets: Array.from( group.uniforms, u => [ convert( u.offset ), u.index ] ) };
    };
    try {
        if ( request.kind === 'metadata' ) {
            result.fields = Object.fromEntries( Object.getOwnPropertyNames( group ).map( key => [ key, key === 'id' ? '#id' : normal( group[ key ] ) ] ) );
            result.methods = Object.getOwnPropertyNames( Constructor.prototype ).filter( key => key !== 'constructor' ).map( key => { const d = Object.getOwnPropertyDescriptor( Constructor.prototype, key ), fn = d.get ?? d.value;return [ key, fn.name, fn.length, false, d.enumerable, d.configurable, d.writable ?? '#accessor' ]; } );result.arity = Constructor.length;result.groupNode = group.groupNode === groupNode;if ( request.nodeGroup ) result.idDelta = new Constructor().id - group.id;
        } else if ( request.kind === 'layout' ) { result.initial = [ group._buffer === null, group._values === null ];result.byteLength = group.byteLength;const buffer = group.buffer, values = group.values;result.identity = [ buffer === group.buffer, values === group.values ];result.snapshot = snapshot();
        } else if ( request.kind === 'updates' ) {
            result.rows = [];for ( const step of request.steps ) { if ( step.clear ) group.clearUpdateRanges();if ( step.values ) step.values.forEach( ( value, i ) => { group.uniforms[ i ].nodeUniform.value = decode( value ); } );const updated = group.update();const row = snapshot();row.updated = normal( updated );result.rows.push( row ); }
        } else if ( request.kind === 'ranges' ) {
            group.buffer;const uniform = group.uniforms[ 0 ];group.addUniformUpdateRange( uniform );const range = group.updateRanges[ 0 ];group.addUniformUpdateRange( uniform );result.dedup = group.updateRanges.length;group.clearUpdateRanges();uniform.offset = 7;uniform.itemSize = 3;group.addUniformUpdateRange( uniform );result.identity = range === group.updateRanges[ 0 ];result.range = normal( range );result.cacheSize = group._updateRangeCache.size;
            const oldBuffer = group.buffer, oldValues = group.values;group.release();result.released = [ group._buffer === null, group._values === null ];result.newIdentity = [ oldBuffer !== group.buffer, oldValues !== group.values ];result.rangeAfterRelease = normal( group.updateRanges );
        } else if ( request.kind === 'addRemove' ) {
            const uniform = group.uniforms[ 0 ];result.add = group.addUniform( uniform ) === group;result.remove = group.removeUniform( uniform ) === group;result.missing = group.removeUniform( {} ) === group;result.length = group.uniforms.length;result.lastIdentity = group.uniforms[ group.uniforms.length - 1 ] === uniform;result.snapshot = snapshot();
        } else if ( request.kind === 'protocol' ) {
            const uniform = state.uniform = group.uniforms[ 0 ];group.buffer;group.values;const value = state.value = uniform.nodeUniform.value;state.values = group.values;state.buffer = group.buffer;let valueCalls = 0;
            for ( const name of [ 'getValue', 'getType' ] ) getter( uniform, name, function () { event( 'get', name, this );return function ( ...args ) { event( name, this, args );if ( name === 'getValue' ) { const index = valueCalls ++;if ( request.reenter && index === 0 ) event( 'reentered', group[ request.method ]( this ) );return value; }if ( request.replaceValues ) group._values = [ 99, 98, 97, 96 ];if ( request.throwType ) throw new TypeError( 'type' );return request.uniforms[ 0 ].type; }; } );
            let offset = Object.hasOwn( request, 'offset' ) ? decode( request.offset ) : 0;
            if ( request.offsetObject ) {
                offset = { [ Symbol.toPrimitive ]( hint ) { event( 'offset:key', hint, state.values[ 0 ] );return 0; } };
                let stored = state.values[ 0 ];Object.defineProperty( state.values, '0', { configurable: true, enumerable: true, get() { return stored; }, set( value ) { stored = value;event( 'values:set', value ); } } );
            }
            getter( uniform, 'offset', () => { event( 'offset' );return offset; } );
            if ( value && typeof value === 'object' ) for ( const component of [ 'x', 'y', 'z', 'w', 'r', 'g', 'b' ] ) if ( Object.hasOwn( value, component ) ) { let n = 0;const original = value[ component ];getter( value, component, () => { event( 'component', component, n ++ );return original; } ); }
            for ( const member of [ 'values', 'buffer', '_getBufferForType', 'addUniformUpdateRange' ] ) { const original = member === 'values' ? state.values : member === 'buffer' ? state.buffer : group[ member ];getter( group, member, function () { event( 'group:get', member, this );if ( typeof original !== 'function' ) return original;return function ( ...args ) { event( 'group:call', member, this, args );return original.apply( this, args ); }; } ); }
            result.updated = normal( group[ request.method ]( uniform ) );result.snapshot = snapshot( true );
        } else if ( request.kind === 'iterator' ) {
            const uniform = group.uniforms[ 0 ];let n = 0;group.uniforms = { [ Symbol.iterator ]() { return { next() { return { done: n ++ >= 2, value: uniform }; }, return() { event( 'iterator:return' );return {}; } }; } };group.updateByType = () => { event( 'updateByType' );if ( request.throw ) throw new TypeError( 'update' );return decode( request.result ); };result.updated = normal( group.update() );
        } else if ( request.kind === 'super' ) {
            const alternate = Object.create( oldParent );getter( alternate, request.method, function () { event( 'super:get', this );return function ( ...args ) { event( 'super:call', this, args );return 7; }; } );Object.setPrototypeOf( proto, alternate );result.returned = normal( group[ request.method ]() );result.values = normal( group._values );
        }
    } catch ( error ) { result.error = error.name; } finally { Object.setPrototypeOf( proto, oldParent );Object.setPrototypeOf( Constructor, oldCtorParent );setConsoleFunction( null ); }
    result.events = state.events;return result;
}
const native = spawnSync( executable, [], { input: JSON.stringify( requests ), encoding: 'utf8', maxBuffer: 32 * 1024 * 1024, env: { ...process.env, TEMP: 'D:\\Temp\\surui', TMP: 'D:\\Temp\\surui' } } );
assert.equal( native.status, 0, native.stderr );const actual = JSON.parse( native.stdout ), expected = requests.map( run );
if ( outputDirectory ) { mkdirSync( outputDirectory, { recursive: true } );for ( const [ name, value ] of Object.entries( { 'fixtures.json': requests, 'native.json': actual, 'upstream.json': expected } ) ) writeFileSync( path.join( outputDirectory, name ), JSON.stringify( value, null, 2 ) + '\n' ); }
for ( let i = 0; i < requests.length; ++i ) assert.deepStrictEqual( actual[ i ], expected[ i ], `case ${ i }: ${ JSON.stringify( requests[ i ] ) }` );
console.log( `UniformsGroup/NodeUniformsGroup: ${ requests.length } direct pinned-upstream cases passed` );
