#pragma once

#include <exception>
#include <string>

class MyException : public std::exception
{
private:
    std::string message;
public:
    MyException(std::string message);
    void setMessage(std::string message);
    virtual const char* what() const noexcept;
};
