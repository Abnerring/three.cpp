import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';

// The tested implementation is imported from the pinned source checkout.
// This fixture supplies hosts and nodes; it does not duplicate NodeFrame's rules.
const upstream = path.resolve(process.argv[2]);
const binary = process.argv[3] && process.argv[3] !== '--oracle' ? path.resolve(process.argv[3]) : undefined;
const outputDirectory = process.argv[4] ? path.resolve(process.argv[4]) : undefined;
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const {default: NodeFrame} = await import(pathToFileURL(path.join(upstream, 'src/nodes/core/NodeFrame.js')));
const {NodeUpdateType} = await import(pathToFileURL(path.join(upstream, 'src/nodes/core/constants.js')));

const encode = value => value === undefined ? '#undefined' : typeof value === 'symbol' ? '#symbol' :
    typeof value === 'function' ? '#function' : value !== null && typeof value === 'object' ? '#object' :
    typeof value === 'number' && Number.isNaN(value) ? '#NaN' : value === Infinity ? '#Infinity' :
    value === -Infinity ? '#-Infinity' : Object.is(value, -0) ? '#-0' : value;
function decode(spec, state, label = '') {
    if (spec === null || typeof spec !== 'object') return spec;
    if (spec.tag === 'undefined') return undefined;
    if (spec.tag === 'nan') return NaN;
    if (spec.tag === 'infinity') return Infinity;
    if (spec.tag === 'negative-zero') return -0;
    if (spec.tag === 'symbol') return Symbol('fixture');
    if (spec.tag === 'coerce') return {[Symbol.toPrimitive](hint) {
        state.events.push(['coerce', label, hint]);
        if (spec.throw) throw new TypeError('coercion');
        return decode(spec.value);
    }};
    return {};
}
const method = stage => stage === 'before' ? 'updateBeforeNode' : stage === 'after' ? 'updateAfterNode' : 'updateNode';
const callback = stage => stage === 'before' ? 'updateBefore' : stage === 'after' ? 'updateAfter' : 'update';
const typeMethod = stage => stage === 'before' ? 'getUpdateBeforeType' : stage === 'after' ? 'getUpdateAfterType' : 'getUpdateType';
const mapName = stage => stage === 'before' ? 'updateBeforeMap' : stage === 'after' ? 'updateAfterMap' : 'updateMap';
const newState = (request, frame) => ({request, frame, events: [], calls: 0, ref: {}});
const recordSnapshot = record => record !== null && (typeof record === 'object' || typeof record === 'function') ?
    [encode(record.frameId), encode(record.renderId), Object.keys(record)] : encode(record);
