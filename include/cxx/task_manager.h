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

class TaskManager
{
public:
  static TaskManager* instance()
  {
    static std::unique_ptr<TaskManager> ptr(new TaskManager);
    return ptr.get();
  }

  bool initialized() const { return !m_Pool.empty(); }

  void start(size_t size, size_t max_queue_size=0)
  {
    if (m_Pool.empty())
    {
      m_BusyThreads = 0;
      if (max_queue_size == 0) max_queue_size = size;
      m_Pool.resize(size);
      for (auto& t : m_Pool)
        t = std::thread(&TaskManager::thread_main, this);
      m_MaxQueueSize = max_queue_size;
    }
  }

  void wait_sleep()
  {
    size_t jobs = 1;
    while (jobs>0)
    {
      {
        SYNCHRONIZED;
        jobs = m_Tasks.size() + m_BusyThreads;
      }
      if (jobs > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  void wait(bool prints)
  {
    size_t jobs = 1;
    while (jobs>0)
    {
      {
        SYNCHRONIZED;
        jobs = m_Tasks.size() + m_BusyThreads;
      }
      if (jobs > 0)
      {
        if (prints)
        {
          std::cout << jobs << "       \r";
          std::cout.flush();
        }
        m_UserQueue.wait(10);
      }
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
        m_ThreadsQueue.notify();
      }
    }
  }

  void terminate()
  {
    if (!m_Terminate)
    {
      m_Terminate = true;
      m_ThreadsQueue.notify(true);
      for (auto& t : m_Pool)
        t.join();
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
    //m_Thread = std::thread(&TaskManager::run, this); 
  }
  ~TaskManager() {}
  TaskManager(const TaskManager&) {}
  TaskManager& operator= (const TaskManager&) { return *this; }

  void thread_main()
  {
    while (!m_Terminate)
    {
      callable task;
      {
        SYNCHRONIZED;
        if (!m_Tasks.empty())
        {
          m_BusyThreads++;
          task = m_Tasks.front();
          m_Tasks.pop_front();
        }
      }
      if (task)
      {
        task();
        {
          SYNCHRONIZED;
          --m_BusyThreads;
        }
        m_UserQueue.notify();
      }
      else
        m_ThreadsQueue.wait();
    }
  }

  typedef std::vector<std::thread> threads_vec;

  SYNC_MUTEX;
  threads_vec m_Pool;
  Waiter      m_ThreadsQueue;
  Waiter      m_UserQueue;
  call_seq    m_Tasks;
  bool        m_Terminate;
  size_t      m_MaxQueueSize;
  size_t      m_BusyThreads;
};

// Automate the cleanup at the end of execution
class TaskManagerCleaner
{
public:
  TaskManagerCleaner(size_t n, size_t qs) { TaskManager::instance()->start(n,qs); }
  ~TaskManagerCleaner() { TaskManager::instance()->terminate(); }
};

#define TASK_MANAGER_POOL(n,qs) std::unique_ptr<cxx::TaskManagerCleaner> l_TaskManagerCleaner_##__LINE__; if (!cxx::TaskManager::instance()->initialized()) l_TaskManagerCleaner_##__LINE__.reset(new cxx::TaskManagerCleaner(n,qs))
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


