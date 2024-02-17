#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <cstdlib>

bool compareIgnoreCase(const std::string& a, const std::string& b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
    [](char a, char b) { return std::tolower(a) < std::tolower(b); });
}

int main(){
    
    std::filesystem::path pathsss = "C:\\Data\\TW\\Software\\Coding\\ConfigSync";
    std::string pathss = pathsss.generic_string();
    std::vector<std::string> v;

    std::string xxx;
    
    for(const auto &entry : std::filesystem::directory_iterator(pathss)){
        // if(entry.is_directory()){
        v.push_back(entry.path().filename().generic_string());
        // }
    }

    std::sort(v.begin(), v.end(), compareIgnoreCase);
    int x = 0;
    for (const auto& i : v){
        std::cout << x << "   " << i << std::endl;
        x++;
    }
    std::cout << "0: " << v[0] << std::endl << "1: " << v[1] << std::endl << "2: " << v[2] << std::endl << "3: " << v[3] << std::endl;
    return 0;
}