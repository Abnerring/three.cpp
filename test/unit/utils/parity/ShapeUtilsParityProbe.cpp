#include "ShapeUtilsFixtures.h"
#include "animation/AnimationUtils.h"
#include <iostream>
int main(){const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto cases=Animation_detail::parseJSON(input);auto results=JSValue::array();for(unsigned i=0;i<cases.length();++i){try{ShapeUtilsFixtures::push(results,ShapeUtilsFixtures::run(cases.get(i)));}catch(...){std::cerr<<"ShapeUtils scenario "<<i<<": "<<nativeExceptionValue(std::current_exception()).toString()<<'\n';return 2;}}std::cout<<results.toJSON()->dump()<<'\n';}
