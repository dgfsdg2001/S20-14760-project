
# Network Topology
The ip addresses use the same mask 255.255.255.0.
```text
   ----lteBaseStation---- 
   |                    |
   | (10.1.2.0)         | (10.2.1.0)
   |                    |
mobile                 server
   |                    |
   | (10.1.3.0)         | (10.3.1.0)
   |                    |
   ------ wifiApNode---
```

The mobile device sends either UDP or TCP traffic to server through Wi-Fi connection.

## TODO
1. Flow Monitor is designed for IP-based statistic. Our project needs application-based statistical results. Need to track packets in application
module. Refer to ns-3.30.1/examples/stats/wifi-example-apps.cc
2. Replace point-to-point to lte channel model for mobile-LteBaseStation.


## Config ns3
```shell
./waf clean
./waf configure --build-profile=optimized --enable-examples --enable-tests
./waf configure --build-profile=debug --enable-examples --enable-tests
```

## Build ns3
```shell
./waf
./waf --check-profile
```

## Build & Run myApp
https://www.nsnam.org/docs/manual/html/logging.html#logging
```shell
# Run TCP traffic
NS_LOG="myApp" ./waf --run "myApp"

# Run UDP traffic
NS_LOG="myApp" ./waf --run "myApp --isUdp=1"

# Enable log of InetSocketAddress
# Notice that only logs are only printed under debug configuration.
NS_LOG="myApp:myApp-tg:<ComponentName>" ./waf --run "myApp"

# Run with gdb
./waf --run "myApp" --command-template="gdb %s"
```

| Arguments        | Default   | 
| -----------------|:---------:| 
| startTime        | 0         |
| endTime          | 5         |
| isUdp            | 0         | 
| packetSize       | 1040      | 
| burstPktNum      | 3         | 
| burstItvSec      | 0.1 sec   |
| delayLteServer   | 2 ms      |
| errateLteServer  | 1 %       |
| delayWifiServer  | 2 ms      |
| errateWifiServer | 1 %       |


* These components may be useful for debugging:
 - TcpSocketBase
```
TcpSocketBase
+0.000000000s -1  [node 0] TcpSocketBase:AddOptions(0x55a3bfbc2960, 49153 > 50000 [SYN] Seq=0 Ack=0 Win=65535)
+0.000000000s -1  [node 0] TcpSocketBase:Send(): [LOGIC] txBufSize=131040 state SYN_SENT
+0.000000000s 3  [node 3] TcpSocketBase:Listen(): [DEBUG] CLOSED -> LISTEN
+3.004160000s 0  [node 0] TcpSocketBase:DoForwardUp(): [LOGIC] At state SYN_SENT received packet of seq [0:0) without TS option. Silently discard it
.....
```

## Result Analysis
### Flow Monitor (**IP-based**)
https://www.nsnam.org/docs/models/html/flow-monitor.html

**MPTCP locates at transportation layer. The IP-based statistical result is only for reference.**


* Statistical results are shown on the std output. 
```
Flow 1 (10.1.2.1 -> 10.2.1.2)
  Tx Packets: 150
  Tx Bytes:   160200
  lostPackets:0
  TxOffered:  0.1602 Mbps
  Rx Packets: 143
  Rx Bytes:   152724
  Throughput: 0.152724 Mbps
```

* Or use `flowmon-parse-results.py` to get delay/jitter histogram
```shell
python3 ns-3.30.1/scratch/myApp/flowmon-parse-results.py myApp.flowmon.xml

...
Delay Histogram
(0.0020-0.0030): 47
(0.0040-0.0050): 40
(0.0050-0.0060): 7
(0.0060-0.0070): 16
(0.0070-0.0080): 30
(0.0220-0.0230): 1
(0.0250-0.0260): 1
(0.0270-0.0280): 1

Jitter Histogram
(0.0020-0.0030): 94
(0.0030-0.0040): 2
(0.0040-0.0050): 26
(0.0050-0.0060): 19
(0.0250-0.0260): 1
```

Delay: End-to-End delay for a received packet

Jitter: Delay variation ([RFC3393](https://tools.ietf.org/html/rfc3393.html))


### NetAnim
https://www.nsnam.org/docs/models/html/animation.html

**Build NetAnim**
```shell
sudo apt-get install qt5-default
cd netanim-3.108
make clean
qmake NetAnim.pro
make -j8
```

**Run NetAnim and load the 'ns-3.30.1/myApp.xml'**
```shell
./NetAnim
```

## Modifty behavior of traffic generator
```cpp
@network_setup.cc
appClient->Setup (
    ns3Socket,
    InetSocketAddress (ipWifiServer.GetAddress (1), tg_port),
    1040,  // Packet Size
    100,   // Number of packet sent per burst
    0.05   // Burst interval in seocnd
);
```
