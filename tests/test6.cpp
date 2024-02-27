#include <iostream>

int main(){
    std::cout << "short: " << sizeof(short) << '\n'; 
    std::cout << "unsigned short: " << sizeof(unsigned short) << '\n'; 
    std::cout << "int: " << sizeof(int) << '\n'; 
    std::cout << "unsigned int: " << sizeof(unsigned int) << '\n'; 
    std::cout << "long: " << sizeof(long) << '\n'; 
    std::cout << "long long: " << sizeof(long long) << '\n'; 
    std::cout << "float: " << sizeof(float) << '\n'; 
    std::cout << "double: " << sizeof(double) << '\n'; 
    std::cout << "long double: " << sizeof(long double) << '\n'; 
    
    return 0;
}