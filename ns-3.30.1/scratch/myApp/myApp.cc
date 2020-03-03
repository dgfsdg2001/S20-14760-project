#include "ns3/core-module.h"
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MyApp");

int main(int argc, char** argv) {
    NS_LOG_UNCOND ("Hello" << "MyApp"); /* Only work in debug build */
    std::cout << "MyApp..." << std::endl;

    return 0;
}