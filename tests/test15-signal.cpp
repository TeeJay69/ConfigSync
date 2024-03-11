#include <iostream>
#include <csignal>
#include <unistd.h>
#include <math.h>

volatile sig_atomic_t interrupted = 0;

void signalHandler(int signum) {
    static int i = 1;
    if(interrupted != 1){
        interrupted = 1;
        std::cout << "Caught Ctrl C for first time\n";
        std::signal(SIGINT, signalHandler);
    }
    else{
        std::cout << "Caught Ctrl C " << i << std::endl;
        i++;
        std::signal(SIGINT, signalHandler);
    }
}

int main() {
       
    std::signal(SIGINT, signalHandler);


    std::cout << "Calculating...\n";

    unsigned int result = 0;
    for (unsigned int i = 0; i < 100; ++i) {
        result = i * 81271; // Some mathematical calculation
        // Add a sleep to simulate a long-running loop
        usleep(1000);
    }

    if (interrupted) {
        std::cout << "Interrupted! Partial result: " << result << "\n";
    } else {
        std::cout << "Calculation completed. Result: " << result << "\n";
    }

    std::cout << "Program finished!\n";

    return 0;
}