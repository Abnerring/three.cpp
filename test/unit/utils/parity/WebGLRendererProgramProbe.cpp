#include "WebGLRendererProgramFixtures.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <iterator>
int main(int argc,char** argv){try{const auto requests=Animation_detail::parseJSON(argc>1?std::string(argv[1]):std::string(std::istreambuf_iterator<char>(std::cin),{}));auto out=JSValue::array();for(double i=0;i<requests.length();++i)WebGLRendererProgramFixtures::push(out,WebGLRendererProgramFixtures::run(requests.get(JSValue(i))));std::cout<<out.toJSON()->dump()<<'\n';return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
