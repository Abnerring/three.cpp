#include "BSDFLookupFixtures.h"
#include <iostream>
int main(){try{std::cout<<BSDFLookupFixtures::all(false).toJSON()->dump()<<'\n';return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