function snapshot(state) {
    const frame = state.frame, out = {};
    for (const key of ['time', 'deltaTime', 'frameId', 'renderId', 'lastTime']) out[key] = encode(frame[key]);
    out.hasLastTime = Object.hasOwn(frame, 'lastTime');
    out.calls = state.calls;
    out.mark = recordSnapshot(WeakMap.prototype.get.call(frame[mapName(state.request.phase)], state.ref));
    return out;
}
function invoke(frame, name, args = []) {
    try { return encode(frame[name](...args)); }
    catch (error) { return '#' + error.name; }
}
function makeNode(state, label = 'node') {
    const stage = state.request.phase, node = {};
    node[typeMethod(stage)] = function(frame) {
        state.events.push(['type', label, this === state[label], frame === state.frame]);
        return state.request.type;
    };
    node.updateReference = function(frame) {
        state.events.push(['reference', label, this === state[label], frame === state.frame]);
        return state.ref;
    };
    node[callback(stage)] = function(frame) {
        const count = ++state.calls, request = state.request;
        state.events.push(['callback', label, count, this === state[label], frame === state.frame,
            encode(frame.frameId), encode(frame.renderId)]);
        if (request.behavior === 'throw' && count === 1) throw new TypeError('callback');
        if (request.behavior === 'mutate') frame[request.axis] = decode(request.nextId);
        if (request.behavior === 'reenter' && count === 1)
            state.events.push(['nestedResult', invoke(frame, method(stage), [this])]);
        return decode(request.returns?.[count - 1]);
    };
    state[label] = node;
    return node;
}
function runBasic(request) {
    const frame = new NodeFrame(), state = newState(request, frame), node = makeNode(state);
    if (Object.hasOwn(request, 'frameId')) frame.frameId = decode(request.frameId);
    if (Object.hasOwn(request, 'renderId')) frame.renderId = decode(request.renderId);
    const second = request.shared ? makeNode(state, 'other') : node, steps = [];
    for (const operation of request.steps) {
        for (const key of ['frameId', 'renderId']) if (Object.hasOwn(operation, key)) frame[key] = decode(operation[key]);
        const selected = operation.other ? second : node;
        steps.push({result: invoke(frame, method(request.phase), [selected]), state: snapshot(state)});
    }
    return {steps, events: state.events};
}
function runOrder(request) {
    const frame = new NodeFrame(), state = newState(request, frame), node = state.node = {};
    frame.frameId = 1; frame.renderId = 2;
    const maps = state.record = {frameId: 0, renderId: 0}, fakeMap = state.fakeMap = {};
    for (const name of [typeMethod(request.phase), 'updateReference', callback(request.phase), '_getMaps']) {
        const owner = name === '_getMaps' ? frame : node;
        Object.defineProperty(owner, name, {enumerable: true, configurable: true, get() {
            state.events.push(['get', name, this === owner]);
            if (request.throwAt === 'get:' + name) throw new TypeError('getter');
            return function(...args) {
                state.events.push(['call', name, this === owner, args[0] === (name === '_getMaps' ? fakeMap : frame)]);
                if (request.throwAt === 'call:' + name) throw new TypeError('call');
                if (name === typeMethod(request.phase)) return request.type;
                if (name === 'updateReference') return state.ref;
                if (name === '_getMaps') {
                    state.events.push(['referenceArgument', args[1] === state.ref]);
                    return maps;
                }
                state.calls++;
                if (request.changeId) frame.frameId = 9;
                return decode(request.result);
            };
        }});
    }
    Object.defineProperty(frame, mapName(request.phase), {enumerable: true, configurable: true, get() {
        state.events.push(['mapArgument', this === frame]);
        if (request.throwAt === 'map') throw new TypeError('map');
        return fakeMap;
    }});
    for (const name of ['frameId', 'renderId']) {
        state['stored:' + name] = maps[name];
        Object.defineProperty(maps, name, {enumerable: true, configurable: true, get() {
            state.events.push(['markGet', name, this === maps]);
            if (request.throwAt === 'markGet') throw new TypeError('mark');
            return state['stored:' + name];
        }, set(value) {
            state.events.push(['markSet', name, encode(value), this === maps]);
            if (request.throwAt === 'markSet') throw new TypeError('mark');
            state['stored:' + name] = value;
        }});
    }
    return {result: invoke(frame, method(request.phase), [node]), frameId: encode(frame.frameId),
        marks: [encode(state['stored:frameId']), encode(state['stored:renderId'])], calls: state.calls, events: state.events};
}
function runMaps(request) {
    const frame = new NodeFrame(), state = newState(request, frame), map = state.map = new WeakMap(), key = state.key = decode(request.key);
    const originalGet = state.originalGet = map.get, originalSet = state.originalSet = map.set;
    if (Object.hasOwn(request, 'initial')) originalSet.call(map, key, decode(request.initial));
    state.setFunction = function(...args) {
        state.events.push(['set', this === map, args[0] === key, recordSnapshot(args[1])]);
        if (request.throwAt === 'set') throw new TypeError('set');
        return originalSet.apply(this, args);
    };
    Object.defineProperty(map, 'set', {enumerable: true, configurable: true, get() {
        state.events.push(['get:set', this === map]);
        if (request.throwAt === 'get:set') throw new TypeError('set getter');
        return state.setFunction;
    }});
    Object.defineProperty(map, 'get', {enumerable: true, configurable: true, get() {
        state.events.push(['get:get', this === map]);
        if (request.throwAt === 'get:get') throw new TypeError('get getter');
        return function(...args) {
            state.events.push(['get', this === map, args[0] === key]);
            if (request.throwAt === 'get') throw new TypeError('get');
            if (request.replaceSet) state.setFunction = function(...args) {
                state.events.push(['replacementSet', this === map, args[0] === key, recordSnapshot(args[1])]);
                return originalSet.apply(this, args);
            };
            return originalGet.apply(this, args);
        };
    }});
    const results = [];
    for (let i = 0; i < 2; i++) {
        try { results.push(recordSnapshot(frame._getMaps(map, key))); }
        catch (error) { results.push('#' + error.name); }
    }
    return {results, events: state.events};
}
function runMapPrototype(request) {
    const frame = new NodeFrame(), state = newState(request, frame), map = new WeakMap(), reference = {};
    const original = {renderId: 23, frameId: 47};
    if (request.hit) map.set(reference, original);
    const saved = request.targets.map(name => [name, Object.getOwnPropertyDescriptor(Object.prototype, name)]);
    for (const name of request.targets) {
        const descriptor = {configurable: true, enumerable: false};
        if (request.pollution === 'readonly') Object.assign(descriptor, {value: 88, writable: false});
        else if (request.pollution === 'setter') descriptor.set = function(value) {
            state.events.push(['prototypeSet', name, encode(value)]);
        };
        else descriptor.get = function() {
            state.events.push(['prototypeGet', name]);
            return 99;
        };
        Object.defineProperty(Object.prototype, name, descriptor);
    }
    const out = {};
    try {
        try {
            const record = frame._getMaps(map, reference);
            out.result = recordSnapshot(record);
            out.sameOnSecondCall = frame._getMaps(map, reference) === record;
            out.sameAsInitial = record === original;
            out.descriptors = ['renderId', 'frameId'].map(name => {
                const descriptor = Object.getOwnPropertyDescriptor(record, name);
                return descriptor ? [name, encode(descriptor.value), !!descriptor.writable, !!descriptor.enumerable,
                    !!descriptor.configurable] : null;
            });
        } catch (error) { out.result = '#' + error.name; }
        out.events = state.events;
    } finally {
        for (const [name, descriptor] of saved) {
            if (descriptor) Object.defineProperty(Object.prototype, name, descriptor);
            else delete Object.prototype[name];
        }
    }
    return out;
}
function runClock(request) {
    const frame = new NodeFrame(), state = newState(request, frame);
    state.clockIndex = state.nowGets = 0;
    const performance = state.performance = {};
    Object.defineProperty(performance, 'now', {enumerable: true, configurable: true, get() {
        const count = ++state.nowGets;
        state.events.push(['get:now', count, this === performance]);
        if (request.throwAt === 'get:now:' + count) throw new TypeError('now getter');
        return function(...args) {
            const index = state.clockIndex++;
            state.events.push(['now', index + 1, this === performance, args.length]);
            if (request.throwAt === 'now:' + (index + 1)) throw new TypeError('now');
            return decode(request.ticks[index], state, 'tick:' + (index + 1));
        };
    }});
    const old = Object.getOwnPropertyDescriptor(globalThis, 'performance');
    Object.defineProperty(globalThis, 'performance', {value: performance, writable: true, configurable: true});
    try {
        for (const name of ['time', 'deltaTime', 'frameId', 'renderId', 'lastTime']) {
            if (Object.hasOwn(request, name)) frame[name] = decode(request[name], state, name);
            state['field:' + name] = frame[name];
            state['has:' + name] = Object.hasOwn(frame, name);
            if (request.accessors) Object.defineProperty(frame, name, {enumerable: true, configurable: true, get() {
                state.events.push(['fieldGet', name, this === frame]);
                if (request.throwAt === 'get:' + name) throw new TypeError('field getter');
                return state['field:' + name];
            }, set(value) {
                state.events.push(['fieldSet', name, encode(value), this === frame]);
                if (request.throwAt === 'set:' + name) throw new TypeError('field setter');
                state['field:' + name] = value;
                state['has:' + name] = true;
            }});
        }
        const results = [];
        for (let i = 0; i < request.count; i++) {
            const row = {result: invoke(frame, 'update')};
            for (const name of ['time', 'deltaTime', 'frameId', 'renderId', 'lastTime'])
                row[name] = encode(request.accessors ? state['field:' + name] : frame[name]);
            row.hasLastTime = Object.hasOwn(frame, 'lastTime');
            results.push(row);
        }
        return {results, events: state.events};
    } finally { Object.defineProperty(globalThis, 'performance', old); }
}
function runMetadata() {
    const frame = new NodeFrame(), prototype = NodeFrame.prototype;
    return {own: Object.getOwnPropertyNames(frame), keys: Object.keys(frame), prototypeOwn: Object.getOwnPropertyNames(prototype),
        prototypeMatches: Object.getPrototypeOf(frame) === prototype, constructorName: prototype.constructor.name,
        constructorLength: prototype.constructor.length, constructorPrototype: prototype.constructor.prototype === prototype,
        callConstructor: invoke(prototype, 'constructor'),
        fields: Object.getOwnPropertyNames(frame).map(name => {
            const descriptor = Object.getOwnPropertyDescriptor(frame, name);
            return [name, encode(frame[name]), !!descriptor.writable, !!descriptor.enumerable, !!descriptor.configurable];
        }), methods: Object.getOwnPropertyNames(prototype).map(name => {
            const descriptor = Object.getOwnPropertyDescriptor(prototype, name), fn = prototype[name];
            let constructible = true;
            try { Reflect.construct(Object, [], fn); } catch { constructible = false; }
            return [name, fn.name, fn.length, !!descriptor.writable, !!descriptor.enumerable, !!descriptor.configurable,
                constructible, Object.hasOwn(fn, 'prototype')];
        }), mapsDistinct: frame.updateMap !== frame.updateBeforeMap && frame.updateMap !== frame.updateAfterMap && frame.updateBeforeMap !== frame.updateAfterMap,
        mapBrands: [frame.updateMap, frame.updateBeforeMap, frame.updateAfterMap].map(value => value instanceof WeakMap)};
}
function mutableValue(spec,state){return spec?.tag==='match'?state.type:decode(spec,state,'enum');}
function mutablePoint(state,point){
    state.events.push(['point',point,state.depth]);const request=state.request;
    if(request.throwAt===point)throw new TypeError(point);
    if(request.mutateAt===point){for(const [key,value]of Object.entries(request.changes))state.enumValues[key]=mutableValue(value,state);state.events.push(['mutated',point]);}
    if(request.reenterAt===point&&!state.reentered){state.reentered=true;state.depth=1;const result=invoke(state.frame,method(request.phase),[state.node]);state.depth=0;state.events.push(['nested',point,result]);}
}
function runMutable(request){
    const frame=new NodeFrame(),state=newState(request,frame),node=state.node={};state.depth=0;state.type=decode(request.type,state,'type');state.enumValues={};state.enumReads={};
    state['field:frameId']=Object.hasOwn(request,'frameId')?decode(request.frameId):1;state['field:renderId']=Object.hasOwn(request,'renderId')?decode(request.renderId):2;
    const saved=['FRAME','RENDER','OBJECT'].map(key=>Object.getOwnPropertyDescriptor(NodeUpdateType,key));
    // The C++ module exposes a mutable facade; its private import still refers to this object.
    const constantsFacade={NodeUpdateType};
    for(const key of ['FRAME','RENDER','OBJECT']){
        state.enumReads[key]=0;state.enumValues[key]=Object.hasOwn(request.enums,key)?mutableValue(request.enums[key],state):key.toLowerCase();
        Object.defineProperty(NodeUpdateType,key,{enumerable:true,configurable:true,get(){const count=++state.enumReads[key];state.events.push(['enumGet',key,count,state.depth,this===NodeUpdateType]);mutablePoint(state,'enum:'+key);const sequence=request.sequences?.[key];return Array.isArray(sequence)&&sequence.length?mutableValue(sequence[Math.min(count-1,sequence.length-1)],state):state.enumValues[key];}});
    }
    if(request.replaceFacade)constantsFacade.NodeUpdateType={};
    for(const key of ['frameId','renderId'])Object.defineProperty(frame,key,{enumerable:true,configurable:true,get(){mutablePoint(state,'frameGet:'+key);return state['field:'+key];},set(value){mutablePoint(state,'frameSet:'+key);state['field:'+key]=value;}});
    const originalMaps=frame._getMaps,stage=request.phase,originalMap=frame[mapName(stage)];
    Object.defineProperty(frame,mapName(stage),{enumerable:true,configurable:true,get(){mutablePoint(state,'mapArgument');return originalMap;}});
    Object.defineProperty(frame,'_getMaps',{enumerable:true,configurable:true,get(){mutablePoint(state,'maps.get');return function(...a){mutablePoint(state,'maps.call');state.events.push(['mapsArguments',this===frame,a[0]===originalMap,a[1]===state.ref]);const maps=originalMaps.apply(this,a);if(state.record!==maps){state.record=maps;for(const key of ['frameId','renderId']){state['stored:'+key]=maps[key];Object.defineProperty(maps,key,{enumerable:true,configurable:true,get(){mutablePoint(state,'markGet:'+key);return state['stored:'+key];},set(value){mutablePoint(state,'markSet:'+key);state.events.push(['markValue',key,encode(value)]);state['stored:'+key]=value;}});}}return maps;};}});
    for(const name of [typeMethod(stage),'updateReference',callback(stage)]){const role=name===typeMethod(stage)?'type':name==='updateReference'?'reference':'callback';Object.defineProperty(node,name,{enumerable:true,configurable:true,get(){mutablePoint(state,role+'.get');return function(...a){mutablePoint(state,role+'.call');state.events.push(['receiver',role,this===node,a[0]===frame]);if(role==='type')return state.type;if(role==='reference')return state.ref;const count=++state.calls;return decode(request.returns[count-1],state);};}});}
    try{const steps=[];for(const operation of request.steps){for(const key of ['frameId','renderId'])if(Object.hasOwn(operation,key))frame[key]=decode(operation[key]);if(Object.hasOwn(operation,'type'))state.type=decode(operation.type,state,'stepType');if(operation.enums)for(const [key,value]of Object.entries(operation.enums))state.enumValues[key]=mutableValue(value,state);steps.push({result:invoke(frame,method(stage),[node]),calls:state.calls,ids:[encode(state['field:frameId']),encode(state['field:renderId'])],marks:state.record===undefined?'#undefined':[encode(state['stored:frameId']),encode(state['stored:renderId'])]});}return {steps,events:state.events,reads:state.enumReads};}
    finally{for(let i=0;i<saved.length;++i)Object.defineProperty(NodeUpdateType,['FRAME','RENDER','OBJECT'][i],saved[i]);}
}
function run(request) {
    if (request.kind === 'metadata') return runMetadata();
    if (request.kind === 'basic') return runBasic(request);
    if (request.kind === 'order') return runOrder(request);
    if (request.kind === 'maps') return runMaps(request);
    if (request.kind === 'mapPrototype') return runMapPrototype(request);
    if (request.kind === 'clock') return runClock(request);
    if (request.kind === 'mutable') return runMutable(request);
    throw new TypeError('unknown oracle kind');
}

