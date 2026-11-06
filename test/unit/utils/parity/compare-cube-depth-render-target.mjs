import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

const [ upstreamArg, executable, outputArg ] = process.argv.slice( 2 );
const upstream = path.resolve( upstreamArg ), output = outputArg && path.resolve( outputArg );
const lock = JSON.parse( fs.readFileSync( new URL( '../../../../upstream.json', import.meta.url ) ) );
assert.equal( execFileSync( 'git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' } ).trim(), lock.commit );
const { CubeDepthTexture } = await import( pathToFileURL( path.join( upstream, 'src/textures/CubeDepthTexture.js' ) ) );
const { DepthTexture } = await import( pathToFileURL( path.join( upstream, 'src/textures/DepthTexture.js' ) ) );
const { Texture } = await import( pathToFileURL( path.join( upstream, 'src/textures/Texture.js' ) ) );
const { WebGLCubeRenderTarget } = await import( pathToFileURL( path.join( upstream, 'src/renderers/WebGLCubeRenderTarget.js' ) ) );
const { RenderTarget } = await import( pathToFileURL( path.join( upstream, 'src/core/RenderTarget.js' ) ) );
const expected = [], row = ( kind, value ) => expected.push( { kind, value } );
function encode( value ) {
    if ( value === undefined ) return '#undefined'; if ( value === null ) return null;
    if ( typeof value === 'number' ) return Number.isNaN( value ) ? '#NaN' : ! Number.isFinite( value ) ? value < 0 ? '#-Infinity' : '#Infinity' : Object.is( value, - 0 ) ? '#-0' : value;
    if ( typeof value === 'function' ) return '#function'; if ( typeof value !== 'object' ) return value;
    if ( Object.hasOwn( value, 'tag' ) ) return { ref: value.tag };
    if ( Array.isArray( value ) ) return Array.from( value, encode );
    return Object.fromEntries( Object.keys( value ).map( key => [ key, encode( value[ key ] ) ] ) );
}
function attempt( callback ) { try { return encode( callback() ); } catch ( error ) { return error.name; } }
function textureState( texture, target = null ) {
    const image = texture.image, cube = Array.isArray( image ) || image && Object.hasOwn( image, '0' );
    return { own: Object.getOwnPropertyNames( texture ).filter( key => key !== 'id' && key !== 'uuid' ),
        options: [ 'mapping', 'wrapS', 'wrapT', 'magFilter', 'minFilter', 'format', 'type', 'anisotropy', 'colorSpace', 'flipY', 'generateMipmaps', 'compareFunction', 'isDepthTexture', 'isCubeTexture', 'isCubeDepthTexture', 'isRenderTargetTexture', 'version' ].map( key => encode( texture[ key ] ) ),
        image: encode( image ), imageArray: Array.isArray( image ), aliases: cube && image[ 0 ] === image[ 5 ], imagesAlias: texture.images === image,
        sourceVersion: texture.source.version, owner: texture.renderTarget === target };
}
function targetState( target ) {
    return { own: Object.getOwnPropertyNames( target ), size: [ target.width, target.height, target.depth ].map( encode ),
        flags: [ target.isWebGLRenderTarget, target.isWebGLCubeRenderTarget, target.depthBuffer, target.stencilBuffer, target.samples, target.scissorTest ].map( encode ),
        viewport: encode( target.viewport.toArray() ), textures: target.textures.map( texture => textureState( texture, target ) ) };
}
function metadata() {
    for ( const ctor of [ CubeDepthTexture, WebGLCubeRenderTarget ] ) {
        const prototype = ctor.prototype, entries = Object.getOwnPropertyNames( prototype ).map( key => {
            const d = Object.getOwnPropertyDescriptor( prototype, key ), fn = d.value ?? d.get;
            return [ key, d.writable ?? null, d.enumerable, d.configurable, fn.name, fn.length, d.set ? [ d.set.name, d.set.length ] : null, fn.toString(), d.set?.toString() ?? null ];
        } );
        row( 'metadata', [ ctor.name, ctor.length, Object.getPrototypeOf( ctor ).name, Object.getPrototypeOf( prototype ) === Object.getPrototypeOf( ctor ).prototype, entries, attempt( () => ctor() ) ] );
    }
    row( 'module identity', [ true, true, Object.getPrototypeOf( CubeDepthTexture ) === DepthTexture ] );
}
function construction() {
    const cases = [ [], [ 3 ], [ null ], [ - 0 ], [ '4' ], [ { tag: 'size' } ], [ NaN ],
        [ 5, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined ],
        [ 2, 1015, 305, 1000, 1002, 1006, 1008, 3, 1026 ], [ 2, null, null, false, 'wrap', null, true, 'anisotropy', 1026 ], [ 2, 1014, 301, 1001, 1001, 1003, 1003, 1, 1027 ] ];
    for ( const args of cases ) row( 'depth construction', textureState( new CubeDepthTexture( ...args ) ) );
    row( 'depth invalid format', attempt( () => new CubeDepthTexture( 2, undefined, undefined, undefined, undefined, undefined, undefined, undefined, 999 ) ) );
    for ( const size of [ undefined, null, - 0, - 2.5, '4', { tag: 'size' }, NaN ] ) for ( const count of [ 0, 1, 2 ] ) {
        row( 'cube target construction', targetState( new WebGLCubeRenderTarget( size, { count, depth: 3, depthBuffer: false, samples: 2, minFilter: 1003, wrapS: 1000, colorSpace: 'srgb' } ) ) );
    }
    for ( const options of [ undefined, null, false, 0, 'x' ] ) row( 'cube primitive options', attempt( () => targetState( new WebGLCubeRenderTarget( 3, options ) ) ) );
}
function aliasesAndCopies() {
    const get = Object.getOwnPropertyDescriptor( CubeDepthTexture.prototype, 'images' ).get, set = Object.getOwnPropertyDescriptor( CubeDepthTexture.prototype, 'images' ).set;
    for ( const value of [ undefined, null, false, 3, { tag: 'image' }, [ { width: 4 } ] ] ) {
        const self = { image: { tag: 'old' } };row( 'images accessor', [ attempt( () => set.call( self, value ) ), encode( self.image ), get.call( self ) === value ] );
    }
    for ( let mode = 0; mode < 4; ++ mode ) {
        const source = new CubeDepthTexture( 7 );source.compareFunction = 515;source.name = 'source';
        const result = mode % 2 ? new CubeDepthTexture( 2 ).copy( source ) : source.clone();
        const originalImage = source.image;result.image[ 0 ].width = 19;
        row( 'depth structural copy', [ mode, textureState( result ), result !== source, result.source !== source.source, result.image !== source.image,
            result.image[ 0 ] === source.image[ 0 ], source.image === originalImage, source.image[ 5 ].width, Object.getPrototypeOf( result ) === CubeDepthTexture.prototype ] );
    }
    for ( let mode = 0; mode < 4; ++ mode ) {
        const source = new WebGLCubeRenderTarget( 4, { count: 2 } );source.depthTexture = new CubeDepthTexture( 4 );
        const result = mode % 2 ? new WebGLCubeRenderTarget( 2 ).copy( source ) : source.clone();
        row( 'cube structural copy', [ mode, targetState( result ), result !== source, result.texture.source !== source.texture.source,
            result.texture.image !== source.texture.image, result.texture.image[ 0 ] === source.texture.image[ 0 ], result.depthTexture !== source.depthTexture,
            result.depthTexture.source !== source.depthTexture.source, result.depthTexture.renderTarget === result, Object.getPrototypeOf( result ) === WebGLCubeRenderTarget.prototype ] );
    }
    for ( const source of [ new CubeDepthTexture(), new WebGLCubeRenderTarget() ] ) {
        const trace = [], result = { copy( value ) { trace.push( value === source );return 'copied'; } };
        Object.defineProperty( source, 'constructor', { configurable: true, get() { trace.push( 'constructor' );return function Replacement() { trace.push( [ 'construct', arguments.length, new.target === Replacement ] );return result; }; } } );
        row( 'dynamic clone', [ attempt( () => source.clone() ), trace ] );
    }
}
function dynamicSuper() {
    for ( const cube of [ false, true ] ) for ( let mode = 0; mode < 4; ++ mode ) {
        const ctor = cube ? WebGLCubeRenderTarget : CubeDepthTexture, previous = Object.getPrototypeOf( ctor ), trace = [];
        function Sub() {} Sub.prototype = Object.create( ctor.prototype );
        function Base( ...args ) {
            trace.push( [ 'super', new.target === Sub, args.map( encode ) ] );if ( mode === 1 ) throw new TypeError( 'super' );
            const result = Object.create( new.target.prototype );
            if ( cube ) { result.textures = [];result._setTextureOptions = function ( options ) { trace.push( [ 'options', this === result, encode( options ), this.texture.isCubeTexture ] );if ( mode === 2 ) throw new TypeError( 'options' ); }; }
            else if ( mode === 2 ) Object.defineProperty( result, 'image', { set() { trace.push( 'set image' );throw new TypeError( 'image' ); } } );
            if ( mode === 3 ) Object.defineProperty( result, cube ? 'isWebGLCubeRenderTarget' : 'isCubeDepthTexture', { set( value ) { trace.push( [ 'flag', value ] );throw new TypeError( 'flag' ); } } );
            return result;
        }
        Object.setPrototypeOf( ctor, Base );let value;
        const result = attempt( () => { value = Reflect.construct( ctor, cube ? [ 6, { tag: 'options' } ] : [ 6 ], Sub );return [ Object.getPrototypeOf( value ) === Sub.prototype, value.isCubeDepthTexture, value.isCubeTexture, value.isWebGLCubeRenderTarget, encode( cube ? value.texture.image : value.image ) ]; } );
        Object.setPrototypeOf( ctor, previous );row( 'dynamic super', [ cube, mode, result, trace ] );
    }
}
function optionsOrder() {
    const prototype = RenderTarget.prototype, original = prototype._setTextureOptions;
    for ( let mode = 0; mode < 4; ++ mode ) {
        const trace = [], options = {};let reads = 0;
        Object.defineProperty( options, 'minFilter', { enumerable: true, get() { trace.push( [ 'minFilter', ++ reads ] );return reads % 2 ? 1003 : 1006; } } );
        prototype._setTextureOptions = function ( value ) {
            trace.push( [ 'options', value === options, this.isWebGLRenderTarget === true, this.texture.isCubeTexture === true, this.texture.renderTarget === this ] );
            if ( mode === 1 && this.texture.isCubeTexture ) throw new TypeError( 'second' );
            if ( mode === 2 ) return 'ignored';return original.call( this, value );
        };
        let result;try { result = attempt( () => targetState( new WebGLCubeRenderTarget( 3, options ) ) ); } finally { prototype._setTextureOptions = original; }
        row( 'options source order', [ mode, result, trace ] );
    }
}
function clearCases() {
    for ( let mode = 0; mode < 12; ++ mode ) {
        const target = new WebGLCubeRenderTarget(), previous = { tag: 'previous' }, trace = [];let face = - 1, nested = false;
        const renderer = { getRenderTarget() { trace.push( [ 'get', this === renderer ] );if ( mode === 3 ) throw new TypeError( 'get' );return previous; },
            setRenderTarget( value, next ) { face = next;trace.push( [ 'set', this === renderer, value === target ? 'target' : encode( value ), encode( next ), arguments.length ] );if ( mode === 1 && next === 2 ) throw new TypeError( 'set' ); },
            clear( ...args ) { trace.push( [ 'clear', this === renderer, args.map( encode ) ] );if ( mode === 2 && face === 2 ) throw new TypeError( 'clear' );if ( mode === 10 && ! nested ) { nested = true;target.clear( renderer, false, true, false ); } } };
        if ( mode === 11 ) { const old = renderer.clear;Object.defineProperty( renderer, 'clear', { configurable: true, get() { trace.push( 'get clear' );return old; } } ); }
        const args = mode < 4 || mode >= 10 ? [ renderer ] : [ renderer, mode === 4 ? undefined : mode === 5 ? null : mode === 6 ? false : mode === 7 ? 0 : { tag: 'color' }, false, 0 ];
        row( 'clear', [ mode, attempt( () => target.clear( ...args ) ), trace ] );
    }
}
function equirectCases() {
    for ( let mode = 0; mode < 10; ++ mode ) {
        const target = new WebGLCubeRenderTarget( 8 ), texture = new Texture(), previous = { tag: 'previous' }, trace = [];
        const values = { type: 1015, colorSpace: 'srgb', generateMipmaps: true, minFilter: mode === 6 ? 1003 : 1008, magFilter: 1006 };
        for ( const key of Object.keys( values ) ) Object.defineProperty( texture, key, { configurable: true, enumerable: true, get() { trace.push( [ 'get texture', key ] );return values[ key ]; }, set( value ) { trace.push( [ 'set texture', key, encode( value ) ] );values[ key ] = value; } } );
        let current = previous, active = 3, mip = 2, draws = 0, meshSeen;
        const renderer = { coordinateSystem: mode === 8 ? 2001 : 2000, xr: { enabled: true }, autoClear: false,
            isWebGLRenderer: mode !== 9, reversedDepthBuffer: mode === 9,
            state: { buffers: { depth: { getReversed() { trace.push( 'reversed' );return mode === 7; } } } },
            getRenderTarget() { trace.push( 'get target' );return current; },getActiveCubeFace() { trace.push( 'get face' );return active; },getActiveMipmapLevel() { trace.push( 'get mip' );return mip; },
            setRenderTarget( value, face, level ) { current = value;active = face;mip = level;trace.push( [ 'set', this === renderer, value === target ? 'target' : encode( value ), encode( face ), encode( level ) ] ); },
            clear( ...args ) { trace.push( [ 'clear', this === renderer, args.map( encode ) ] ); },
            clearDepth() { trace.push( 'clearDepth' ); },
            render( mesh, camera ) {
                trace.push( [ 'render', this === renderer, mesh.isMesh, camera.isPerspectiveCamera, active, mip, texture.minFilter, target.texture.mipmapsAutoUpdate, renderer.xr.enabled, camera.near, camera.far, encode( camera.up.toArray() ) ] );
                if ( draws ++ === 0 ) {
                    meshSeen = mesh;
                    trace.push( [ 'payload', encode( mesh.geometry.parameters ), mesh.geometry.attributes.position.count, mesh.geometry.index.count,
                        mesh.material.name, mesh.material.side, mesh.material.blending, mesh.material.uniforms.tEquirect.value === texture, mesh.material.vertexShader, mesh.material.fragmentShader ] );
                    mesh.geometry.addEventListener( 'dispose', function ( event ) { trace.push( [ 'dispose geometry', event.target === this, values.minFilter, renderer.xr.enabled ] );if ( mode === 5 ) throw new TypeError( 'dispose' ); } );
                    mesh.material.addEventListener( 'dispose', function ( event ) { trace.push( [ 'dispose material', event.target === this, values.minFilter, renderer.xr.enabled ] ); } );
                } else if ( mode !== 4 ) trace.push( [ 'same mesh', mesh === meshSeen ] );
                if ( mode === 1 && active === 2 ) throw new TypeError( 'render' );
                if ( mode === 2 && active === 0 ) texture.minFilter = 1003;
                if ( mode === 3 && active === 5 ) mesh.geometry = { dispose() { trace.push( [ 'replacement dispose', this === mesh.geometry ] ); } };
                if ( mode === 4 && active === 0 ) { trace.push( [ 'nested clear result', encode( target.clear( renderer, false, false, true ) ) ] ); }
            } };
        const result = attempt( () => target.fromEquirectangularTexture( renderer, texture ) === target );
        row( 'equirect source', [ mode, result, trace, encode( values ), current === previous, encode( active ), encode( mip ), renderer.xr.enabled, target.texture.mipmapsAutoUpdate, encode( target.texture.needsPMREMUpdate ),
            [ target.texture.type, target.texture.colorSpace, target.texture.generateMipmaps, target.texture.minFilter, target.texture.magFilter ] ] );
    }
}
metadata();construction();aliasesAndCopies();dynamicSuper();optionsOrder();clearCases();equirectCases();
if ( output ) { fs.mkdirSync( output, { recursive: true } );fs.writeFileSync( path.join( output, 'cube-depth-render-target-upstream.json' ), JSON.stringify( expected, null, 2 ) ); }
if ( executable === '--source-only' ) { console.log( `Pinned CubeDepthTexture/WebGLCubeRenderTarget oracle: ${ expected.length } rows` );process.exit( 0 ); }
let text;try { text = execFileSync( executable, [], { encoding: 'utf8', maxBuffer: 24 * 1024 * 1024 } ); } catch ( error ) { if ( output ) { fs.writeFileSync( path.join( output, 'cube-depth-render-target-native.jsonl' ), error.stdout ?? '' );fs.writeFileSync( path.join( output, 'cube-depth-render-target-native.err.log' ), error.stderr ?? '' ); }throw error; }
const actual = text.trim().split( /\r?\n/ ).filter( Boolean ).map( line => JSON.parse( line ) );
if ( output ) fs.writeFileSync( path.join( output, 'cube-depth-render-target-native.json' ), JSON.stringify( actual, null, 2 ) );
assert.equal( actual.length, expected.length );for ( let i = 0; i < expected.length; ++ i ) assert.deepStrictEqual( actual[ i ], expected[ i ], `row ${ i }: ${ expected[ i ].kind }` );
console.log( `CubeDepthTexture/WebGLCubeRenderTarget: ${ expected.length } actual native/pinned source rows passed` );
