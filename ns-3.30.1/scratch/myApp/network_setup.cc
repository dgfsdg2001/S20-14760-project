#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "traffic-generator.h"

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
#define START_SIMULATION_TIEM_SEC 0.0
#define END_SIMULATION_TIME_SEC 5.0

AnimationInterface * pAnim = 0;

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

    // Set position of nodes
    AnimationInterface::SetConstantPosition (allNodes.Get(MOBILE_NODE), 30, 30);
    AnimationInterface::SetConstantPosition (allNodes.Get(WIFI_AP_NODE), 50, 10);
    AnimationInterface::SetConstantPosition (allNodes.Get(LTE_BASESTATION_NDOE), 50, 50);
    AnimationInterface::SetConstantPosition (allNodes.Get(SERVER_NODE), 70, 30);

    // Install network stacks on the nodes
    InternetStackHelper internet;
    internet.Install (allNodes);

    // We create the channels first without any IP addressing information
    NS_LOG_INFO ("Create channels.");
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer devMobileWifi = p2p.Install (mobileWifi);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer devMobileLte = p2p.Install (mobileLte);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer devWifiServer = p2p.Install (wifiServer);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer devLteServer = p2p.Install (lteServer);

    // Later, we add IP addresses.
    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;

    ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    
    ipv4.SetBase ("10.1.3.0", "255.255.255.0");
    ipv4.Assign (devMobileLte);

    ipv4.SetBase ("10.2.1.0", "255.255.255.0");
    ipv4.Assign (devWifiServer);
    Ipv4InterfaceContainer ipWifiServer = ipv4.Assign (devWifiServer);

    ipv4.SetBase ("10.3.1.0", "255.255.255.0");
    ipv4.Assign (devLteServer);

    //Turn on global static routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Create a packet sink at server to receive packets
    uint16_t tg_port = 33456;
    PacketSinkHelper pktSinkHelperServer ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), tg_port));
    ApplicationContainer appServer = pktSinkHelperServer
                        .Install (allNodes.Get(SERVER_NODE));
    appServer.Start (Seconds (START_SIMULATION_TIEM_SEC));
    appServer.Stop (Seconds (END_SIMULATION_TIME_SEC));

    // Create client application to send packets
    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (
            allNodes.Get(MOBILE_NODE), TcpSocketFactory::GetTypeId ());
    Ptr<TrafficGenerator> appClient = CreateObject<TrafficGenerator> ();
    appClient->Setup (
        ns3TcpSocket,
        InetSocketAddress (ipWifiServer.GetAddress (1), tg_port),
        1040,
        100,
        0.05
    );
    allNodes.Get(MOBILE_NODE)->AddApplication(appClient);
    appClient->SetStartTime (Seconds (START_SIMULATION_TIEM_SEC));
    appClient->SetStopTime (Seconds (END_SIMULATION_TIME_SEC));

    // [Shaomin] Maybe useful in future?
    //localSocket->SetAttribute("SndBufSize", UintegerValue(4096));
    //Ask for ASCII and pcap traces of network traffic
    // AsciiTraceHelper ascii;
    // p2p.EnableAsciiAll (ascii.CreateFileStream ("tcp-large-transfer.tr"));
    // p2p.EnablePcapAll ("tcp-large-transfer");
    Simulator::Stop (Seconds (END_SIMULATION_TIME_SEC + 1));

    // Create the animation object and configure for specified output
    pAnim = new AnimationInterface ("myApp.xml");
    pAnim->UpdateNodeDescription(allNodes.Get(MOBILE_NODE), "Mobile Device");
    pAnim->UpdateNodeDescription(allNodes.Get(WIFI_AP_NODE), "Wi-Fi AP");
    pAnim->UpdateNodeDescription(allNodes.Get(LTE_BASESTATION_NDOE), "LTE Base Station");
    pAnim->UpdateNodeDescription(allNodes.Get(SERVER_NODE), "Server");

    // Provide the absolute path to the resource
    // [Shaomin] Maybe useful in future
    // uint32_t (global) resourceId1 = pAnim->AddResource ("/Users/john/ns3/netanim-3.105/ns-3-logo1.png");
    // uint32_t (global) resourceId2 = pAnim->AddResource ("/Users/john/ns3/netanim-3.105/ns-3-logo2.png");
    // pAnim->SetBackgroundImage ("/Users/john/ns3/netanim-3.105/ns-3-background.png", 0, 0, 0.2, 0.2, 0.1);

    // Install FlowMonitor on all nodes
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    // Trace routing tables
    // Ipv4GlobalRoutingHelper g;
    // Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("myApp.routes", std::ios::out);
    // g.PrintRoutingTableAllAt (Seconds (0), routingStream);

    Simulator::Run ();
    std::cout << "Animation Trace file created:"
                << "myApp.xml" << std::endl;

    // Print per flow statistics
    monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        std::cout << "Flow " << i->first - 2 << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  lostPackets:" << i->second.lostPackets << "\n";
        std::cout << "  TxOffered:  " << i->second.txBytes * 1.0 / 1000 / 1000  << " Mbps\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  Throughput: " << i->second.rxBytes * 1.0 / 1000 / 1000  << " Mbps\n";
    }

    Simulator::Destroy ();
    delete pAnim;
    return 0;
}
