#include "my_exception.h"

MyException::MyException(std::string message)
{
    setMessage(message);
}

void MyException::setMessage(std::string message)
{
    this->message = message;
}

const char* MyException::what() const noexcept
{
    return this->message.c_str();
}
