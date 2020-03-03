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
```shell
./waf --run myApp
```

Run myApp with gdb
```shell
./waf --run myApp --command-template="gdb %s"
```

Enable logs
Refer to the website for more details.
https://www.nsnam.org/docs/manual/html/logging.html#logging
```shell
NS_LOG="myApp" ./waf --run myApp
```
