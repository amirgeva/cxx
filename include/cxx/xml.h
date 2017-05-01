#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <map>
#include <cxx/xstring.h>

namespace cxx {

typedef std::shared_ptr<class xml_element> xml_ptr;

class xml_element
{
  typedef xml_element& reference;
  typedef xml_ptr pointer;
  typedef std::vector<pointer> child_vec;
  typedef std::map<xstring, xstring> attr_map;

  xstring    m_Type;
  child_vec m_Children;
  attr_map  m_Attributes;
  xstring    m_Content;

  xml_element(const xml_element&) {}
  xml_element& operator= (const xml_element&) { return *this; }
public:
  xml_element(const xstring& type = "") : m_Type(type) {}

  void set_type(const xstring& type) { m_Type = type; }
  const xstring& get_type() const { return m_Type; }

  int  get_child_count() const { return int(m_Children.size()); }
  void add_child(pointer p) { m_Children.push_back(p); }

  void set_content(const xstring& c) { m_Content = c; }
  const xstring& get_content() const { return m_Content; }

  xml_ptr add_child(const xstring& type)
  {
    xml_ptr child(new xml_element(type));
    add_child(child);
    return child;
  }

  void remove(pointer child)
  {
    for (iterator it = begin(); it != end(); ++it)
    {
      xml_ptr c = *it;
      if (c == child) { m_Children.erase(it); break; }
    }
  }

  xml_ptr find_child(const xstring& type, const xstring& attr_name, const xstring& attr_val, bool recursive=false)
  {
    for (iterator it = begin(); it != end(); ++it)
    {
      xml_ptr c = *it;
      if (c->get_type() == type && c->get_attribute(attr_name) == attr_val) return c;
    }
    if (recursive)
    {
      for (iterator it = begin(); it != end(); ++it)
      {
        xml_ptr c = *it;
        xml_ptr res = c->find_child(type, attr_name, attr_val, recursive);
        if (res) return res;
      }
    }
    return xml_ptr();
  }

  xml_ptr find_child(const xstring& type, bool recursive=false)
  {
    for (iterator it = begin(); it != end(); ++it)
    {
      xml_ptr c = *it;
      if (c->get_type() == type) return c;
    }
    if (recursive)
    {
      for (iterator it = begin(); it != end(); ++it)
      {
        xml_ptr c = *it;
        xml_ptr res = c->find_child(type, recursive);
        if (res) return res;
      }
    }
    return xml_ptr();
  }

  bool has_attribute(const xstring& name) const { return m_Attributes.count(name) > 0; }
  xml_element* set_attribute(const xstring& name, const xstring& value) 
  { 
    m_Attributes[name] = value; 
    return this;
  }
  xstring get_attribute(const xstring& name) const
  {
    attr_map::const_iterator it = m_Attributes.find(name);
    if (it == m_Attributes.end()) return "";
    return it->second;
  }
  typedef child_vec::iterator iterator;
  typedef child_vec::const_iterator const_iterator;
  iterator begin() { return m_Children.begin(); }
  iterator end()   { return m_Children.end(); }
  const_iterator begin() const { return m_Children.begin(); }
  const_iterator end()   const { return m_Children.end(); }

  typedef attr_map::const_iterator attr_iterator;
  attr_iterator attr_begin() const { return m_Attributes.begin(); }
  attr_iterator attr_end()   const { return m_Attributes.end(); }

  void print(std::ostream& os = std::cout, int indent = 0, bool packed = false) const
  {
    xstring spaces = packed ? xstring("") : xstring(indent, ' ');
    xstring eol = packed ? xstring("") : xstring("\n");
    os << spaces << "<" << get_type();
    for (attr_iterator it = attr_begin(); it != attr_end(); ++it)
      os << " " << it->first << "=\"" << it->second << "\"";
    if (get_child_count() == 0 && m_Content.empty()) { os << "/>" << eol; return; }
    os << ">";
    if (get_child_count() > 0)
    {
      os << eol;
      if (!m_Content.empty()) os << spaces << m_Content << eol;
      for (const_iterator ci = begin(); ci != end(); ++ci)
        (*ci)->print(os, indent + 2, packed);
      os << spaces;
    }
    else os << m_Content;
    os << "</" << get_type() << ">" << eol;
  }

  xstring print(bool packed)
  {
    std::ostringstream os;
    print(os, 0, packed);
    return os.str();
  }
};

class xml_parser
{
public:
  xml_parser() : m_InQuotes(false), m_LineNumber(1) {}
private:
  enum Token { LTAG, RTAG, EQ, QUOTES, SLASH, EXCLPOINT, QUESTION, IDENT, TEXT, XEOF };

  bool m_InQuotes;
  int  m_LineNumber;

  static bool is_white_space(char c)
  {
    return (c <= 32);
  }

  static bool quotes_pred(char c)
  {
    return c == '"';
  }

