#ifndef APPLICATIONPROTOCOL_H
#define APPLICATIONPROTOCOL_H

#include <cstdint>

constexpr uint8_t CMD_SEND_CONFIG = 0x01;
constexpr uint8_t CMD_START_PROCESSING = 0x02;
constexpr uint8_t CMD_CHECK_STATUS = 0x03;
constexpr uint8_t CMD_GET_RESULT = 0x04;
constexpr uint8_t CMD_END_SESSION = 0xFF;

constexpr uint8_t STATUS_PROCESSING = 0x0A;
constexpr uint8_t STATUS_DONE = 0x0B;
constexpr uint8_t STATUS_ERROR = 0x0C;

#pragma pack(push, 1)
struct MessageHeader
{
	uint8_t commandId;
	uint32_t messageLength;
};

struct ConfigMessage
{
	uint32_t threadsNum;
	uint32_t matrixSize;
};

#pragma pack(pop)

#endif //APPLICATIONPROTOCOL_H