#pragma once
#include <iostream>
class LingBuffer
{
public:
	enum
	{
		RING_BUFFER_SIZE = 1000
	};
#pragma pack(push,1)
	struct PacketHeader
	{
		uint16_t packetCode;
		uint16_t packetLength;
		uint32_t packetType;
	};
#pragma pack(pop)

public:
	bool DequeueBuffer(char* outBuffer);
	bool Dequeue(char* data);
	bool Peek(char* data, int index);
	bool Enqueue(char data);
	bool EnqueueBuffer(char* buffer, size_t len);

	LingBuffer();
private:
	//-------------------------------
	// Packet 헤더는 6바이트로 고정
	//-------------------------------
	
	int32_t m_Front;
	int32_t m_Rear;
	size_t m_Size;

	char m_Buffer[RING_BUFFER_SIZE];

};