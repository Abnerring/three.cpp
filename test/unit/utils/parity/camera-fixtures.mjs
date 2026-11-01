export const fixtures=[];
const add=(name,spec)=>fixtures.push({name,...spec});
const call=(method,...args)=>({method,args});
const ref=name=>({$ref:name});
const classes=['Camera','PerspectiveCamera','OrthographicCamera','ArrayCamera','CubeCamera','StereoCamera'];
for(const cls of classes){add(cls+'.constructor',{class:cls});add(cls+'.metadata',{class:cls,special:'metadata'});add(cls+'.privateModule',{class:cls,special:'module'});add(cls+'.newTarget',{class:cls,special:'newTarget'});}
for(const constructorArgs of [[75,1.7,.2,300],[{$undefined:true},2,{$undefined:true},50],[null,null,null,null],[0,0,0,0],['65','1.3','.2','300']])add('PerspectiveCamera.arguments.'+JSON.stringify(constructorArgs),{class:'PerspectiveCamera',constructorArgs});
for(const constructorArgs of [[-2,3,4,-5,0,100],[{$undefined:true},{$undefined:true},{$undefined:true},{$undefined:true},0,50],[null,null,null,null,null,null],['-2','3','4','-5','.1','100']])add('OrthographicCamera.arguments.'+JSON.stringify(constructorArgs),{class:'OrthographicCamera',constructorArgs});
add('CubeCamera.arguments.undefined',{class:'CubeCamera',constructorArgs:[{$undefined:true},{$undefined:true},ref('target')]});
add('Camera.reversedDepth',{class:'Camera',properties:{_reversedDepth:true},steps:[{get:'reversedDepth'}]});
add('Camera.reversedDepthReadonly',{class:'Camera',special:'readonly'});
for(const cls of ['Camera','PerspectiveCamera','OrthographicCamera']){
 add(cls+'.copy',{class:cls,steps:[call('copy',ref('source'),true)]});
 add(cls+'.copyNonrecursive',{class:cls,steps:[call('copy',ref('source'),false)]});
 add(cls+'.clone',{class:cls,properties:{_reversedDepth:true},steps:[call('clone')]});
 add(cls+'.copyObserved',{class:cls,observed:['matrixWorldInverse','projectionMatrix','projectionMatrixInverse','coordinateSystem','view','zoom','fov','near','far'],steps:[call('copy',ref('source'),false)]});
 add(cls+'.superHome',{class:cls,special:'superHome'});
 for(const scale of [[1,1,1],[2,3,4],[-2,3,4],[0,1,1]])for(const method of ['updateMatrixWorld','updateWorldMatrix'])add(cls+'.'+method+'.'+scale.join('_'),{class:cls,transform:{position:[2,-3,4],rotation:[.2,.4,-.1],scale},parent:true,steps:[call(method,true,true,true),call('getWorldDirection',ref('direction'))]});
}
for(const cls of ['PerspectiveCamera','OrthographicCamera']){
 for(const properties of [{zoom:0},{zoom:{$number:'NaN'}},{near:'0.2',far:'100'},{fov:null,aspect:0,left:'-2',right:'3'}])add(cls+'.coercions.'+JSON.stringify(properties),{class:cls,properties,steps:[call('updateProjectionMatrix')]});
 for(const coordinateSystem of [2000,2001,99])for(const reversed of [false,true])for(const zoom of [1,2.5])add(cls+'.projection.'+coordinateSystem+'.'+reversed+'.'+zoom,{class:cls,properties:{coordinateSystem,_reversedDepth:reversed,zoom},steps:[call('updateProjectionMatrix')]});
 for(const nearFar of [[0,10],[1,1],[-2,20],[.1,{$number:'Infinity'}]])add(cls+'.range.'+JSON.stringify(nearFar),{class:cls,properties:{near:nearFar[0],far:nearFar[1]},steps:[call('updateProjectionMatrix')]});
 add(cls+'.viewOffset',{class:cls,properties:{zoom:1.6},steps:[call('setViewOffset',1920,1080,200,150,640,360),call('toJSON')]});
 add(cls+'.clearViewOffset',{class:cls,steps:[call('setViewOffset',1800,1000,200,100,600,500),call('clearViewOffset'),call('toJSON')]});
 add(cls+'.clearNullView',{class:cls,steps:[call('clearViewOffset')]});
 add(cls+'.projectionObserved',{class:cls,properties:{filmOffset:3.5},observed:['near','far','fov','zoom','aspect','filmOffset','filmGauge','view','left','right','top','bottom','coordinateSystem','reversedDepth','projectionMatrix','projectionMatrixInverse'],steps:[call('updateProjectionMatrix')]});
 add(cls+'.projectionThrows',{class:cls,observed:['projectionMatrix','projectionMatrixInverse'],throwAt:'get:projectionMatrixInverse',steps:[call('updateProjectionMatrix')]});
 add(cls+'.JSON',{class:cls,steps:[call('toJSON')]});
}
for(const aspect of [.4,1,2])add('PerspectiveCamera.film.'+aspect,{class:'PerspectiveCamera',properties:{aspect,filmGauge:70,zoom:1.7},steps:[call('getFilmWidth'),call('getFilmHeight'),call('getFocalLength'),call('getEffectiveFOV'),call('setFocalLength',35),call('getFocalLength')]});
for(const distance of [0,1,7,-3])add('PerspectiveCamera.bounds.'+distance,{class:'PerspectiveCamera',properties:{aspect:1.7,filmOffset:1},steps:[call('updateProjectionMatrix'),call('getViewBounds',distance,ref('min'),ref('max')),call('getViewSize',distance,ref('size'))]});
add('PerspectiveCamera.boundsView',{class:'PerspectiveCamera',steps:[call('setViewOffset',1000,800,200,100,400,300),call('getViewBounds',4,ref('min'),ref('max')),call('getViewSize',4,ref('size'))]});
add('PerspectiveCamera.boundsReentry',{class:'PerspectiveCamera',special:'boundsReentry'});
add('PerspectiveCamera.dynamicMath',{class:'PerspectiveCamera',special:'dynamicMath'});
add('ArrayCamera.sharedArray',{class:'ArrayCamera',special:'arrayIdentity'});
for(const coordinateSystem of [2000,2001,99])add('CubeCamera.coordinate.'+coordinateSystem,{class:'CubeCamera',properties:{coordinateSystem},steps:[call('updateCoordinateSystem')]});
for(const coordinateSystem of [2000,2001])for(const webgl of [false,true])for(const reversed of [false,true])add('CubeCamera.update.'+coordinateSystem+'.'+webgl+'.'+reversed,{class:'CubeCamera',renderer:{coordinateSystem,webgl,reversed,autoClear:false},steps:[call('update',ref('renderer'),ref('scene'))]});
add('CubeCamera.updateAutoClear',{class:'CubeCamera',renderer:{coordinateSystem:2000,reversed:true,autoClear:true},steps:[call('update',ref('renderer'),ref('scene'))]});
add('CubeCamera.renderThrows',{class:'CubeCamera',renderer:{coordinateSystem:2001},throwAt:'render:2',steps:[call('update',ref('renderer'),ref('scene'))]});
add('CubeCamera.targetThrows',{class:'CubeCamera',renderer:{coordinateSystem:2000},throwAt:'setRenderTarget:4',steps:[call('update',ref('renderer'),ref('scene'))]});
add('CubeCamera.reentry',{class:'CubeCamera',renderer:{coordinateSystem:2001,reversed:true,autoClear:false},reenterAt:'render:1',steps:[call('update',ref('renderer'),ref('scene'))]});
add('CubeCamera.invalidRendererCoordinate',{class:'CubeCamera',renderer:{coordinateSystem:73},steps:[call('update',ref('renderer'),ref('scene'))]});
for(const fault of ['next','bindingReturn','body','bodyReturn'])add('CubeCamera.iterator.'+fault,{class:'CubeCamera',special:'iterator',fault});
for(const props of [{},{aspect:.5,eyeSep:.12},{eyeSep:0},{eyeSep:-.1}])add('StereoCamera.update.'+JSON.stringify(props),{class:'StereoCamera',properties:props,steps:[call('update',ref('source')),call('update',ref('source'))]});
add('StereoCamera.reentry',{class:'StereoCamera',special:'stereoReentry'});
for(const method of ['makePerspective','makeOrthographic'])for(const system of [2000,2001,33])for(const reversed of [false,true])add('Matrix4.'+method+'.'+system+'.'+reversed,{class:'Camera',special:'mathMethod',mathClass:'Matrix4',method,args:[-2,3,4,-1,.2,100,system,reversed]});
add('Vector2.subVectors',{class:'Camera',special:'mathMethod',mathClass:'Vector2',method:'subVectors',args:[{$vector2:[5,-2]},{$vector2:[1,3]}]});
for(const method of ['tan','atan'])for(const input of [0,{$number:'-0'},.3,-2,1e20,{$number:'Infinity'},{$number:'NaN'},null,'0.5'])add('Math.'+method+'.'+JSON.stringify(input),{class:'Camera',special:'mathMethod',mathClass:'Math',method,args:[input]});
