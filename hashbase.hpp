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
    //  // Iterator typedef to define iterator type for hashbase
    // using iterator = std::tuple<std::vector<std::string>::iterator,
    //                             std::vector<std::string>::iterator,
    //                             std::vector<std::string>::iterator,
    //                             std::vector<std::string>::iterator>;

    // // Function to return begin iterator of hashbase
    // iterator begin() {
    //     return std::make_tuple(ha.begin(), hb.begin(), pa.begin(), pb.begin());
    // }

    // // Function to return end iterator of hashbase
    // iterator end() {
    //     return std::make_tuple(ha.end(), hb.end(), pa.end(), pb.end());
    // }

    void push_all(const std::string& str){
        ha.push_back(str);
        hb.push_back(str);
        pa.push_back(str);
        pb.push_back(str);
    }

    std::ranges::zip_view<std::ranges::ref_view<std::vector<std::string>>, std::ranges::ref_view<std::vector<std::string>>, std::ranges::ref_view<std::vector<std::string>>, std::ranges::ref_view<std::vector<std::string>>> zip_view(){ // or auto
        return std::views::zip(ha, hb, pa, pb);
    }
};

#endif