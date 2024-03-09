#include <iostream>
#include <thread>
#include <chrono>
#include <math.h>

int main(){
    std::cout << "Warning: any changes to the json file could make the file become unreadable, please know what you are doing!" << std::endl; // Warning

    int counter = 2;
    while(counter != 0){
        if(counter == 1){
            std::cout << "\rProceeding in: " << counter << " second" << std::endl;
        }
        else{
            std::cout << "\rProceeding in: " << counter << " seconds." << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        counter--;
    }

    long double x = 1232423432432423423243243248999.0;
    long double y = 908.0;

    long double r = std::log(x);
    long double c = std::pow(991,23);
    std::cout << "Result: " << r << std::endl;
    std::cout << "Result: " << c << std::endl;
}