const cases = [{kind: 'metadata'}], stages = ['before', 'update', 'after'];
const plainSteps = [{}, {}, {frameId: 1, renderId: 1}, {}, {frameId: 1, renderId: 2}, {}, {frameId: 2, renderId: 2}];
for (const phase of stages) {
    for (const type of [...Object.values(NodeUpdateType), 'unknown', null, false, 0, {tag: 'object'}]) {
        for (const id of [0, 1, '0', null, {tag: 'undefined'}, {tag: 'nan'}, {tag: 'negative-zero'}])
            cases.push({kind: 'basic', phase, type, frameId: id, renderId: id, steps: plainSteps});
    }
    for (const type of ['frame', 'render', 'object']) {
        for (const result of [false, true, 0, '', null, {tag: 'undefined'}, {tag: 'nan'}, {tag: 'object'}])
            cases.push({kind: 'basic', phase, type, frameId: 1, renderId: 1, returns: [result, result, true], steps: [{}, {}, {}]});
        for (const behavior of ['throw', 'reenter']) for (const result of [false, {tag: 'undefined'}])
            cases.push({kind: 'basic', phase, type, frameId: 1, renderId: 1, behavior, returns: [result, result, true], steps: [{}, {}, {}]});
        for (const nextId of [9, 0, '1', {tag: 'nan'}])
            cases.push({kind: 'basic', phase, type, frameId: 1, renderId: 1, behavior: 'mutate', axis: type === 'render' ? 'renderId' : 'frameId',
                nextId, returns: [true, true, true], steps: [{}, {}, {}]});
        for (const result of [false, true, {tag: 'undefined'}])
            cases.push({kind: 'basic', phase, type, frameId: 1, renderId: 1, shared: true, returns: [result, true, true],
                steps: [{}, {other: true}, {}, {other: true}, {frameId: 2, renderId: 2, other: true}, {}]});
    }
    for (const type of ['frame', 'render', 'object', 'none']) {
        const throwPoints = ['', 'get:' + typeMethod(phase), 'call:' + typeMethod(phase), 'get:updateReference', 'call:updateReference',
            'get:_getMaps', 'call:_getMaps', 'map', 'markGet', 'get:' + callback(phase), 'call:' + callback(phase), 'markSet'];
        for (const throwAt of throwPoints) for (const result of [false, true])
            cases.push({kind: 'order', phase, type, throwAt, result, changeId: true});
    }
}
for (const key of [{tag: 'object'}, null, 4, 'x', {tag: 'undefined'}, {tag: 'symbol'}])
    for (const throwAt of ['', 'get:get', 'get', 'get:set', 'set']) for (const replaceSet of [false, true])
        cases.push({kind: 'maps', key, throwAt, replaceSet});
