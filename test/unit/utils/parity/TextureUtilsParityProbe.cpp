#include "TextureUtilsFixture.h"
using namespace TextureUtilsTest;
namespace {
JsonValue scenario(const JSValue& spec){
 auto trace=JSValue::array();JsonValue result;
 try{
  const auto args=spec.get("args");Args values;for(unsigned i=0;i<args.length();++i)values.push_back(decode(args.get(std::to_string(i)),trace));
  if(spec.get("mode").strictEquals("type")){
   const auto a=nativeTextureGetTextureTypeByteLength(argument(values,0));const auto b=nativeTextureGetTextureTypeByteLength(argument(values,0));
   A names,descriptors;for(const auto& name:a.ownPropertyNames()){names.emplace_back(name);const auto d=a.getOwnPropertyDescriptor(name);descriptors.push_back(A{d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)});}
   result=O{{"value",encode(a)},{"own",names},{"descriptors",descriptors},{"fresh",!a.sameValue(b)},{"ordinaryPrototype",a.getPrototype().sameValue(JSValue::object().getPrototype())}};
  }else result=encode(nativeTextureGetByteLength(argument(values,0),argument(values,1),argument(values,2),argument(values,3)));
 }catch(const JSTypeError&){result=O{{"error","TypeError"}};}catch(const JSRangeError&){result=O{{"error","RangeError"}};}catch(const std::exception& e){result=O{{"error","Error"},{"message",e.what()}};}
 return O{{"name",spec.get("name").toString()},{"result",result},{"trace",encode(trace)}};
}
}
int main(int argc,char** argv){if(argc!=2)return 2;try{std::ifstream input(argv[1],std::ios::binary);std::string line;while(std::getline(input,line)){{const auto spec=Animation_detail::parseJSON(line);std::cout<<scenario(spec).dump()<<'\n';}JSValue::nativeSafepoint();}return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
