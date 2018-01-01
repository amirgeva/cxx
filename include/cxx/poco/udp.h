#pragma once

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/DNS.h>
#include <queue>
#include <string>
#include <thread>
#include <chrono>

namespace cxx {

inline void send_udp(const char* host, int port, const char* message, int len)
{
  Poco::Net::SocketAddress sa(host, port);
  Poco::Net::DatagramSocket s;
  s.sendTo(message,len,sa);
}

inline void send_udp(const char* host, int port, const std::string& message)
{
  send_udp(host,port,message.c_str(),message.length());
}

inline void send_udp(int port, const std::string& message)
{
  Poco::Net::SocketAddress sa(Poco::Net::IPAddress("\177\000\000\001",4),port);
  Poco::Net::DatagramSocket s;
  s.sendTo(message.c_str(),message.length(),sa);
}

inline void ms_delay(unsigned ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms));}

struct UDPMessage
{
  UDPMessage(const std::string& s="", const std::string& c="")
  : sender(s), content(c) {}
  std::string sender;
  std::string content;
};

typedef std::queue<UDPMessage> message_queue;
typedef std::function<void(const std::string&)> udp_callback;

class UDPReceiver
{
  Poco::Net::SocketAddress  m_Address;
  Poco::Net::DatagramSocket m_Socket;
  std::vector<char>         m_Buffer;
  std::thread               m_ReceiveThread;
  bool                      m_Done;
  message_queue             m_Queue;
  udp_callback              m_Callback;
  
public:
  UDPReceiver(int port, int maxlen)
  : m_Address(Poco::Net::DNS::hostName(), port)
  , m_Socket(m_Address)
  , m_Buffer(maxlen)
  , m_Done(false)
  {
    m_Socket.setBlocking(false);
    m_ReceiveThread=std::thread([&](){receive_loop();});
  }
  
  ~UDPReceiver()
  {
    m_Done=true;
    m_ReceiveThread.join();
  }

  void set_callback(udp_callback cb) { m_Callback=cb; }

  void receive_loop()
  {
    while (!m_Done)
    {
      try
      {
        Poco::Net::SocketAddress sender;
        int n = m_Socket.receiveFrom(&m_Buffer[0], m_Buffer.size()-1, sender);
        if (n<=0) ms_delay(10);
        else
        {
          m_Buffer[n] = '\0';
          std::string msg(&m_Buffer[0]);
          if (m_Callback) m_Callback(msg);
          else
            m_Queue.push(UDPMessage(sender.toString(),msg));
        }
      } catch (const Poco::TimeoutException&)
      {
        ms_delay(10);
      }
    }
  }

  bool empty() const { return m_Queue.empty(); }
  UDPMessage pop() 
  {
    if (empty()) return UDPMessage();
    auto m=m_Queue.front(); 
    m_Queue.pop(); 
    return m; 
  }
};

} // namespace cxx


