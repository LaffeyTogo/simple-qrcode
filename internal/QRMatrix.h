/*
* ====================================================================================
* QRMatrix.h
* QR Code matrix data structure and controlled access layer
* ------------------------------------------------------------------------------------
* This file defines the core 2D matrix representation used throughout the QR pipeline.
* It provides:
*   - A compact storage model for QR modules (pixels)
*   - Status tracking for each module (empty / data / template / reserved)
*   - A controlled operator interface for safe modifications
* ------------------------------------------------------------------------------------
* Design overview:
*
*   Data model:
*       - Matrix is stored as a 1D array (row-major)
*       - Each module (QRPixel) contains:
*           • Value  (black / white)
*           • Status (semantic role)
*
*   Status semantics:
*       - EMPTY:    not yet assigned
*       - DATA:     data codewords (maskable)
*       - TEMPLATE: fixed patterns (finder, timing, etc.)
*       - RESERVED: reserved for format/version info
*
*   Access control:
*       - QRMatrix:   read-only interface + storage
*       - Operator:   write interface with semantic constraints
* ------------------------------------------------------------------------------------
* Design goals:
*
*   - Prevent invalid writes (e.g. overwriting function patterns)
*   - Separate "data ownership" from "mutation logic"
*   - Enable masking and placement logic to operate safely
* ------------------------------------------------------------------------------------
* Notes:
*
*   - Indexing is unchecked in release mode (performance-oriented)
*   - Debug mode enables boundary validation
*   - All mutation should go through QRMatrix_Operator
* ====================================================================================
*/
#pragma once
#include <vector>
#include <cstdint>

#include "QRSpec.h"





//#define DEBUG
/*
* Core QR matrix storage (read-oriented interface)
*
* Represents the QR code as a 2D grid stored in a 1D array.
* Provides read access to module values and matrix dimensions.
*
* Notes:
*   - Does NOT expose direct write access
*   - Each pixel carries both value and semantic status
*   - Index calculation is optimized (row-major)
*/
class QRMatrix
{
	friend class QRMatrix_Operator;
	private:
		enum class status : uint8_t
		{
			EMPTY,
			DATA,
			TEMPLATE,
			RESERVED
		};

		struct QRPixel
		{
			bool Value;
			status Status;
		};

	public:

		QRMatrix(uint32_t Verson) :_data(QRSpec::GetQRSize(Verson), { false, status::EMPTY })
		{
			_Side = QRSpec::GetQRSide(Verson);
		}

		bool GetPixel(uint32_t X, uint32_t Y)
		{
			return _data[Index(X, Y)].Value;
		}

		uint32_t GetSide()
		{
			return _Side;
		}

	private:
		
		constexpr  uint32_t Index(uint32_t X, uint32_t Y) const noexcept
		{
			#ifdef DEBUG
				if (X >= _Side || Y >= _Side)
				{
					throw std::invalid_argument("Out of bounds");
				}
			#endif

			return Y * _Side + X;
		}


		uint32_t _Side;
		std::vector<QRPixel> _data;
};





/*
* Controlled write interface for QR matrix
*
* Provides all mutation operations on the matrix while enforcing
* semantic correctness (data vs template vs reserved).
*
* Responsibilities:
*   - Writing data bits
*   - Placing template patterns
*   - Checking mask eligibility
*   - Ensuring safe modifications
*
* Notes:
*   - All writes should go through this class
*   - Prevents accidental overwrite of protected modules
*/
class QRMatrix_Operator
{

	public:

		QRMatrix_Operator(QRMatrix& _Data):Data(_Data)
		{}

	
		uint32_t GetSide() const
		{
			return Data._Side;
		}


		bool GetPixel(uint32_t X , uint32_t Y) const
		{
			return Data._data[Data.Index(X, Y)].Value;
		}


		void Placeholder(uint32_t X, uint32_t Y) const
		{
			SetTemplate(X, Y, 1);
		}


		void SetData(uint32_t X, uint32_t Y, bool data) const
		{
			QRMatrix::QRPixel& p = Data._data[Data.Index(X, Y)];
			p.Status = QRMatrix::status::DATA;
			p.Value = data;
		}


		void SetTemplate(uint32_t X, uint32_t Y, bool data) const
		{
			Data._data[Data.Index(X, Y)].Status = QRMatrix::status::TEMPLATE;
			Data._data[Data.Index(X, Y)].Value = data;
		}


		bool NeedMask(uint32_t X, uint32_t Y) const
		{
			return Data._data[Data.Index(X, Y)].Status == QRMatrix::status::DATA;
		}


		bool IsEmpty(uint32_t X, uint32_t Y) const
		{
			return Data._data[Data.Index(X, Y)].Status == QRMatrix::status::EMPTY;
		}


		void ReversePixel(uint32_t X, uint32_t Y) const
		{
			#ifdef DEBUG
				// data interface should not make any logic fallback decisions.
				if (_data[Index(X, Y)].Status != status::DATA))
				{
					throw std::invalid_argument("Flipped pixels that shouldn't have been changed");
				}
			#endif
			
			
				Data._data[Data.Index(X,Y)].Value = not(Data._data[Data.Index(X, Y)].Value);
		}

	private:

		QRMatrix& Data;
};