  static bool not_alnum(char c)
  {
    return ((c<'A' || c>'Z') && (c<'a' || c>'z') && (c<'0' || c>'9') && c != '_' && c!=':');
  }

  Token analyze(std::istream& is, xstring& token_text)
  {
    char ch = ' ';
    while (!is.eof() && is_white_space(ch))
    {
      ch = is.get();
      if (ch == '\n') ++m_LineNumber;
    }
    token_text = "";
    if (is.eof()) return XEOF;
    token_text = xstring(1, ch);
    if (ch == '!') return EXCLPOINT;
    if (ch == '?') return QUESTION;
    if (ch == '"')
    {
      m_InQuotes = !m_InQuotes;
      return QUOTES;
    }
    if (m_InQuotes)
    {
      token_text += read_until(is, quotes_pred,false);
      return TEXT;
    }
    if (ch == '<') { return LTAG; }
    if (ch == '>') { return RTAG; }
    if (ch == '=') { return EQ; }
    if (ch == '/') { return SLASH; }
    token_text += read_until(is, not_alnum,false);
    return IDENT;
  }

  template<class PRED>
  xstring read_until(std::istream& is, PRED p, bool extract)
  {
    xstring res;
    while (!is.eof())
    {
      char ch = is.peek();
      if (p(ch))
      {
        if (extract) is.get();
        return res;
      }
      ch = is.get();
      res += xstring(1, ch);
    }
    return res;
  }

  xstring read_until(std::istream& is, char stop, bool extract)
  {
    xstring res;
    while (!is.eof())
    {
      char ch = is.peek();
      if (ch == stop)
      {
        if (extract) is.get();
        return res;
      }
      ch = is.get();
      res += xstring(1, ch);
    }
    return res;
  }

#define SYNTAX_ERROR throw std::runtime_error("XML Syntax Error")
#define EXPECT(t) { token=analyze(is,last); if (token!=t) SYNTAX_ERROR; }

  void parse_element(std::istream& is, xml_ptr parent)
  {
    xstring last;
    Token  token;
    xstring content = read_until(is, '<',false);
    parent->set_content(content);
    while (true)
    {
      token = analyze(is, last);
      if (token == XEOF) return;
      if (token == LTAG)
      {
        token = analyze(is, last);
        if (token == EXCLPOINT || token == QUESTION)
        {
          read_until(is, '>',true);          
        }
        else
          if (token == SLASH)
          {
          if (!parent) SYNTAX_ERROR;
          EXPECT(IDENT);
          if (last != parent->get_type()) SYNTAX_ERROR;
          EXPECT(RTAG);
          return;
          }
          else
            if (token == IDENT)
            {
          xml_ptr child(new xml_element);
          child->set_type(last);
          parent->add_child(child);
          while (true)
          {
            token = analyze(is, last);
            if (token == XEOF) return;
            if (token == IDENT)  // Attribute
            {
              xstring attr_name = last;
              EXPECT(EQ);
              EXPECT(QUOTES);
              token = analyze(is, last);
              xstring attr_value;
              if (token == TEXT)
              {
                attr_value = last;
                EXPECT(QUOTES);
              }
              else
                if (token != QUOTES) SYNTAX_ERROR;
              child->set_attribute(attr_name, attr_value);
            }
            else
              if (token == SLASH)
              {
              EXPECT(RTAG);
              break;
              }
              else
                if (token == RTAG)
                {
              parse_element(is, child);
              break;
                }
          }
            }
            else
              SYNTAX_ERROR;
      }
    }
  }

public:
  xml_ptr parse(std::istream& is)
  {
    if (is.fail()) return 0;
    xml_ptr root(new xml_element);
    try
    {
      parse_element(is, root);
      int n = root->get_child_count();
      if (n > 1) throw "Error: Multiple root nodes";
      if (n == 0) throw "Error: No root node";
      xml_ptr new_root = *(root->begin());
      root->remove(new_root);
      root = new_root;
    }
    catch (const char* msg)
    {
      std::cerr << "Line " << m_LineNumber << " - " << msg << std::endl;
      root.reset();
    }
    return root;
  }
};

inline xml_ptr load_xml_from_file(const char* filename)
{
  std::ifstream fin(filename);
  if (fin.fail()) return 0;
  return xml_parser().parse(fin);
}

inline xml_ptr load_xml_from_file(const xstring& filename)
{
  return load_xml_from_file(filename.c_str());
}

inline void save_xml_to_file(const xstring& filename, xml_ptr root)
{
  std::ofstream f(filename);
  root->print(f);
}

inline xml_ptr load_xml_from_text(const xstring& text)
{
  std::istringstream is(text);
  return xml_parser().parse(is);
}

inline xstring get_xml_text(xml_ptr root)
{
  std::ostringstream os;
  root->print(os, 0, false);
  return os.str();
}


} // namespace cxx



