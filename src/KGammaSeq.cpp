/*
 * KGammaSeq.cpp
 *
 *  Created on: 2013-2-25
 *      Author: zxd
 */

#include "KGammaSeq.hpp"
using namespace paradise::index;

int KGammaSeq::encodeUint32(char* des, const uint32_t* src, uint32_t encodeNum) {
	return encode<uint32_t> (des, src, encodeNum);
}
int KGammaSeq::decodeUint32(uint32_t* des, const char* src, uint32_t decodeNum) {
	return decode<uint32_t> (des, src, decodeNum);
}

int KGammaSeq::encodeUint16(char* des, const uint16_t* src, uint32_t encodeNum) {
	return encode<uint16_t> (des, src, encodeNum);
}
int KGammaSeq::decodeUint16(uint16_t* des, const char* src, uint32_t decodeNum) {
	return decode<uint16_t> (des, src, decodeNum);
}

int KGammaSeq::encodeUint8(char* des, const uint8_t* src, uint32_t encodeNum) {
	return encode<uint8_t> (des, src, encodeNum);
}
int KGammaSeq::decodeUint8(uint8_t* des, const char* src, uint32_t decodeNum) {
	return decode<uint8_t> (des, src, decodeNum);
}
Compressor* KGammaSeq::clone() {
	Compressor* pNewComp = new KGammaSeq(*this);
	return pNewComp;
}


