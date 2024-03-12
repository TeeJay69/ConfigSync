#ifndef HASHBASE_HPP
#define HASHBASE_HPP
#include <iostream>
#include <string>
#include <vector>
#include <ranges>

struct hashbase{
    std::vector<std::string> ha;
    std::vector<std::string> hb;
    std::vector<std::string> pa;
    std::vector<std::string> pb;
    std::vector<std::pair<std::string, std::string>> hh;
    std::vector<std::pair<std::string, std::string>> pp;

    void push_all(const std::string& str){
        ha.push_back(str);
        hb.push_back(str);
        pa.push_back(str);
        pb.push_back(str);
    }

    std::ranges::zip_view<std::ranges::ref_view<std::vector<std::string>>, std::ranges::ref_view<std::vector<std::string>>, std::ranges::ref_view<std::vector<std::string>>, std::ranges::ref_view<std::vector<std::string>>> zipview(){ // or auto
        return std::views::zip(ha, hb, pa, pb);
    }
};

#endif