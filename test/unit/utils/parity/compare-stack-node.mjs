import { execFileSync, spawnSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';
import path from 'node:path';
import { mkdirSync, writeFileSync } from 'node:fs';
import assert from 'node:assert/strict';
const [ upstream, executable, outputDirectory ] = process.argv.slice( 2 );
if ( ! upstream || ! executable ) throw Error( 'compare-stack-node.mjs <pinned-three.js> <probe>' );
assert.equal( execFileSync( 'git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' } ).trim(), '11738a9d55e502fb484cefffcea3a66dc44e9fe9' );
const load = file => import( pathToFileURL( path.join( upstream, 'src', file ) ) );
const { default: StackNode } = await load( 'nodes/core/StackNode.js' );
const { default: Node } = await load( 'nodes/core/Node.js' );
const { getCurrentStack, setCurrentStack } = await load( 'nodes/tsl/TSLBase.js' );
const { setConsoleFunction } = await load( 'utils.js' );
const n = ( name, rest = {} ) => ( { name, ...rest } );
const requests = [ { kind: 'constructor' }, { kind: 'types', nodes: [] }, { kind: 'types', nodes: [ n( 'out', { trace: true } ) ], output: 'out' } ];
const additions = [ n( 'a' ), n( 'b' ), n( 'c' ), n( 'd' ), n( 'bad', { isNode: 1 } ) ];
requests.push( { kind: 'add', nodes: additions, initial: [ 'a', 'b' ], operations: [ { node: 'c' }, { node: 'd', before: true }, { node: 'bad' } ] } );
requests.push( { kind: 'add', nodes: additions, initial: [ 'a', 'b' ], current: 'a', operations: [ { node: 'c' }, { node: 'd' }, { node: 'b', before: true }, { current: 'b' }, { node: 'c' }, { current: null }, { node: 'a', index: -2 } ] } );
for ( const index of [ -1, '#undefined', -9, 1, 100, null, '1', '-1', 1.9 ] ) requests.push( { kind: 'add', nodes: additions, initial: [ 'a', 'b' ], operations: [ { node: 'c', index } ] } );
for ( const assign of [ true, false, 1, null ] ) requests.push( { kind: 'setup', overrideChildren: true, propertyOutput: 'existing', nodes: [ n( 'a', { trace: true } ), n( 'v', { trace: true, isVarNode: true, intent: true, assign } ), n( 'b', { trace: true } ) ], initial: [ 'a', 'v', 'b' ] } );
requests.push( { kind: 'setup', nodes: [ n( 'a' ), n( 'out' ) ], initial: [ 'a' ], output: 'out' } );
requests.push( { kind: 'setup', overrideChildren: true, nodes: [ n( 'a', { isVarNode: true, intent: true, throw: 'isAssign', trace: true } ) ], initial: [ 'a' ] } );
for ( const stage of [ 'setup', 'analyze', 'generate', 'other' ] ) {
    for ( const nodeType of [ 'float', 'void' ] ) requests.push( { kind: 'build', stage, nodes: [ n( 'a', { trace: true } ), n( 'b', { trace: true } ), n( 'out', { trace: true, nodeType } ) ], initial: [ 'a', 'b' ], output: 'out' } );
    requests.push( { kind: 'build', stage, baseStub: true, nodes: [ n( 'a', { trace: true } ) ], initial: [ 'a' ] } );
}
for ( const assign of [ true, false, 1, null ] ) for ( const stage of [ 'setup', 'analyze', 'generate' ] ) requests.push( { kind: 'build', stage, nodes: [ n( 'v', { trace: true, isVarNode: true, intent: true, assign } ), n( 'b', { trace: true } ), n( 'out' ) ], initial: [ 'v', 'b' ], output: 'out' } );
for ( const parents of [ [], [ 'stack' ], [ 'other' ], [ 'false' ], [ 'stack', 'stack' ] ] ) requests.push( { kind: 'build', stage: 'generate', nodes: [ n( 'v', { trace: true, isVarNode: true, intent: false, parents } ), n( 'out' ) ], initial: [ 'v' ], output: 'out' } );
for ( const stage of [ 'setup', 'analyze', 'generate' ] ) requests.push( { kind: 'build', stage, nodes: [ n( 'a', { insert: true, insertBefore: 'before', insertAfter: 'after' } ), n( 'before' ), n( 'after' ), n( 'out' ) ], initial: [ 'a' ], output: 'out' } );
requests.push( { kind: 'build', stage: 'setup', nodes: [ n( 'a', { changeStage: 'generate' } ), n( 'b', { trace: true } ), n( 'out', { nodeType: 'void' } ) ], initial: [ 'a', 'b' ], output: 'out' } );
for ( const failure of [ 'child', 'output', 'setActiveStack', 'getDataFromNode', 'removeActiveStack' ] ) requests.push( { kind: 'build', stage: 'generate', throwBuilder: failure, nodes: [ n( 'a', { trace: true, throw: failure === 'child' ? 'build' : '' } ), n( 'out', { trace: true, throw: failure === 'output' ? 'build' : '' } ) ], initial: [ 'a' ], output: 'out' } );
// Keep the original 58 requests as a strict prefix of this extended suite.
for ( const baseOverride of [ 'method', 'getter', 'getter-throw', 'method-throw', 'getter-noncallable', 'ancestor-getter', 'null' ] ) requests.push( { kind: 'build', stage: 'setup', baseOverride, nodes: [], initial: [] } );
for ( const baseOverride of [ 'method', 'getter' ] ) requests.push( { kind: 'build', stage: 'analyze', baseOverride, deferBase: true, nodes: [ n( 'a', { trace: true, changeBase: true } ) ], initial: [ 'a' ] } );

