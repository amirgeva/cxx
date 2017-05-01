#pragma once

#include <iostream>
#include <vector>

namespace cxx {

template<class T>
class TMatrix
{
  typedef TMatrix<T> self;
  unsigned m_Width, m_Height;
  typedef std::vector<T> data_vec;
  data_vec m_Data;
public:
  TMatrix() : m_Width(0), m_Height(0) {}
  TMatrix(unsigned w, unsigned h, const T& init = T())
    : m_Width(w),
    m_Height(h),
    m_Data(w*h, init)
  {}

  void resize(unsigned w, unsigned h, const T& init=T())
  {
    m_Width = w;
    m_Height = h;
    m_Data.resize(w*h, init);
  }

  T& get(unsigned x, unsigned y)
  {
    if (x >= m_Width || y >= m_Height) throw 1;
    return m_Data[y*m_Width + x];
  }

  const T& get(unsigned x, unsigned y) const
  {
    if (x >= m_Width || y >= m_Height) throw 1;
    return m_Data[y*m_Width + x];
  }

  void set(unsigned x, unsigned y, const T& value)
  {
    if (x >= m_Width || y >= m_Height) throw 1;
    m_Data[y*m_Width + x] = value;
  }

  T& operator() (unsigned x, unsigned y)
  {
    return get(x, y);
  }

  const T& operator() (unsigned x, unsigned y) const
  {
    return get(x, y);
  }

  unsigned get_width() const { return m_Width; }
  unsigned get_height() const { return m_Height; }

  T* get_row(unsigned row) { return &m_Data[row*m_Width]; }
  const T* get_row(unsigned row) const { return &m_Data[row*m_Width]; }

  typedef typename data_vec::const_iterator const_iterator;
  const_iterator begin() const { return m_Data.begin(); }
  const_iterator end() const { return m_Data.end(); }

  void print(std::ostream& os, char delim=' ') const
  {
    for (unsigned y = 0; y < get_height(); ++y)
    {
      const T* row = get_row(y);
      for (unsigned x = 0; x < get_width(); ++x)
      {
        if (x > 0) os << delim;
        os << row[x];
      }
      if (y < (get_height() - 1))
        os << std::endl;
    }
  }

  self transpose()
  {
    self t(get_height(), get_width());
    for (unsigned y = 0; y < get_height(); ++y)
      for (unsigned x = 0; x < get_width(); ++x)
        t(y, x) = get(x, y);
    return t;
  }
  
  void save_csv(const char* filename)
  {
    std::ofstream f(filename);
    print(f,',');
  }
};

template<class T>
inline std::ostream& operator<< (std::ostream& os, const TMatrix<T>& m)
{
  m.print(os);
  return os;
}


} // namespace cxx

