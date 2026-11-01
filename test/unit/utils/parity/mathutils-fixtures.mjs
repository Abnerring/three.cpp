export const cases=[];const add=(name,fn,args=[],extra={})=>cases.push({name,fn,args,...extra});
for(const fn of ['generateUUID','clamp','euclideanModulo','mapLinear','inverseLerp','lerp','damp','pingpong','smoothstep','smootherstep','randInt','randFloat','randFloatSpread','seededRandom','degToRad','radToDeg','isPowerOfTwo','ceilPowerOfTwo','floorPowerOfTwo','setQuaternionFromProperEuler','normalize','denormalize'])cases.push({name:fn+'.metadata',special:'metadata',fn});
cases.push({name:'exportsAndIdentity',special:'exportsAndIdentity'},{name:'privateBindings',special:'privateBindings'},{name:'functionConstruct',special:'functionConstruct'},{name:'dynamicGlobals',special:'dynamicGlobals'});
const scalars=[0,{$number:'-0'},1,-1,.5,2,3,100000,1e30,{$number:'NaN'},{$number:'Infinity'},{$number:'-Infinity'},null,{$undefined:true},'2',true];
for(const value of scalars)for(const fn of ['degToRad','radToDeg','isPowerOfTwo','ceilPowerOfTwo','floorPowerOfTwo'])add(fn+'.'+JSON.stringify(value),fn,[value]);
for(const n of [-5,-1,0,.25,1,3,20])for(const m of [-3,0,.5,2,7])add('euclideanModulo.'+n+'.'+m,'euclideanModulo',[n,m]);
for(const fn of ['clamp','smoothstep','smootherstep'])for(const x of [-2,0,.2,.5,1,2])for(const bounds of [[0,1],[1,0],[0,0]])add(fn+'.'+x+'.'+bounds.join(','),fn,[x,...bounds]);
for(const t of [-1,0,.25,1,2,{$number:'NaN'}]){add('lerp.'+JSON.stringify(t),'lerp',[2,8,t]);add('inverseLerp.'+JSON.stringify(t),'inverseLerp',[2,8,t]);add('mapLinear.'+JSON.stringify(t),'mapLinear',[t,0,1,-4,6]);add('damp.'+JSON.stringify(t),'damp',[2,8,3,t]);add('pingpong.'+JSON.stringify(t),'pingpong',[t]);}
for(const fn of ['clamp','mapLinear','inverseLerp','lerp','damp','pingpong','smoothstep','smootherstep']){
 const arity={clamp:3,mapLinear:5,inverseLerp:3,lerp:3,damp:4,pingpong:2,smoothstep:3,smootherstep:3}[fn];
 add(fn+'.coercion',fn,Array.from({length:arity},(_,i)=>({$coerce:i+.25,tag:'a'+i})));
 add(fn+'.strings',fn,Array.from({length:arity},(_,i)=>String(i+.25)));
 add(fn+'.throws',fn,Array.from({length:arity},(_,i)=>({$coerce:i+.25,tag:'a'+i,...(i===0?{throw:43}:{})})));
}
for(const samples of [[0,0,0,0],[.5,.25,.75,.1],[1,1,1,1],[-1,1.5,{$number:'NaN'},{$number:'Infinity'}]])add('generateUUID.'+JSON.stringify(samples),'generateUUID',[],{random:samples});
for(const fn of ['randInt','randFloat','randFloatSpread'])for(const random of [0,.125,.5,.999999,1,-.25])add(fn+'.'+random,fn,fn==='randFloatSpread'?[8]:[-3,7],{random:[random]});
for(const seed of [0,1,-1,1234567,4294967295,4294967296,'3',null,{$number:'NaN'},{$number:'Infinity'}])add('seededRandom.'+JSON.stringify(seed),'seededRandom',[seed],{repeat:6});
for(const value of [0,1,2,3,4,8,16,1024,2**31,2**32,2**40,2**53,2**100,Number.MIN_VALUE,Number.MAX_VALUE])add('power.'+value,'isPowerOfTwo',[value]);
for(const order of ['XYX','YZY','ZXZ','XZX','YXY','ZYZ','bad',null])add('properEuler.'+order,'setQuaternionFromProperEuler',[{$quaternion:true},.2,-.4,.7,order]);
add('properEuler.setterThrows','setQuaternionFromProperEuler',[{$quaternion:true},.2,-.4,.7,'XYX'],{setThrows:true});
for(const fn of ['normalize','denormalize'])for(const typed of ['Float32Array','Float64Array','Int8Array','Uint8Array','Uint8ClampedArray','Int16Array','Uint16Array','Int32Array','Uint32Array'])for(const value of [-2,-1,-.5,{$number:'-0'},0,.5,1,2,{$number:'NaN'}])add(fn+'.'+typed+'.'+JSON.stringify(value),fn,[value,{$typed:typed}]);
for(const fn of ['normalize','denormalize']){add(fn+'.floatIdentity',fn,[{$coerce:7,tag:'value'},{$typed:'Float32Array'}]);add(fn+'.constructorGetter',fn,[.5,{$constructor:'Uint8Array'}]);add(fn+'.constructorThrows',fn,[.5,{$constructor:'Int8Array',throw:29}]);}
for(const method of ['exp','log','log2'])for(const v of scalars)add('Math.'+method+'.'+JSON.stringify(v),method,[v],{global:'Math'});
for(const method of ['pow','imul'])for(const pair of [[2,3],[-2,.5],[{$number:'-0'},-3],[{$number:'NaN'},0],[4294967295,5],[2147483648,2],[{$coerce:3,tag:'left'},{$coerce:4,tag:'right'}]])add('Math.'+method+'.'+JSON.stringify(pair),method,pair,{global:'Math'});
for(const v of scalars)add('Number.isInteger.'+JSON.stringify(v),'isInteger',[v],{global:'Number'});
for(const values of [[],[{$undefined:true}],[null],[' 0x20 '],['bad'],[{$number:'-0'}],[{$coerce:3,tag:'number'}]])cases.push({name:'Number.construct.'+JSON.stringify(values),special:'numberConstruct',args:values});
