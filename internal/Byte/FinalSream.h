/*
* ====================================================================================
* FinalStream.h
* QR Code final codeword stream builder (Block + ECC + Interleave stage)
* ------------------------------------------------------------------------------------
* This file implements the final transformation from raw data codewords
* into the interleaved QR Code codeword stream.
*	1. Split data into blocks (small / large blocks defined by QRSpec)
*	2. Generate Reed-Solomon ECC for each block
*	3. Interleave all blocks into a single codeword stream
* ------------------------------------------------------------------------------------
* Core concepts:
*	Block structure:
*		Each block = DataCodewords + ECCCodewords
*	Block types:
*		- Small blocks: shorter data length
*		- Large blocks: longer data length (QR standard requirement)
*   Interleaving:
*		- Codewords are NOT concatenated block-by-block
*		- Instead, bytes are taken column-wise across all blocks
*		- Ensures better error distribution across the QR matrix
* ------------------------------------------------------------------------------------
* Input:
*	- iVersion: QR version (1~40)
*   - ECC: error correction level (L / M / Q / H)
*   - Text: data codewords (from encoder, 8-bit aligned)
* Output:
*   - Final interleaved codeword stream
*   - Layout: [Data interleave] + [ECC interleave]
* ------------------------------------------------------------------------------------
* Design notes:
*   - BlocksBuffer stores full blocks (Data + ECC) for random access
*   - Interleaving is performed via index-based extraction
*   - Memory is preallocated to avoid reallocation overhead
* ------------------------------------------------------------------------------------
* Specification:
*   - Follows ISO/IEC 18004 (QR Code standard)
*   - Corresponds to "Structure Final Message" step
* ====================================================================================
*/
#pragma once
#include <vector>
#include <cstdint>

#include "QRSpec.h"
#include "ECCCalculator.h"



/*
* QR Code data block cache structure
*
* In the QR standard:
*	- Data is divided into multiple blocks (divided into small blocks and large blocks)
*	- Each block = DataCodewords + ECCCodewords
*
* This class is used for:
*	- Storing data by block
*	- Writing data codewords (Data)
*	- Writing error correction codewords (ECC)
*/
class BlocksBuffer
{
	public:
		BlocksBuffer(int iVersion, QREccLevel ECC)
		{
			const size_t smallCount = QRSpec::GetSmallBlockNumber(iVersion, ECC);
			const size_t smallSize  = QRSpec::GetSmallBlockSize(iVersion, ECC);
			const size_t largeCount = QRSpec::GetLargeBlockNumber(iVersion, ECC);
			const size_t largeSize  = QRSpec::GetLargeBlockSize(iVersion, ECC);
			const size_t EccLen     = QRSpec::GetBlockEccLen(iVersion, ECC);

		
			blocks_.reserve(QRSpec::GetTotalBlocksNumber(iVersion, ECC));

			// small blocks
			for (size_t i = 0; i < smallCount; ++i)
				blocks_.emplace_back(smallSize + EccLen);

			// large blocks
			for (size_t i = 0; i < largeCount; ++i)
				blocks_.emplace_back(largeSize + EccLen);
		}

		void WriteDataWord(std::vector<uint8_t>& src,  uint32_t BlockId)
		{
			std::vector<uint8_t>& dst = getBlock(BlockId);
			std::copy(src.begin(), src.end(), dst.begin());
		}

		void WriteECC(const std::vector<uint8_t>& src, uint32_t BlockId ,uint32_t DataWordLen)
		{
			std::vector<uint8_t>& dst = getBlock(BlockId );
			std::copy(src.begin(), src.end(), dst.begin() + DataWordLen);
		}

		std::vector<uint8_t>& getBlock(uint32_t BlockId)
		{
			return blocks_[BlockId];
		}

		uint8_t GetBlockByte(uint32_t BlockId , uint32_t Offset)
		{
			return blocks_[BlockId][Offset];
		};
	private:

		std::vector<std::vector<uint8_t>> blocks_;
};






/*
* Construct the final QR Code codeword stream
*
* The process corresponds to the QR standard:
*	1. Split into blocks
*	2. Calculate ECC (Reed-Solomon) for each block
*	3. Interleave (Data + ECC)
*
* Text Encoded data codewords (bitstream aligned to 8 bits)
* The final output codeword stream
*/
std::vector<uint8_t> BuildFinalStream (int iVersion, QREccLevel ECC, std::vector<uint8_t>& Text)
{
	const uint32_t smallCount = QRSpec::GetSmallBlockNumber(iVersion, ECC);
	const uint32_t smallSize  = QRSpec::GetSmallBlockSize(iVersion, ECC);
	const uint32_t largeCount = QRSpec::GetLargeBlockNumber(iVersion, ECC);
	const uint32_t largeSize  = QRSpec::GetLargeBlockSize(iVersion, ECC);
	const uint32_t EccLen     = QRSpec::GetBlockEccLen(iVersion, ECC);
	const uint32_t BlockNum   = QRSpec::GetTotalBlocksNumber(iVersion, ECC);
	BlocksBuffer buffer(iVersion, ECC);


	// (1 Segment block
	uint32_t i = 0,byetCounter = 0;
	for (; i < smallCount; i++)
	{
		std::vector<uint8_t>BlockDataWordTemp;
		BlockDataWordTemp.reserve(smallSize);
		BlockDataWordTemp.assign(Text.begin() + byetCounter , Text.begin() + byetCounter + smallSize);
		
		buffer.WriteDataWord(BlockDataWordTemp , i);
		buffer.WriteECC(CalculateECC(BlockDataWordTemp , EccLen) , i , smallSize);

		byetCounter += smallSize;
	}
	for (; i < largeCount + smallCount; i++)
	{
		std::vector<uint8_t>BlockDataWordTemp;
		BlockDataWordTemp.reserve(largeSize);
		BlockDataWordTemp.assign(Text.begin() + byetCounter, Text.begin() + byetCounter + largeSize);

		buffer.WriteDataWord(BlockDataWordTemp, i);
		buffer.WriteECC(CalculateECC(BlockDataWordTemp, EccLen), i, largeSize);

		byetCounter += largeSize;
	}


	// (2 The first part loops through and extracts each one, while the second part only extracts large block.
	std::vector<uint8_t> FinalCodeWord;
	FinalCodeWord.reserve(Text.size() + BlockNum * EccLen);
	for (i = 0; i < smallSize + EccLen; i++)
	{
		for(uint32_t j = 0 ; j < BlockNum ; j++)
		{
			FinalCodeWord.emplace_back(buffer.GetBlockByte(j, i));
		}
	}
	for (; i < largeSize; i++)
	{
		for (uint32_t j = smallCount + EccLen; j < BlockNum; j++)
		{
			FinalCodeWord.emplace_back(buffer.GetBlockByte(j, i));
		}
	}

	return FinalCodeWord;
}