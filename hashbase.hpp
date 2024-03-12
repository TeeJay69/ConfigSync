#ifndef HASHBASE_HPP
#define HASHBASE_HPP
#include <iostream>
#include <string>
#include <vector>
#include <ranges>

struct hashbase{
    std::vector<std::pair<std::string, std::string>> hh;
    std::vector<std::pair<std::string, std::string>> pp;

    std::ranges::zip_view<std::ranges::ref_view<std::vector<std::pair<std::string, std::string>>>, std::ranges::ref_view<std::vector<std::pair<std::string, std::string>>>> zipview(){ // or auto
        return std::views::zip(hh, pp);
    }
};

#endif