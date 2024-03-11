#ifndef CFGSEXCEPT_HPP
#define CFGSEXCEPT_HPP

#include <iostream>
#include <exception>

class cfgsexcept : public std::exception {
    private:
        const char* message;

    public:
        cfgsexcept(const char* errorMessage) : message(errorMessage) {}

        const char* what() const noexcept override{
            return message;
        }
};

#endif