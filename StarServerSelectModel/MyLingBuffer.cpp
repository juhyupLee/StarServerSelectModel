#include "MyLingBuffer.h"

bool LingBuffer::DequeueBuffer(char* outBuffer)
{
    //-------------------------------
    // ��Ŷ��������� ���̸�ŭ�� ��� ť���־�� ��ť�� ���� ������ �Ǵ��� �Ѵ�.
    //-------------------------------
    if (sizeof(PacketHeader) > m_Size)
    {
        return false;
    }
    //-------------------------------
    // ����� �������� (���� ������ ���� Peek����..)
    //-------------------------------
    char buf[8];

    for (size_t i = 0; i < sizeof(PacketHeader); i++)
    {
        if (false == Peek(&buf[i],i))
        {
            return false;
        }
    }
    //-------------------------------
    // ��Ŷ �ڵ带 Ȯ���غ��� 
    //-------------------------------
    PacketHeader* tempPacket;
    tempPacket = (PacketHeader*)&buf;
    if (tempPacket->packetCode != 0x1234)
    {
        return false;
    }
    // ��� + ������ 
    if (sizeof(PacketHeader)+tempPacket->packetLength > m_Size)
    {
        return false;
    }

    //-------------------------------
    // ��Ŷ�� ������,  �����͸� �����Ѵ�
    //-------------------------------
    char data;

    for (size_t i = 0; i < sizeof(PacketHeader); i++)
    {
        if (false == Dequeue(&data))
        {
            return false;
        }
    }

    for (size_t i = 0; i < tempPacket->packetLength; i++)
    {
        if (false == Dequeue(&outBuffer[i]))
        {
            return false;
        }
    }
    return true;
}

bool LingBuffer::Dequeue(char* data)
{
    //---------------------------
    // ����ٸ�..
    //---------------------------
    if (m_Rear == m_Front)
    {
        return false;
    }
    *data = m_Buffer[m_Front];

    m_Front = (m_Front + 1) % RING_BUFFER_SIZE;
    --m_Size;

    return true;
}

bool LingBuffer::Peek(char* data, int index)
{
    if (index > RING_BUFFER_SIZE-1)
    {
        return false;
    }

    int tempFront = m_Front;

    tempFront = (tempFront + index) % RING_BUFFER_SIZE;

    *data = m_Buffer[tempFront];
    return true;
}

bool LingBuffer::Enqueue(char data)
{
    //---------------------------
    // �����ִٸ�..
    //---------------------------
    if ((m_Rear + 1) % RING_BUFFER_SIZE == m_Front)
        return false;
   
    m_Buffer[m_Rear] = data;

    m_Rear = (m_Rear + 1) % RING_BUFFER_SIZE;

    ++m_Size;
    return true;
}

bool LingBuffer::EnqueueBuffer(char* buffer, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (false == Enqueue(buffer[i]))
        {
            return false;
        }
    }
    return true;
}

LingBuffer::LingBuffer()
    :
    m_Buffer{0},
    m_Front(0),
    m_Rear(0),
    m_Size(0)
{
}
