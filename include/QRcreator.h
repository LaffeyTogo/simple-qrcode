/*
* ============================================================================================================
* QRCodeGenerator.h
* ------------------------------------------------------------------------------------------------------------
* High-level entry points for QR Code generation.
* ------------------------------------------------------------------------------------------------------------
* This module provides a simplified interface that wraps the full
* QR encoding pipeline:
*	input -> encoding -> error correction -> structure building  -> matrix filling  -> masking  -> format info
* ------------------------------------------------------------------------------------------------------------
* Responsibilities:
*	- Expose user-friendly APIs for different input types
*	- Hide internal encoding and matrix construction details
* ------------------------------------------------------------------------------------------------------------
* Notes:
*	- Currently supports BYTE mode only
*	- ECC level is provided as integer (0¨C3 -> L/M/Q/H)
* ============================================================================================================
*/
#pragma once
#include <cstdint>


#include "QRSpec.h"
#include "ByteEncoder.h"
#include "FinalSream.h"
#include "StructureBuilder.h"
#include "QRMatrix.h"
#include "Mask.h"





/*
* Internal QR generation pipeline.
*
* Steps:
*	1. Convert ECC level to enum
*	2. Encode input into data codewords
*	3. Generate final stream (add ECC)
*	4. Build QR matrix structure
*	5. Apply masking
*	6. Write format information
*
* Input:
*	- text: raw byte data
*	- ECCLevel: 0 - 3 (L/M/Q/H)
*
* Returns:
*	- Fully constructed QRMatrix
*
* Note:
*	- Assumes all submodules behave correctly
*/
namespace 
{
	QRMatrix GenerateQRcode_(std::span<const uint8_t> text, uint32_t ECCLevel)
	{
		QREccLevel ecc;
		switch (ECCLevel)
		{
			case 0:ecc = QREccLevel::L; break;
			case 1:ecc = QREccLevel::M; break;
			case 2:ecc = QREccLevel::Q; break;
			case 3:ecc = QREccLevel::H; break;
			default:ecc = QREccLevel::L;break;
		}
		

		auto [Codeword, version] = encode(reinterpret_cast<const char*>(text.data()), ecc);


		Codeword = BuildFinalStream(version, ecc, Codeword);


		QRMatrix QRMatrix_(version);
		Filltemplate(QRMatrix_, version, Codeword);


		uint32_t maskid = SetMask(QRMatrix_);		


		SetupFormatInfo(QRMatrix_, maskid, ecc);


		return QRMatrix_;
	}
}





/*
* Generate QR code from binary data.
*
* text:
*	- Input byte array
*	- Not modified
*
* ECCLevel:
*	- 0: L
*	- 1: M
*	- 2: Q
*	- 3: H
*
* Returns:
*	- Generated QR matrix
*/
QRMatrix GenerateQRcode(std::vector<uint8_t>& text, uint32_t ECCLevel)
{
	return GenerateQRcode_(text, ECCLevel);
}





/*
* Generate QR code from C-string.
*
* text:
*	- Null-terminated string
*
* ECCLevel:
*	- 0: L
*	- 1: M
*	- 2: Q
*	- 3: H
*
* Throws:
*	- std::invalid_argument if text is null (debug only)
*/
QRMatrix GenerateQRcode(const char* text,uint32_t ECCLevel)
{
	
	#ifdef DEBUG
		if (!text)
			throw std::invalid_argument("null pointer");
	#endif


	size_t len = std::strlen(text);

	return GenerateQRcode_
	(
		std::span<const uint8_t>
		(
			reinterpret_cast<const uint8_t*>(text),len
		),
		ECCLevel
	);
}





/*
* Generate QR code using default ECC level (Q).
* default = 2 -> Q
*/
QRMatrix GenerateQRcode(const char* text)
{
	return GenerateQRcode(text , 2);
}