function run( request ) {
    const state = { events: [], stack: new StackNode(), children: {}, specs: {} }, stack = state.stack;stack.fixtureName = 'root';
    const previous = { fixtureName: 'previous' };setCurrentStack( previous );
    const builder = state.builder = { buildStage: request.stage ?? 'setup', shaderStage: 'vertex' };
    const normal = value => {
        if ( value === undefined ) return '#undefined';if ( typeof value === 'function' ) return '#function';
        if ( value && typeof value === 'object' ) { if ( value === stack ) return '#stack';if ( value === builder ) return '#builder';if ( value.fixtureName !== undefined ) return `@${ value.fixtureName }`;if ( Array.isArray( value ) ) return value.map( normal );return '#object'; }return value;
    };
    const event = ( ...values ) => state.events.push( values.map( normal ) );
    const getter = ( object, name, get ) => Object.defineProperty( object, name, { enumerable: true, configurable: true, get } );
    for ( const spec of request.nodes ?? [] ) {
        const object = { fixtureName: spec.name, isNode: Object.hasOwn( spec, 'isNode' ) ? spec.isNode : true };
        if ( spec.trace ) getter( object, 'isVarNode', () => { event( 'get', spec.name, 'isVarNode' );return spec.isVarNode; } );else object.isVarNode = spec.isVarNode;
        for ( const name of [ 'isIntent', 'isAssign', 'build', 'getElementType', 'getNodeType', 'getMemberType' ] ) {
            const method = function ( ...args ) {
                const row = [ name, spec.name, args ];if ( name === 'build' ) row.push( getCurrentStack(), stack._currentNode );event( ...row );
                if ( spec.throw === name ) throw new TypeError( 'fixture' );
                if ( name === 'isIntent' ) return spec.intent;if ( name === 'isAssign' ) return spec.assign;
                if ( name === 'getElementType' ) return Object.hasOwn( spec, 'elementType' ) ? spec.elementType : 'element';
                if ( name === 'getNodeType' ) return Object.hasOwn( spec, 'nodeType' ) ? spec.nodeType : 'float';if ( name === 'getMemberType' ) return 'member';
                if ( spec.changeBase ) Object.setPrototypeOf( StackNode.prototype, state.alternateBase );
                if ( Object.hasOwn( spec, 'changeStage' ) ) builder.buildStage = spec.changeStage;
                if ( spec.insert && ! this.inserted ) { this.inserted = true;for ( const key of [ 'insertBefore', 'insertAfter' ] ) if ( spec[ key ] !== undefined ) stack[ key === 'insertBefore' ? 'addToStackBefore' : 'addToStack' ]( state.children[ spec[ key ] ] ); }
                return `built:${ spec.name }`;
            };
            if ( spec.trace ) getter( object, name, () => { event( 'get', spec.name, name );return method; } );else object[ name ] = method;
        }state.children[ spec.name ] = object;state.specs[ spec.name ] = spec;
    }
    const result = {};
    if ( request.kind === 'constructor' ) {
        const parent = {}, object = new StackNode( parent );
        return { fields: Object.getOwnPropertyNames( object ), parent: object.parent === parent, defaultParent: stack.parent, nodes: normal( stack.nodes ), isNode: stack.isNode, isStackNode: stack.isStackNode, type: stack.type, isMap: stack._nodeDataLibrary instanceof Map, defaults: [ stack.outputNode, stack._currentCond, stack._expressionNode, stack._currentNode ], methods: [ 'isCacheable', 'getElementType', 'generateNodeType', 'getMemberType', 'addToStack', 'addToStackBefore', 'setup', 'build' ].map( name => [ name, StackNode.prototype[ name ].length, false ] ) };
    }
    stack.nodes.push( ...( request.initial ?? [] ).map( name => state.children[ name ] ) );if ( Object.hasOwn( request, 'current' ) ) stack._currentNode = state.children[ request.current ];if ( Object.hasOwn( request, 'output' ) ) stack.outputNode = state.children[ request.output ];
    const properties = {};if ( Object.hasOwn( request, 'propertyOutput' ) ) properties.outputNode = request.propertyOutput;
    for ( const name of [ 'setActiveStack', 'removeActiveStack', 'getDataFromNode', 'getNodeProperties' ] ) builder[ name ] = function ( ...args ) {
        event( name, args, getCurrentStack() );if ( request.throwBuilder === name ) throw new TypeError( 'builder' );
        if ( name === 'getNodeProperties' ) return properties;
        if ( name === 'getDataFromNode' ) { const spec = state.specs[ args[ 0 ].fixtureName ];return Object.hasOwn( spec, 'parents' ) ? { stages: { vertex: spec.parents.map( parent => parent === 'stack' ? stack : parent === 'false' ? false : {} ) } } : {}; }
    };
    const originalBuild = Object.getOwnPropertyDescriptor( Node.prototype, 'build' );
    const originalParent = Object.getPrototypeOf( StackNode.prototype );
    try {
        if ( Object.hasOwn( request, 'baseOverride' ) ) {
            const mode = request.baseOverride;let alternate = Object.create( Node.prototype );
            const method = function ( ...args ) { event( 'alternate.build', this, args );if ( mode === 'method-throw' ) throw new TypeError( 'alternate' );return 'alternate-result'; };
            if ( [ 'getter', 'getter-throw', 'getter-noncallable', 'ancestor-getter' ].includes( mode ) ) getter( alternate, 'build', function () { event( 'get:alternate.build', this );if ( mode === 'getter-throw' ) throw new TypeError( 'alternate getter' );if ( mode === 'getter-noncallable' ) return 17;return method; } );else alternate.build = method;
            if ( mode === 'ancestor-getter' ) alternate = Object.create( alternate );if ( mode === 'null' ) alternate = null;
            state.alternateBase = alternate;if ( ! request.deferBase ) Object.setPrototypeOf( StackNode.prototype, alternate );
        }
        if ( request.kind === 'add' ) {
            setConsoleFunction( ( level, message ) => event( 'error', level, message.startsWith( 'THREE.TSL: Invalid node added to stack.' ) ) );result.returns = [];
            for ( const op of request.operations ) { if ( Object.hasOwn( op, 'current' ) ) { stack._currentNode = op.current === null ? null : state.children[ op.current ];continue; }const args = [ state.children[ op.node ] ];if ( Object.hasOwn( op, 'index' ) ) args.push( op.index === '#undefined' ? undefined : op.index );result.returns.push( stack[ op.before ? 'addToStackBefore' : 'addToStack' ]( ...args ) === stack ); }
        } else if ( request.kind === 'types' ) { result.cacheable = stack.isCacheable( builder );result.values = [ stack.getElementType( builder ), stack.generateNodeType( builder ), stack.getMemberType( builder, 'field' ) ];
        } else if ( request.kind === 'setup' ) {
            if ( request.overrideChildren ) stack.getChildren = function () { event( 'getChildren' );return stack.nodes.values(); };
            result.value = normal( stack.setup( builder ) );result.properties = Object.fromEntries( Object.getOwnPropertyNames( properties ).map( key => [ key, normal( properties[ key ] ) ] ) );
        } else if ( request.kind === 'build' ) {
            if ( request.baseStub ) Object.defineProperty( Node.prototype, 'build', { ...originalBuild, value( ...args ) { event( 'super.build', args );return 'base-result'; } } );
            result.value = normal( stack.build( builder, 'float', 17 ) );
        }
    } catch ( error ) { result.error = error.name; } finally { Object.setPrototypeOf( StackNode.prototype, originalParent );Object.defineProperty( Node.prototype, 'build', originalBuild );setConsoleFunction( null ); }
    Object.assign( result, { events: state.events, nodes: normal( stack.nodes ), current: normal( stack._currentNode ), currentStack: normal( getCurrentStack() ), stage: builder.buildStage, delta: Object.fromEntries( Object.entries( state.children ).flatMap( ( [ name, child ] ) => { const item = stack._nodeDataLibrary.get( child );return item === undefined ? [] : [ [ name, item.delta ] ]; } ) ) } );
    setCurrentStack( null );return result;
}
const native = spawnSync( executable, [], { input: JSON.stringify( requests ), encoding: 'utf8', maxBuffer: 8 * 1024 * 1024, env: { ...process.env, TEMP: 'D:\\Temp\\surui', TMP: 'D:\\Temp\\surui' } } );
assert.equal( native.status, 0, native.stderr );const actual = JSON.parse( native.stdout ), expected = requests.map( run );
if ( outputDirectory ) {
    mkdirSync( outputDirectory, { recursive: true } );
    for ( const [ file, value ] of Object.entries( { 'fixtures.json': requests, 'upstream.json': expected, 'native.json': actual } ) ) writeFileSync( path.join( outputDirectory, file ), JSON.stringify( value, null, 2 ) + '\n' );
}
for ( let i = 0; i < requests.length; ++ i ) assert.deepStrictEqual( actual[ i ], expected[ i ], `case ${ i }: ${ JSON.stringify( requests[ i ] ) }` );
console.log( `StackNode: ${ requests.length } direct pinned-upstream cases passed` );
