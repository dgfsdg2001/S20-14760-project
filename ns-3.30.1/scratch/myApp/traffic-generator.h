#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

class TrafficGenerator : public Application
{
public:
    TrafficGenerator ();
    virtual ~TrafficGenerator ();

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId (void);
    void Setup (
        Ptr<Socket> socket,
        Address address,
        uint32_t packetSize,
        uint32_t burstPktNum,
        double burstItvSec
    );

private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void ScheduleTx (void);
    void SendPacket (void);

    Ptr<Socket>     m_socket;
    Address         m_peer;
    uint32_t        m_packetSize;
    uint32_t        m_burstPktNum;
    double          m_burstItvSec;
    EventId         m_sendEvent;
    bool            m_running;
    uint32_t        m_packetsSent;
};