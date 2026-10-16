#include "WebGLStateFixture.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
    if(argc!=2)return 2;
    try{
        std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("Scenario file missing");
        std::string line;while(std::getline(input,line)){
            {const auto spec=Animation_detail::parseJSON(line);std::cout<<StateFixture::scenario(spec).dump()<<'\n';}
            JSValue::object().collectGarbage();JSValue::nativeSafepoint();
        }
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
