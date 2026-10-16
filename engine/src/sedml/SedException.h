#pragma once

#include "../BNException.h"


namespace MaBoSS {

class SedException: public BNException {
    using BNException::BNException;
};

} // namespace MaBoSS
