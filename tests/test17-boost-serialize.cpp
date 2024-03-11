#include <iostream>
#include <fstream>
#include <unordered_map>
#include <map>
#include <string>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

int main() {
    std::unordered_map<std::string, std::pair<std::string, std::string>> myMap;
    
    // Populate the map with some data
    myMap["key1"] = std::make_pair("data1", "data2");
    myMap["key2"] = std::make_pair("data3", "data4");

    // Serialize the map
    {
        std::ofstream ofs("data.txt");
        boost::archive::text_oarchive oa(ofs);
        oa << myMap;
    }

    // Deserialize the map
    std::unordered_map<std::string, std::pair<std::string, std::string>> loadedMap;
    {
        std::ifstream ifs("data.txt");
        boost::archive::text_iarchive ia(ifs);
        ia >> loadedMap;
    }

    // Display the deserialized map
    for (const auto& pair : loadedMap) {
        std::cout << pair.first  << " => "
                  << pair.second.first << ", " << pair.second.second << std::endl;
    }

    return 0;
}