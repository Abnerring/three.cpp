import {upstreamModule} from './threejs-source.mjs';
const {default:Node}=await upstreamModule('src/nodes/core/Node.js');
function encode(value){if(value===undefined)return['undefined'];if(typeof value==='number'){if(Number.isNaN(value))return['number','NaN'];if(!Number.isFinite(value))return['number',value>0?'Infinity':'-Infinity'];if(Object.is(value,-0))return['number','-0'];}return value;}
export function run(request){const instance=new Node(),getter=Object.getOwnPropertyDescriptor(Node.prototype,'uuid').get;let receiver=request.receiver==='host-null'?{_uuid:null}:request.receiver==='host-undefined'?{}:request.receiver==='number'?17:request.receiver==='null'?null:instance;
 if(request.cached)instance._uuid=request.cached==='undefined'?undefined:request.cached==='zero'?0:request.cached==='false'?false:'existing';
 const events=[],values=[];let randomCalls=0,cached=null;
 if(request.cacheGetter)Object.defineProperty(instance,'_uuid',{enumerable:true,configurable:true,get(){events.push('get:_uuid');if(request.cacheGetThrows)throw new TypeError('cache get');return cached;},set(value){events.push('set:_uuid');if(request.cacheSetThrows)throw new TypeError('cache set');cached=value;}});
 const original=Math.random;Math.random=request.notCallable?17:function(){++randomCalls;events.push('random:'+randomCalls);if(request.throwAt===randomCalls)throw new TypeError('random');return request.randomTag==='NaN'?NaN:request.randomTag==='Infinity'?Infinity:request.randomTag==='-0'?-0:Object.hasOwn(request,'random')?request.random:randomCalls%257/257;};
 try{for(let i=0;i<(request.reads??1);++i){if(i&&request.reset)receiver._uuid=null;try{values.push({value:encode(Reflect.apply(getter,receiver,[]))});}catch(error){values.push({error:error.constructor.name});}}}finally{Math.random=original;}
 return{name:request.name,id:instance.id,values,randomCalls,events};
}
