Config ns3 
```shell
./waf clean
./waf configure --build-profile=optimized --enable-examples --enable-tests
./waf configure --build-profile=debug --enable-examples --enable-tests
```

Build ns3
```shell
./waf
./waf --check-profile
```

Build & Run myApp
https://www.nsnam.org/docs/manual/html/logging.html#logging
```shell
NS_LOG="myApp" ./waf --run myApp
NS_LOG="myApp:<ComponentName>" ./waf --run myApp # Enable InetSocketAddress log

# These components may be useful:
#   TcpSocketBase
```
TcpSocketBase
+0.000000000s -1  [node 0] TcpSocketBase:AddOptions(0x55a3bfbc2960, 49153 > 50000 [SYN] Seq=0 Ack=0 Win=65535)
+0.000000000s -1  [node 0] TcpSocketBase:Send(): [LOGIC] txBufSize=131040 state SYN_SENT
+0.000000000s 3  [node 3] TcpSocketBase:Listen(): [DEBUG] CLOSED -> LISTEN
+3.004160000s 0  [node 0] TcpSocketBase:DoForwardUp(): [LOGIC] At state SYN_SENT received packet of seq [0:0) without TS option. Silently discard it
.....


Run myApp with gdb
```shell
./waf --run myApp --command-template="gdb %s"
```

NetAnim
https://www.nsnam.org/docs/models/html/animation.html

Build NetAnim
```shell
sudo apt-get install qt5-default
cd netanim-3.108
make clean
qmake NetAnim.pro
make -j8
```

Run NetAnim and load the 'ns-3.30.1/myApp.xml'
```shell
./NetAnim
```