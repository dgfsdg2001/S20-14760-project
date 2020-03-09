
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
1. All connection all uses point-to-point netDevice. There is no overhead from link 
   layer and physical layer. Our plan is to user Wi-Fi and LTE model.

2. Add packet lost rate and latency for each network.

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
NS_LOG="myApp:<ComponentName>" ./waf --run "myApp"

# Run with gdb
./waf --run "myApp" --command-template="gdb %s"
```

These components may be useful for debugging:
1. TcpSocketBase
```
TcpSocketBase
+0.000000000s -1  [node 0] TcpSocketBase:AddOptions(0x55a3bfbc2960, 49153 > 50000 [SYN] Seq=0 Ack=0 Win=65535)
+0.000000000s -1  [node 0] TcpSocketBase:Send(): [LOGIC] txBufSize=131040 state SYN_SENT
+0.000000000s 3  [node 3] TcpSocketBase:Listen(): [DEBUG] CLOSED -> LISTEN
+3.004160000s 0  [node 0] TcpSocketBase:DoForwardUp(): [LOGIC] At state SYN_SENT received packet of seq [0:0) without TS option. Silently discard it
.....
```

## Result Analysis
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
