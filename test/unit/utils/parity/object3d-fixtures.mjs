export const methodCases = [
 ['constructor',[]],['onBeforeShadow',[]],['onAfterShadow',[]],['onBeforeRender',[]],['onAfterRender',[]],
 ['applyMatrix4',[{matrix:[2,0,0,0,0,3,0,0,0,0,4,0,2,-1,3,1]}]],['applyQuaternion',[{quaternion:[0,Math.sin(.3),0,Math.cos(.3)]}]],
 ['setRotationFromAxisAngle',[{vector:[0,1,0]},.6]],['setRotationFromEuler',[{euler:[.2,-.3,.4,'ZXY']}]],['setRotationFromMatrix',[{matrix:[0,1,0,0,-1,0,0,0,0,0,1,0,0,0,0,1]}]],['setRotationFromQuaternion',[{quaternion:[.1,.2,.3,.9]}]],
 ['rotateOnAxis',[{vector:[1,0,0]},.3]],['rotateOnWorldAxis',[{vector:[0,1,0]},.3]],['rotateX',[.3]],['rotateY',[-.4]],['rotateZ',[.7]],
 ['translateOnAxis',[{vector:[0,0,1]},4]],['translateX',[2]],['translateY',[-3]],['translateZ',[5]],['localToWorld',[{vector:[1,2,3]}]],['worldToLocal',[{vector:[4,5,6]}]],['lookAt',[2,-1,4]],
 ['add',[{ref:'c'}]],['remove',[{ref:'a'}]],['removeFromParent',[]],['clear',[]],['attach',[{ref:'c'}]],['getObjectById',[{id:'b'}]],['getObjectByName',['b']],['getObjectByProperty',['name','b']],['getObjectsByProperty',['type','Object3D']],
 ['getWorldPosition',[{vector:[9,9,9]}]],['getWorldQuaternion',[{quaternion:[0,0,0,1]}]],['getWorldScale',[{vector:[9,9,9]}]],['getWorldDirection',[{vector:[9,9,9]}]],['raycast',[]],['intersectsFrustum',[]],
 ['traverse',[{callback:'visit'}]],['traverseVisible',[{callback:'visit'}]],['traverseAncestors',[{callback:'visit'}]],['updateMatrix',[]],['updateMatrixWorld',[]],['updateWorldMatrix',[true,true]],['toJSON',[]],['clone',[]],['copy',[{ref:'c'}]],['dispose',[]]
];
export const cases=methodCases.map(([method,args])=>({name:'method/'+method,method,args}));
for(const method of ['applyMatrix4','localToWorld','worldToLocal','lookAt','attach','getWorldPosition','getWorldQuaternion','getWorldScale','getWorldDirection','updateMatrix','updateMatrixWorld','updateWorldMatrix']){
 const base=methodCases.find(x=>x[0]===method);cases.push({name:'parented/'+method,method,args:base[1],parented:true});cases.push({name:'pivot/'+method,method,args:base[1],pivot:true});
}
for(const [name,method,args,extra]of [
 ['multiAdd','add',[{ref:'c'},{ref:'b'}],{}],['multiRemove','remove',[{ref:'a'},{ref:'b'}],{}],['selfAdd','add',[{ref:'root'}],{}],['nullAdd','add',[null],{}],['missingAdd','add',[],{}],['badAdd','add',[5],{}],['missingRemove','remove',[],{}],
 ['nonrecursiveClone','clone',[false],{}],['nonrecursiveCopy','copy',[{ref:'c'},false],{}],['copyFalseLike','copy',[{ref:'c'},0],{}],
 ['hiddenTraversal','traverseVisible',[{callback:'visit'}],{hidden:true}],['nameMissing','getObjectByName',['absent'],{}],['collectWithExisting','getObjectsByProperty',['name','b',{array:['sentinel']}],{}],
 ['cameraLookAt','lookAt',[{vector:[2,3,-4]}],{camera:true,parented:true}],['lightLookAt','lookAt',[{vector:[-1,4,2]}],{light:true,parented:true}],['coincidentLookAt','lookAt',[{vector:[1,2,3]}],{}],
 ['autoUpdateFalse','updateMatrixWorld',[true],{autoUpdate:false}],['worldAutoFalse','updateWorldMatrix',[true,true,true],{worldAuto:false}],['negativeScale','applyMatrix4',[{matrix:[-2,0,0,0,0,3,0,0,0,0,4,0,0,0,0,1]}],{}],['zeroScale','applyMatrix4',[{matrix:[0,0,0,0,0,0,0,0,0,0,0,0,4,5,6,1]}],{}],
 ['matrixCallbackThrow','setRotationFromEuler',[{euler:[1,2,3,'XYZ']}],{throwAt:'rotationSync'}],['addThrow','add',[{ref:'c'}],{throwAt:'c:added'}],['childAddedThrow','add',[{ref:'c'}],{throwAt:'root:childadded'}],['removeThrow','remove',[{ref:'a'}],{throwAt:'a:removed'}],['childRemovedThrow','remove',[{ref:'a'}],{throwAt:'root:childremoved'}],
 ['traverseThrow','traverse',[{callback:'visit'}],{throwAt:'visit:a'}],['traverseRemove','traverse',[{callback:'visit'}],{mutateAt:'visit:root'}],['addedReentry','add',[{ref:'c'}],{reenterAt:'c:added'}],['sharedEventReentry','add',[{ref:'c'}],{reenterAt:'root:childadded'}],
 ['jsonStringMeta','toJSON',['key'],{}],['jsonNullMeta','toJSON',[null],{}],['jsonUserData','toJSON',[],{userData:true}],['copyUserData','copy',[{ref:'c'}],{userData:true}],['jsonResourceAlias','toJSON',[],{resources:'alias'}],['jsonSameUUID','toJSON',[],{resources:'sameUUID'}],['jsonScene','toJSON',[],{resources:'scene'}],['jsonInstanced','toJSON',[],{resources:'instanced'}],['jsonBatched','toJSON',[],{resources:'batched'}],['jsonSkinned','toJSON',[],{resources:'skinned'}],['jsonResourceThrow','toJSON',[],{resources:'alias',throwAt:'resource:geometry'}],
 ['jsonMetadataThrow','toJSON',[],{resources:'lockedMetadata'}],['jsonResourcePrimitive','toJSON',[],{resources:'primitive'}],['jsonCachePrototype','toJSON',[],{resources:'prototype'}]
])cases.push({name,method,args,...extra});
for(const method of ['applyMatrix4','lookAt','updateMatrix','updateMatrixWorld','updateWorldMatrix','copy','toJSON','add','remove','traverse']){
 const base=methodCases.find(x=>x[0]===method);cases.push({name:'observed/'+method,method,args:base[1],observed:true});
}
cases.push({name:'metadata',special:'metadata'},{name:'constructor-defaults',special:'defaults'},{name:'constructor-descriptors',special:'constructorDescriptors'},{name:'constructor-super',special:'super'},{name:'module-private-binding',special:'module'});

cases.push({name:'callback-original-bindings',special:'callbackBindings'},{name:'defineProperties-atomic-conversion',special:'defineProperties'});

cases.push({name:'lookAt-shared-scratch-reentry',method:'lookAt',args:[2,-1,4],scratchReentry:true});

cases.push({name:'event-symbol-key',special:'eventKeys'});
