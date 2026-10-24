import { execFileSync, spawnSync } from 'node:child_process';
import { mkdirSync, writeFileSync } from 'node:fs';
import { pathToFileURL } from 'node:url';
import path from 'node:path';
import assert from 'node:assert/strict';
const [ upstream, executable, outputDirectory ] = process.argv.slice( 2 );
if ( ! upstream || ! executable ) throw Error( 'compare-node-input.mjs <pinned-three.js> <probe> [output-directory]' );
assert.equal( execFileSync( 'git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' } ).trim(), '11738a9d55e502fb484cefffcea3a66dc44e9fe9' );
const load = file => import( pathToFileURL( path.join( upstream, 'src', file ) ) );
const classes = {};
for ( const name of [ 'InputNode', 'ConstNode', 'UniformNode', 'UniformGroupNode', 'NodeBuilder' ] ) classes[ name ] = ( await load( `nodes/core/${ name }.js` ) ).default;
const groups = await load( 'nodes/core/UniformGroupNode.js' );const { setConsoleFunction } = await load( 'utils.js' );
const undef = { tag: 'undefined' }, nan = { tag: 'nan' }, negzero = { tag: 'negative-zero' };
const requests = [];
for ( const name of [ 'InputNode', 'ConstNode', 'UniformNode', 'UniformGroupNode' ] ) requests.push( { kind: 'metadata', class: name, args: [ 7 ] } );
for ( const name of [ 'InputNode', 'ConstNode', 'UniformNode' ] ) for ( const value of [ undef, null, false, 0, 5, 'text', { tag: 'buffer', values: [ 1, 2, 3 ] } ] ) for ( const type of [ null, 'bool', 'float', undef, '' ] ) requests.push( { kind: 'type', class: name, args: [ value, type ], precision: value } );
for ( const type of [ 'float', 'int', 'uint', 'myint', 'bool', 'vec2' ] ) for ( const value of [ -1.5, 1.5, negzero, nan, { tag: 'coerce', name: 'value', values: [ 2.7, 7.9 ] } ] ) for ( const output of [ 'float', 'uint', 'vec2', undef ] ) requests.push( { kind: 'const', class: 'ConstNode', protocol: true, type, args: [ value, type ], output } );
requests.push( { kind: 'const', class: 'ConstNode', protocol: true, direct: true, type: 'float', args: [ 7, 'float' ], output: 'vec2' } );
for ( const name of [ 'InputNode', 'ConstNode' ] ) for ( const value of [ null, 3, 'text', { tag: 'buffer', values: [ 0, 127, 255 ] }, { tag: 'arrayObject', vector2: true, values: [ 1, 2 ] } ] ) requests.push( { kind: 'serialize', class: name, args: [ value ], weak: true, precision: 'high' } );
for ( const name of [ 'InputNode', 'ConstNode' ] ) for ( const data of [ {}, { value: 4, nodeType: null }, { value: 'AQI=', valueType: 'ArrayBuffer', precision: 0, isWeak: true }, { value: [ 1, 2 ], valueType: 'vec2', nodeType: 'vec2', precision: 'medium', isWeak: 1 }, { value: [ 3 ], valueType: 'float', precision: false }, { value: { tag: 'arrayObject', values: [ 9 ] }, precision: 'low', isWeak: true } ] ) requests.push( { kind: 'deserialize', class: name, args: [ 0 ], data } );
requests.push( { kind: 'serialize', class: 'UniformGroupNode', args: [ 'shared-name', true, 4, 'frame' ] } );
requests.push( { kind: 'deserialize', class: 'UniformGroupNode', args: [ 'old', false, 9, 'render' ], data: { name: 'new', version: 7, shared: true, order: 1, updateType: 'object' } } );
for ( const shared of [ false, true ] ) for ( const args of [ [ 'test' ], [ 'test', 7, 'frame' ], [ 'test', undef, undef ], [ null, null, null ] ] ) requests.push( { kind: 'groups', shared, args } );
for ( const returns of [ [ undef, false, 0, null, 'new' ], [ 1, 2, 3 ] ] ) for ( const customBind of [ false, true ] ) requests.push( { kind: 'update', args: [ 5 ], returns, calls: returns.length, customBind, updateType: 'frame' } );
requests.push( { kind: 'update', args: [ 5 ], returns: [ 11, 22 ], calls: 1, reenter: true, customBind: true, updateType: 'object' } );
requests.push( { kind: 'update', args: [ 5 ], returns: [ undef, undef ], calls: 2, mutate: true, updateType: 'render' } );
requests.push( { kind: 'update', args: [ 5 ], returns: [ 8, 9 ], calls: 2, throw: 0, updateType: 'render' } );
for ( const type of [ 'float', 'bool' ] ) for ( const name of [ '', 'named' ] ) for ( const found of [ undef, 'self', null, false ] ) requests.push( { kind: 'uniform', args: [ 1, type ], type, name, found, label: 'context-name', calls: 2, output: 'float' } );
for ( const cached of [ null, false, '', 'existing' ] ) requests.push( { kind: 'uniform', args: [ true, 'bool' ], type: 'bool', cached, calls: 1, output: 'bool' } );
for ( const failure of [ 'getNodeFromHash', 'setHashNode', 'getUniformFromNode', 'getPropertyName', 'getVarFromNode', 'format', 'addLineFlowCode' ] ) requests.push( { kind: 'uniform', args: [ true, 'bool' ], type: 'bool', calls: 1, output: 'bool', throw: failure } );
for ( const type of [ 'float', 'int', 'uint', 'bool' ] ) requests.push( { kind: 'integration', class: 'ConstNode', args: [ type === 'bool' ? true : 7.5, type ], output: 'float' } );
for ( const type of [ 'float', 'bool' ] ) requests.push( { kind: 'integration', args: [ type === 'bool' ? true : 3, type ], output: type } );
requests.push( { kind: 'super', method: 'getInputType', args: [ true, 'bool' ] } );

