#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <sstream>
#include <cxx/threading.h>

namespace cxx {

// Basically anything that can be called like a function that looks like:
// void func();
typedef std::function<void()> callable;
typedef std::list<callable> call_seq;

class TaskThread
{
  std::thread m_Thread;
  bool        m_Terminate;
  callable    m_Current;
  SYNC_MUTEX;

  void run()
  {
    while (!m_Terminate)
    {
      if (m_Current)
      {
        m_Current();
        {
          SYNCHRONIZED;
          m_Current = callable();
        }
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
    SYNCHRONIZED;
    m_Current = c;
  }

  // Stop the thread and wait for it to end
  void terminate()
  {
    m_Terminate = true;
    m_Thread.join();
  }
};

typedef std::shared_ptr<TaskThread> task_thread_ptr;

class TaskManager
{
public:
  static TaskManager* instance()
  {
    static std::unique_ptr<TaskManager> ptr(new TaskManager);
    return ptr.get();
  }

  void start(unsigned size, unsigned max_queue_size=0)
  {
    if (m_Pool.empty())
    {
      if (max_queue_size==0) max_queue_size=size;
      m_Pool.resize(size);
      for(auto& tt : m_Pool)
        tt.reset(new TaskThread);
      m_MaxQueueSize=max_queue_size;
    }
  }

  void wait(bool prints)
  {
    while (true)
    {
      unsigned queue = m_Tasks.size();
      unsigned busy = busy_threads();
      if (queue==0 && busy==0) break;
      if (prints)
      {
        std::cout << timestamp << "   Q=" << queue << "   A=" << busy << "       \r";
        std::cout.flush();
        delay(100);
      }
      else
        delay(10);
    }
  }

  void add_task(callable c)
  {
    if (m_Pool.empty()) c();
    else
    {
      while (m_Tasks.size() >= m_MaxQueueSize)
        delay(10);
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
        t->terminate();
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
      if (!t->empty()) ++n;
    return n;
  }

  void run()
  {
    while (!m_Terminate)
    {
      size_t pool_size=m_Pool.size();
      for (size_t i = 0; i < pool_size && !m_Tasks.empty();++i)
      {
        if (m_Pool[i]->empty())
        {
          SYNCHRONIZED;
          //sync_print("Dispatching task");
          m_Pool[i]->set_task(m_Tasks.front());
          m_Tasks.pop_front();
        }
      }
      delay(10);
    }
  }

  typedef std::vector<task_thread_ptr> tt_vec;

  std::thread m_Thread;
  tt_vec      m_Pool;
  SYNC_MUTEX;
  call_seq    m_Tasks;
  bool        m_Terminate;
  unsigned    m_MaxQueueSize;
};

// Automate the cleanup at the end of execution
class TaskManagerCleaner
{
public:
  TaskManagerCleaner(unsigned n, unsigned qs) { TaskManager::instance()->start(n,qs); }
  ~TaskManagerCleaner() { TaskManager::instance()->terminate(); }
};

#define TASK_MANAGER_POOL(n,qs) cxx::TaskManagerCleaner l_TaskManager_##__LINE__(n,qs)
#define TASKS_WAIT_PRINTS cxx::TaskManager::instance()->wait(true)
#define TASKS_WAIT cxx::TaskManager::instance()->wait(false)
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


