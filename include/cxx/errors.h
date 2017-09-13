#pragma once

#include <cxx/xstring.h>
#include <sstream>

namespace cxx {

#define BUILD_STRING(x)\
(static_cast<std::ostringstream&>(std::ostringstream() << x)).str()

#define THROW_ERROR(x) throw BUILD_STRING(x)

} // namespace cxx

