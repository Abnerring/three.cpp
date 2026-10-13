import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {writeFileSync} from 'node:fs';
if(typeof global.gc!=='function')throw Error('Run Node with --expose-gc');
let held;
function setup(map,mode){
 const object=map?new Map():new Set(),key={name:'old'},value={name:'value'},fresh={name:'new'};
 object[map?'set':'add'](key,value);held=object.keys();
 if(mode===1)held.next();if(mode===2)object.delete(key);
 if(mode===3){object.clear();object[map?'set':'add'](fresh,value)}
 if(mode===4){held.next();object.delete(key);object[map?'set':'add'](key,value)}
 return [object,key,value,fresh].map(v=>new WeakRef(v));
}
async function collect(){for(let i=0;i<6;i++){await new Promise(setImmediate);global.gc()}}
function drain(){const result=[];for(const key of held)result.push(key.name);return result}
const expected=[];
for(const map of [true,false])for(let mode=0;mode<5;mode++){
 const refs=setup(map,mode);await collect();const before=refs.map(r=>!!r.deref());
 const yielded=drain();await collect();expected.push({map,mode,before,yielded,after:refs.map(r=>!!r.deref())});held=undefined;
}
function symbolSetup(){const symbol=Symbol('kept');held={[symbol]:1};return new WeakRef(symbol)}
const symbol=symbolSetup();await collect();const symbolBefore=!!symbol.deref();held=undefined;await collect();expected.push({symbolBefore,symbolAfter:!!symbol.deref()});
const actual=execFileSync(process.argv[2],{encoding:'utf8'}).trim().split(/\r?\n/).map(JSON.parse);
if(process.argv[3]) {
 writeFileSync(process.argv[3]+'-node.json',JSON.stringify(expected,null,2)+'\n');
 writeFileSync(process.argv[3]+'-native.json',JSON.stringify(actual,null,2)+'\n');
}
assert.equal(actual.length,expected.length);
for(let i=0;i<actual.length;i++) {
 if(expected[i].mode===3) {
  // V8 retains the old table until next(). Native clear removes old strong
  // edges immediately. Compare every semantic result and other lifetime edge;
  // document this one storage-policy difference instead of emulating rehash.
  assert.equal(actual[i].before[1],false,'Native clear releases the old key');
  const native=structuredClone(actual[i]),node=structuredClone(expected[i]);
  native.before.splice(1,1);node.before.splice(1,1);assert.deepEqual(native,node);
 } else assert.deepEqual(actual[i],expected[i]);
}
console.log(`Native collection lifetime: 9 exact Node snapshots and 2 clear-policy boundary scenarios passed (${process.version}); GC timing is not an ECMAScript guarantee.`);
