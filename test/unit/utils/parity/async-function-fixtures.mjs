export function createCaseFunction(r,ctx){
 const events=ctx.events,normal=ctx.normal;
 const initialize=()=>{events.push('initialize');if(r.initThrows)throw ctx.value;return ctx.value;};
 switch(r.body){
  case 'return':case 'lowReturn':return async function fixture(x){events.push('body');return x;};
  case 'throw':case 'lowThrow':return async function fixture(x){events.push('body');throw x;};
  case 'await':case 'lowAwait':return async function fixture(x){events.push('body');const value=await x;events.push(['resume',normal(value)]);return value;};
  case 'catch':return async function fixture(x){events.push('body');try{return await x;}catch(error){events.push(['catch',normal(error)]);return error;}finally{events.push('finally');}};
  case 'twoAwait':return async function fixture(x){events.push('body');const first=await x;events.push('between');const last=await first;events.push('after');return last;};
  case 'afterThrow':case 'lowAfterThrow':return async function fixture(x){events.push('body');const value=await x;events.push(['resume',normal(value)]);throw value;};
  case 'default':return async function fixture(x=initialize()){events.push('body');return x;};
  case 'receiver':return async function fixture(x){events.push(['this',normal(this)]);return x;};
  default:throw Error('Unknown native async fixture body '+r.body);
 }
}
export function fixtures(){
 const values=[{tag:'undefined'},null,false,0,17,'value',{tag:'object'},{tag:'symbol'},{tag:'NaN'},{tag:'-0'},{tag:'Infinity'},{tag:'-Infinity'}];
 const result=[{kind:'metadata'},{kind:'empty',construct:false},{kind:'empty',construct:true},{kind:'privateIntrinsics'},{kind:'bound'}];
 for(const error of ['TypeError','RangeError','Error'])result.push({kind:'nativeError',error});
 for(const mode of ['object','null','number','bound','proxyObject','proxyNull','proxyThrow','revoked','nonConstructor'])result.push({kind:'newTarget',mode});
 for(const body of ['return','throw','await','catch','twoAwait','afterThrow','default','receiver','lowReturn','lowThrow','lowAwait','lowAfterThrow'])for(const value of values)result.push({kind:'invoke',body,value});
 for(const mode of ['fulfilled','rejected','thenableFulfill','thenableReject','thenableThrow','thenGetterThrow','thenNoncallable','thenTwice','poisonThenFulfilled','poisonConstructor','pendingFulfill','pendingReject'])for(const body of ['return','await','catch','twoAwait','lowAwait'])result.push({kind:'invoke',body,value:{tag:'object'},mode});
 for(const body of ['return','await','lowAwait'])for(const receiver of ['undefined','null','number'])result.push({kind:'invoke',body,value:23,receiver});
 for(const value of values)result.push({kind:'invoke',body:'default',value,initThrows:true,omit:true});
 for(const body of ['return','await','lowAwait'])for(const mode of ['thenableFulfill','thenGetterThrow'])result.push({kind:'invoke',body,value:19,mode,reenter:true});
 for(const body of ['return','await','lowReturn','lowAwait'])result.push({kind:'invoke',body,value:31,poisonGlobalPromise:true});
 for(const r of result)if(r.kind==='invoke')r.source=Function.prototype.toString.call(createCaseFunction(r,{events:[],normal:v=>v}));
 return result;
}