for (const initial of [null, false, 0, {tag: 'object'}, {tag: 'undefined'}])
    cases.push({kind: 'maps', key: {tag: 'object'}, initial});
const ticks = [1000, 1015, 1020, 1050, 1055, 1090, 1100];
for (const accessors of [false, true]) {
    cases.push({kind: 'clock', ticks, count: 2, accessors});
    for (const lastTime of [null, 900, '900', {tag: 'undefined'}, {tag: 'nan'}, {tag: 'coerce', value: 900}])
        cases.push({kind: 'clock', ticks, count: 2, accessors, lastTime});
    for (const time of ['prefix', null, false, {tag: 'undefined'}, {tag: 'coerce', value: 'time:'}, {tag: 'coerce', value: 4}, {tag: 'symbol'}])
        cases.push({kind: 'clock', ticks, count: 1, accessors, time});
    for (const frameId of ['4', null, false, {tag: 'undefined'}, {tag: 'nan'}, {tag: 'coerce', value: '4'}, {tag: 'symbol'}, {tag: 'coerce', throw: true}])
        cases.push({kind: 'clock', ticks, count: 1, accessors, frameId});
    for (const throwAt of ['get:now:1', 'get:now:2', 'get:now:3', 'now:1', 'now:2', 'now:3'])
        cases.push({kind: 'clock', ticks, count: 2, accessors, throwAt});
}
for (const throwAt of ['get:frameId', 'set:frameId', 'get:lastTime', 'set:lastTime', 'get:deltaTime', 'set:deltaTime', 'get:time', 'set:time'])
    cases.push({kind: 'clock', ticks, count: 2, accessors: true, throwAt});
