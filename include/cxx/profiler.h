#pragma once

#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <chrono>
#include <cxx/threading.h>
#include <cxx/xstring.h>

namespace cxx {

class Timer
{
public:
  Timer() : m_Start(get()) {}
  
  typedef std::chrono::time_point<std::chrono::system_clock> time_point;

  static time_point get()
  {
    return std::chrono::system_clock::now();
  }

  void reset()
  {
    m_Start = get();
  }

  double elapsed(bool reset)
  {
    time_point cur = get();
    std::chrono::duration<double> elapsed_seconds = cur - m_Start;
    if (reset) m_Start=cur;
    return elapsed_seconds.count();
  }
private:
  time_point m_Start;
};

typedef std::unique_ptr<Timer> timer_ptr;

class Profiler
{
  Timer m_Timer;
public:
  Profiler() { reset(); }

  void reset() { m_Timer.reset(); }

  void print(std::ostream& os, const char* name, int N=1)
  {
    double t = 1000 * m_Timer.elapsed(false);
    os << "Total: " << std::setprecision(3) << std::fixed << t << "ms  for " << name;
    if (N > 1)
    {
      double d = t / N;
      std::string units = "ms";
      if (d < 0.1)
      {
        d *= 1000.0;
        units = "us";
        if (d < 0.1)
        {
          d *= 1000.0;
          units = "ns";
        }
      }
      os << ".  " << d << units << " per instance.";
    }
    os << std::endl;
  }
};

class SectionProfiler : public Profiler
{
  const char* m_Name;
  int         m_N;
public:
  SectionProfiler(const char* name, int N=1) : m_Name(name), m_N(N) {}
  ~SectionProfiler()
  {
    print(std::cout,m_Name,m_N);
  }
};

class RunningProfiler
{
  SYNC_MUTEX;
  xstring m_Name;
  double  m_TotalTime;
  xstring m_Report;
  Timer   m_Timer;
public:
  RunningProfiler() {}
  RunningProfiler(const xstring& name)
  : m_Name(name)
  , m_TotalTime(0)
  {}
  
  ~RunningProfiler()
  {
    if (!m_Name.empty())
    {
      m_TotalTime+=m_Timer.elapsed(true);
      m_Report+="  Total:"+xstring(1000.0*m_TotalTime);
      std::cout << m_Report << std::endl;
    }
  }
  
  void mark(const xstring& id)
  {
    if (!m_Name.empty())
    {
      SYNCHRONIZED;
      if (!m_Report.empty()) m_Report+="  ";
      double t=m_Timer.elapsed(true);
      m_TotalTime+=t;
      m_Report+=id+":"+xstring(1000.0*t);
    }
  }
};


} // namespace cxx

#define PROFILER(x) cxx::SectionProfiler l_Prof_##__LINE__ (#x)
#define PROFILER_N(x,N) cxx::SectionProfiler l_Prof_##__LINE__ (#x,N)


