#pragma once

#include <cxx/xstring.h>
#include <sstream>

namespace cxx {

//#define BUILD_STRING(x) xstring(x)

#define THROW_ERROR(x) { xstring msg; msg << x; throw msg; }

} // namespace cxx

