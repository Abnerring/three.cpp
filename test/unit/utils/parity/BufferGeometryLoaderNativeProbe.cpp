#include "BufferGeometryLoaderNativeFixtures.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <iterator>
int main(int argc,char** argv){
    try{
        const auto request=Animation_detail::parseJSON(argc>1?std::string(argv[1]):std::string(std::istreambuf_iterator<char>(std::cin),{}));
        auto output=JSValue::array();
        if(request.type()==JSValue::Type::Array){for(double i=0;i<request.length();++i)ObjectLoaderNativeFixtures::push(output,BufferGeometryLoaderNativeFixtures::run(request.get(JSValue(i))));}
        else output=BufferGeometryLoaderNativeFixtures::run(request);
        std::cout<<output.toJSON()->dump()<<'\n';return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
