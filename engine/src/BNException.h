#pragma once

#include <string>
#include <iostream>

namespace MaBoSS {

class BNException {

    std::string msg;
  
  public:
    BNException(const std::string& _msg) : msg(_msg) { }
  
    const std::string& getMessage() const {return msg;}
    const std::string what() const {return msg;}
};

std::ostream& operator<<(std::ostream& os, const BNException& e);

} // namespace MaBoSS
