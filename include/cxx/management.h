#pragma once

#include <unordered_map>
#include <cxx/xstring.h>

namespace cxx {

template<class T>
class Registrar
{
  std::unordered_map<xstring, T*> mapping;
public:
  void register_name(const xstring& name, T* t)
  {
    mapping[name] = t;
  }

  T* get(const xstring& name)
  {
    auto it = mapping.find(name);
    if (it == mapping.end()) return nullptr;
    return it->second;
  }
};

#define REGISTER_NAME(mgr,x) struct x##_Register {\
x m_##x;\
x##_Register() { mgr::instance()->register_name(#x,&m_##x); }\
} g_##x##_Register

#define SINGLETON_INSTANCE(type) static type* instance() {\
  static std::unique_ptr<type> ptr(new type);\
  return ptr.get(); }

#define SINGLETON_TORS(type)\
friend struct std::default_delete<type>;\
type() {} ~type() {} type(const type&) {}\
type& operator= (const type&) { return *this; }

#define SINGLETON_SUBCLASS(type,parent)\
class type : public parent {\
public:  SINGLETON_INSTANCE(type)\
private: SINGLETON_TORS(type) }

} // namespace cxx

