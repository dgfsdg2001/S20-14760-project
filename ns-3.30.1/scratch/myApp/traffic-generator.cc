#include "traffic-generator.h"


NS_LOG_COMPONENT_DEFINE ("myApp-tg");

TrafficGenerator::TrafficGenerator ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_burstPktNum (0),
    m_burstItvSec (0.0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

TrafficGenerator::~TrafficGenerator ()
{
  m_socket = 0;
}

/* static */
TypeId TrafficGenerator::GetTypeId (void) {
    static TypeId tid = TypeId ("TrafficGenerator")
        .SetParent<Application> ()
        .SetGroupName ("TrafficGenerator")
        .AddConstructor<TrafficGenerator> ();
    return tid;
}

void
TrafficGenerator::Setup (
    Ptr<Socket> socket,
    Address address,
    uint32_t packetSize,
    uint32_t burstPktNum,
    double burstItvSec
) {
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_burstPktNum = burstPktNum;
    m_burstItvSec = burstItvSec;
}

void
TrafficGenerator::StartApplication (void) {
    m_running = true;
    m_packetsSent = 0;
    if (InetSocketAddress::IsMatchingType (m_peer))
        m_socket->Bind ();
    else
        m_socket->Bind6 ();
    m_socket->Connect (m_peer);
    SendPacket ();
}

void
TrafficGenerator::StopApplication (void) {
    m_running = false;

    if (m_sendEvent.IsRunning ())
        Simulator::Cancel (m_sendEvent);

    if (m_socket)
        m_socket->Close ();
}

void
TrafficGenerator::SendPacket (void) {
    for (uint32_t i = 0; i < m_burstPktNum; i += 1) {
        Ptr<Packet> packet = Create<Packet> (m_packetSize);
        if (m_socket->Send (packet) == -1) {
            NS_LOG_INFO("TxBuffer full, stop sending traffic");
            break;
        }
    }
    ScheduleTx ();
}

void
TrafficGenerator::ScheduleTx (void) {
    if (m_running) {
        Time tNext (Seconds (m_burstItvSec));
        m_sendEvent = Simulator::Schedule (tNext, &TrafficGenerator::SendPacket, this);
    }
}