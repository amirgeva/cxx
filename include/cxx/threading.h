#pragma once

namespace cxx {

inline void delay(unsigned ms)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline double timestamp()
{
  static auto start=std::chrono::system_clock::now();
  auto cur=std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(cur-start).count();
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

} // namespace cxx

