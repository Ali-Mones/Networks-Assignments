#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <vector>
#include <queue>
#include <unordered_set>

#include "Client.h"
#include "Packets.h"

unordered_set<int> map;

using namespace std;

Client::Client(const char* serverAddress, int serverPortNumber)
    : m_Socket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))
{
    if (m_Socket < 0)
    {
        cerr << "Failed to create socket" << endl;
        exit(1);
    }

    m_WelcomingServerAddress.sin_family = AF_INET;
    m_WelcomingServerAddress.sin_port = htons(serverPortNumber);
    m_WelcomingServerAddress.sin_addr.s_addr = inet_addr(serverAddress);
}

Client::~Client()
{
    close(m_Socket);
}

void Client::RequestFile(string filepath)
{
    if (sendto(m_Socket, filepath.c_str(), filepath.size(), 0, (struct sockaddr*)&m_WelcomingServerAddress, sizeof(m_WelcomingServerAddress)) < 0)
    {
        cerr << "Failed to send data to server" << endl;
        exit(1);
    }

    m_File = ofstream("client_files/" + filepath.substr(13));
}

void Client::ReceiveFile()
{
    socklen_t reliableServerAddressLen = sizeof(m_ReliableServerAddress);

    uint32_t ackno = 0;
    while (true)
    {
        Packet* p = new Packet();
        recvfrom(m_Socket, p, sizeof(Packet), 0, (sockaddr*)&m_ReliableServerAddress, &reliableServerAddressLen);

        if (p->len == 65535)
            break;

        if (p->seqno > ackno)
        {
            // not in order
            m_Packets.push(p);

            cout << "Out of order packet received!" << endl;

            SendAck(ackno);
        }
        else if (p->seqno == ackno)
        {
            ackno += p->len;

            m_File.write((char*)p->data, p->len);

            // cout << "written-" << p->data << endl;

            if (!m_Packets.empty())
            {
                Packet* qp = m_Packets.top();

                while (qp && qp->seqno == ackno)
                {
                    if (m_File.tellp() == qp->seqno)
                    {
                        ackno += qp->len;
                        m_File.write((char*)qp->data, qp->len);
                    }
                    delete qp;
                    m_Packets.pop();

                    if (m_Packets.empty())
                        break;

                    qp = m_Packets.top();
                }
            }

            SendAck(ackno);
        }
    }
}

void Client::SendAck(uint32_t ackno)
{
    cout << "Sending ack! - " << ackno << endl;
    socklen_t reliableServerAddressLen = sizeof(m_ReliableServerAddress);
    AckPacket ack;
    ack.ackno = ackno;
    sendto(m_Socket, &ack, sizeof(ack), 0, (sockaddr*)&m_ReliableServerAddress, reliableServerAddressLen);
}
