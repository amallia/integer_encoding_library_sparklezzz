/*
 * KScheme_1Bit_CU_opt.hpp
 *
 *  KScheme_1Bit_Complete Unary
 *
 *  Created on: 2013-3-2
 *      Author: zxd
 */

#ifndef KSCHEME_1BIT_CU_OPT_HPP_
#define KSCHEME_1BIT_CU_OPT_HPP_

#include <stdint.h>
#include <cmath>
#include <assert.h>
#include <string.h>
#include "Compressor.hpp"
#include "VarByte.hpp"
#include <iostream>
#include <immintrin.h>
#include <nmmintrin.h>
//#include "kscheme_1bit_cu_unpack.hpp"

using namespace std;

namespace paradise {
namespace index {

class KScheme_1Bit_CU_opt: public Compressor {
public:
	KScheme_1Bit_CU_opt() {
	}
	virtual ~KScheme_1Bit_CU_opt() {
	}

public:
	virtual int
	encodeUint32(char* des, const uint32_t* src, uint32_t encodeNum);
	virtual int
	decodeUint32(uint32_t* des, const char* src, uint32_t decodeNum);

	virtual int
	encodeUint16(char* des, const uint16_t* src, uint32_t encodeNum);
	virtual int
	decodeUint16(uint16_t* des, const char* src, uint32_t decodeNum);

	virtual int encodeUint8(char* des, const uint8_t* src, uint32_t encodeNum);
	virtual int decodeUint8(uint8_t* des, const char* src, uint32_t decodeNum);
	virtual std::string getCompressorName() {
		return "KScheme_1Bit_CU_opt";
	}
	virtual Compressor* clone();
private:
	template<typename T>
	int encode(char* des, const T* src, uint32_t length);

	template<typename T>
	int decode(T* des, const char* src, uint32_t length);

	template<typename T>
	int encodeBlock(char* des, const T* src, uint32_t size);

	template<typename T>
	int decodeBlock(T* des, const char* src, uint32_t size);

	template<typename T>
	static int genModeArr(char* des, const T* maxQuadIntArr, uint8_t* modeArr,
			uint32_t quadEncodeNum);

