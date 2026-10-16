#include "BNException.h"

namespace MaBoSS {

std::ostream& operator<<(std::ostream& os, const BNException& e)
{
  os << "BooleanNetwork exception: " << e.getMessage() << '\n';
  return os;
}

} // namespace MaBoSS
