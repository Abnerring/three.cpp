export function fixtures(){
 const out=[];
 for(const mode of ['omitted','undefined','null','array'])for(const count of [0,2])out.push({kind:'constructor',mode,count});
 const candidates=[[],[[]],[['float']],[['int'],['float']],[['vec3'],['vec4']],[['float'],['float']],[['float','int'],['int','float']],[['float','int','float'],['int']],[null],[['float'],null]];
 for(const types of [[],['float'],['int'],['float','int']])for(const definitions of candidates)for(const method of ['getCandidateFn','generateNodeType','setup'])out.push({kind:'score',types,definitions,method});
 for(const mode of ['chosen','none','undefined','false','parameters-get-first'])out.push({kind:'cache',mode});
 for(const mode of ['normal','shader-get','shader-undefined','layout-get','layout-null','layout-undefined','inputs-get','params-length','inputs-length','param-get','method-get','method-call','input-get','input-type','next-get','next-call','result-primitive','done-get','value-get','return-get','return-call','return-primitive','return-not-callable','dynamic-length','mutate-type','reentrant','gc'])out.push({kind:'protocol',mode});
 for(const mode of ['normal','next-call','value-get','result-primitive','call-throw','parameters-get-second'])out.push({kind:'setup-iteration',mode});
 for(const mode of ['empty','node','primitive','real-fn'])out.push({kind:'factory',mode});
 return out;
}
