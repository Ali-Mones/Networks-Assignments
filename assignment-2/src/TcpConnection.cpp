#include "TcpConnection.h"

#include <iostream>
#include <unistd.h>
#include <chrono>

#include "Packets.h"

using namespace std;

TcpConnection::TcpConnection(sockaddr_in clientAddress, string filepath)
    : m_ClientAddress(clientAddress), m_ClientAddressLen(sizeof(m_ClientAddress)),
    m_File(ifstream("files/" + filepath, ios::ate)), m_FileSize(m_File.tellg()),
    m_Socket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)),
    m_SendBase(0), m_NextSeqNo(0),
    m_State(TcpCongestionState::SlowStart),
    m_Mss(sizeof(Packet::data)), m_Cwnd(m_Mss), m_DuplicateAckCount(0), m_Ssthresh(64 * 1024)
{
    m_File.seekg(0);
}

void TcpConnection::HandleConnection()
{

    // send first packet
    uint32_t remainingSize = m_FileSize - m_File.tellg();

    Packet* p = new Packet();
    p->len = min(remainingSize, m_Cwnd + m_SendBase - m_NextSeqNo);
    p->seqno = 0;

    m_UnAckedPackets.push_back(p);

    m_File.read((char*)&p->data, p->len);

    // add probability to fail
    sendto(m_Socket, p, sizeof(Packet), 0, (sockaddr*)&m_ClientAddress, m_ClientAddressLen);

    m_NextSeqNo += p->len;

    chrono::time_point timer = chrono::system_clock::now();

    while (!m_UnAckedPackets.empty())
    {
        AckPacket ack;
        recvfrom(m_Socket, &ack, sizeof(ack), MSG_DONTWAIT, (sockaddr*)&m_ClientAddress, &m_ClientAddressLen);

        if (ack.ackno != 0)
        {
            // ack received
            Packet* p = m_UnAckedPackets.front();
            while (p && p->seqno < ack.ackno)
            {
                delete p;
                m_UnAckedPackets.pop_front();
                p = m_UnAckedPackets.front();
            }

            cout << "ackno = " << ack.ackno << endl;

            if (ack.ackno > m_SendBase)
                HandleNewAck();
            else
                HandleDuplicateAck();

            m_SendBase = max(m_SendBase, ack.ackno);
            if (m_NextSeqNo > m_SendBase)
            {
                // start timer
                timer = chrono::system_clock::now();
            }

            cout << "ackno = " << ack.ackno << endl;
            cout << "remaining packets = " << m_UnAckedPackets.size() << endl;
        }

        chrono::time_point now = chrono::system_clock::now();
        if (chrono::duration_cast<chrono::seconds>(now - timer) > chrono::seconds(4))
        {
            // handle timeout
            cout << "timed out!" << endl;
            cout << "send base = " << m_SendBase << endl;

            HandleTimeout();
            // Packet* p = m_UnAckedPackets.front();
            // sendto(m_Socket, p, sizeof(Packet), 0, (sockaddr*)&m_ClientAddress, m_ClientAddressLen);
            timer = chrono::system_clock::now();
        }
    }

    cout << "Closing connection!" << endl;

    Packet endPacket;
    endPacket.len = -1;
    sendto(m_Socket, &endPacket, sizeof(endPacket), 0, (sockaddr*)&m_ClientAddress, m_ClientAddressLen);

    close(m_Socket);
}

void TcpConnection::HandleNewAck()
{
    cout << "Handling new ack!" << endl;

    if (m_State == TcpCongestionState::SlowStart)
    {
        m_Cwnd += m_Mss;
        m_DuplicateAckCount = 0;

        // transmit new segment(s) as allowed
        TransmitNewPacket();

        if (m_Cwnd >= m_Ssthresh)
            m_State = TcpCongestionState::CongestionAvoidance;
    }
    else if (m_State == TcpCongestionState::CongestionAvoidance)
    {
        m_Cwnd += (float)m_Mss * m_Mss / m_Cwnd;
        m_DuplicateAckCount = 0;

        // transmit new segment(s) as allowed
        TransmitNewPacket();
    }
    else if (m_State == TcpCongestionState::FastRecovery)
    {
        m_Cwnd = m_Ssthresh;
        m_DuplicateAckCount = 0;
        m_State = TcpCongestionState::CongestionAvoidance;
    }
}

void TcpConnection::HandleDuplicateAck()
{
    if (m_State == TcpCongestionState::SlowStart || m_State == TcpCongestionState::CongestionAvoidance)
    {
        m_DuplicateAckCount++;

        if (m_DuplicateAckCount == 3)
        {
            m_Ssthresh = m_Cwnd / 2;
            m_Cwnd = m_Ssthresh + 3 * m_Mss;

            // retransmit missing segment
            RetransmitMissingPacket();
        }
    }
    else if (m_State == TcpCongestionState::FastRecovery)
    {
        m_Cwnd += m_Mss;

        // transmit new segment(s) as allowed
        TransmitNewPacket();
    }
}

void TcpConnection::HandleTimeout()
{
    m_Ssthresh = m_Cwnd / 2;
    m_Cwnd = m_Mss;
    m_DuplicateAckCount = 0;

    // retransmit missing segment
    RetransmitMissingPacket();

    m_State = TcpCongestionState::SlowStart;
}

void TcpConnection::TransmitNewPacket()
{
    cout << "Sending new packet" << endl;

    uint32_t remainingSize = m_FileSize - m_File.tellg();

    if (remainingSize == 0)
        return;

    Packet* p = new Packet();
    p->len = min(remainingSize, m_Cwnd + m_SendBase - m_NextSeqNo);
    p->seqno = m_NextSeqNo;

    m_UnAckedPackets.push_back(p);

    m_File.read((char*)&p->data, p->len);

    // add probability to fail
    sendto(m_Socket, p, sizeof(Packet), 0, (sockaddr*)&m_ClientAddress, m_ClientAddressLen);

    m_NextSeqNo += p->len;
}

void TcpConnection::RetransmitMissingPacket()
{
    cout << "Retransmitting packet!" << endl;

    Packet* p = m_UnAckedPackets.front();
    sendto(m_Socket, p, sizeof(Packet), 0, (sockaddr*)&m_ClientAddress, m_ClientAddressLen);
}
