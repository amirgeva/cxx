#pragma once

#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

namespace cxx {

inline void delay(unsigned ms)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline double timestamp()
{
  static auto start=std::chrono::system_clock::now();
  auto cur=std::chrono::system_clock::now();
  return double(std::chrono::duration_cast<std::chrono::milliseconds>(cur-start).count());
}

typedef std::recursive_mutex Mutex;

class Monitor
{
  Mutex& m_Mutex;
public:
  Monitor(Mutex& m) : m_Mutex(m) { m_Mutex.lock(); }
  ~Monitor() { m_Mutex.unlock(); }
};

#define SYNC_MUTEX cxx::Mutex m_Mutex
#define MONITOR(m) cxx::Monitor l_##__LINE__(m)
#define SYNCHRONIZED MONITOR(m_Mutex)

class Waiter
{
  std::mutex m_Mutex;
  std::condition_variable m_Cond;
public:
  void wait(unsigned ms=0)
  {
    std::unique_lock<std::mutex> lock(m_Mutex);
    if (ms==0)
      m_Cond.wait(lock);
    else
      m_Cond.wait_for(lock,std::chrono::milliseconds(ms));
  }

  void notify(bool all=false)
  {
    if (all) m_Cond.notify_all();
    else
      m_Cond.notify_one();
  }
};

} // namespace cxx

