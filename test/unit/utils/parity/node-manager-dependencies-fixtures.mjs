const ref=name=>({$ref:name}),undef={$value:'undefined'},set=(name,value)=>({op:'set',name,value}),emit=value=>({op:'emit',value}),fn=(label,value)=>({$fn:{label,value}});
const exports=['fog.rangeFogFactor','fog.densityFogFactor','fog.exponentialHeightFogFactor','fog.fog','equirect.equirectUV','equirect.equirectDirection'];
const selectors=['x','y','z','rgb','a'],methods=['negate','mul','sub','max','toConst','exp','oneMinus','toFloat','mix','toVec3','atan','add','clamp','asin','cos','sin'];
export function fixtures(){const requests=[];
 for(const name of exports)for(const iterator of ['array','custom','short','nextThrow','returnThrow','returnPrimitive']){
  const steps=[set('fn',{$binding:name}),set('node',{$new:'Node',args:['float']}),set('node.fixtureTag','protocol-node')];
  for(const key of selectors)steps.push({op:'define',name:'node',key,get:fn('get:node.'+key,ref('node'))});
  for(const key of methods)steps.push({op:'define',name:'node',key,get:fn('get:node.'+key,fn('call:node.'+key,ref('node')))});
  steps.push(set('builder',{context:{getViewZ:fn('context.getViewZ',ref('node'))}}));
  const values=iterator==='short'?[]:[ref('node'),ref('node')],input=iterator==='array'?values:{$iterable:{values,nextThrow:iterator==='nextThrow',returnThrow:iterator==='returnThrow',...(iterator==='returnPrimitive'?{returnValue:0}:{})}};
  steps.push({op:'apply',name:'fn.shaderNode.jsFunc',args:[input,ref('builder')],save:'result'},emit(ref('result')));requests.push({label:name+' '+iterator,steps});
 }
 for(const name of exports)for(const values of [[],[0],[1,2],[-2,.4],[{$value:'NaN'},{$value:'-0'}]]){
  const steps=[set('fn',{$binding:name}),set('builder',{context:{}})];const args=values.map(v=>({$node:'float',args:[v]}));steps.push({op:'apply',name:'fn.shaderNode.jsFunc',args:[args,ref('builder')],save:'result'},emit(ref('result')));requests.push({label:name+' real '+JSON.stringify(values),steps});
 }
 const typed={
 'fog.rangeFogFactor':[{$node:'float',args:[1]},{$node:'float',args:[10]}],
 'fog.densityFogFactor':[{$node:'float',args:[.1]}],
 'fog.exponentialHeightFogFactor':[{$node:'float',args:[.1]},{$node:'float',args:[20]}],
 'fog.fog':[{$node:'vec3',args:[.1,.2,.3]},{$node:'float',args:[.2]}],
 'equirect.equirectUV':[{$node:'vec3',args:[1,0,0]}],
 'equirect.equirectDirection':[{$node:'vec2',args:[.3,.8]}]
 };for(const name of exports)for(const view of [false,true])requests.push({label:name+' real active Stack '+view,steps:[set('stack',{$stack:true}),set('fn',{$binding:name}),set('builder',{context:view?{getViewZ:fn('getViewZ',{$node:'float',args:[5]})}:{}}),{op:'apply',name:'fn.shaderNode.jsFunc',args:[typed[name],ref('builder')],save:'result'},emit(ref('result')),emit(ref('stack'))]});
 return requests;
}
