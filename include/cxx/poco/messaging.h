#pragma once

#include <thread>
#include <queue>
#include <map>
#include <cxx/poco/udp.h>
#include <cxx/xstring.h>
#include <cxx/task_manager.h>

namespace cxx {
  
  const int MAX_MESSAGE_LENGTH = 65000;
  
  class Messaging
  {
  public:
    static Messaging* instance()
    {
      static std::unique_ptr<Messaging> ptr(new Messaging);
      return ptr.get();
    }
    
    bool start_listening(int port)
    {
      if (m_Receivers.count(port)>0) return false; // Already listening
      receiver_ptr r(new UDPReceiver(port,MAX_MESSAGE_LENGTH));
      m_Receivers[port]=r;
      return true;
    }
    
    bool stop_listening(int port)
    {
      auto it=m_Receivers.find(port);
      if (it==m_Receivers.end()) return false;
      m_Receivers.erase(it);
      return true;
    }
    
    bool send_message(int port, const xstring& message)
    {
      if (message.length()>MAX_MESSAGE_LENGTH) return false;
      send_udp(port,message);
      return true;
    }
    
    bool empty(int port)
    {
      auto it=m_Receivers.find(port);
      if (it==m_Receivers.end()) return true;
      UDPReceiver& r=*(it->second);
      return r.empty();
    }
    
    xstring get(int port)
    {
      auto it=m_Receivers.find(port);
      if (it==m_Receivers.end()) return "";
      UDPReceiver& r=*(it->second);
      return r.pop().content;
    }
    
  private:
    friend struct std::default_delete<Messaging>;
    Messaging() {}
    ~Messaging() {}
    Messaging(const Messaging& rhs) {}
    Messaging& operator= (const Messaging& rhs) { return *this; }
    
    typedef std::shared_ptr<UDPReceiver> receiver_ptr;
    typedef std::map<int,receiver_ptr>   receivers_map;
    receivers_map                        m_Receivers;
  };

  
}