	template<typename T>
	static int encodeAllGroups(char* des, const T* src, uint32_t encodeNum,
			const uint8_t* modeArr, uint32_t totalIntsNeeded);
};

template<typename T>
int KScheme_1Bit_CU_opt::encode(char* des, const T* src, uint32_t encodeNum) {
	if (encodeNum < 4) {
		return VarByte::encode(des, src, encodeNum);
	}

	int compLen = 0;
	int left = encodeNum % 4;
	if (left > 0) {
		compLen = encodeBlock(des, src, encodeNum - left);
		compLen += VarByte::encode(des + compLen, src + encodeNum - left, left);
	} else {
		compLen = encodeBlock(des, src, encodeNum);
	}
	return compLen;
}

template<typename T>
int KScheme_1Bit_CU_opt::decode(T* des, const char* src, uint32_t decodeNum) {
	if (decodeNum < 4) {
		return VarByte::decode(des, src, decodeNum);
	}	

	int decodeLen = 0;
	int left = decodeNum % 4;
	if (left > 0) {
		decodeLen = decodeBlock(des, src, decodeNum - left);
		decodeLen += VarByte::decode(des + decodeNum - left, src + decodeLen,
				left);
	} else {
		decodeLen = decodeBlock(des, src, decodeNum);
	}

	return decodeLen;
}
template<typename T>
int KScheme_1Bit_CU_opt::encodeBlock(char* des, const T* src, uint32_t encodeNum) {
	const static uint32_t unary_code[33] = {-1, 0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f,	// INVALID, 0, 01, 011, 0111, 01111, 011111, 0111111, 01111111
										0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff,		// 011111111,...
										0x1fff, 0x3fff, 0x7fff, 0xffff, 0x1ffff,
										0x3ffff, 0x7ffff, 0xfffff, 0x1fffff,
										0x3fffff, 0x7fffff, 0xffffff, 0x1ffffff,
										0x3ffffff, 0x7ffffff, 0xfffffff, 0x1fffffff,
										0x3fffffff, 0x7fffffff};
	uint32_t quadEncodeNum = encodeNum / 4;
	uint32_t *tmpModeSegBeg = new uint32_t[quadEncodeNum];	//max control bit for one int: 32
	uint32_t *tmpModeSegIdx = tmpModeSegBeg;
	memset(tmpModeSegBeg, 0, quadEncodeNum * sizeof(uint32_t));

	char *desBeg = des;
	uint32_t *desInt = (uint32_t*)des;
	char *dataSegBeg = des + 4;	//leave first 4 byte to record the begin position of data segment
	uint32_t *dataSegPosInt = (uint32_t*)dataSegBeg;

	if (encodeNum > 0)
		memset(dataSegPosInt, 0, 4 * sizeof(uint32_t));

	int k = 0;
	uint32_t curBitOffset = 0;	//range from 0 to 32
	uint32_t controlBitOffset = 0;	//range from 0 to 32
	for (uint32_t i=0; i<encodeNum; i+=4) {
		// step1: get quad max int
		T quadMaxInt = src[0];
		for (uint32_t j=1; j<4; j++) {
			quadMaxInt |= src[j];
		}

		// step2: get bytes needed for quad max int
		if (quadMaxInt == 0)
			quadMaxInt ++;
		int r = __builtin_clz(quadMaxInt);
		uint32_t bitCount = 32 - r;
		//while (quadMaxInt > 0) {
		//	quadMaxInt >>= 1;
		//	bitCount ++;
		//}

		// step3: record the unary descriptor
		if (controlBitOffset + bitCount > 32) {	//start new mode and new data seg
			// pad the descriptor with all 1
			uint32_t pad = (0xffffffffLL >> controlBitOffset) << controlBitOffset;
			*tmpModeSegIdx |= pad;
			*tmpModeSegIdx = ~(*tmpModeSegIdx);   //set not e.g. 01101 -> 10010
			tmpModeSegIdx ++;

			controlBitOffset = controlBitOffset + bitCount - 32;
			*tmpModeSegIdx |= unary_code[controlBitOffset];
		}
		else {
			*tmpModeSegIdx |= unary_code[bitCount] << controlBitOffset;
			controlBitOffset += bitCount;
		}

		// step4: encode group of 4 ints
		uint32_t bitLeftInUInt = 32 - curBitOffset;
		if (bitLeftInUInt >= bitCount) {	//new data should cross uint boundary
			// write data with effective bit length of the max val
			for (uint32_t k=0; k<4; k++) {
				*(dataSegPosInt + k) |= (src[k] & mask_map[bitCount]) << curBitOffset;
			}
			curBitOffset += bitCount;
		}
		else {
			// write data with effective bit length of the max val
			for (uint32_t k=0; k<4; k++) {
				*(dataSegPosInt + k) |= (src[k] & mask_map[bitLeftInUInt]) << curBitOffset;
			}
			//start a new group and left val
			dataSegPosInt += 4;
			uint32_t bitUnwritten = bitCount - bitLeftInUInt;
			for (uint32_t k=0; k<4; k++) {
				*(dataSegPosInt + k) = (src[k] >> bitLeftInUInt) & mask_map[bitUnwritten] ;
			}
			curBitOffset = bitUnwritten;
		}

		src += 4;
	}

	// pad the descriptor
	uint32_t pad = (0xffffffffLL >> controlBitOffset) << controlBitOffset;
	*tmpModeSegIdx |= pad;
	*tmpModeSegIdx = ~(*tmpModeSegIdx); // set not
	tmpModeSegIdx ++;
	dataSegPosInt += 4;

	// record the start offset of mode segment
	*desInt = 4 + ((char*)dataSegPosInt - dataSegBeg);

	// append the mode seg to the end of data seg
	char *modeSegBeg = (char*)dataSegPosInt;
	uint32_t modeSegSize = (tmpModeSegIdx - tmpModeSegBeg) * sizeof(uint32_t);
	memcpy(modeSegBeg, tmpModeSegBeg, modeSegSize);

	delete [] tmpModeSegBeg;

	return (modeSegBeg - desBeg) + modeSegSize;
}
/*
template<typename T>
int KScheme_1Bit_CU_opt::decodeBlock(T* des, const char* src, uint32_t decodeNum) {

	const uint32_t *srcInt = (const uint32_t*)src;

	uint32_t modeSegOffset = *srcInt;
	const uint8_t *dataSegPos = (uint8_t*)src + 4;
	const uint8_t *modeSegBeg = (uint8_t*)(src + modeSegOffset);	//note that we still use 8 bit for mode seg here!
	const uint8_t *modeSegPos = modeSegBeg;
	const uint32_t *wordPos = (const uint32_t*)dataSegPos;

	uint32_t decoded = 0;
	uint32_t curBitOffset = 0;
	uint32_t lastStatus = 0;

	while (decoded < decodeNum) {
		// get a valid unary descriptor

		uint8_t unpackIdx = *modeSegPos;
		const KS1CUUnpackInfo &unpackInfo = KS1CUUnpackInfoArr[unpackIdx];

		int validControlBitNum = unpackInfo.m_intNum >> 2;

		uint32_t lastGroupBitCount = lastStatus;
		bool flag = true;

		for (int i=0; i<validControlBitNum; i++) {
			uint32_t bitCount = unpackInfo.m_bitCountArr[i] + lastGroupBitCount * flag;
			flag = false;
			uint32_t bitLeftInUInt = 32 - curBitOffset;
			for	(uint32_t k=0; k<4; k++) {
				*(des+k) = (*(wordPos+k) >> curBitOffset) & mask_map[bitCount];
			}

			wordPos += ((curBitOffset + bitCount) >> 5) << 2;
			curBitOffset = (curBitOffset + bitCount) & 0x1f;

			if (bitLeftInUInt < bitCount) {	//start to read a new group
				uint32_t restBitCount = bitCount - bitLeftInUInt;

				for (uint32_t k=0; k<4; k++) {
					*(des+k) |= (*(wordPos+k) & mask_map[restBitCount]) << bitLeftInUInt;
				}
			}

			des += 4;
		}

		modeSegPos ++;
		decoded += unpackInfo.m_intNum;

		//if (unpackInfo.m_statusGen == 8) {	// follow last control byte
		//	lastStatus += unpackInfo.m_statusGen;
		//}
		//else {
		//	lastStatus = unpackInfo.m_statusGen;
		//}
		lastStatus = unpackInfo.m_statusGen + lastStatus * ((unpackInfo.m_statusGen >> 3) & 0x1) ;
	}

	uint32_t modeSegSize = modeSegPos - modeSegBeg;
	if (modeSegSize % 4 != 0) {	//note that the original unit of mode seg is 16 bit
		modeSegPos = modeSegBeg + (modeSegSize + 3) / 4 * 4;
	}

	if (curBitOffset != 0)
		wordPos += 4;

	return (char*)modeSegPos - src;
}
*/


template<typename T>
int KScheme_1Bit_CU_opt::decodeBlock(T* des, const char* src, uint32_t decodeNum) {

	const uint32_t *srcInt = (const uint32_t*)src;

	uint32_t modeSegOffset = *srcInt;
	const uint8_t *dataSegPos = (uint8_t*)src + 4;
	const uint32_t *modeSegPos = (uint32_t*)(src + modeSegOffset);
	const uint32_t *wordPos = (const uint32_t*)dataSegPos;

	uint32_t decoded = 0;
	uint32_t curBitOffset = 0;
	uint32_t controlBitOffset = 0;
	uint32_t modeBitSet = *modeSegPos;
	while (decoded < decodeNum) {
		// get a valid unary descriptor
		uint32_t bitCount = 0;
		if (modeBitSet != 0) {
			bitCount = __builtin_ctz(modeBitSet);
			bitCount ++;
			controlBitOffset += bitCount;
			modeBitSet = (uint64_t)(modeBitSet) >> bitCount;
		} else {
			bitCount = 33 - controlBitOffset;
			controlBitOffset = 33;
		}
		//while (! (((uint64_t)(*modeSegPos) >> controlBitOffset) & 0x1) && controlBitOffset < 32) {
		//	controlBitOffset ++;
		//	bitCount ++;
		//}
		//bitCount ++;
		//controlBitOffset ++;
		if (controlBitOffset == 32) {
			modeSegPos ++;
			modeBitSet = *modeSegPos;
			controlBitOffset = 0;
		}
		else if (controlBitOffset > 32) {
			modeSegPos ++;
			modeBitSet = *modeSegPos;
			controlBitOffset = 0;
			// read from new mode seg idx
			
			int r = __builtin_ctz(modeBitSet);
			controlBitOffset += r + 1; 
			bitCount += r;
			modeBitSet >>= controlBitOffset;

			//while (! (((uint64_t)(*modeSegPos) >> controlBitOffset) & 0x1) && controlBitOffset < 32) {
			//	controlBitOffset ++;
			//	bitCount ++;
			//}
			//controlBitOffset ++;	// only increment the control bit offset
		}

		uint32_t bitLeftInUInt = 32 - curBitOffset;
		for	(uint32_t k=0; k<4; k++) {
			*(des+k) = (*(wordPos+k) >> curBitOffset) & mask_map[bitCount];
		}

		wordPos += ((curBitOffset + bitCount) >> 5) << 2;
		curBitOffset = (curBitOffset + bitCount) & 0x1f;

		if (bitLeftInUInt < bitCount) {	//start to read a new group
			uint32_t restBitCount = bitCount - bitLeftInUInt;

			for (uint32_t k=0; k<4; k++) {
				*(des+k) |= (*(wordPos+k) & mask_map[restBitCount]) << bitLeftInUInt;
			}
		}

		decoded += 4;
		des += 4;
	}

	if (controlBitOffset > 0)
		modeSegPos ++;

	if (curBitOffset > 0)
		wordPos += 4;

	return (char*)modeSegPos - src;
}
/*
template<typename T>
int KScheme_1Bit_CU_opt::decodeBlock(T* des, const char* src, uint32_t decodeNum) {

	const uint32_t *srcInt = (const uint32_t*)src;

	uint32_t modeSegOffset = *srcInt;
	const uint8_t *dataSegPos = (uint8_t*)src + 4;
	const uint32_t *modeSegPos = (uint32_t*)(src + modeSegOffset);
	const uint32_t *wordPos = (const uint32_t*)dataSegPos;

	uint32_t decoded = 0;
	uint32_t curBitOffset = 0;
	uint32_t controlBitOffset = 0;
	while (decoded < decodeNum) {
		// get a valid unary descriptor
		uint32_t bitCount = 0;
		while (! (((uint64_t)(*modeSegPos) >> controlBitOffset) & 0x1) && controlBitOffset < 32) {
			controlBitOffset ++;
			bitCount ++;
		}
		bitCount ++;
		controlBitOffset ++;
		if (controlBitOffset == 32) {
			modeSegPos ++;
			controlBitOffset = 0;
		}
		else if (controlBitOffset > 32) {
			modeSegPos ++;
			controlBitOffset = 0;
			// read from new mode seg idx
			while (! (((uint64_t)(*modeSegPos) >> controlBitOffset) & 0x1) && controlBitOffset < 32) {
				controlBitOffset ++;
				bitCount ++;
			}
			controlBitOffset ++;	// only increment the control bit offset
		}

		uint32_t bitLeftInUInt = 32 - curBitOffset;
		for	(uint32_t k=0; k<4; k++) {
			*(des+k) = (*(wordPos+k) >> curBitOffset) & mask_map[bitCount];
		}

		wordPos += ((curBitOffset + bitCount) >> 5) << 2;
		curBitOffset = (curBitOffset + bitCount) & 0x1f;

		if (bitLeftInUInt < bitCount) {	//start to read a new group
			uint32_t restBitCount = bitCount - bitLeftInUInt;

			for (uint32_t k=0; k<4; k++) {
				*(des+k) |= (*(wordPos+k) & mask_map[restBitCount]) << bitLeftInUInt;
			}
		}

		decoded += 4;
		des += 4;
	}

	if (controlBitOffset > 0)
		modeSegPos ++;

	if (curBitOffset > 0)
		wordPos += 4;

	return (char*)modeSegPos - src;
}
*/
}
}


#endif /* KSCHEME_1BIT_CU_OPT_HPP_ */
