import vm from'node:vm';import assert from'node:assert/strict';
assert.equal(typeof global.gc,'function');assert.equal(Number(process.versions.node.split('.')[0]),26);
const next=()=>new Promise(resolve=>setImmediate(resolve));
async function gone(weak){for(let i=0;i<80;i++){await next();global.gc();if(weak.deref()===undefined)return true;}return false;}
async function alive(weak){for(let i=0;i<3;i++){await next();global.gc();assert.notEqual(weak.deref(),undefined);}return true;}
function freeRealm(){let context=vm.createContext({});const ctor=vm.runInContext('Promise',context),weak=new WeakRef(ctor);context=null;return weak;}
function heldMethodRealm(){let context=vm.createContext({});const ctor=vm.runInContext('Promise',context),weak=new WeakRef(ctor),held=ctor.prototype.then;context=null;return{weak,held};}
function cycleRealm(){let context=vm.createContext({});const pair=vm.runInContext('(()=>{const promise=new Promise(()=>{});promise.then(()=>promise);return [Promise,promise];})()',context);const ctor=new WeakRef(pair[0]),pending=new WeakRef(pair[1]),held=pair[0];context=null;return{ctor,pending,held};}
const observations=[];
for(let round=0;round<3;round++){
 const free=freeRealm();assert.equal(await gone(free),true);observations.push({round,phase:'actual-vm-realm-retired',gone:true});
 let method=heldMethodRealm();assert.equal(await alive(method.weak),true);observations.push({round,phase:'actual-promise-method-held',alive:true});const weakMethod=method.weak;method=null;assert.equal(await gone(weakMethod),true);observations.push({round,phase:'actual-promise-method-dropped',gone:true});
 let cycle=cycleRealm();assert.equal(await gone(cycle.pending),true);assert.equal(await alive(cycle.ctor),true);observations.push({round,phase:'actual-unreachable-pending-cycle-collected',gone:true,realmAlive:true});const weakCycle=cycle.ctor;cycle=null;assert.equal(await gone(weakCycle),true);observations.push({round,phase:'actual-cycle-realm-retired',gone:true});
 const context=vm.createContext({});const semantic=await vm.runInContext(`(async()=>{
   const IntrinsicPromise=Promise,events=[];
   globalThis.Promise=17;
   const result=(async()=>3)();
   const privateAuthority=Object.getPrototypeOf(result)===IntrinsicPromise.prototype;
   delete globalThis.Promise;
   const adopted=IntrinsicPromise.resolve({get then(){events.push('get then');return(resolve,reject)=>{events.push('call then');resolve(23);reject(24);};}});
   await adopted.then(value=>events.push(value===23?'fulfilled':'wrong'));
   const reason={};let thrownSame=false;
   await new IntrinsicPromise(()=>{const generator=(function*(){throw reason;})();generator.next();}).then(undefined,value=>{thrownSame=value===reason;});
   return{privateAuthority,events,thrownSame};
 })()`,context);
 assert.equal(semantic.privateAuthority,true);assert.deepEqual(Array.from(semantic.events),['get then','call then','fulfilled']);assert.equal(semantic.thrownSame,true);observations.push({round,phase:'actual-vm-private-intrinsic-and-thenable',privateAuthority:true,thenableOrder:true,rawGeneratorRejectionIdentity:true});
}
const first=performance.now(),second=performance.now();assert.ok(Number.isFinite(first)&&first>=0&&second>=first);assert.ok(Number.isFinite(performance.timeOrigin)&&performance.timeOrigin>0);assert.equal(performance.now.name,'now');assert.equal(performance.now.length,0);
process.stdout.write(JSON.stringify({rounds:3,observations,actualVMRealmRetirements:observations.filter(v=>['actual-vm-realm-retired','actual-promise-method-dropped','actual-cycle-realm-retired'].includes(v.phase)).length,realMonotonicClock:true,nativeExecuted:false}));
