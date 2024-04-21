#include <iostream>
#include <string>
#include <cstring>

int argfcmp(std::string& buff, char** argv, int argc, const char* cmp[], size_t cmp_size){
    for(int i = 0; i < cmp_size; i++){
        for(int ii = 0; ii < argc; ii++){
            if(strcmp(argv[ii], cmp[i]) == 0){
                if(ii + 1 < argc){
                    buff = argv[ii + 1];
                    return 1;
                }
            }
        }
    }
    return 0;
}

int main(int argc, char* argv[]){
    if(argv[1] == NULL){
        std::cout << "Fatal: Missing argument or value." << std::endl;
    }
    else if(std::string(argv[1]) == "test"){
        const char* cmp[] = {"--date", "-d"};
        const size_t cmpSize = sizeof(cmp) / sizeof(cmp[0]);
        std::string buff;
        if(argfcmp(buff, argv, argc, cmp, cmpSize) == 1){
            std::cout << "buff: " << buff << std::endl;
        }
    }
}