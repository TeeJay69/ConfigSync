#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "database.hpp"
#include "hashbase.hpp"

int main() {
    hashbase HB;

    HB.ha.push_back("HashA1");
    HB.ha.push_back("HashA2");
    HB.hb.push_back("HashB2");
    HB.hb.push_back("HashB2");
    HB.pa.push_back("pathA1");
    HB.pa.push_back("pathA2");
    HB.pb.push_back("pathB1");
    HB.pb.push_back("pathB2");

    // Storing and reading hashbase data
    std::ofstream of("hashbase.bin");
    database::storeHashbase("hashbase.bin", HB);
    HB.ha.clear();
    HB.hb.clear();
    HB.pa.clear();
    HB.pb.clear();
    database::readHashbase("hashbase.bin", HB);

    // Iterate over the vectors
    for (int i = 0; i < HB.ha.size(); ++i) {
        const auto& ha = HB.ha[i];
        const auto& hb = HB.hb[i];
        const auto& pa = HB.pa[i];
        const auto& pb = HB.pb[i];

        // Use ha, hb, pa, pb as needed
        std::cout << "ha: " << ha << ", hb: " << hb << ", pa: " << pa << ", pb: " << pb << std::endl;
    }

    return 0;
}
