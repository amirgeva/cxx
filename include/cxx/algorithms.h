#pragma once

#include <algorithm>

namespace cxx {

  template<class C, class PRED>
  C& erase_if(C& c, PRED pred)
  {
    c.erase(std::remove_if(c.begin(), c.end(), pred), c.end());
    return c;
  }

#define CXX_PREDICATE(cond) [](const auto& value){return cond;}

} // namespace cxx
