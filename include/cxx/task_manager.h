#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <cxx/threading.h>
#include <cxx/xstring.h>

namespace cxx {

// Basically anything that can be called like a function that looks like:
// void func();
typedef std::function<void()> callable;
typedef std::list<callable> call_seq;

struct Task
{
  Task(callable c=callable(), const xstring& grp="") : work(c), group(grp) {}
  callable work;
  xstring  group;
};

typedef std::list<Task> task_seq;

struct TaskGroup
{
  TaskGroup() : count(0) {}
  int count;
  void increment() { ++count; }
  void decrement() { --count; }
};

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
  
  void group_wait(const xstring& group)
  {
    size_t jobs = 1;
    while (jobs>0)
    {
      {
        SYNCHRONIZED;
        jobs = m_Groups[group].count;
      }
      if (jobs > 0)
        m_UserQueue.wait(10);
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
          std::cerr << ' ' << jobs << "       \r";
          std::cerr.flush();
        }
        m_UserQueue.wait(10);
      }
    }
  }

  void add_task(callable c, const xstring& group="")
  {
    if (m_Pool.empty()) c();
    else
    {
      while (m_Tasks.size() >= m_MaxQueueSize)
        delay(10);
      {
        SYNCHRONIZED;
        if (!group.empty())
          m_Groups[group].increment();
        m_Tasks.push_back(Task(c,group));
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
      Task task;
      {
        SYNCHRONIZED;
        if (!m_Tasks.empty())
        {
          m_BusyThreads++;
          task = m_Tasks.front();
          m_Tasks.pop_front();
        }
      }
      if (task.work)
      {
        task.work();
        {
          SYNCHRONIZED;
          --m_BusyThreads;
          if (!task.group.empty())
            m_Groups[task.group].decrement();
        }
        m_UserQueue.notify();
      }
      else
        m_ThreadsQueue.wait();
    }
  }

  typedef std::vector<std::thread> threads_vec;

  SYNC_MUTEX;
  threads_vec                           m_Pool;
  Waiter                                m_ThreadsQueue;
  Waiter                                m_UserQueue;
  task_seq                              m_Tasks;
  std::unordered_map<xstring,TaskGroup> m_Groups;
  bool                                  m_Terminate;
  size_t                                m_MaxQueueSize;
  size_t                                m_BusyThreads;
};

// Automate the cleanup at the end of execution
class TaskManagerCleaner
{
public:
  TaskManagerCleaner(size_t n, size_t qs) { TaskManager::instance()->start(n,qs); }
  ~TaskManagerCleaner() { TaskManager::instance()->terminate(); }
};

#define TASK_MANAGER_POOL(n,qs) std::unique_ptr<cxx::TaskManagerCleaner> l_TaskManagerCleaner_##__LINE__; if (!cxx::TaskManager::instance()->initialized()) l_TaskManagerCleaner_##__LINE__.reset(new cxx::TaskManagerCleaner(n,qs))
inline void wait_all_tasks(bool prints=false) { cxx::TaskManager::instance()->wait(prints); }
inline void wait_group(const xstring& name) { cxx::TaskManager::instance()->group_wait(name); }
inline void add_task(callable c, const xstring& group="") { cxx::TaskManager::instance()->add_task(c,group); }
inline void call_task(callable c) { c(); }
inline void add_task(bool parallel, callable c, const xstring& group="") { if (parallel) add_task(c,group); else c(); }

template<class T>
inline void sync_print(const T& t)
{
  std::ostringstream os;
  os << t;
  TaskManager::instance()->sync_print(os.str());
}

} // namespace cxx


