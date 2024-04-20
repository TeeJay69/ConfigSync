#include <iostream>
#include <string>

int main(){
    std::string str = "C:\\Users\\TJ\\Software\\Coding";
    std::cout << "str: " << str << std::endl;
    const std::string pat = "Users\\";
    const size_t pos = str.find(pat);
    const size_t posx = pos + pat.length();
    const size_t posend = str.find_first_of("\\", posx);
    str.replace(posx, posend - posx, "TestName");
    std::cout << "Pattern: " << pat << std::endl;
    std::cout << "pos start pat: " << pos << std::endl;
    std::cout << "pos end pat: " << posx << std::endl;
    std::cout << "unknown match end pos: " << posend << std::endl;
    std::cout << "str after replace: " << str << std::endl;
    
    return 0;
}