#include <iostream>
#include <string>

static int replaceSubstr(std::string& str, const std::string& substr, const std::string& __rep){
    size_t pos = str.find(substr);
    if(pos == std::string::npos){
        return 0;
    }
    str.replace(pos, substr.length(), __rep);
    return 1;
}

int main(){
    std::string str = "C:\\Users\\TJ\\Software\\Coding";
    const size_t pos = str.find(":");
    if(pos == std::string::npos){
        return 1;
    }
    
    std::string substr = str.substr(pos - 1, 2); // == std::string substr = str.substr(pos - 1, (pos + 2) - pos);
    std::string rep = "G:";
    std::cout << "str: " << str << std::endl;
    std::cout << "pos: " << pos << std::endl;
    std::cout << "substr: " << substr << std::endl;
    std::cout << "rep: " << rep << std::endl;
    replaceSubstr(str, substr, rep);
    std::cout << "After replacement: " << str << std::endl;

    return 0;
}   