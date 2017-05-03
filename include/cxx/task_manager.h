#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <sstream>

namespace cxx {

inline void delay(unsigned ms)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

typedef std::recursive_mutex Mutex;

class Monitor
{
  Mutex& m_Mutex;
public:
  Monitor(Mutex& m) : m_Mutex(m) { m_Mutex.lock(); }
  ~Monitor() { m_Mutex.unlock(); }
};

#define SYNC_MUTEX Mutex m_Mutex
#define MONITOR(m) Monitor l_##__LINE__(m)
#define SYNCHRONIZED MONITOR(Mutex)

// Basically anything that can be called like a function that looks like:
// void func();
typedef std::function<void()> callable;
typedef std::list<callable> call_seq;

class TaskThread
{
  std::thread m_Thread;
  bool        m_Terminate;
  callable    m_Current;

  void run()
  {
    while (!m_Terminate)
    {
      if (m_Current)
      {
        m_Current();
        m_Current = callable();
      }
      else
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
public:
  TaskThread()
    : m_Terminate(false)
  {
    m_Thread = std::thread(&TaskThread::run, this);
  }

  // Returns true if there is no task being executed by this thread
  bool empty() const { return !m_Current; }

  // Assign a task to this thread
  void set_task(callable c)
  {
    m_Current = c;
  }

  // Stop the thread and wait for it to end
  void terminate()
  {
    m_Terminate = true;
    m_Thread.join();
  }
};

class TaskManager
{
public:
  static TaskManager* instance()
  {
    static std::unique_ptr<TaskManager> ptr(new TaskManager);
    return ptr.get();
  }

  void start(unsigned size)
  {
    m_Pool.resize(size);
  }

  void wait()
  {
    while (!m_Tasks.empty() || busy_threads() > 0)
      delay(10);
  }

  void add_task(callable c)
  {
    if (m_Pool.empty()) c();
    else
    {
      while (m_Tasks.size() > m_Pool.size()) delay(10);
      {
        SYNCHRONIZED;
        m_Tasks.push_back(c);
      }
    }
  }

  void terminate()
  {
    if (!m_Terminate)
    {
      m_Terminate = true;
      m_Thread.join();
      for (auto& t : m_Pool)
        t.terminate();
    }
  }

  void sync_print(const std::string& s)
  {
    SYNCHRONIZED;
    std::cout << s << std::endl;
  }
  
private:
  friend struct std::default_delete<TaskManager>;
  TaskManager() 
  : m_Terminate(false)
  { 
    m_Thread = std::thread(&TaskManager::run, this); 
  }
  ~TaskManager() {}
  TaskManager(const TaskManager&) {}
  TaskManager& operator= (const TaskManager&) { return *this; }

  unsigned busy_threads() const
  {
    unsigned n = 0;
    for (auto& t : m_Pool)
      if (!t.empty()) ++n;
    return n;
  }

  void run()
  {
    while (!m_Terminate)
    {
      for (unsigned i = 0; i < m_Pool.size() && !m_Tasks.empty();++i)
      {
        if (m_Pool[i].empty())
        {
          SYNCHRONIZED;
          m_Pool[i].set_task(m_Tasks.front());
          m_Tasks.pop_front();
        }
      }
      delay(10);
    }
  }

  typedef std::vector<TaskThread> tt_vec;

  std::thread m_Thread;
  tt_vec      m_Pool;
  SYNC_MUTEX;
  call_seq    m_Tasks;
  bool        m_Terminate;
};

// Automate the cleanup at the end of execution
class TaskManagerCleaner
{
public:
  TaskManagerCleaner(unsigned n) { TaskManager::instance()->start(n); }
  ~TaskManagerCleaner() { TaskManager::instance()->terminate(); }
};

#define TASK_MANAGER_POOL(n) TaskManagerCleaner l_TaskManager_##__LINE__(n)
#define TASKS_WAIT cxx::TaskManager::instance()->wait()
inline void TASK(callable c) { cxx::TaskManager::instance()->add_task(c); }
inline void CALL(callable c) { c(); }
inline void TASK(bool parallel, callable c) { if (parallel) TASK(c); else c(); }

template<class T>
inline void sync_print(const T& t)
{
  std::ostringstream os;
  os << t;
  TaskManager::instance()->sync_print(os.str());
}

} // namespace cxx


