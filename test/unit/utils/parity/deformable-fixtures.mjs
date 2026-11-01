
const U={$:'undefined'},Z={$:'-0'},N={$:'NaN'},P={$:'Infinity'},M={$:'-Infinity'},S={$:'symbol'};
const A=items=>({$:'array',items}),O=entries=>({$:'object',props:Object.entries(entries).map(([key,value])=>({key,value}))}),R=id=>({$:'ref',id}),path=(...keys)=>({$:'path',keys}),New=(name,args=[],extra={})=>({$:'new',class:name,args,...extra});
const matrix=(x=0,y=0,z=0)=>({$:'matrix',elements:[1,0,0,0,0,1,0,0,0,0,1,0,x,y,z,1]});
const geo=(extra={})=>({$:'geometry',...extra});
const step=(method,args=[],extra={})=>({method,args,...extra});
const ray=()=>O({ray:New('Ray',[New('Vector3',[.5,.5,10]),New('Vector3',[0,0,-1])]),near:0,far:100});
const camera={$:'camera'};
export function fixtures(){const out=[],add=(C,q)=>out.push({class:C,...q});
 for(const C of ['InstancedMesh','SkinnedMesh','BatchedMesh'])add(C,{kind:'metadata'});
 for(const C of ['InstancedMesh','SkinnedMesh','BatchedMesh']){
  const base=C==='BatchedMesh'?[3,18,18]:C==='InstancedMesh'?[geo(),New('MeshBasicMaterial'),3]:[geo({skin:true}),New('MeshBasicMaterial')];
  for(const mode of [{},{newTarget:true},{withoutNew:true},{badBase:true}])add(C,{kind:'construct',args:base,...mode});
  for(const setter of C==='BatchedMesh'?['isBatchedMesh','_matricesTexture','_indirectTexture']:C==='InstancedMesh'?['isInstancedMesh','instanceMatrix','count']:['isSkinnedMesh','bindMatrix','bindMode'])for(const throwSetter of [false,true])add(C,{kind:'construct',args:base,setter,throwSetter});
  for(const v of [U,null,false,0,-1,1,2.5,'2',N,S]){const args=C==='BatchedMesh'?[v,9,9]:C==='InstancedMesh'?[geo(),New('MeshBasicMaterial'),v]:[v,New('MeshBasicMaterial')];add(C,{kind:'construct',args});}
 }
 const specs={InstancedMesh:[['computeBoundingBox',[]],['computeBoundingSphere',[]],['getMatrixAt',[1,New('Matrix4')]],['getColorAt',[0,New('Color')]],['setMatrixAt',[1,matrix(3,-2,1)]],['setColorAt',[1,New('Color',[.2,.4,.6])]],['setMorphAt',[1,O({morphTargetInfluences:A([.25,.5])})]],['getMorphAt',[1,O({morphTargetInfluences:A([0,0])})]],['raycast',[ray(),A([])]],['updateMorphTargets',[]],['dispose',[]]],SkinnedMesh:[['computeBoundingBox',[]],['computeBoundingSphere',[]],['getVertexPosition',[1,New('Vector3')]],['applyBoneTransform',[1,New('Vector3',[2,0,0])]],['normalizeSkinWeights',[]],['pose',[]],['updateMatrixWorld',[true]],['bind',[R('skeleton')]],['raycast',[ray(),A([])]]],BatchedMesh:[['computeBoundingBox',[]],['computeBoundingSphere',[]],['validateInstanceId',[0]],['validateGeometryId',[0]],['setCustomSort',[null]],['_initMatricesTexture',[]],['_initIndirectTexture',[]],['_initColorsTexture',[]],['_initializeGeometry',[geo()]],['_validateGeometry',[geo()]],['getBoundingBoxAt',[0,New('Box3')]],['getBoundingSphereAt',[0,New('Sphere')]],['setMatrixAt',[0,matrix(3,-2,1)]],['getMatrixAt',[0,New('Matrix4')]],['setColorAt',[0,New('Vector4',[.2,.4,.6,.8])]],['getColorAt',[0,New('Color')]],['setVisibleAt',[0,false]],['getVisibleAt',[0]],['setGeometryIdAt',[0,0]],['getGeometryIdAt',[0]],['getGeometryRangeAt',[0]],['setInstanceCount',[6]],['setGeometrySize',[24,24]],['raycast',[ray(),A([])]],['addGeometry',[geo()]],['addInstance',[0]],['setGeometryAt',[0,geo({bounds:true})]],['deleteGeometry',[0]],['deleteInstance',[0]],['optimize',[]],['dispose',[]],['_updateDrawParameters',[camera,path('geometry'),path('material')]],['onBeforeRender',[O({isWebGPURenderer:false}),null,camera,path('geometry'),path('material')]],['onBeforeShadow',[O({isWebGPURenderer:false}),null,camera,camera,path('geometry'),path('material')]]]};
 // Calls are written as data; production code is never evaluated in the native fixture.
 for(const C of Object.keys(specs))for(const [method,args0]of specs[C]){const args=Array.isArray(args0)?args0:[];for(const observed of [false,true])add(C,{method,args,observed});}
 for(const getter of ['maxInstanceCount','instanceCount','unusedVertexCount','unusedIndexCount'])add('BatchedMesh',{steps:[{get:getter}]});
 for(const C of ['InstancedMesh','SkinnedMesh','BatchedMesh']){
  const source=C==='BatchedMesh'?New(C,[2,6,6]):C==='InstancedMesh'?New(C,[geo(),New('MeshBasicMaterial'),2]):New(C,[geo({skin:true}),New('MeshBasicMaterial')]);
  add(C,{refs:{source},method:'copy',args:[R('source'),false],inspect:['source']});
  for(const throwAt of [1,2,3,5,9,14])for(const method of ['computeBoundingBox','computeBoundingSphere'])add(C,{method,observed:true,throwAt});
  for(const gcAt of [1,2,4,8,12])add(C,{method:'computeBoundingSphere',observed:true,gcAt});
  for(const reenterAt of [1,3,6,10])add(C,{method:'computeBoundingBox',observed:true,reenterAt,reenter:{ref:'nested',method:'computeBoundingSphere'},refs:{nested:source}});
 }
 for(const id of [-1,0,1,2,9,'0',null,U,Z,N,S,{$:'convert',value:1,gc:true}]){
  for(const method of ['setMatrixAt','getMatrixAt','setColorAt','getColorAt'])for(const C of ['InstancedMesh','BatchedMesh'])add(C,{method,args:[id,method.includes('Color')?New('Color',[.1,.3,.5]):matrix(7,1,-2)],observed:true});
  for(const method of ['getVisibleAt','getGeometryIdAt','deleteInstance','validateInstanceId','validateGeometryId','deleteGeometry'])add('BatchedMesh',{method,args:[id],observed:true});
  for(const method of ['getVertexPosition','applyBoneTransform'])add('SkinnedMesh',{method,args:[id,New('Vector3',[1,2,3])],observed:true});
 }
 for(const relative of [false,true])for(const count of [0,1,3])add('InstancedMesh',{count,geometry:{relative},steps:[step('setMorphAt',[0,O({morphTargetInfluences:A([.3,.7])})]),step('getMorphAt',[0,O({morphTargetInfluences:A([0,0])},{id:'output'})])],morph:true});
 for(const weights of [[0,0,0,0,0,0,0,0,0,0,0,0],[2,-1,0,0,1,1,1,1,0,0,0,0],[NaN,1,0,0,0,Infinity,0,0,1,0,0,0]]){if(weights.some(x=>!Number.isFinite(x)))continue;add('SkinnedMesh',{geometry:{weights},steps:[step('normalizeSkinWeights'),step('getVertexPosition',[1,New('Vector3')]),step('computeBoundingBox'),step('computeBoundingSphere')]});}
 for(const bindMode of ['attached','detached','invalid',null,U])add('SkinnedMesh',{fields:[{key:'bindMode',value:bindMode}],method:'updateMatrixWorld',args:[true],observed:true});
 for(const index of [false,[0,1,2]])for(const reservedVertices of [-1,3,6])for(const reservedIndices of [-1,3,6])add('BatchedMesh',{geometries:[{index},{index}],instances:[0,1],reservedVertices,reservedIndices,steps:[step('deleteGeometry',[0]),step('optimize'),step('setGeometrySize',[30,30]),step('setInstanceCount',[6]),step('computeBoundingSphere')]});
 for(const mismatch of [{index:false},{attributes:{position:{items:[0,0,0,1,0,0,0,1,0],itemSize:3}}},{attributes:{position:{items:[0,0,1,1,2,2],itemSize:2},normal:{items:[0,0,1],itemSize:3},uv:{items:[0,0],itemSize:2}}}])add('BatchedMesh',{method:'addGeometry',args:[geo(mismatch)],observed:true});
 for(const sortObjects of [false,true])for(const perObjectFrustumCulled of [false,true])for(const transparent of [false,true])for(const wireframe of [false,true])add('BatchedMesh',{refs:{camera},fields:[{key:'sortObjects',value:sortObjects},{key:'perObjectFrustumCulled',value:perObjectFrustumCulled},{ref:'material',key:'transparent',value:transparent},{ref:'material',key:'wireframe',value:wireframe}],steps:[step('setMatrixAt',[1,matrix(0,0,-5)]),step('_updateDrawParameters',[R('camera'),path('geometry'),path('material')]),step('_updateDrawParameters',[R('camera'),path('geometry'),path('material')])]});
 for(const C of ['InstancedMesh','BatchedMesh'])for(const global of ['Math','Float32Array','Uint32Array'])add(C,{global,method:C==='BatchedMesh'?'setInstanceCount':'setColorAt',args:C==='BatchedMesh'?[7]:[0,New('Color',[.2,.3,.4])],observed:true});
 for(const C of ['InstancedMesh','BatchedMesh'])for(const throwListener of [false,true]){const q={refs:{resource:{$:'texture'}},fields:C==='InstancedMesh'?[{key:'morphTexture',value:R('resource')}]:[],listeners:[{throw:throwListener},{ref:'geometry'},{ref:'resource'}],steps:[step('dispose'),step('dispose')],inspect:['resource']};add(C,q);}
 for(const mode of [false,true])add('BatchedMesh',{refs:{camera,sort:{$:'function',name:'customSort',self:false,result:U,gc:true}},steps:[step('setCustomSort',[R('sort')]),step('onBeforeRender',[O({isWebGPURenderer:mode}),null,R('camera'),path('geometry'),path('material')]),step('onBeforeShadow',[O({isWebGPURenderer:mode}),null,R('camera'),R('camera'),path('geometry'),path('material')])]});
 for(const C of ['InstancedMesh','SkinnedMesh','BatchedMesh'])add(C,{kind:'metadata'});
 // Append-only extensions retain the original 498 requests and expectations.
 for(const color of [New('Color',[.1,.2,.3]),New('Vector4',[.1,.2,.3,.4])])add('BatchedMesh',{steps:[step('setColorAt',[1,color]),step('getColorAt',[1,New('Vector4')]),step('getColorAt',[1,New('Color')])]});
 add('BatchedMesh',{steps:[step('deleteInstance',[1]),step('deleteInstance',[0]),step('addInstance',[0]),step('addInstance',[0]),step('getGeometryRangeAt',[0])]});
 add('BatchedMesh',{geometries:[{},{}],instances:[0,1],steps:[step('deleteGeometry',[1]),step('deleteGeometry',[0]),step('addGeometry',[geo()]),step('addGeometry',[geo()]),step('addInstance',[0]),step('optimize')]});
 add('BatchedMesh',{refs:{source:New('BatchedMesh',[3,18,18])},steps:[step('addGeometry',[geo()],{ref:'source'}),step('addInstance',[0],{ref:'source'}),step('setColorAt',[0,New('Vector4',[.2,.3,.4,.5])],{ref:'source'}),step('computeBoundingBox',[],{ref:'source'}),step('computeBoundingSphere',[],{ref:'source'}),step('copy',[R('source')])],inspect:['source']});
 add('BatchedMesh',{method:'addGeometry',args:[geo({attributes:{position:{items:[0,0,0,2,0,0,0,2,0],itemSize:3,array:'Uint16Array'},normal:{items:[0,0,1,0,0,1,0,0,1],itemSize:3,array:'Uint16Array'},uv:{items:[0,0,1,0,0,1],itemSize:2,array:'Uint16Array'}}})]});
 for(const count of [0,1,2])add('BatchedMesh',{refs:{camera:{$:'camera',props:[{key:'isArrayCamera',value:true},{key:'cameras',value:A(Array.from({length:count},()=>camera))}]}},method:'_updateDrawParameters',args:[R('camera'),path('geometry'),path('material')]});
 for(const C of ['InstancedMesh','SkinnedMesh','BatchedMesh'])add(C,{method:'raycast',args:[ray(),A([])],inspect:[]});

 add('BatchedMesh',{refs:{src:geo(),attr:{$:'path',ref:'src',keys:['attributes','position']},attrsProto:O({}),attrs:{$:'object',proto:R('attrsProto'),props:[{key:'position',getter:true,value:R('attr'),mutations:[{op:'set',ref:'attrsProto',key:'late',value:R('attr')}]}]}},fields:[{key:'_geometryInitialized',value:false},{ref:'src',key:'attributes',value:R('attrs')}],method:'_initializeGeometry',args:[R('src')],inspect:['attrsProto']});
 return out;}