function run( request ) {
    const state = { events: [] }, Constructor = classes[ request.class ?? 'UniformNode' ];
    const normal = ( value, depth = 0 ) => {
        if ( value === undefined ) return '#undefined';if ( typeof value === 'function' ) return '#function';if ( typeof value === 'symbol' ) return '#symbol';
        if ( typeof value === 'number' ) { if ( Number.isNaN( value ) ) return '#NaN';if ( value === Infinity ) return '#Infinity';if ( value === -Infinity ) return '#-Infinity';if ( Object.is( value, -0 ) ) return '#-0'; }
        if ( ! value || typeof value !== 'object' ) return value;
        for ( const key of [ 'node', 'builder', 'frame', 'callback', 'uniform', 'variable' ] ) if ( value === state[ key ] ) return `#${ key }`;
        if ( value === groups.objectGroup ) return '#objectGroup';if ( value instanceof ArrayBuffer ) return [ ...new Uint8Array( value ) ];if ( depth > 8 ) return '#depth';
        const out = Array.isArray( value ) ? new Array( value.length ) : {};for ( const key of Object.keys( value ) ) { if ( key === 'id' || key === '_uuid' ) continue;out[ key ] = normal( value[ key ], depth + 1 ); }return out;
    };
    const event = ( ...values ) => state.events.push( values.map( value => normal( value ) ) );
    const getter = ( object, name, get ) => Object.defineProperty( object, name, { enumerable: true, configurable: true, get } );
    const decode = value => {
        if ( ! value || typeof value !== 'object' ) return value;if ( Array.isArray( value ) ) return value.map( decode );
        if ( value.tag === 'undefined' ) return undefined;if ( value.tag === 'nan' ) return NaN;if ( value.tag === 'negative-zero' ) return -0;if ( value.tag === 'infinity' ) return Infinity;if ( value.tag === 'symbol' ) return Symbol( 'fixture' );
        if ( value.tag === 'buffer' ) return new Uint8Array( decode( value.values ) ).buffer;
        if ( value.tag === 'coerce' ) { let n = 0;return { [ Symbol.toPrimitive ]( hint ) { const i = n ++;event( 'coerce', value.name, hint, i );return decode( value.values[ Math.min( i, value.values.length - 1 ) ] ); } }; }
        if ( value.tag === 'arrayObject' ) { const out = { isVector2: value.vector2 };for ( const key of [ 'toArray', 'fromArray' ] ) getter( out, key, () => { event( 'get', key );return function ( ...args ) { event( key, args );return key === 'toArray' ? decode( value.values ) : this; }; } );return out; }
        return Object.fromEntries( Object.entries( value ).map( ( [ k, v ] ) => [ k, decode( v ) ] ) );
    };
    const args = ( request.args ?? [] ).map( decode ), node = state.node = new Constructor( ...args );
    const builder = state.builder = new classes.NodeBuilder( null, { debug: { diagnostics: { keywords: false } } }, null );builder.shaderStage = 'fragment';
    setConsoleFunction( ( level, message ) => event( 'console', level, message.startsWith( 'THREE.TSL:' ) ) );
    const proto = Constructor.prototype, originalParent = Object.getPrototypeOf( proto ), ctorParent = Object.getPrototypeOf( Constructor ), result = {};
    try {
        if ( request.kind === 'metadata' ) {
            result.fields = Object.fromEntries( Object.getOwnPropertyNames( node ).filter( key => key !== 'id' && key !== '_uuid' ).map( key => [ key, normal( node[ key ] ) ] ) );
            result.methods = Object.getOwnPropertyNames( proto ).filter( key => key !== 'constructor' ).map( key => { const fn = proto[ key ], d = Object.getOwnPropertyDescriptor( proto, key );return [ key, fn.name, fn.length, false, d.enumerable, d.configurable, d.writable ]; } );result.arity = Constructor.length;result.type = node.type;result.cacheable = node.isCacheable( builder );
        } else if ( request.kind === 'type' ) { result.generated = normal( node.generateNodeType( builder ) );result.input = normal( node.getInputType( builder ) );result.precisionReturn = node.setPrecision( decode( request.precision ) ) === node;result.precision = normal( node.precision );
        } else if ( request.kind === 'const' ) {
            if ( request.protocol ) { for ( const name of [ 'generateConst', 'format' ] ) getter( builder, name, () => { event( 'get', name );return function ( ...args ) { event( name, args );return name === 'generateConst' ? 'literal' : 'formatted'; }; } );node.getNodeType = () => { event( 'getNodeType' );return decode( request.type ); }; }
            result.value = normal( node[ request.direct ? 'generateConst' : 'generate' ]( builder, decode( request.output ) ) );
        } else if ( request.kind === 'serialize' || request.kind === 'deserialize' ) {
            node.getSerializeChildren = () => { event( 'getSerializeChildren' );return []; };const data = Object.hasOwn( request, 'data' ) ? decode( request.data ) : {};
            if ( Object.hasOwn( request, 'precision' ) ) node.precision = decode( request.precision );if ( Object.hasOwn( request, 'weak' ) ) node.isWeak = decode( request.weak );
            result.return = normal( node[ request.kind ]( data ) );result.data = normal( data );result.fields = Object.fromEntries( [ 'value', 'nodeType', 'precision', 'isWeak', 'name', 'version', 'shared', 'order', 'updateType' ].map( key => [ key, normal( node[ key ] ) ] ) );
        } else if ( request.kind === 'groups' ) {
            result.presets = [ 'frameGroup', 'renderGroup', 'objectGroup' ].map( name => { const group = groups[ name ];return [ name, group.name, group.shared, group.order, group.updateType, group === groups[ name ] ]; } );result.uniformGroup = node.groupNode === groups.objectGroup;
            const group = new classes.UniformGroupNode( 'custom', true, 7, 'frame' );group.update();group.update();result.updated = group.version;result.needsUpdateOwn = Object.hasOwn( group, 'needsUpdate' );
            const fresh = groups[ request.shared ? 'sharedUniformGroup' : 'uniformGroup' ]( ...args );result.factory = Object.fromEntries( [ 'name', 'shared', 'order', 'updateType', 'nodeType' ].map( key => [ key, normal( fresh[ key ] ) ] ) );
        } else if ( request.kind === 'update' ) {
            const frame = state.frame = {}, values = [];let n = 0;const callback = state.callback = function ( ...args ) { const i = n ++;event( 'callback', this, args, i );if ( request.throw === i ) throw new TypeError( 'callback' );if ( request.reenter && i === 0 ) node.update( args[ 0 ] );if ( request.mutate ) node.value = 'mutated';return i < request.returns.length ? decode( request.returns[ i ] ) : undefined; };
            if ( request.customBind ) { const bind = callback.bind;getter( callback, 'bind', function () { event( 'get:bind', this );return function ( ...args ) { event( 'call:bind', this, args );return bind.apply( this, args ); }; } ); }
            result.onUpdateReturn = node.onUpdate( callback, request.updateType ) === node;result.updateType = normal( node.updateType );for ( let i = 0; i < request.calls; ++ i ) { try { node.update( frame );values.push( normal( node.value ) ); } catch ( error ) { values.push( `#${ error.name }` ); } }result.values = values;
        } else if ( request.kind === 'uniform' ) {
            const data = state.data = {};if ( Object.hasOwn( request, 'cached' ) ) data.propertyName = decode( request.cached );const uniform = state.uniform = {}, variable = state.variable = {}, context = {};if ( Object.hasOwn( request, 'label' ) ) context.nodeName = decode( request.label );builder.context = context;
            node.getNodeType = () => { event( 'getNodeType' );return request.type; };node.getUniformHash = function ( ...args ) { event( 'getUniformHash', args );return 'fixture-hash'; };if ( Object.hasOwn( request, 'name' ) ) node.name = decode( request.name );
            for ( const name of [ 'getNodeFromHash', 'setHashNode', 'getUniformFromNode', 'getPropertyName', 'getDataFromNode', 'getVarFromNode', 'format', 'addLineFlowCode' ] ) getter( builder, name, () => {
                event( 'get', name );return function ( ...args ) { event( name, args );if ( request.throw === name ) throw new TypeError( 'builder' );if ( name === 'getNodeFromHash' ) return request.found === 'self' ? node : Object.hasOwn( request, 'found' ) ? decode( request.found ) : undefined;if ( name === 'getUniformFromNode' ) return uniform;if ( name === 'getPropertyName' ) return args[ 0 ] === uniform ? 'u_value' : 'v_bool';if ( name === 'getDataFromNode' ) return data;if ( name === 'getVarFromNode' ) return variable;if ( name === 'format' ) return `fmt(${ args[ 0 ] },${ args[ 1 ] },${ args[ 2 ] })`; };
            } );
            const values = [];for ( let i = 0; i < request.calls; ++ i ) values.push( node.generate( builder, request.output ) );result.values = values;result.data = normal( data );result.context = normal( context );
        } else if ( request.kind === 'integration' ) {
            result.values = [];for ( const stage of [ 'setup', 'analyze', 'generate' ] ) { builder.buildStage = stage;result.values.push( normal( node.build( builder, request.output ) ) ); }result.flow = normal( builder.flow );result.uniforms = builder.uniforms.fragment.length;
        } else if ( request.kind === 'super' ) {
            const alternate = Object.create( originalParent );getter( alternate, request.method, function () { event( 'super:get', this );return function ( ...args ) { event( 'super:call', this, args );return 'bool'; }; } );Object.setPrototypeOf( proto, alternate );result.value = normal( node[ request.method ]( builder ) );
        }
    } catch ( error ) { result.error = error.name; } finally { Object.setPrototypeOf( proto, originalParent );Object.setPrototypeOf( Constructor, ctorParent );setConsoleFunction( null ); }
    result.events = state.events;return result;
}
const native = spawnSync( executable, [], { input: JSON.stringify( requests ), encoding: 'utf8', maxBuffer: 32 * 1024 * 1024, env: { ...process.env, TEMP: 'D:\\Temp\\surui', TMP: 'D:\\Temp\\surui' } } );
assert.equal( native.status, 0, native.stderr );const actual = JSON.parse( native.stdout ), expected = requests.map( run );
if ( outputDirectory ) { mkdirSync( outputDirectory, { recursive: true } );for ( const [ name, value ] of Object.entries( { 'fixtures.json': requests, 'native.json': actual, 'upstream.json': expected } ) ) writeFileSync( path.join( outputDirectory, name ), JSON.stringify( value, null, 2 ) + '\n' ); }
for ( let i = 0; i < requests.length; ++ i ) assert.deepStrictEqual( actual[ i ], expected[ i ], `case ${ i }: ${ JSON.stringify( requests[ i ] ) }` );
console.log( `InputNode/ConstNode/UniformGroupNode/UniformNode: ${ requests.length } direct pinned-upstream cases passed` );
