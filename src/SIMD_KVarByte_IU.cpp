/*
 * SIMD_KVarByte_IU.cpp
 *
 *  Created on: 2013-2-25
 *      Author: zxd
 */

#ifdef USE_SSE_INSTRUCTION

#include "SIMD_KVarByte_IU.hpp"
using namespace paradise::index;

int SIMD_KVarByte_IU::encodeUint32(char* des, const uint32_t* src, uint32_t encodeNum) {
	return encode<uint32_t> (des, src, encodeNum);
}
int SIMD_KVarByte_IU::decodeUint32(uint32_t* des, const char* src, uint32_t decodeNum) {
	return decode<uint32_t> (des, src, decodeNum);
}

int SIMD_KVarByte_IU::encodeUint16(char* des, const uint16_t* src, uint32_t encodeNum) {
	return encode<uint16_t> (des, src, encodeNum);
}
int SIMD_KVarByte_IU::decodeUint16(uint16_t* des, const char* src, uint32_t decodeNum) {
	return decode<uint16_t> (des, src, decodeNum);
}

int SIMD_KVarByte_IU::encodeUint8(char* des, const uint8_t* src, uint32_t encodeNum) {
	return encode<uint8_t> (des, src, encodeNum);
}
int SIMD_KVarByte_IU::decodeUint8(uint8_t* des, const char* src, uint32_t decodeNum) {
	return decode<uint8_t> (des, src, decodeNum);
}
Compressor* SIMD_KVarByte_IU::clone() {
	Compressor* pNewComp = new SIMD_KVarByte_IU(*this);
	return pNewComp;
}

#endif
