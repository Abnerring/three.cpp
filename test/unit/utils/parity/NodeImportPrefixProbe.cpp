#include "materials/nodes/NodeMaterial.h"
#include "materials/Material.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"
#include "math/Color.h"
#include "math/Vector3.h"
#include "math/Plane.h"
#include "textures/Texture.h"
#include "textures/CubeTexture.h"
#include "core/JSGlobal.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <string>
#include <stdexcept>
int main(int argc,char**argv){try{if(argc!=2)throw std::logic_error("prefix count required");const auto prefix=std::stoi(argv[1]);if(prefix<0||prefix>14)throw std::logic_error("prefix must be 0..14");auto state=JSValue::object();state.set("calls",0);auto math=nativeGlobalBinding("Math");math.set("random",JSValue::function(+[](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& cap)->JSValue{auto state=cap[0];const auto calls=state.get("calls").number()+1;state.set("calls",calls);return std::fmod(calls,257.)/257.;},{state},"random",0));
 const char* modules[]={"materials/nodes/NodeMaterial","materials/Material","materials/MeshBasicMaterial","objects/Mesh","core/BufferGeometry","core/BufferAttribute","nodes/core/NodeBuilder","nodes/tsl/TSLBase","math/Color","math/Vector3","math/Plane","textures/Texture","textures/CubeTexture","utils"};
 for(int index=0;index<prefix;++index)switch(index){case 0:nativeNodeMaterialConstructor();break;case 1:nativeMaterialConstructor();break;case 2:nativeMeshBasicMaterialConstructor();break;case 3:nativeMeshConstructor();break;case 4:nativeBufferGeometryConstructor();break;case 5:nativeFloat32BufferAttributeConstructor();break;case 6:nativeNodeBuilderConstructor();break;case 7:nativeTSLBase();break;case 8:nativeColorConstructor();break;case 9:nativeVector3Constructor();break;case 10:nativePlaneConstructor();break;case 11:nativeTextureConstructor();break;case 12:nativeCubeTextureConstructor();break;case 13:ThreeUtils::nativeWarnFunction();break;}
 const auto node=nativeNodeConstructor().construct();auto out=JSValue::object();out.set("prefix",prefix);out.set("lastModule",prefix?JSValue(modules[prefix-1]):JSValue(nullptr));out.set("nextNodeId",node.get("id"));out.set("randomCalls",state.get("calls"));std::cout<<out.toJSON()->dump();
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
