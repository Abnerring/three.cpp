const defaults={boxed:false,accessor:false,action:'return',alias:'none',operation:'clone',value:17};
export const fixtures=[
 {name:'outer Symbol enumerable uniform',level:'outer'},
 {name:'inner Symbol enumerable value',level:'inner'},
 {name:'outer primitive getter preserves receiver and repeated reads',level:'outer',accessor:true},
 {name:'inner primitive getter preserves receiver',level:'inner',accessor:true},
 {name:'outer boxed getter preserves wrapper receiver',level:'outer',boxed:true,accessor:true},
 {name:'inner boxed getter preserves wrapper receiver',level:'inner',boxed:true,accessor:true},
 {name:'outer getter abrupt numeric completion',level:'outer',accessor:true,action:'throwNumber'},
 {name:'inner getter real TypeError completion',level:'inner',accessor:true,action:'throwTypeError'},
 {name:'outer getter reenters real cloneUniforms',level:'outer',accessor:true,action:'reenter'},
 {name:'inner getter reenters real cloneUniforms',level:'inner',accessor:true,action:'reenter'},
 {name:'merge outer Symbol survives global and constructor data aliases',level:'outer',alias:'data',operation:'merge'},
 {name:'inner Symbol survives throwing global and constructor accessors',level:'inner',alias:'getters'}
].map(request=>({...defaults,...request}));
