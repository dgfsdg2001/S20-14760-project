#ifndef NETWORK_PARAMS_H
#define NETWORK_PARAMS_H

#include "ns3/core-module.h"

class Parameters {
public:
    double endTime;

    //Traffic
    uint16_t UseTCP;
    uint64_t packetSize;
    uint64_t burstPktNum;
    double burstItvSec;

    // CSMA 
    uint64_t delayLteServer;
    uint64_t delayWifiServer;
    uint64_t errateLteServer;  // uniform distribution
    uint64_t errateWifiServer; // uniform distribution

    Parameters() {
        UseTCP = 0;
        delayLteServer = 2;
        delayWifiServer = 2;
        errateLteServer = 1;
        errateWifiServer = 1;
        packetSize = 1464;
        burstPktNum = 1;
        burstItvSec = 0.5;
        endTime = 6.0;
    }
};


Parameters parseArgsToParams(int argc, char** argv);
std::string parmsToJSONObj();

#endif