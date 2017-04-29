#pragma once

#include <xstring.h>

namespace cxx {

class ProgressBar
{
  int m_Size, m_Current;
  std::ostream* m_Output;
  char m_DoneChar, m_LeftChar;

  void print()
  {
    std::ostream& os = *m_Output;
    os << '[';
    if (m_Current > 0) os << xstring(m_Current, m_DoneChar);
    int left = m_Size - m_Current;
    if (left > 0)
      os << xstring(left, m_LeftChar);
    os << "]\r";
    os.flush();
  }
public:
  ProgressBar(int size=60) 
  : m_Size(size)
  , m_Current(-1)
  , m_Output(&std::cout)
  , m_DoneChar('@')
  , m_LeftChar('-')
  {}

  ~ProgressBar()
  {
    *m_Output << xstring(m_Size + 2, ' ') << '\r';
  }

  void set_output(std::ostream& os)
  {
    m_Output = &os;
  }

  void set_progress(int i, int n)
  {
    set_progress(double(i) / n);
  }

  void set_progress(double d)
  {
    int c = int(d*m_Size);
    if (c != m_Current)
    {
      m_Current = c;
      print();
    }
  }
};

} // namespace cxx

