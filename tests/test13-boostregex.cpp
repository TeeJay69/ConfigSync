#include <iostream>
#include <string>
#include <boost\regex.hpp>

int main(){
    std::string str = "C:\\Users\\gustav\\AppData\\qbittorrent";
    const boost::regex pattern("C:\\\\(Users)\\\\(.+?)\\\\");
    auto matches = boost::smatch{}; // Holds our capture groups

    if(boost::regex_search(str, matches, pattern)){
        std::cout << "1st cap group: " << matches[0] << std::endl;
        std::cout << "2nd cap group: " << matches[1] << std::endl;
        std::cout << "3rd cap group: " << matches[2] << std::endl;
    }
    else{
        std::cout << "regex failed" << std::endl;
    }

    
    /* Replace capturing group 1 */
    
    std::string stringToSearch = str; //((std::string)matches[1]); 
    boost::regex regex("(?<=C:\\\\Users\\\\)(.+?)\\\\"); // Part to replace 
    std::string newtext = "JAMES\\";

    std::string result = boost::regex_replace(stringToSearch, regex, newtext); // Replace


    std::cout << "Result: " << result << std::endl;
    std::cout << "stringToSearch: " << stringToSearch << std::endl;

    return 0;
}