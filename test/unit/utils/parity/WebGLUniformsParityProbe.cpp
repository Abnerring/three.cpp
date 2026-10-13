#include "WebGLUniformsFixture.h"
#include <iostream>
using namespace UniformsFixture;
void emit(JsonValue value){std::cout<<value.dump()<<'\n';JSValue::object().collectGarbage();}
int main(){try{
 for(unsigned type=0;type<types().size();++type)for(unsigned pure=0;pure<2;++pure)for(unsigned mode=0;mode<18;++mode)if(finiteCase(type,pure,mode))emit(typeCase(type,pure,mode));
 for(unsigned variant=0;variant<paths().size();++variant)emit(pathCase(variant));
 for(unsigned mode=0;mode<5;++mode)emit(structureCase(mode));
 for(unsigned mode=0;mode<15;++mode)emit(uploadCase(mode));
 for(unsigned mode=0;mode<14;++mode)emit(observedCase(mode));
 for(unsigned mode=0;mode<6;++mode)emit(optionalCase(mode));
 for(unsigned type=0;type<types().size();++type)for(unsigned pure=0;pure<2;++pure)emit(metadataCase(type,pure));
 for(unsigned type=19;type<types().size();++type)for(unsigned mode=0;mode<3;++mode)emit(textureSequenceCase(type,mode));
 emit(shadowOrderCase());
 for(unsigned type=0;type<6;++type)for(unsigned mode=0;mode<10;++mode)emit(flattenCase(type,mode));
 for(unsigned dimension=2;dimension<5;++dimension)emit(matrixScratchCase(dimension));
 for(unsigned mode=0;mode<5;++mode)emit(constructCase(mode));
 for(unsigned mode=0;mode<6;++mode)emit(regexReentryCase(mode));
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
