#ifndef _MONITOR_APPLICATION_H_
#define _MONITOR_APPLICATION_H_

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/application.h"
#include "ns3/stats-module.h"
#include "network-params.h"

using namespace ns3;


struct PAYLOAD {
    uint32_t size;
    uint64_t app_sn;
    int64_t timestamp_ms;
};

//----------------------------------------------------------------------
//-- Sender
//----------------------------------------------------------------------
class Sender : public Application {
public:
  static TypeId GetTypeId (void);
  Sender();
  virtual ~Sender();

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void SendPacket ();

  uint32_t        m_pktSize;
  uint32_t        m_pktNum;
  Ipv4Address     m_destAddr;
  uint32_t        m_destPort;
  Ptr<ConstantRandomVariable> m_interval;

  Ptr<Socket>     m_socket;
  EventId         m_sendEvent;

  TracedCallback<Ptr<const Packet> > m_txTrace;

  uint8_t         m_useTCP;
  uint8_t         *m_buf;
  struct PAYLOAD  payload_temp;
};


//----------------------------------------------------------------------
//-- Receiver
//----------------------------------------------------------------------
class Receiver : public Application {
public:
  static TypeId GetTypeId (void);
  Receiver();
  virtual ~Receiver();

  void SetCounter (Ptr<CounterCalculator<> > calc);
  void SetDelayTracker (Ptr<TimeMinMaxAvgTotalCalculator> delay);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void Receive (Ptr<Socket> socket);

  Ptr<Socket>     m_socket;

  uint32_t        m_port;

  Ptr<CounterCalculator<> > m_calc;
  Ptr<TimeMinMaxAvgTotalCalculator> m_delay;

  uint8_t         m_useTCP;
  uint32_t        m_segBytes;
};

std::string monStatsToJSONObj();

#endif // _MONITOR_APPLICATION_H_