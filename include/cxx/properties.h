#pragma once

#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <sstream>
#include <cxx/xstring.h>

namespace cxx {

class Properties
{
  typedef std::map<xstring,xstring> str_map;
  str_map m_Values;
  
  void expand_macros(xstring& value)
  {
    bool done=false;
    while (!done)
    {
      int p=value.find("$(");
      if (p<0) done=true;
      else
      {
        int e=value.find(")",p+2);
        if (e<0) done=true;
        else
        {
          xstring key=value.substr(p+2,e-p-2);
          value=value.substr(0,p)+get(key)+value.substr(e+1);
        }
      }
    }
  }
public:
  typedef str_map::const_iterator const_iterator;
  const_iterator begin() const { return m_Values.begin(); }
  const_iterator end()   const { return m_Values.end(); }

  Properties(const xstring& filename = "")
  {
    if (!filename.empty())
      if (!load(filename)) throw xstring("File not found: "+filename);
  }

  bool has(const xstring& name) const { return m_Values.count(name) > 0; }

  bool load(const xstring& filename)
  {
    std::ifstream fin(filename);
    if (fin.fail()) return false;
    return load(fin);
  }

  bool load(std::istream& is)
  {
    xstring line;
    while (line.read_line(is))
    {
      line.trim();
      if (line.empty()) continue;
      if (line.startswith("include "))
      {
        load(line.substr(8));
        continue;
      }
      int p=int(line.find('='));
      if (p<=0) continue;
      xstring name=line.substr(0,p);
      xstring value=line.substr(p+1);
      expand_macros(value);
      set(name,value);
    }
    return true;
  }

  bool save(std::ostream& os) const
  {
    for(const_iterator it=begin();it!=end();++it)
      os << it->first << "=" << it->second << std::endl;
    return true;
  }

  bool save(const xstring& filename) const
  {
    std::ofstream fout(filename);
    if (fout.fail()) return false;
    return save(fout);
  }

  void insert(const Properties& ps)
  {
    m_Values.insert(ps.begin(),ps.end());
  }

  template<class T>
  void set(const xstring& name, const T& value)
  {
    m_Values[name]=xstring(value);
  }

  const xstring& get(const xstring& name) const
  {
    static const xstring Null;
    const_iterator it=m_Values.find(name);
    if (it==m_Values.end()) return Null;
    return it->second;
  }
  
  int getn(const xstring& name) const
  {
    return get(name).as_int();
  }

  template<class T>
  bool get(const xstring& name, T& value) const
  {
    const_iterator it=m_Values.find(name);
    if (it==m_Values.end()) return false;
    std::istringstream is(it->second);
    is >> value;
    return !is.fail();
  }
  
  template<class T>
  void get(const xstring& name, T& value, const T& default_value) const
  {
    if (!get(name,value)) value=default_value;
  }
};

typedef std::list<Properties> prop_stack;

inline void push_properties(prop_stack& stack, const xstring& filename)
{
  if (stack.empty()) stack.push_back(Properties());
  else stack.push_back(stack.back());
  Properties ps;
  if (ps.load(filename))
    stack.back().insert(ps);
}

inline void pop_properties(prop_stack& stack)
{
  if (!stack.empty()) stack.pop_back();
}

} // namespace cxx

