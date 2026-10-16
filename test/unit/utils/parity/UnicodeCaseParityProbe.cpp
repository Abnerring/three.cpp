#include "core/UnicodeCase.h"
#include <iostream>
#include <sstream>
#include <string>
namespace {
void emit(const std::u16string& text) {
    bool first=true;
    for (auto unit:text) {if (!first) std::cout << ',';first=false;std::cout << unsigned(unit);}
    std::cout << '\n';
}
}
int main(int argc, char**) {
    std::cout << std::hex;
    if (argc>1) {
        for (unsigned p=0;p<=0x10ffff;++p) {
            std::u16string value;
            if(p<=0xffff)value.push_back(static_cast<char16_t>(p));
            else{const auto n=p-0x10000;value.push_back(static_cast<char16_t>(0xd800+(n>>10)));value.push_back(static_cast<char16_t>(0xdc00+(n&1023)));}
            emit(unicodeToUpperCase(value));
        }
    } else {
        std::string line;
        while(std::getline(std::cin,line)) {
            std::istringstream input(line);input >> std::hex;
            std::u16string value;unsigned unit;
            while(input>>unit) {if(unit>0xffff)return 2;value.push_back(static_cast<char16_t>(unit));}
            emit(unicodeToUpperCase(value));
        }
    }
}
