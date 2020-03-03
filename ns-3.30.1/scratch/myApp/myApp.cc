#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;
//
// Network topology (v0.1)
//
//        lteBaseStation 
//        ,         `
//  mobile          server
//        `         , 
//         wifiApNode 

#define TOTAL_NODES 4
#define MOBILE_NODE 0
#define LTE_BASESTATION_NDOE 1
#define WIFI_AP_NODE 2
#define SERVER_NODE 3
#define START_SEC 0.0
#define END_SEC 10.0

// The number of bytes to send in this simulation.
static const uint32_t totalTxBytes = 2000000;
static uint32_t currentTxBytes = 0;
// Perform series of 1040 byte writes (this is a multiple of 26 since
// we want to detect data splicing in the output stream)
static const uint32_t writeSize = 1040;
uint8_t data[writeSize];

// These are for starting the writing process, and handling the sending 
// socket's notification upcalls (events).  These two together more or less
// implement a sending "Application", although not a proper ns3::Application
// subclass.
void StartFlow (Ptr<Socket>, Ipv4Address, uint16_t);
void WriteUntilBufferFull (Ptr<Socket>, uint32_t);

NS_LOG_COMPONENT_DEFINE ("myApp");

int main(int argc, char** argv) {
    // Here, we will create TOTAL_NODES for network topology
    NS_LOG_INFO ("Create nodes.");
    NodeContainer allNodes;
    allNodes.Create(TOTAL_NODES);
    NodeContainer mobileLte = NodeContainer(
        allNodes.Get(MOBILE_NODE), allNodes.Get(LTE_BASESTATION_NDOE));
    NodeContainer mobileWifi = NodeContainer(
        allNodes.Get(MOBILE_NODE), allNodes.Get(WIFI_AP_NODE));
    NodeContainer lteServer = NodeContainer(
        allNodes.Get(LTE_BASESTATION_NDOE), allNodes.Get(SERVER_NODE));
    NodeContainer wifiServer = NodeContainer(
        allNodes.Get(WIFI_AP_NODE), allNodes.Get(SERVER_NODE));

    // Install network stacks on the nodes
    InternetStackHelper internet;
    internet.Install (allNodes);

    // We create the channels first without any IP addressing information
    NS_LOG_INFO ("Create channels.");
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer devMobileLte = p2p.Install (mobileLte);
    
    p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer devMobileWifi = p2p.Install (mobileWifi);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer devLteServer = p2p.Install (lteServer);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer devWifiServer = p2p.Install (wifiServer);

    // Later, we add IP addresses.
    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    
    ipv4.SetBase ("10.2.1.0", "255.255.255.0");
    ipv4.Assign (devMobileWifi);

    ipv4.SetBase ("10.3.1.0", "255.255.255.0");
    ipv4.Assign (devLteServer);
    Ipv4InterfaceContainer ipLteServer = ipv4.Assign (devMobileLte);

    ipv4.SetBase ("10.4.1.0", "255.255.255.0");
    ipv4.Assign (devWifiServer);
    Ipv4InterfaceContainer ipWifiServer = ipv4.Assign (devMobileLte);

    //Turn on global static routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Create a packet sink to receive these packets ...
    uint16_t servPort = 50000;
    PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), servPort));
    ApplicationContainer apps = sink.Install (allNodes.Get(SERVER_NODE));
    apps.Start (Seconds (START_SEC));
    apps.Stop (Seconds (END_SEC));

    // Create and bind the socket...
    Ptr<Socket> localSocket =
        Socket::CreateSocket (allNodes.Get(MOBILE_NODE), TcpSocketFactory::GetTypeId ());
    localSocket->Bind ();

    // Trace changes to the congestion window
    // [Shaomin] Maybe useful in future?
    // Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));

    // ...and schedule the sending "Application"; This is similar to what an 
    // ns3::Application subclass would do internally.
    Simulator::ScheduleNow (&StartFlow, localSocket,
                            ipWifiServer.GetAddress (1), servPort);

    // [Shaomin] Maybe useful in future?
    //localSocket->SetAttribute("SndBufSize", UintegerValue(4096));
    //Ask for ASCII and pcap traces of network traffic
    // AsciiTraceHelper ascii;
    // p2p.EnableAsciiAll (ascii.CreateFileStream ("tcp-large-transfer.tr"));
    // p2p.EnablePcapAll ("tcp-large-transfer");
    Simulator::Stop (Seconds (1000));
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//begin implementation of sending "Application"
void StartFlow (Ptr<Socket> localSocket,
                Ipv4Address servAddress,
                uint16_t servPort)
{
  NS_LOG_LOGIC ("Starting flow at time " <<  Simulator::Now ().GetSeconds ());
  localSocket->Connect (InetSocketAddress (servAddress, servPort)); //connect

  // tell the tcp implementation to call WriteUntilBufferFull again
  // if we blocked and new tx buffer space becomes available
  localSocket->SetSendCallback (MakeCallback (&WriteUntilBufferFull));
  WriteUntilBufferFull (localSocket, localSocket->GetTxAvailable ());
}

void WriteUntilBufferFull (Ptr<Socket> localSocket, uint32_t txSpace)
{
  while (currentTxBytes < totalTxBytes && localSocket->GetTxAvailable () > 0) 
    {
      uint32_t left = totalTxBytes - currentTxBytes;
      uint32_t dataOffset = currentTxBytes % writeSize;
      uint32_t toWrite = writeSize - dataOffset;
      toWrite = std::min (toWrite, left);
      toWrite = std::min (toWrite, localSocket->GetTxAvailable ());
      int amountSent = localSocket->Send (&data[dataOffset], toWrite, 0);
      if(amountSent < 0)
        {
          // we will be called again when new tx space becomes available.
          return;
        }
      currentTxBytes += amountSent;
    }
  localSocket->Close ();
}