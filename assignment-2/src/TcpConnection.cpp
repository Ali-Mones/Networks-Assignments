#include "TcpConnection.h"

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <random>

#include "Packets.h"

using namespace std;

TcpConnection::TcpConnection(sockaddr_in clientAddress, string filepath, float plp)
    : m_ClientAddress(clientAddress), m_ClientAddressLen(sizeof(m_ClientAddress)),
    m_PLP(plp),
    m_File(ifstream(filepath, ios::ate)), m_FileSize(m_File.tellg()),
    m_Socket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)),
    m_SendBase(0), m_NextSeqNo(0),
    m_State(TcpCongestionState::SlowStart),
    m_Mss(sizeof(Packet::data)), m_Cwnd(m_Mss), m_DuplicateAckCount(0), m_Ssthresh(64 * 1024)
{
    srand(time(0));
    m_File.seekg(0);
}

void TcpConnection::HandleConnection()
{
    // send first packet
    uint32_t remainingSize = m_FileSize - m_File.tellg();

    Packet* p = new Packet();
    p->len = min(remainingSize, min(m_Cwnd + m_SendBase - m_NextSeqNo, (uint32_t)sizeof(Packet::data)));
    p->seqno = 0;

    m_UnAckedPackets.push_back(p);

    m_File.read((char*)&p->data, p->len);

    if ((float)rand() / RAND_MAX > m_PLP)
        sendto(m_Socket, p, sizeof(Packet), 0, (sockaddr*)&m_ClientAddress, m_ClientAddressLen);

    m_NextSeqNo += p->len;

    chrono::time_point timer = chrono::system_clock::now();

    while (!m_UnAckedPackets.empty())
    {
        AckPacket ack;
        recvfrom(m_Socket, &ack, sizeof(ack), MSG_DONTWAIT, (sockaddr*)&m_ClientAddress, &m_ClientAddressLen);

        if (ack.ackno != 0)
        {
            cout << "Ack received! - " << ack.ackno << endl;

            // ack received
            Packet* x = m_UnAckedPackets.front();

            while (x && x->seqno < ack.ackno)
            {
                delete x;

                m_UnAckedPackets.pop_front();

                if (m_UnAckedPackets.size() == 0)
                    break;

                x = m_UnAckedPackets.front();
            }


            uint32_t oldSendBase = m_SendBase;
            m_SendBase = max(m_SendBase, ack.ackno);

            if (ack.ackno > oldSendBase)
                HandleNewAck();
            else
                HandleDuplicateAck();

            if (m_NextSeqNo > m_SendBase)
            {
                // start timer
                timer = chrono::system_clock::now();
            }
        }

        chrono::time_point now = chrono::system_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(now - timer) > chrono::milliseconds(10))
        {
            // handle timeout
            cout << "timed out!" << endl;

            HandleTimeout();
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
        TransmitNewPacket();
    }
}

void TcpConnection::HandleDuplicateAck()
{
    cout << "Handling duplicate ack!" << endl;
    if (m_State == TcpCongestionState::SlowStart || m_State == TcpCongestionState::CongestionAvoidance)
    {
        m_DuplicateAckCount++;

        if (m_DuplicateAckCount == 3)
        {
            m_Ssthresh = m_Cwnd / 2;
            m_Cwnd = m_Ssthresh + 3 * m_Mss;

            // retransmit missing segment
            RetransmitMissingPacket();

            m_State = TcpCongestionState::FastRecovery;
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
    cout << "Sending new packet!" << endl;

    while (true)
    {
        uint32_t remainingSize = m_FileSize - m_File.tellg();
        cout << remainingSize << " bytes remaining!" << endl;

        if (remainingSize == 0)
            return;

        Packet* p = new Packet();
        p->len = min(remainingSize, min(m_Cwnd + m_SendBase - m_NextSeqNo, (uint32_t)sizeof(Packet::data)));
        p->seqno = m_NextSeqNo;

        if (p->len == 0 || m_SendBase + m_Cwnd < m_NextSeqNo + p->len)
        {
            delete p;
            break;
        }

        m_UnAckedPackets.push_back(p);
        
        m_File.read((char*)&p->data, p->len);
        // cout << "Sending " << p->data << endl;

        if ((float)rand() / RAND_MAX > m_PLP)
            sendto(m_Socket, p, sizeof(Packet), 0, (sockaddr*)&m_ClientAddress, m_ClientAddressLen);

        m_NextSeqNo += p->len;
    }
}

void TcpConnection::RetransmitMissingPacket()
{
    cout << "Retransmitting missing packet!" << endl;
    Packet* p = m_UnAckedPackets.front();

    if ((float)rand() / RAND_MAX > m_PLP)
        sendto(m_Socket, p, sizeof(Packet), 0, (sockaddr*)&m_ClientAddress, m_ClientAddressLen);
}
