#include "NativeClippingMathFixture.h"
#include <iostream>
using namespace ClippingMathFixture;
int main(){try{A results;for(unsigned mode=0;mode<4;++mode)results.push_back(initialization(mode));for(unsigned seed=0;seed<64;++seed)for(bool native:{false,true})results.push_back(numeric(seed,native));for(unsigned kind=0;kind<10;++kind)for(unsigned mode=0;mode<5;++mode)results.push_back(observed(kind,mode));for(unsigned mode=0;mode<7;++mode)results.push_back(chaining(mode));for(unsigned mode=0;mode<2;++mode)results.push_back(reentry(mode));std::cout<<JsonValue(results).dump();}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
