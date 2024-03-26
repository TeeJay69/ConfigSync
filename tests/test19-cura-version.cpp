#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>   
#include <cstdlib>
#include <map>
#include <chrono>
#include <thread>
#include <set>
#include <cctype>
#include <csignal>
#include <windows.h>
#include <thread>
#include <TlHelp32.h>
#include <ranges>
#include <boost\uuid\uuid.hpp>
#include <boost\uuid\uuid_generators.hpp>
#include <boost\filesystem\operations.hpp>
#include <boost\filesystem\path.hpp>
#include <boost\property_tree\ptree.hpp>
#include <boost\property_tree\json_parser.hpp>
#include <boost/dll.hpp>
#include "..\synchronizer.hpp"

int main(){
    std::cout << ProgramConfig::Get_Cura_Version("TJ") << std::endl;

    return 0;
}