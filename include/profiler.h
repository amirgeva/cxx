#ifndef H_PROFILER
#define H_PROFILER

#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <chrono>

class Timer
{
public:
  typedef std::chrono::time_point<std::chrono::system_clock> time_point;

  static time_point get()
  {
    return std::chrono::system_clock::now();
  }

  void reset()
  {
    m_Start = get();
  }

  double elapsed() const
  {
    time_point cur = get();
    std::chrono::duration<double> elapsed_seconds = cur - m_Start;
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

  void print(std::ostream& os, const char* name, int N=1) const
  {
    double t = 1000 * m_Timer.elapsed();
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

#define PROFILER(x) SectionProfiler l_Prof_##__LINE__ (#x)
#define PROFILER_N(x,N) SectionProfiler l_Prof_##__LINE__ (#x,N)

#endif // H_PROFILER
