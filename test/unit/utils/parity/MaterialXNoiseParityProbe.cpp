#include "MaterialXNativeFixtures.h"
#include <iostream>
int main(int argc,char** argv){try{const std::string input=argc==2?argv[1]:std::string((std::istreambuf_iterator<char>(std::cin)),{});std::cout<<MaterialXNativeFixtures::run(Animation_detail::parseJSON(input)).toJSON()->dump();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