for (const transformed of [[null, '1020', 1030], [{tag: 'coerce', value: 1000}, {tag: 'coerce', value: 1020}, {tag: 'coerce', value: 1040}],
    [1000, {tag: 'undefined'}, 1040], [1000, {tag: 'symbol'}, 1040]])
    cases.push({kind: 'clock', ticks: transformed, count: 1, accessors: true});
// Append regression inputs without changing the original 782 scenarios.
for (const pollution of ['setter', 'readonly', 'getter-only'])
    for (const targets of [['renderId'], ['frameId'], ['renderId', 'frameId']])
        for (const hit of [false, true]) cases.push({kind: 'mapPrototype', pollution, targets, hit});

// Preserve the original 800 inputs and expected results as an unchanged prefix.
assert.equal(cases.length,800);
const match={tag:'match'},undefinedValue={tag:'undefined'};
for(const phase of stages){
    const base={kind:'mutable',phase,type:'custom',enums:{},sequences:{},steps:[{},{},{}],returns:[true,false,true]};
    for(const branch of ['FRAME','RENDER','OBJECT'])for(const type of ['custom',0,false,null,undefinedValue,{tag:'nan'},{tag:'negative-zero'},{tag:'object'},{tag:'symbol'},{tag:'coerce',throw:true}])for(const result of [false,true,0,null,undefinedValue])
        cases.push({...base,type,enums:{[branch]:match},returns:[result,result,result],replaceFacade:true});
    for(const type of ['frame','render','object','unknown',1,'1',{},undefinedValue])cases.push({...base,type});
    for(const enums of [{FRAME:match,RENDER:match,OBJECT:match},{FRAME:0,RENDER:match},{FRAME:false,RENDER:0,OBJECT:match}])cases.push({...base,enums});
    for(const [branch,sequences]of [['FRAME',{FRAME:[match,'miss',match]}],['RENDER',{FRAME:['miss'],RENDER:[match,'miss',match]}],['OBJECT',{FRAME:['miss'],RENDER:['miss'],OBJECT:[match,'miss',match]}]])for(const result of [false,true,0])cases.push({...base,enums:{[branch]:match},sequences,returns:[result,result,result]});
    for(const branch of ['FRAME','RENDER','OBJECT'])for(const id of [0,1,'1',null,undefinedValue,{tag:'nan'}])cases.push({...base,enums:{[branch]:match},frameId:id,renderId:id});
    for(const branch of ['FRAME','RENDER','OBJECT'])for(const reenterAt of ['type.call','reference.call','enum:FRAME','enum:RENDER','enum:OBJECT','callback.get','callback.call'])for(const result of [false,true])cases.push({...base,enums:{[branch]:match},reenterAt,returns:[result,true,true],gc:true});
    for(const [mutateAt,changes]of [['type.call',{FRAME:match}],['reference.call',{RENDER:match}],['enum:FRAME',{RENDER:match}],['enum:RENDER',{OBJECT:match}],['callback.call',{FRAME:'miss',OBJECT:match}],['maps.call',{FRAME:'miss',RENDER:match}]])cases.push({...base,enums:{FRAME:mutateAt==='callback.call'||mutateAt==='maps.call'?match:'miss',RENDER:'miss',OBJECT:'miss'},mutateAt,changes});
    for(const branch of ['FRAME','RENDER','OBJECT'])for(const throwAt of ['type.get','type.call','reference.get','reference.call','enum:FRAME','enum:RENDER','enum:OBJECT','maps.get','mapArgument','maps.call','markGet:frameId','frameGet:frameId','markGet:renderId','frameGet:renderId','markSet:frameId','markSet:renderId','callback.get','callback.call'])cases.push({...base,enums:{[branch]:match},throwAt});
    cases.push({...base,enums:{FRAME:match},steps:[{},{enums:{FRAME:'miss',RENDER:match}},{enums:{RENDER:'miss',OBJECT:match}},{frameId:3,renderId:4,enums:{FRAME:match}}]});
}
const expected = cases.map(run);
if (outputDirectory) {
    fs.mkdirSync(outputDirectory, {recursive: true});
    fs.writeFileSync(path.join(outputDirectory, 'node-frame-input.json'), JSON.stringify(cases, null, 2));
    fs.writeFileSync(path.join(outputDirectory, 'node-frame-expected.json'), JSON.stringify(expected, null, 2));
}
if (!binary) {
    console.log(`NodeFrame actual upstream oracle: ${cases.length} scenarios`);
} else {
const actual = JSON.parse(execFileSync(binary, [], {input: JSON.stringify(cases), encoding: 'utf8', maxBuffer: 128 * 1024 * 1024}));
if (outputDirectory) fs.writeFileSync(path.join(outputDirectory, 'node-frame-actual.json'), JSON.stringify(actual, null, 2));
assert.equal(actual.length, expected.length);
const differences = [];
for (let i = 0; i < cases.length; i++) {
    try { assert.deepEqual(actual[i], expected[i]); }
    catch { differences.push({index: i, request: cases[i], expected: expected[i], actual: actual[i]}); }
}
if (outputDirectory) fs.writeFileSync(path.join(outputDirectory, 'node-frame-differences.json'), JSON.stringify(differences, null, 2));
assert.equal(differences.length, 0, `${differences.length} NodeFrame differences; first ${JSON.stringify(differences[0])}`);
console.log(`NodeFrame dynamic protocol parity: ${cases.length} scenarios`);
}

