#pragma once

#include <cstdint>
#include <arpa/inet.h>
#include <string>
#include <fstream>
#include <deque>

#include "Packets.h"

using namespace std;

enum class TcpCongestionState
{
    SlowStart,
    CongestionAvoidance,
    FastRecovery
};

class TcpConnection
{
public:
    TcpConnection(sockaddr_in clientAddress, string filepath);
    void HandleConnection();
private:
    void HandleNewAck();
    void HandleDuplicateAck();
    void HandleTimeout();
    void TransmitNewPacket();
    void RetransmitMissingPacket();
private:
    TcpCongestionState m_State;

    int m_Socket;
    sockaddr_in m_ClientAddress;
    socklen_t m_ClientAddressLen;

    ifstream m_File;
    size_t m_FileSize;

    uint32_t m_SendBase;
    uint32_t m_NextSeqNo;
    deque<Packet*> m_UnAckedPackets;

    uint32_t m_Mss;
    uint32_t m_Cwnd;
    uint32_t m_DuplicateAckCount;
    uint32_t m_Ssthresh;
};