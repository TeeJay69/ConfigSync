#ifndef CFGSINDEX_HPP
#define CFGSINDEX_HPP

#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <map>
#include <algorithm>


struct Index {
    std::vector<std::pair<unsigned long long, std::string>> time_uuid;
};

#endif