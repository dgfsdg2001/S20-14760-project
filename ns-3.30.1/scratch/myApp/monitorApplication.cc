#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/stats-module.h"

#include "monitorApplication.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("myApp-mon");

const uint64_t default_port = 33456;

//----------------------------------------------------------------------
//-- Sender
//----------------------------------------------------------------------
TypeId
Sender::GetTypeId (void)
{
  static TypeId tid = TypeId ("Sender")
    .SetParent<Application> ()
    .AddConstructor<Sender> ()
    .AddAttribute ("PacketSize", "The size of packets transmitted.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&Sender::m_pktSize),
                   MakeUintegerChecker<uint32_t>(sizeof(payload_temp)))
    .AddAttribute ("PacketNum", "The number of packets sent per interval.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&Sender::m_pktNum),
                   MakeUintegerChecker<uint32_t>(1))
    .AddAttribute ("Destination", "Target host address.",
                   Ipv4AddressValue ("255.255.255.255"),
                   MakeIpv4AddressAccessor (&Sender::m_destAddr),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("Port", "Destination app port.",
                   UintegerValue (default_port),
                   MakeUintegerAccessor (&Sender::m_destPort),
                   MakeUintegerChecker<uint32_t>())
    .AddAttribute ("Interval", "Delay between transmissions.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                   MakePointerAccessor (&Sender::m_interval),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("UseTCP", "true for TCP, false for UDP",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Sender::m_useTCP),
                   MakeUintegerChecker <uint8_t>())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&Sender::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}


Sender::Sender()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_interval = CreateObject<ConstantRandomVariable> ();
  m_socket = 0;
  m_buf = 0;
}


Sender::~Sender()
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_buf)
    delete [] m_buf;
}


void
Sender::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
  Application::DoDispose ();
}


void Sender::StartApplication ()
{
    NS_LOG_FUNCTION_NOARGS ();
    if (m_socket == 0) {
      Ptr<SocketFactory> socketFactory = GetNode ()->GetObject<SocketFactory>
          (m_useTCP? TcpSocketFactory::GetTypeId () : UdpSocketFactory::GetTypeId());
      m_socket = socketFactory->CreateSocket ();
      m_socket->Bind ();

      if (m_useTCP) {
        m_socket->Connect (InetSocketAddress (m_destAddr, m_destPort));
      }
  }

  if (m_buf == 0) {
    m_buf = new uint8_t[m_pktSize];
    memset(m_buf, 0, m_pktSize);  
  }
  payload_temp.app_sn = 0;
  payload_temp.size = m_pktSize;

  Simulator::Cancel (m_sendEvent);
  m_sendEvent = Simulator::ScheduleNow (&Sender::SendPacket, this);
}


void Sender::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Simulator::Cancel (m_sendEvent);
}


void Sender::SendPacket ()
{ 
  NS_LOG_INFO ("Sending packet at " << Simulator::Now () << " to " << m_destAddr);

  for (uint32_t i = 0; i < m_pktNum; i += 1) {
    payload_temp.app_sn += 1;
    payload_temp.timestamp_ns = Simulator::Now ().GetNanoSeconds();
    memccpy(m_buf, &payload_temp, sizeof(payload_temp), m_pktSize);
    Ptr<Packet> packet = Create<Packet>(m_buf, m_pktSize);
    if (m_useTCP)
        m_socket->Send(packet);
    else
        m_socket->SendTo (packet, 0, InetSocketAddress (m_destAddr, m_destPort));
    
    m_txTrace (packet);
  }
  m_sendEvent = Simulator::Schedule (Seconds (m_interval->GetValue ()),
                                     &Sender::SendPacket, this);
}


//----------------------------------------------------------------------
//-- Receiver
//----------------------------------------------------------------------
TypeId
Receiver::GetTypeId (void)
{
  static TypeId tid = TypeId ("Receiver")
    .SetParent<Application> ()
    .AddConstructor<Receiver> ()
    .AddAttribute ("Port", "Listening port.",
                   UintegerValue (default_port),
                   MakeUintegerAccessor (&Receiver::m_port),
                   MakeUintegerChecker<uint32_t>())
    .AddAttribute ("UseTCP", "true for TCP, false for UDP",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Receiver::m_useTCP),
                   MakeUintegerChecker <uint8_t>())
  ;
  return tid;
}

Receiver::Receiver() :
  m_calc (0),
  m_delay (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
}

Receiver::~Receiver()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
Receiver::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
  Application::DoDispose ();
}

void
Receiver::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_socket == 0) {
      Ptr<SocketFactory> socketFactory = GetNode ()->GetObject<SocketFactory>
          (m_useTCP? TcpSocketFactory::GetTypeId() : UdpSocketFactory::GetTypeId());
      m_socket = socketFactory->CreateSocket ();
      m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_port));

      if (m_useTCP) {
          m_socket->Listen ();
      }
  }
  m_segBytes = 0;  

  m_socket->SetRecvCallback (MakeCallback (&Receiver::Receive, this));
  m_socket->SetRecvCallbackTCP (MakeCallback (&Receiver::Receive, this));
}


void
Receiver::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_socket != 0) {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }
}

void
Receiver::SetCounter (Ptr<CounterCalculator<> > calc)
{
  m_calc = calc;
}
void
Receiver::SetDelayTracker (Ptr<TimeMinMaxAvgTotalCalculator> delay)
{
  m_delay = delay;
}

# include <unistd.h>
#include <memory>

void
Receiver::Receive (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from))) {
      if (InetSocketAddress::IsMatchingType (from)) {
          NS_LOG_INFO ("Received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 ());
      }

      uint8_t *buffer = new uint8_t[packet->GetSize ()];
      uint8_t *ptr = buffer;
      uint32_t size = packet->CopyData(buffer, packet->GetSize ());
      
      // Packet might be segmented into one sockets or multiple sockets.
      if (m_segBytes >= size) {
          m_segBytes -= size;
          size = 0;
      } else {
          size -= m_segBytes;
          ptr += m_segBytes;
      }
      while (size > 0) {
          
          struct PAYLOAD *payload =  reinterpret_cast<PAYLOAD *>(ptr);
          std::cout << size << "," << payload->app_sn << std::endl;

          m_segBytes = payload->size;
          if (m_segBytes <= size) {
              size -= m_segBytes;
              ptr += m_segBytes;
              m_segBytes = 0;
          } else {
              m_segBytes -= size;
              size = 0;
          }
      }
      if (m_calc != 0) {
          m_calc->Update ();
      }

      delete [] buffer;
  }
}
