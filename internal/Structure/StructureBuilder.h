/*
* ====================================================================================
* StructureBuilder.h
* QR Code module placement and template construction stage
* ------------------------------------------------------------------------------------
* This file is responsible for constructing the QR matrix layout, including:
*   - Function patterns (Finder / Alignment / Timing)
*   - Format & Version information areas
*   - Final placement of data codewords
*   - Reserve and fill format / version information areas
*   - Fill data bits into remaining modules (zigzag scan)
* ------------------------------------------------------------------------------------
* Core concepts:
*   Function patterns:
*       - Finder patterns: 3 large position markers
*       - Alignment patterns: smaller position correction markers
*       - Timing patterns: alternating lines for coordinate reference
*   Data placement:
*       - Codewords are written bit-by-bit
*       - Traversal follows zigzag pattern (right -> left, vertical scan)
*       - Skips all reserved / function modules
*   Format / Version info:
*       - Encodes ECC level and mask pattern
*       - Placed in predefined fixed positions
* ------------------------------------------------------------------------------------
* Specification:
*   - Based on ISO/IEC 18004
*   - Corresponds to "Module Placement in Matrix" stage
* ====================================================================================
*/
#pragma once
#include "QRMatrix.h"
#include "QRSpec.h"





/*
* Reserve format information areas in the QR matrix
*
* Marks the positions used to store format information (ECC level + mask pattern).
* These modules are reserved before actual format bits are written.
*
* Placement includes:
*	- Top-left (horizontal & vertical around finder)
*   - Top-right (horizontal)
*   - Bottom-left (vertical)
*   - Dark module (fixed black module)
*
* Notes:
*   - Uses placeholder markers to prevent data overwrite
*   - Actual format bits are filled later by SetupFormatInfo()
*/
void SetupFormatHolder(QRMatrix& m)
{
	uint32_t i;
	uint32_t side = m.GetSide();
	QRMatrix_Operator Matrix(m);

	// top left horizontal
	for (i = 0; i <= 5; ++i)
		Matrix.Placeholder(i, 8);
	Matrix.Placeholder(7, 8);


	// Top left vertical
	for (i = 0; i <= 5; ++i)
		Matrix.Placeholder(8, i);
	Matrix.Placeholder(8, 7);
	Matrix.Placeholder(8, 8);


	// VBottom left
	for (i = side - 7; i <= side - 1; ++i)
		Matrix.Placeholder(8, i);


	// Upper right
	for (i = side - 8; i <= side - 1; ++i)
		Matrix.Placeholder(i, 8);


	// Dark Module
	Matrix.SetTemplate(8, side - 8, 1);
};





/*
* Place version information bits into the matrix
*
* Writes 18-bit version information into two fixed regions:
*	- Top-right area
*	- Bottom-left area
*
* Only applies to version >= 7 (per QR specification).
*
* Notes:
*	- Bits are retrieved from QRSpec
*	- Written in predefined 3x6 blocks
*/
void SetupVersionInfo(QRMatrix& m, int version)
{
	QRMatrix_Operator Matrix(m);
	
	if (version < 7)
		return;

	const int size = m.GetSide();
	const char (&bits)[18] = QRSpec::GetVersionInfo(version);

	int bitIndex = 0;

	// ÓÒÉÏ
	for (int i = 0; i < 6; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			Matrix.SetTemplate(size - 11 + j, i, bits[bitIndex++]);
		}
	}

	bitIndex = 0;

	// ×óÏÂ
	for (int i = 0; i < 6; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			Matrix.SetTemplate(i, size - 11 + j,bits[bitIndex++]);
		}
	}
}





/*
* Place a single alignment pattern
*
* Writes a 5x5 alignment pattern centered at (cx, cy).
* Alignment patterns improve decoding robustness for distortion.
*
* Notes:
*   - Skips placement if the target area is already occupied
*     (e.g. overlaps with finder patterns)
*   - Pattern structure is fixed (per QR standard)
*/
void SetupAlignment(QRMatrix& m, uint32_t cx, uint32_t cy)
{
	QRMatrix_Operator Matrix(m);

	static constexpr int pattern[5][5] =
	{
		{1,1,1,1,1},
		{1,0,0,0,1},
		{1,0,1,0,1},
		{1,0,0,0,1},
		{1,1,1,1,1}
	};
	
	if(m.GetPixel(cx, cy) == 0)
	{
		for (uint32_t dy = 0; dy < 5; dy++)
		{
			for (uint32_t dx = 0; dx < 5; dx++)
			{
				Matrix.SetTemplate(cx - 2 + dx, cy - 2 + dy, pattern[dy][dx]);
			}
		}
	}

}





/*
* Place a single alignment pattern
*
* Writes a 5x5 alignment pattern centered at (cx, cy).
* Alignment patterns improve decoding robustness for distortion.
*
* Notes:
*   - Skips placement if the target area is already occupied
*     (e.g. overlaps with finder patterns)
*   - Pattern structure is fixed (per QR standard)
*/
void SetupFinder(QRMatrix& m, uint32_t cx, uint32_t cy)
{
	QRMatrix_Operator Matrix(m);

	static constexpr int pattern[9][9] =
	{
		{0,0,0,0,0,0,0,0,0},
		{0,1,1,1,1,1,1,1,0},
		{0,1,0,0,0,0,0,1,0},
		{0,1,0,1,1,1,0,1,0},
		{0,1,0,1,1,1,0,1,0},
		{0,1,0,1,1,1,0,1,0},
		{0,1,0,0,0,0,0,1,0},
		{0,1,1,1,1,1,1,1,0},
		{0,0,0,0,0,0,0,0,0}
	};

	for (uint32_t dy = 0; dy < 9; dy++)
	{
		for (uint32_t dx = 0; dx < 9; dx++)
		{
			if (cx - 4u + dx < m.GetSide() && cy - 4u + dy < m.GetSide() )
				Matrix.SetTemplate(cx - 4u + dx, cy - 4u + dy, pattern[dy][dx]);
		}
	}
}





