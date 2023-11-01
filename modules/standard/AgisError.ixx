export module AgisError;

import <sstream>;
import <exception>;
import <string>;
import <source_location>;


namespace Agis {

export class AgisException : public std::exception {
private:
    std::string message;

public:
    AgisException(const std::string& msg) : message(msg) {}

    // Override the what() method to provide a custom error message
    const char* what() const noexcept override {
        return message.c_str();
    }
};


}