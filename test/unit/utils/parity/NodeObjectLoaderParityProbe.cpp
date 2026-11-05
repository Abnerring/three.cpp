#include "NodeObjectLoaderFixtures.h"
#include <iostream>
int main(){const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);auto out=JSValue::array();for(unsigned i=0;i<requests.length();++i){try{NodeObjectLoaderFixtures::push(out,NodeObjectLoaderFixtures::run(requests.get(i)));}catch(...){std::cerr<<"NodeObjectLoader scenario "<<i<<": "<<nativeExceptionValue(std::current_exception()).toString()<<'\n';return 2;}}std::cout<<out.toJSON()->dump()<<'\n';}
