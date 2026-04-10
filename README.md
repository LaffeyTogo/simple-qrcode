## Simple QR Code Generator (C++)
This repository demonstrates how a QR Code generator works internally.
A minimal and readable implementation of the QR Code standard.
Designed for share and understanding how QR Codes algorithm work.

This project is a **simple and educational implementation of QR Code generation**.

It focuses on:

- clear algorithm steps
- semantic correctness
- modular structure
- easy to read code

The goal is **not performance**, but **understanding how QR Codes are constructed**.

## Design Philosophy

This implementation follows several principles:

- minimal abstraction
- explicit data structures
- no hidden magic numbers
- readable control flow
- step-by-step construction
- Header-only

## Encoding Pipeline

The QR code is generated in several clear steps:

1. Encode input data
2. Add error correction codewords
3. Construct QR matrix
4. Apply mask patterns
5. Select the best mask
6. Write format information

## Project Structure
.
├── include
│   └── QRcreator.h					Main encoding pipeline
├── internal
│   ├── byte
│   │   ├── ByteEncoder.h			Convert to bitstream
│   │   ├── FinalStream.h			Intertwined
│   │   └── ECC
│   │       └── ECCCalculator.h		Reed-Solomon error correction
│   ├── Mask
│   │   └── Mask.h					Mask evaluation
│   ├── QRSpec
│   │   ├── QRSpec.h				Constants provided by ISO/IEC 18004:2015 simplify calculations during generation.
│   │   ├── QRSpec_Blocks.h
│   │   ├── QRSpec_Capacity.h
│   │   └── QRSpec_Patterns.h
│   └── Structure
│       └── StructureBuilder.h		Draw a fixed template
└── demo
    └── main.cpp					Simple demo

## Reference

ISO/IEC 18004:2015
QR Code specification
