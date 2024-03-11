#include <iostream>
#include <thread>
#include <vector>

// Function to calculate factorial of a number
unsigned long long factorial(unsigned int n) {
    unsigned long long result = 1;
    for (unsigned int i = 1; i <= n; ++i) {
        result *= i;
    }
    return result;
}

// Function to calculate factorial range for each thread
void calculateFactorialRange(unsigned int start, unsigned int end, std::vector<unsigned long long>& results) {
    for (unsigned int i = start; i <= end; ++i) {
        results[i] = factorial(i);
    }
}

int main() {
    const unsigned int N = 1000; // Factorial of numbers from 1 to N
    std::vector<unsigned long long> results(N + 1); // Store results of factorial calculations

    // Create threads and distribute the factorial calculation task
    const unsigned int numThreads = std::thread::hardware_concurrency(); // Get the number of hardware threads
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < numThreads; ++i) {
        unsigned int start = i * (N / numThreads) + 1;
        unsigned int end = (i + 1) * (N / numThreads);
        threads.emplace_back(calculateFactorialRange, start, end, std::ref(results));
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    // Print the results
    for (unsigned int i = 1; i <= N; ++i) {
        std::cout << "Factorial of " << i << ": " << results[i] << std::endl;
    }

    return 0;
}
