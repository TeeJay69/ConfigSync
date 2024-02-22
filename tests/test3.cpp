#include <iostream>
#include "C:\Data\Tw\Software\Coding\ConfigSync\analyzer.hpp"
#include "C:\Data\Tw\Software\Coding\ConfigSync\programs.hpp"
#include "C:\Data\Tw\Software\Coding\ConfigSync\synchronizer.hpp"
#include "C:\Data\Tw\Software\Coding\ConfigSync\database.hpp"

int main(){
    std::map<std::string, std::string> mapX;
    mapX["Key 1..."] = "Value...1";
    mapX["Key 2..."] = "Value...2";
    mapX["Key 3..."] = "Value...3";

    database data("map.bin");
    data.storeStringMap(mapX);
    
    database data2("map.bin");
    std::map<std::string, std::string> mapY;
    data2.readStringMap(mapY);

    for(const auto& [key, val] : mapY){
        std::cout << "Key: " << key << "     " << "Value: " << val << std::endl;
    }
    std::cout << mapY["Key 1..."] << std::endl;
    std::cout << mapY["Key 2..."] << std::endl;
    std::cout << mapY["Key 3..."] << std::endl;
    std::cout << mapY["Key 4..."] << std::endl;
    return 0;
}