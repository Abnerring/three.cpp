#include "PhysicalLightingModelScenarios.h"
#include <iostream>
int main(){try{std::cout<<PhysicalLightingScenarios::all().toJSON()->dump()<<'\n';return 0;}catch(const JSThrownValue& error){std::cerr<<"thrown: "<<error.value.toJSON()->dump()<<'\n';return 2;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
