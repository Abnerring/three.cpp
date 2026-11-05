#include "WebGLTexturesFixtures.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <iterator>
int main(int argc,char** argv){try{const auto input=Animation_detail::parseJSON(argc>1?std::string(argv[1]):std::string(std::istreambuf_iterator<char>(std::cin),{})),requests=input.get("requests");auto out=JSValue::array();for(double i=0;i<requests.length();++i)WebGLTexturesFixtures::push(out,WebGLTexturesFixtures::run(requests.get(JSValue(i)),input.get("enums")));std::cout<<out.toJSON()->dump()<<'\n';return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
