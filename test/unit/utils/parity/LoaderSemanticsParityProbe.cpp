#include "LoaderSemanticsFixtures.h"
#include "animation/AnimationUtils.h"
#include <iostream>
int main(){
    const std::string input(std::istreambuf_iterator<char>(std::cin),{});
    const auto requests=Animation_detail::parseJSON(input);auto output=JSValue::array();
    for(unsigned i=0;i<requests.length();++i){
        try{LoaderSemanticsFixtures::push(output,LoaderSemanticsFixtures::run(requests.get(std::to_string(i))));}
        catch(...){auto error=LoaderSemanticsFixtures::object();error.set("unexpected",nativeExceptionValue(std::current_exception()));LoaderSemanticsFixtures::push(output,error);}
    }
    std::cout<<output.toJSON()->dump()<<'\n';
}
