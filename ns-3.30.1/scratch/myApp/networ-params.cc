#include "ns3/core-module.h"
#include "ns3/command-line.h"
#include "network-params.h"
#include "myJson.h"

using namespace ns3;


std::stringstream ss;

Parameters params;
Parameters parseArgsToParams(int argc, char** argv) {
    CommandLine cmd;
    cmd.AddValue ("endTime", "Time to stop sending traffic",
        params.endTime);
    cmd.AddValue ("UseTCP", "1 for TCP, 0 for UDP",
        params.UseTCP);
    cmd.AddValue ("packetSize", "Size of Packet sent by traffic generator",
        params.packetSize);
    cmd.AddValue ("burstPktNum", "Number of packet sent by traffic generator at one burst",
        params.burstPktNum);
    cmd.AddValue ("burstItvSec", "Burst interval(sec) of traffic generator",
        params.burstItvSec);
    cmd.AddValue ("delayLteServer", "Delay(ms) between Lte and server",
        params.delayLteServer);
    cmd.AddValue ("delayWifiServer", "Delay(ms) between WiFi and server",
        params.delayWifiServer);
    cmd.AddValue ("errateLteServer", "Error rate between Lte and server [0 - 100]",
        params.errateLteServer);
    cmd.AddValue ("errateWifiServer", "Error rate between WiFi and server [0 - 100]",
        params.errateWifiServer);
    cmd.Parse (argc, argv);

    if (params.packetSize > 1464) {
        std::cerr << "Packet size including TCP/IP header (36) should not exceed MTU of CSMA(1500)" << std::endl;
        exit(-1);
    }
    std::cout << parmsToJSONObj() << std::endl;

    return params;
}

std::string parmsToJSONObj() {
    std::stringstream ss;
    jsonObjStart(ss);
    jsonObjAdd(ss, "UseTCP", params.UseTCP? "true":"false");
    jsonObjAdd(ss, "packetSize", params.packetSize);
    jsonObjAdd(ss, "burstItvSec", params.burstItvSec/1.0);
    jsonObjAdd(ss, "burstPktNum", params.burstPktNum);
    jsonObjAdd(ss, "delayLteServer", params.delayLteServer/1000.0);
    jsonObjAdd(ss, "errateLteServer", params.errateLteServer/100.0);
    jsonObjAdd(ss, "delayWifiServer", params.delayWifiServer/1000.0);
    jsonObjAdd(ss, "errateWifiServer", params.errateWifiServer/100.0);
    jsonObjAdd(ss, "endTime", params.endTime/1.0);
    jsonObjEnd(ss);
    return ss.str();
} 
    