/*
* Place timing patterns (horizontal and vertical)
*
* Writes alternating black/white modules along:
*   - Row 6 (horizontal)
*   - Column 6 (vertical)
*
* Used as a coordinate reference for decoding.
*
* Notes:
*   - Pattern alternates every module (1,0,1,0,...)
*   - Skips finder and reserved areas implicitly
*/
void SetupTimingPattern(QRMatrix& m) noexcept
{
	const uint32_t Side = m.GetSide();
	const uint32_t Border = Side - 8;
	QRMatrix_Operator Matrix(m);

	for (uint32_t r = 8; r < Border; r += 2)
	{
		Matrix.SetTemplate(r, 6, 1);
		Matrix.SetTemplate(6, r, 1);
	}

	for (uint32_t r = 9; r < Border; r += 2)
	{
		Matrix.SetTemplate(r, 6, 0);
		Matrix.SetTemplate(6, r, 0);
	}

}





/*
* Place data bits into the QR matrix
*
* Fills the matrix with data codewords using zigzag scanning:
*   - Starts from bottom-right
*   - Moves in vertical stripes (2 columns at a time)
*   - Alternates direction (up/down)
*
* Notes:
*   - Bits are written MSB-first per byte
*   - Only writes into empty modules (skips function patterns)
*   - Column 6 is skipped (timing pattern)
*   - Remaining unused bits are padded with 0
*/
void SetupFinalCodeword(QRMatrix& m, std::vector<uint8_t>& codeword)
{
	int side = m.GetSide();
	int x = side - 1;
	int y = side - 1;
	int direction = -1;
	QRMatrix_Operator Matrix(m);

	size_t bitIndex = 0;
	size_t totalBits = codeword.size() * 8;

	while (x > 0)
	{
		if (x == 6) x--;

		while (true)
		{
			for (int dx = 0; dx < 2; dx++)
			{
				int xx = x - dx;

				if (Matrix.IsEmpty(xx, y))
				{
					int bit = 0;

					if (bitIndex < totalBits)
					{
						uint8_t byte = codeword[bitIndex / 8];
						bit = (byte >> (7 - (bitIndex % 8))) & 1;
						bitIndex++;
					}

					Matrix.SetData(xx, y, bit);
				}
			}

			y += direction;

			if (y < 0 || y >= side)
			{
				y -= direction;
				direction = -direction;
				break;
			}
		}

		x -= 2;
	}
}





/*
* Build full QR template and place all data
*
* High-level orchestration function that constructs the QR matrix:
*   1. Place finder patterns
*   2. Place alignment patterns
*   3. Place timing patterns
*   4. Place version information (if applicable)
*   5. Reserve format information areas
*   6. Fill data codewords
*
* Notes:
*   - Does NOT apply masking
*   - Format info bits are reserved but not written here
*/
void Filltemplate(QRMatrix& m , uint32_t version, std::vector<uint8_t>& codeword)
{
	// SetupFinder
	SetupFinder(m, 3, 3);
	SetupFinder(m, m.GetSide() - 4, 3);
	SetupFinder(m, 3, m.GetSide() - 4);
	
	
	// SetupAlignment
	uint32_t AlignmentStep = QRSpec::GetAlignmentStep(version);
	for (uint32_t Y = 0; Y < AlignmentStep; Y++)
	{
		for (uint32_t X = 0; X < AlignmentStep; X++)
		{
			SetupAlignment(m, QRSpec::GetAlignmentPos(version, X), QRSpec::GetAlignmentPos(version, Y));
		}
	}


	// SetupTimingPattern
	SetupTimingPattern(m);


	// VersionInfo
	if (version >= 7) SetupVersionInfo(m, version);


	// Format holder
	SetupFormatHolder(m);

	
	// FinalCodeword
	SetupFinalCodeword(m, codeword);
}





/*
* Write format information bits into reserved areas
*
* Encodes and places 15-bit format information, including:
*   - Error correction level
*   - Mask pattern ID
*
* Notes:
*   - Writes into areas reserved by SetupFormatHolder()
*   - Bits are placed in mirrored positions:
*       top-left, top-right, bottom-left
*   - Bit order follows QR specification
*/
void SetupFormatInfo(QRMatrix& m, uint32_t MaskID, QREccLevel ecc)
{
	uint32_t i ,Counter;
	uint32_t side = m.GetSide();
	const char(&bits)[15] = QRSpec::GetFormatInfo(MaskID, ecc);
	QRMatrix_Operator Matrix(m);


	// top left horizontal
	for (i = 0, Counter = 0; i <= 5; i++, Counter++)
		Matrix.SetTemplate(i, 8, bits[Counter]);
	Matrix.SetTemplate(7, 8, bits[6]);


	// Top left vertical 0 - 7
	for (i = 0, Counter = 14; i <= 5; i++, Counter--)
		Matrix.SetTemplate(8, i, bits[Counter]);
	Matrix.SetTemplate(8, 7, bits[8]);
	Matrix.SetTemplate(8, 8, bits[7]);


	// Bottom left
	for (i = side - 7, Counter = 6; i <= side - 1; i++, Counter--)
		Matrix.SetTemplate(8, i, bits[Counter]);
		 

	// Upper right
	for (i = side - 8, Counter = 7; i <= side - 1; i++, Counter++)
		Matrix.SetTemplate(i, 8, bits[Counter]);
}