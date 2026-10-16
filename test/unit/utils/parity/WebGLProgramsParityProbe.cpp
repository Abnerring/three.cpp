#include "WebGLProgramsFixture.h"
#include <iostream>
int main(){
    std::size_t index=0;
    try{
        auto realm=JSValue::object();std::string line;
        while(std::getline(std::cin,line)){
            if(line.empty())continue;
            auto spec=Animation_detail::parseJSON(line);
            std::cout<<ProgramsFixture::fixture(spec).toJSON()->dump()<<'\n';spec={};realm.collectGarbage();++index;
        }
    }catch(const std::exception& error){std::cerr<<"case "<<index<<": "<<error.what()<<'\n';return 1;}
}
