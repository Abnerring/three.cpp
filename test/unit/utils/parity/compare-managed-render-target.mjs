import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';
import path from 'node:path';
import { mkdirSync, writeFileSync } from 'node:fs';
const [ upstream, executable, output ] = process.argv.slice( 2 );
assert.equal( execFileSync( 'git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' } ).trim(), '11738a9d55e502fb484cefffcea3a66dc44e9fe9' );
const load = relative => import( pathToFileURL( path.join( upstream, 'src', relative ) ) );
const [ { RenderTarget }, { default: CubeRenderTarget }, { Texture }, { TextureSource }, { CubeTexture }, { Vector2 }, { Vector3 }, { Vector4 } ] = await Promise.all( [ load( 'core/RenderTarget.js' ), load( 'renderers/common/CubeRenderTarget.js' ), load( 'textures/Texture.js' ), load( 'textures/TextureSource.js' ), load( 'textures/CubeTexture.js' ), load( 'math/Vector2.js' ), load( 'math/Vector3.js' ), load( 'math/Vector4.js' ) ] );
function encode( value ) {
    if ( value === undefined ) return '#undefined';
    if ( typeof value === 'number' ) { if ( Number.isNaN( value ) ) return '#NaN';if ( value === Infinity ) return '#Infinity';if ( value === - Infinity ) return '#-Infinity';if ( Object.is( value, - 0 ) ) return '#-0';return value; }
    if ( typeof value === 'function' ) return '#function';if ( typeof value === 'symbol' ) return '#symbol';
    if ( value === null || typeof value !== 'object' ) return value;
    if ( Object.hasOwn( value, 'tag' ) ) return { ref: encode( value.tag ) };
    if ( Array.isArray( value ) ) return Array.from( value, encode );
    return Object.fromEntries( Object.keys( value ).map( key => [ key, encode( value[ key ] ) ] ) );
}
const expected = [], row = ( kind, value ) => expected.push( { kind, value } );
const names = Object.getOwnPropertyNames;
const attempt = fn => { try { return encode( fn() ); } catch ( error ) { return error.name; } };
function textureState( texture, target ) {
    const image = texture.image;
    return { size: [ image.width, image.height, image.depth ].map( encode ), version: encode( texture.version ), sourceVersion: encode( texture.source.version ), flags: [ texture.isRenderTargetTexture, texture.isArrayTexture, texture.isData3DTexture, texture.isCubeTexture ].map( encode ), owner: texture.renderTarget === target,
        options: [ 'mapping', 'wrapS', 'wrapT', 'magFilter', 'minFilter', 'format', 'type', 'anisotropy', 'colorSpace', 'flipY', 'generateMipmaps', 'mipmapsAutoUpdate', 'internalFormat', 'unpackAlignment' ].map( key => encode( texture[ key ] ) ) };
}
function state( target ) {
    return { size: [ target.width, target.height, target.depth ].map( encode ), scissor: encode( target.scissor.toArray() ), viewport: encode( target.viewport.toArray() ),
        flags: [ 'scissorTest', 'depthBuffer', 'stencilBuffer', 'resolveColorBuffer', 'resolveDepthBuffer', 'resolveStencilBuffer', 'storeMultisampledColorBuffer', 'storeMultisampledDepthBuffer', 'storeMultisampledStencilBuffer', 'multiview', 'useArrayDepthTexture' ].map( key => encode( target[ key ] ) ), samples: encode( target.samples ), textures: target.textures.map( texture => textureState( texture, target ) ), sameSource: target.textures.length > 1 && target.textures[ 0 ].source === target.textures[ 1 ].source, sameImage: target.textures.length > 1 && target.textures[ 0 ].image === target.textures[ 1 ].image };
}
function metadata() {
    for ( const ctor of [ RenderTarget, CubeRenderTarget ] ) { const prototype = ctor.prototype, arities = names( prototype ).flatMap( key => { const d = Object.getOwnPropertyDescriptor( prototype, key );return typeof d.value === 'function' ? [ [ key, encode( d.value.length ) ] ] : []; } );row( 'constructor metadata', [ ctor.name, ctor.length, Object.getPrototypeOf( ctor ).name, names( prototype ), arities ] ); }
    row( 'target own keys', names( new RenderTarget() ) );for ( const ctor of [ Texture, TextureSource, CubeTexture ] ) row( 'resource own keys', names( new ctor() ) );
    const d = Object.getOwnPropertyDescriptor( new Texture(), 'id' );row( 'texture id descriptor', [ d.writable, d.enumerable, d.configurable ] );row( 'canonical identity', [ true, true, true, true ] );
}
function construction() {
    for ( const count of [ undefined, null, false, true, - 1, 0, 1, 2, 1.5, '2', 'bad', NaN ] ) row( 'construct count', state( new RenderTarget( 3, 4, { count, depth: 2, minFilter: 1008, flipY: true, internalFormat: 'R8' } ) ) );
    for ( const value of [ undefined, null, false, true, 0, - 0, - 1, 2.5, '5', '', NaN, Infinity, { tag: 'raw' } ] ) { const options = { count: 0 };for ( const key of [ 'depth', 'depthBuffer', 'stencilBuffer', 'resolveColorBuffer', 'resolveDepthBuffer', 'resolveStencilBuffer', 'storeMultisampledColorBuffer', 'storeMultisampledDepthBuffer', 'storeMultisampledStencilBuffer', 'samples', 'multiview', 'useArrayDepthTexture' ] ) options[ key ] = value;row( 'construct arbitrary values', state( new RenderTarget( value, value, options ) ) ); }
    for ( const options of [ null, false, 0, 'abc', [ 1, 2 ] ] ) row( 'construct primitive options', state( new RenderTarget( 3, 4, options ) ) );
}
function sizeCases() {
    for ( let mode = 0; mode < 12; ++ mode ) {
        const target = new RenderTarget( 3, 4, { count: 2 } ), trace = [];
        if ( mode === 1 ) target.width = '3';if ( mode === 2 ) target.depth = - 0;if ( mode === 3 ) target.textures[ 0 ].isData3DTexture = true;if ( mode === 4 ) target.textures[ 0 ].isData3DTexture = 1;
        target.addEventListener( 'dispose', function ( event ) { trace.push( [ 'dispose', event.target === this, this.width, this.height, this.depth ] );if ( mode === 5 ) throw new TypeError( 'stop' );if ( mode === 6 ) this.viewport = { set() { throw new TypeError( 'viewport' ); } };if ( mode === 7 ) this.width = 12; } );
        const width = mode === 8 ? '5' : mode === 9 ? NaN : 3, depth = mode === 10 ? '2' : mode === 11 ? null : mode >= 3 ? 2 : 1;
        const result = attempt( () => target.setSize( width, 4, depth ) );
        row( 'setSize', mode === 6 ? [ result, encode( trace ), encode( target.width ), encode( target.textures[ 0 ].image ) ] : [ result, encode( trace ), state( target ) ] );
    }
}
function depthCases() {
    for ( let mode = 0; mode < 10; ++ mode ) {
        const target = new RenderTarget(), other = new RenderTarget(), first = { tag: 'first', renderTarget: null };let next = { tag: 'next', renderTarget: mode === 1 ? other : null };target.depthTexture = first;
        if ( mode === 2 ) first.renderTarget = other;if ( mode === 3 ) delete next.renderTarget;if ( mode === 4 ) next = undefined;if ( mode === 5 ) next = null;if ( mode === 6 ) next = first;if ( mode === 7 ) target._depthTexture = undefined;if ( mode === 8 ) target._depthTexture = 4;if ( mode === 9 ) Object.defineProperty( next, 'renderTarget', { value: null, writable: false, configurable: true } );
        const result = attempt( () => { target.depthTexture = next; } );const owner = value => value === null || value === undefined ? encode( value ) : value.renderTarget === target ? 'target' : value.renderTarget === other ? 'other' : encode( value.renderTarget );row( 'depthTexture', [ result, encode( target.depthTexture ), owner( first ), owner( next ) ] );
    }
}
function copyCases() {
    for ( let mode = 0; mode < 12; ++ mode ) {
        let source = new RenderTarget( 4, 5, { count: 2, depth: 2 } );const destination = new RenderTarget( 9, 10 ), trace = [], original = source.texture, clone = original.clone, old = destination.textures;source.samples = 7;
        const capturedSource = source;
        original.clone = function () { const source = capturedSource;trace.push( [ 'clone', source.textures.length, destination.textures.length ] );if ( mode === 1 ) source.textures[ 0 ] = new Texture( { width: 13, height: 14, depth: 15 } );if ( mode === 2 ) source.textures.length = 0;if ( mode === 3 ) source.textures.push( new Texture() );if ( mode === 4 ) destination.textures = [ new Texture() ];if ( mode === 5 ) return this;if ( mode === 6 ) { destination.width = 31;throw new TypeError( 'stop' ); }if ( mode === 7 ) delete source.textures[ 1 ];if ( mode === 8 ) destination.textures = source.textures;return clone.call( this ); };
        if ( mode === 9 ) source.depthTexture = new Texture();if ( mode === 10 ) { const depth = new Texture();depth.renderTarget = destination;source.depthTexture = depth; }if ( mode === 11 ) source = destination;
        const result = attempt( () => destination.copy( source ) === destination );const textures = Array.from( destination.textures, t => t === undefined ? 'hole' : textureState( t, destination ) );row( 'copy', [ result, encode( trace ), encode( destination.width ), encode( destination.height ), encode( destination.depth ), encode( destination.samples ), textures, old === destination.textures, old.length, source.depthTexture === destination.depthTexture ] );
    }
}
function cubeCases() {
    for ( const count of [ 0, 1, 2 ] ) { const target = new CubeRenderTarget( 3, { count, depth: 4 } ), texture = target.texture, images = texture.images;row( 'cube constructor', [ state( target ), images.length, images[ 0 ] === images[ 5 ], encode( images[ 0 ] ), texture.renderTarget === null, Object.getPrototypeOf( texture ) === CubeTexture.prototype ] );row( 'cube clone', state( target.clone() ) ); }
    for ( let mode = 0; mode < 10; ++ mode ) {
        const target = new CubeRenderTarget(), trace = [], previous = { tag: 'previous' }, renderer = { getRenderTarget() { trace.push( 'get' );return previous; }, setRenderTarget( value, face ) { trace.push( [ 'set', value === target ? 'target' : value, encode( face ) ] );if ( mode === 1 && face === 2 ) throw new TypeError( 'set' ); }, clear( ...values ) { trace.push( [ 'clear', values ] );if ( mode === 2 ) throw new TypeError( 'clear' ); } };
        const args = mode >= 3 ? [ renderer, mode === 3 ? undefined : mode === 4 ? null : 'color', false, 0 ] : [ renderer ];row( 'cube clear', [ attempt( () => target.clear( ...args ) ), encode( trace ) ] );
    }
}
function protocols() {
    for ( const resource of [ Texture, TextureSource, CubeTexture ] ) row( 'resource prototype', [ resource.name, names( resource.prototype ) ] );
    for ( let mode = 0; mode < 8; ++ mode ) {
        const target = new RenderTarget( 3, 4, { count: 2 } ), trace = [], options = {};let reads = 0;
        for ( const key of [ 'mapping', 'minFilter', 'flipY', 'internalFormat' ] ) Object.defineProperty( options, key, { enumerable: true, configurable: true, get() { const n = reads ++;trace.push( [ 'get', key, n ] );return key === 'internalFormat' ? 'R8' : n; } } );
        const first = target.texture, base = first.setValues;first.setValues = function ( values ) {
            trace.push( [ 'setValues', encode( values ) ] );if ( mode !== 6 ) base.call( this, values );
            if ( mode === 1 ) target.textures.length = 0;if ( mode === 2 ) target.textures.push( new Texture( { width: 7, height: 8, depth: 1 } ) );if ( mode === 3 ) target.textures = [ this, new Texture( { width: 7, height: 8, depth: 1 } ) ];if ( mode === 4 ) throw new TypeError( 'stop' );if ( mode === 5 ) values.mapping = 'changed';if ( mode === 7 ) target.textures.length = 1;
        };
        row( 'texture options getters', [ attempt( () => target._setTextureOptions( options ) ), encode( trace ), state( target ) ] );
    }
    for ( const value of [ undefined, null, false, true, 0, - 0, '7', { tag: 'dimension' } ] ) { const source = new TextureSource( { width: value, height: value, depth: value } ), target = new Vector3();row( 'source getSize', [ source.getSize( target ) === target, encode( target.toArray() ) ] ); }
    const texture = new Texture( { data: new Uint8Array( [ 1, 2, 3, 4 ] ), width: 2, height: 1 } );texture.uuid = 'texture';texture.source.uuid = 'source';texture.userData = { nested: { v: 2 } };
    row( 'texture serialize', encode( texture.toJSON() ) );const meta = { textures: {}, images: {} }, serial = texture.toJSON( meta );row( 'texture serialization cache', [ serial === texture.toJSON( meta ), encode( meta ) ] );
    for ( const value of [ false, 1, 'true', true ] ) { texture.needsUpdate = value;texture.needsPMREMUpdate = value;row( 'texture update flags', [ texture.version, texture.source.version, texture.pmremVersion ] ); }
    texture.addUpdateRange( 1, 2 );texture.addUpdateRange( 3, 4 );row( 'texture update ranges', encode( texture.updateRanges ) );texture.clearUpdateRanges();row( 'texture clear update ranges', encode( texture.updateRanges ) );
    for ( const mapping of [ 300, 999 ] ) for ( const wrap of [ 1000, 1001, 1002 ] ) { const uv = new Vector2( - 1.25, 2.25 );texture.mapping = mapping;texture.wrapS = wrap;texture.wrapT = wrap;row( 'texture transformUv', [ texture.transformUv( uv ) === uv, encode( uv.toArray() ) ] ); }
}
metadata();construction();sizeCases();depthCases();copyCases();cubeCases();protocols();
// The native probe additionally proves actual managed-cycle collection. These
// expected reachability booleans are separate from the upstream behavior rows.
row( 'managed resource cycle collection', [ true, true, true, true ] );
for ( const value of [ undefined, null, false, 4, 'a😀', [ 1, 2 ], { x: 1 }, new Uint8Array( [ 1, 2 ] ) ] ) { row( 'Object.keys', attempt( () => Object.keys( value ) ) );row( 'Array.isArray', Array.isArray( value ) ); }
const spoof = Object.create( Array.prototype ), branded = [];Object.setPrototypeOf( branded, Object.prototype );row( 'Array.isArray internal brand', [ Array.isArray( spoof ), Array.isArray( branded ) ] );
for ( let mode = 0; mode < 5; ++ mode ) {
    const trace = [], data = { a: 1, get b() { throw new TypeError( 'Object.keys must not read getter' ); }, c: 3 }, proxy = new Proxy( data, {
        ownKeys() { trace.push( 'ownKeys' );return [ 'a', 'b', 'c' ]; },
        getOwnPropertyDescriptor( data, key ) { trace.push( [ 'descriptor', key ] );if ( key === 'a' ) { if ( mode === 1 ) delete data.b;if ( mode === 2 ) Object.defineProperty( data, 'c', { enumerable: false } );if ( mode === 3 ) data.d = 4;if ( mode === 4 ) throw new TypeError( 'descriptor' ); }return Reflect.getOwnPropertyDescriptor( data, key ); }
    } );row( 'Object.keys descriptor protocol', [ attempt( () => Object.keys( proxy ) ), encode( trace ) ] );
}
for ( let mode = 0; mode < 6; ++ mode ) {
    const texture = new Texture(), uv = new Vector2( 0.25, 0.5 ), trace = [];texture.flipY = false;
    Object.defineProperty( uv, 'applyMatrix3', { configurable: true, get() {
        trace.push( 'get applyMatrix3' );if ( mode === 2 ) throw new TypeError( 'method' );if ( mode === 5 ) return 4;
        if ( mode === 4 ) Object.defineProperty( texture, 'matrix', { configurable: true, value: { tag: 'changed matrix' } } );
        return function ( matrix ) { trace.push( [ 'call original', this === uv, matrix ] );return this; };
    } } );
    Object.defineProperty( texture, 'matrix', { configurable: true, get() {
        trace.push( 'get matrix' );if ( mode === 3 ) throw new TypeError( 'matrix' );
        if ( mode === 1 ) Object.defineProperty( uv, 'applyMatrix3', { configurable: true, value() { trace.push( 'call replacement' ); } } );return { tag: 'matrix' };
    } } );row( 'Texture.transformUv evaluation order', [ attempt( () => texture.transformUv( uv ) === uv ), encode( trace ) ] );
}
for ( let mode = 0; mode < 6; ++ mode ) {
    const trace = [], image = {}, entry = {}, prototype = Array.prototype, saved = Object.getOwnPropertyDescriptor( prototype, 'push' ), original = prototype.push, pixels = new Uint8Array( [ 1, 2 ] );
    Object.defineProperty( image, 'data', { configurable: true, get() {
        original.call( trace, 'get data' );if ( mode === 3 ) throw new TypeError( 'data' );
        if ( mode === 4 ) Object.defineProperty( prototype, 'push', { configurable: true, value() { original.call( trace, 'call replacement' );return 99; } } );return pixels;
    } } );image.width = 2;image.height = 1;
    const item = mode === 1 ? entry : image;
    Object.defineProperty( item, 'isDataTexture', { configurable: true, get() { original.call( trace, 'get isDataTexture' );if ( mode === 5 ) throw new TypeError( 'flag' );return mode === 1; } } );
    if ( mode === 1 ) Object.defineProperty( entry, 'image', { configurable: true, get() { original.call( trace, 'get image' );return image; } } );
    const source = new TextureSource( [ item ] );source.uuid = 'source';
    Object.defineProperty( prototype, 'push', { configurable: true, get() {
        original.call( trace, 'get push' );if ( mode === 2 ) throw new TypeError( 'push' );return function ( ...args ) { original.call( trace, 'call original' );return original.apply( this, args ); };
    } } );let result;try { result = attempt( () => source.toJSON() ); } finally { Object.defineProperty( prototype, 'push', saved ); }
    row( 'TextureSource.toJSON evaluation order', [ result, encode( trace ) ] );
}
for ( let count = 2; count <= 4; ++ count ) {
    const ctor = count === 2 ? Vector2 : count === 3 ? Vector3 : Vector4;
    for ( const method of [ 'clone', 'equals' ] ) {
        const self = new ctor(), other = new ctor(), trace = [];
        for ( const value of [ self, other ] ) for ( let i = 0; i < count; ++ i ) { const key = 'xyzw'[ i ];Object.defineProperty( value, key, { configurable: true, get() { trace.push( [ value === self ? 'self' : 'other', key ] );return i + 1; } } ); }
        Object.defineProperty( self, 'constructor', { configurable: true, get() { trace.push( 'get constructor' );return ctor; } } );
        row( 'Vector evaluation order', [ count, method, attempt( () => method === 'clone' ? self.clone().toArray() : self.equals( other ) ), encode( trace ) ] );
    }
}
if ( output ) { mkdirSync( output, { recursive: true } );writeFileSync( path.join( output, 'managed-render-target-upstream.json' ), JSON.stringify( expected, null, 2 ) ); }
let text;
try { text = execFileSync( executable, [], { encoding: 'utf8', maxBuffer: 16 * 1024 * 1024 } ); } catch ( error ) { if ( output ) { writeFileSync( path.join( output, 'managed-render-target-native.jsonl' ), error.stdout ?? '' );writeFileSync( path.join( output, 'managed-render-target-native.err.log' ), error.stderr ?? '' ); }throw error; }
const actual = text.trim().split( /\r?\n/ ).filter( Boolean ).map( line => JSON.parse( line ) );
if ( output ) writeFileSync( path.join( output, 'managed-render-target-native.json' ), JSON.stringify( actual, null, 2 ) );
assert.equal( actual.length, expected.length );for ( let i = 0; i < expected.length; ++ i ) assert.deepStrictEqual( actual[ i ], expected[ i ], `row ${ i }: ${ expected[ i ].kind }` );
console.log( `Managed RenderTarget/CubeRenderTarget: ${ expected.length - 1 } direct upstream rows + 1 actual native GC snapshot passed` );
