#include <iostream>
#include <string>
#include "..\ANSIcolor.hpp"

int main(){

    int i = 0;

    while(i != 256){
        std::cout << "std::cout << ANSI_COLOR_" << i << " << \"Color: " << i << "\" << ANSI_COLOR_RESET << std::endl;" << std::endl;

        i++;
    }

    return 0;
}