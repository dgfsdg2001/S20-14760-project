#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/mobility-helper.h"
#include "monitorApplication.h"
#include "myJson.h"
#include <utility>

using namespace ns3;

enum NETWORK_NODE {
    MOBILE = 0,
    LTE_BASESTATION = 1,
    WIFI_AP = 2,
    SERVER_NODE = 3,
    TOAL_NODES = 4
};


AnimationInterface *pAnim = 0;

NS_LOG_COMPONENT_DEFINE ("myApp");

std::pair<Ipv4Address, Ipv4Address> createCsmaNetwork(
    const Parameters &params,
    const Ptr<Node> n1, const Ptr<Node> n2,
    std::string network, std::string mask,
    uint64_t delay, uint64_t error_rate)
{
    NodeContainer nodes(n1, n2);
    Ptr<CsmaChannel> channel = CreateObjectWithAttributes<CsmaChannel> (
      "DataRate", DataRateValue (DataRate (100*1000*1000)), //bps
      "Delay", TimeValue (MilliSeconds (delay)));

    CsmaHelper csma;
    csma.SetDeviceAttribute ("EncapsulationMode", StringValue ("Llc"));
    NetDeviceContainer devs = csma.Install (nodes, channel);
    
    // Apply a error model to an uniform distribution 
    Ptr<RateErrorModel> em1 =
        CreateObjectWithAttributes<RateErrorModel> (
        "ErrorRate", DoubleValue (error_rate/100.0),
        "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET)
        );
    devs.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));
    devs.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));

    Ipv4AddressHelper ip;
    ip.SetBase (Ipv4Address(network.c_str()), Ipv4Mask(mask.c_str()));
    Ipv4InterfaceContainer addresses = ip.Assign (devs);
    return std::make_pair(addresses.GetAddress(0), addresses.GetAddress(1));
}

std::pair<Ipv4Address, Ipv4Address> createWifiNetwork(
    const Parameters &params,
    const Ptr<Node> sta, const Ptr<Node> ap,
    std::string network, std::string mask)
{
    double distance = 1.0; //meter
    double frequency = 5.0; // GHz
    uint8_t nStreams = 1;

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel.Create ());

    // Set MIMO capabilities
    phy.Set ("Antennas", UintegerValue (nStreams));
    phy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (nStreams));
    phy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (nStreams));

    WifiHelper wifi;
    WifiMacHelper mac;

    StringValue ctrlRate;
    if (frequency == 5.0) {
        wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
        ctrlRate = StringValue ("OfdmRate24Mbps");
    } else if (frequency == 2.4) {
        wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
        Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (40.046));
        ctrlRate = StringValue ("ErpOfdmRate24Mbps");
    }
    else {
        std::cout << "Wrong frequency value!" << std::endl;
        return std::make_pair(nullptr, nullptr);
    }
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                        "DataMode", StringValue ("HtMcs7"), // TODO: Can have more support rate?
                        "ControlMode", ctrlRate);

    // BW20? BW40?
    // Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (40));
    // Set guard interval //TODO SGI?
    // Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue (shortGuardInterval));


    Ssid ssid = Ssid ("ns3-80211n");
    mac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid));
    NetDeviceContainer staDevice = wifi.Install (phy, mac, sta);
    mac.SetType ("ns3::ApWifiMac",
                       "Ssid", SsidValue (ssid));
    NetDeviceContainer apDevice  = wifi.Install (phy, mac, ap);

    // mobility.
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));
    positionAlloc->Add (Vector (distance, 0.0, 0.0));
    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (ap);
    mobility.Install (sta);

    Ipv4AddressHelper ip;
    ip.SetBase (Ipv4Address(network.c_str()), Ipv4Mask(mask.c_str()));
    Ipv4InterfaceContainer ipSta = ip.Assign (staDevice);
    Ipv4InterfaceContainer ipAp = ip.Assign (apDevice);
    return std::make_pair(ipSta.GetAddress(0), ipAp.GetAddress(0));
}

