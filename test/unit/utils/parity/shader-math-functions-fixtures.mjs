const node=(type,...args)=>({node:type,args});
export function fixtures(){
 const out=[{name:'metadata',mode:'metadata'}];
 const values=[0,{$number:'-0'},0.125,0.5,1,-1,4.25,4294967295,4294967296,{$number:'NaN'},{$number:'Infinity'},{$number:'-Infinity'}];
 for(const value of values)for(const type of ['float','int','uint']){
  out.push({name:'hash/body/'+type+'/'+JSON.stringify(value),fn:'hash',mode:'body',args:[node(type,value)],build:true});
 }
 for(const type of ['float','vec2','vec3','vec4'])for(const value of [0.125,0.5,1.125]){
  out.push({name:'hash/wrapper/'+type+'/'+value,fn:'hash',mode:'call',args:[node(type,value)],build:true});
 }
 for(const fn of ['parabola','gain','pcurve','sinc'])for(const type of ['float','vec2','vec3'])for(const value of [0.125,0.5,1.125,-0.75]){
  const args=fn==='pcurve'?[node(type,value),node(type,2.25),node(type,3.125)]:[node(type,value),node(type,2.25)];
  out.push({name:fn+'/'+type+'/'+value,fn,mode:'call',args,build:true});
 }
 for(const fn of ['parabola','gain','pcurve','sinc'])for(const value of [0,null,{$undefined:true},{$symbol:'seed'}]){
  out.push({name:fn+'/invalid/'+JSON.stringify(value),fn,mode:'call',args:[value,node('float',2.25),node('float',3.125)],build:false});
 }
 for(const value of [0,null,{$undefined:true},{$symbol:'seed'}])out.push({name:'hash/body/invalid/'+JSON.stringify(value),fn:'hash',mode:'body',args:[value],build:false});
 for(const fn of ['parabola','gain','sinc'])for(const mode of ['getThrow','reenter','collect']){
  out.push({name:fn+'/real-node-method/'+mode,fn,mode:'call',args:[node('float',0.371),node('float',2.25)],instrument:{argument:fn==='sinc'?1:0,key:fn==='gain'?'lessThan':'mul',mode},build:true});
 }
 for(const mode of ['nextThrow','returnThrow','returnPrimitive','doneThrow','valueThrow','empty','normal'])out.push({name:'hash/body/iterator/'+mode,fn:'hash',mode:'body',args:[node('float',0.371)],iterator:mode,build:false});
 out.push({name:'imports/private-bindings',mode:'bindings'});
 return out;
}
