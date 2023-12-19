#include <cstdint>

/* 508 bytes */
struct Packet
{
    uint16_t cksum;
    uint16_t len;
    uint32_t seqno;
    uint8_t data[500];

    Packet()
    {
        memset(this, 0, sizeof(*this));
    }
};

/* 8 bytes */
struct AckPacket
{
    uint16_t cksum;
    uint16_t len;
    uint32_t ackno;

    AckPacket()
    {
        memset(this, 0, sizeof(*this));
    }
};