assert.equal(typeof global.gc, 'function', 'Run the comparison with node --expose-gc');
async function jobs() {
    await new Promise(setImmediate); await new Promise(setImmediate);
    global.gc(); await new Promise(setImmediate); global.gc();
}
async function runGC() {
    const result = [];
    for (const stage of stages) {
        const frame = new NodeFrame();
        frame.frameId = 1;
        let weakKey, weakRecord, weakPayload, liveKey;
        function setup() {
            const key = {}; key.self = key;
            const payload = {}; payload.self = payload;
            weakKey = new WeakRef(key); weakPayload = new WeakRef(payload);
            const record = frame._getMaps(frame[mapName(stage)], key);
            record.payload = payload; record.key = key;
            weakRecord = new WeakRef(record); liveKey = key;
        }
        setup(); await jobs();
        assert.equal(weakKey.deref(), liveKey);
        assert.equal(frame.frameId, 1);
        result.push([stage, weakKey.deref() !== undefined, weakRecord.deref() !== undefined, weakPayload.deref() !== undefined]);
        liveKey = undefined; await jobs();
        result.push([stage, weakKey.deref() !== undefined, weakRecord.deref() !== undefined, weakPayload.deref() !== undefined]);
        assert.equal(frame.frameId, 1);
        assert.equal(liveKey, undefined);
    }
    let weakFrame, weakKey, weakRecord, liveKey;
    function setup() {
        const frame = new NodeFrame(), key = {}, record = frame._getMaps(frame.updateMap, key);
        record.frame = frame; frame.self = frame;
        weakFrame = new WeakRef(frame); weakKey = new WeakRef(key); weakRecord = new WeakRef(record); liveKey = key;
    }
    setup(); await jobs();
    assert.equal(weakKey.deref(), liveKey);
    result.push(['frameDropped', weakFrame.deref() !== undefined, weakKey.deref() !== undefined, weakRecord.deref() !== undefined]);
    liveKey = undefined; await jobs();
    result.push(['keyDropped', weakFrame.deref() !== undefined, weakKey.deref() !== undefined, weakRecord.deref() !== undefined]);
    assert.equal(liveKey, undefined);
    return result;
}
const expectedGC = await runGC();
const actualGC = binary ? JSON.parse(execFileSync(binary, ['gc'], {encoding: 'utf8', maxBuffer: 16 * 1024 * 1024})) : undefined;
if (outputDirectory) {
    fs.writeFileSync(path.join(outputDirectory, 'node-frame-gc-expected.json'), JSON.stringify(expectedGC, null, 2));
    if (binary) fs.writeFileSync(path.join(outputDirectory, 'node-frame-gc-actual.json'), JSON.stringify(actualGC, null, 2));
}
if (binary) assert.deepEqual(actualGC, expectedGC);
console.log(`NodeFrame ${binary ? 'actual Node/native GC parity' : 'actual Node GC oracle'}: ${expectedGC.length} snapshots`);
