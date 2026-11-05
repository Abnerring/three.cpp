#include "ObjectLoaderNativeFixtures.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <iterator>
int main(int argc,char** argv) {
    try {
        const std::string input=argc>1?argv[1]:std::string(std::istreambuf_iterator<char>(std::cin),{});
        const auto requests=Animation_detail::parseJSON(input);auto output=JSValue::array();
        if(requests.type()==JSValue::Type::Array) {
            for(double i=0;i<requests.length();++i)ObjectLoaderNativeFixtures::push(output,ObjectLoaderNativeFixtures::run(requests.get(JSValue(i))));
            std::cout<<output.toJSON()->dump()<<'\n';
        } else std::cout<<ObjectLoaderNativeFixtures::run(requests).toJSON()->dump()<<'\n';
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
