export function fixtures(){return [
 {name:'lazy-two-reads',reads:2},{name:'reset-null',reads:2,reset:true},{name:'constant-zero',random:0},{name:'constant-one',random:1},{name:'negative-fraction',random:-.25},{name:'large-value',random:4294967296},{name:'negative-zero',randomTag:'-0'},{name:'NaN',randomTag:'NaN'},{name:'Infinity',randomTag:'Infinity'},
 ...['undefined','zero','false','text'].map(cached=>({name:'cached-'+cached,cached,reads:2})),
 ...[1,2,4].map(throwAt=>({name:'random-throw-'+throwAt,throwAt,reads:2})),
 {name:'random-not-callable',notCallable:true},{name:'borrow-host-null',receiver:'host-null',reads:2},{name:'borrow-host-undefined',receiver:'host-undefined',reads:2},{name:'borrow-number',receiver:'number'},{name:'borrow-null',receiver:'null'},
 {name:'getter-observation',cacheGetter:true,reads:2},{name:'getter-abrupt',cacheGetter:true,cacheGetThrows:true,reads:2},{name:'setter-abrupt',cacheGetter:true,cacheSetThrows:true,reads:2}
 ];}