int main(int argc, char** argv) {

    Parameters params = parseArgsToParams(argc, argv);

    // Here, we will create TOTAL_NODES for network topology
    NS_LOG_INFO ("Create nodes.");
    NodeContainer allNodes;
    allNodes.Create(NETWORK_NODE::TOAL_NODES);
    NodeContainer mobileLte = NodeContainer(
        allNodes.Get(NETWORK_NODE::MOBILE), allNodes.Get(NETWORK_NODE::LTE_BASESTATION));
    NodeContainer mobileWifi = NodeContainer(
        allNodes.Get(NETWORK_NODE::MOBILE), allNodes.Get(NETWORK_NODE::WIFI_AP));

    // Set position of nodes
    AnimationInterface::SetConstantPosition (allNodes.Get(NETWORK_NODE::MOBILE), 30, 30);
    AnimationInterface::SetConstantPosition (allNodes.Get(NETWORK_NODE::WIFI_AP), 50, 10);
    AnimationInterface::SetConstantPosition (allNodes.Get(NETWORK_NODE::LTE_BASESTATION), 50, 50);
    AnimationInterface::SetConstantPosition (allNodes.Get(NETWORK_NODE::SERVER_NODE), 70, 30);

    // Create the animation object and configure for specified output
    pAnim = new AnimationInterface ("myApp.xml");
    pAnim->UpdateNodeDescription(allNodes.Get(NETWORK_NODE::MOBILE), "Mobile Device");
    pAnim->UpdateNodeDescription(allNodes.Get(NETWORK_NODE::WIFI_AP), "Wi-Fi AP");
    pAnim->UpdateNodeDescription(allNodes.Get(NETWORK_NODE::LTE_BASESTATION), "LTE Base Station");
    pAnim->UpdateNodeDescription(allNodes.Get(NETWORK_NODE::SERVER_NODE), "Server");

    // Install network stacks on the nodes
    InternetStackHelper internet;
    internet.Install (allNodes);

    // Create Network
    NS_LOG_INFO ("Create CSMA network between Wi-Fi AP and server.");
    std::pair<Ipv4Address, Ipv4Address> ipsWiFiServer = createCsmaNetwork(
        params,
        allNodes.Get(NETWORK_NODE::WIFI_AP), allNodes.Get(NETWORK_NODE::SERVER_NODE),
        "10.2.1.0", "255.255.255.0",
        params.delayWifiServer, params.errateWifiServer);

    NS_LOG_INFO ("Create CSMA network between LTE and server.");
    std::pair<Ipv4Address, Ipv4Address> ipsLteServer = createCsmaNetwork(
        params,
        allNodes.Get(NETWORK_NODE::LTE_BASESTATION), allNodes.Get(NETWORK_NODE::SERVER_NODE),
        "10.3.1.0", "255.255.255.0",
        params.delayLteServer, params.errateLteServer);
    NS_UNUSED(ipsLteServer);

    NS_LOG_INFO ("Create Wi-Fi network between mobile and Wi-Fi AP.");
    std::pair<Ipv4Address, Ipv4Address> ipsMobileWifi = createWifiNetwork(
        params,
        allNodes.Get(NETWORK_NODE::MOBILE), allNodes.Get(NETWORK_NODE::WIFI_AP),
        "10.1.2.0", "255.255.255.0");
    NS_UNUSED(ipsMobileWifi);

    NS_LOG_INFO ("Create point-2-point network between mobile and LTE.");
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer devMobileLte = p2p.Install (mobileLte);

    // Later, we add IP addresses.
    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;

    ipv4.SetBase ("10.1.3.0", "255.255.255.0");
    ipv4.Assign (devMobileLte);

    //Turn on global static routing
    NS_LOG_INFO ("Enable global static routing.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


    NS_LOG_INFO ("Create traffic source & sink.");
    Ptr<Node> appSource = allNodes.Get(MOBILE);
    Ptr<Sender> sender = CreateObject<Sender>();
    appSource->AddApplication (sender);
    sender->SetStartTime (Seconds (1));
    sender->SetStopTime(Seconds (params.endTime));
    sender->SetAttribute("Destination",  Ipv4AddressValue(ipsWiFiServer.second));
    sender->SetAttribute("UseTCP",  UintegerValue(params.UseTCP));
    sender->SetAttribute("PacketSize",  UintegerValue(params.packetSize));
    sender->SetAttribute("PacketNum",  UintegerValue(params.burstPktNum));
    std::stringstream ss;
    
    ss << "ns3::ConstantRandomVariable[Constant=" << params.burstItvSec << "]"; 

    sender->SetAttribute("Interval", StringValue(ss.str()));

    Ptr<Node> appSink = allNodes.Get(SERVER_NODE);
    Ptr<Receiver> receiver = CreateObject<Receiver>();
    appSink->AddApplication (receiver);
    receiver->SetStartTime (Seconds (1));
    receiver->SetStopTime(Seconds (params.endTime));
    receiver->SetAttribute("UseTCP",  UintegerValue(params.UseTCP));

    // [Shaomin] Maybe useful in future?
    //localSocket->SetAttribute("SndBufSize", UintegerValue(4096));
    //Ask for ASCII and pcap traces of network traffic
    // AsciiTraceHelper ascii;
    // p2p.EnableAsciiAll (ascii.CreateFileStream ("tcp-large-transfer.tr"));
    // p2p.EnablePcapAll ("tcp-large-transfer");
    Simulator::Stop (Seconds (params.endTime + 5));

    // Provide the absolute path to the resource
    // [Shaomin] Maybe useful in future
    // uint32_t (global) resourceId1 = pAnim->AddResource ("/Users/john/ns3/netanim-3.105/ns-3-logo1.png");
    // uint32_t (global) resourceId2 = pAnim->AddResource ("/Users/john/ns3/netanim-3.105/ns-3-logo2.png");
    // pAnim->SetBackgroundImage ("/Users/john/ns3/netanim-3.105/ns-3-background.png", 0, 0, 0.2, 0.2, 0.1);

    // Install FlowMonitor on all nodes
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll ();

    // Trace routing tables
    // Ipv4GlobalRoutingHelper g;
    // Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("myApp.routes", std::ios::out);
    // g.PrintRoutingTableAllAt (Seconds (0), routingStream);

    Simulator::Run ();
    std::cout << "Animation Trace file created:" << "myApp.xml" << std::endl;

    // Save FlowMonitor results to file
    std::cout << "Statistical results file created:" << "myApp.flowmon.xml" << std::endl;
    flowmonHelper.SerializeToXmlFile ("myApp.flowmon.xml", true, true);
    

#if 0 // Not necessary
    // Print per flow statistics
    monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        std::cout << "*IP* Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  TxOffered:  " << i->second.txBytes * 1.0 / 1000 / 1000  << " Mbps\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  Throughput: " << i->second.rxBytes * 1.0 / 1000 / 1000  << " Mbps\n";
    }
#endif

    std::cout << "Ooutput simulation results to myApp.appStats.json" << std::endl;
    std::ofstream os ("myApp.appStats.json", std::ios::out|std::ios::binary);
    jsonObjStart(os);
    jsonObjAdd(os, "params", parmsToJSONObj());
    jsonObjAdd(os, "stats", monStatsToJSONObj());
    jsonObjEnd(os);
    os.close();

    Simulator::Destroy ();
    delete pAnim;
    return 0;
}
