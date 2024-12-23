#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>
#include <ctime>

// Custom comparator function to compare two file paths based on their filenames
bool compareFilenames(const std::string& path1, const std::string& path2) {
    // Extract filenames from the paths
    std::string filename1 = std::filesystem::path(path1).filename().generic_string();
    std::string filename2 = std::filesystem::path(path2).filename().generic_string();
    
    // Compare filenames
    return filename1 < filename2;
}

int main() {
    // Sample vector of file paths
    std::vector<std::string> paths = {"/aaaaa/aaaa/file3.txt", "/bbbbbbbb/bbbb/file1.txt", "/bbbbbbbb/bbbb/File1.txt", "/path/to/file2.txt"};

    // Sort the vector based on filenames using the custom comparator function
    std::sort(paths.begin(), paths.end(), compareFilenames);

    // Output the sorted vector
    std::cout << "Sorted vector based on filenames:\n";
    for (const auto& path : paths) {
        std::cout << path << "\n";
    }

    char out[11];
    std::time_t t=std::time(NULL);
    std::strftime(out, sizeof(out), "%Y-%m-%d", std::localtime(&t));
    std::cout<<out;

    return 0;
}