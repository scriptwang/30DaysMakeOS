// Compress/RangeCoder/RangeCoderBitTree.h

#ifndef __COMPRESS_RANGECODER_BIT_TREE_H
#define __COMPRESS_RANGECODER_BIT_TREE_H

#include "RangeCoderBit.h"
#include "RangeCoderOpt.h"

namespace NCompress {
namespace NRangeCoder {

//////////////////////////
// CBitTreeEncoder

template <int numMoveBits, UInt32 NumBitLevels>
class CBitTreeEncoder
{
  CBitEncoder<numMoveBits> Models[1 << NumBitLevels];
public:
  void Init()
  {
    for(UInt32 i = 1; i < (1 << NumBitLevels); i++)
      Models[i].Init();
  }
  void Encode(CEncoder *rangeEncoder, UInt32 symbol)
  {
    UInt32 modelIndex = 1;
    for (UInt32 bitIndex = NumBitLevels; bitIndex > 0 ;)
    {
      bitIndex--;
      UInt32 bit = (symbol >> bitIndex) & 1;
      Models[modelIndex].Encode(rangeEncoder, bit);
      modelIndex = (modelIndex << 1) | bit;
    }
  };
  UInt32 GetPrice(UInt32 symbol) const
  {
    UInt32 price = 0;
    UInt32 modelIndex = 1;
    for (UInt32 bitIndex = NumBitLevels; bitIndex > 0 ;)
    {
      bitIndex--;
      UInt32 bit = (symbol >> bitIndex) & 1;
      price += Models[modelIndex].GetPrice(bit);
      modelIndex = (modelIndex << 1) + bit;
    }
    return price;
  }
};

//////////////////////////
// CBitTreeDecoder

template <int numMoveBits, UInt32 NumBitLevels>
class CBitTreeDecoder
{
  CBitDecoder<numMoveBits> Models[1 << NumBitLevels];
public:
  void Init()
  {
    for(UInt32 i = 1; i < (1 << NumBitLevels); i++)
      Models[i].Init();
  }
  UInt32 Decode(CDecoder *rangeDecoder)
  {
    UInt32 modelIndex = 1;
    RC_INIT_VAR
    for(UInt32 bitIndex = NumBitLevels; bitIndex > 0; bitIndex--)
    {
      // modelIndex = (modelIndex << 1) + Models[modelIndex].Decode(rangeDecoder);
      RC_GETBIT(numMoveBits, Models[modelIndex].Prob, modelIndex)
    }
    RC_FLUSH_VAR
    return modelIndex - (1 << NumBitLevels);
  };
};

////////////////////////////////
// CReverseBitTreeEncoder

template <int numMoveBits>
class CReverseBitTreeEncoder2
{
  CBitEncoder<numMoveBits> *Models;
  UInt32 NumBitLevels;
public:
  CReverseBitTreeEncoder2(): Models(0) { }
  ~CReverseBitTreeEncoder2() { delete []Models; }
  void Create(UInt32 numBitLevels)
  {
    NumBitLevels = numBitLevels;
    Models = new CBitEncoder<numMoveBits>[1 << numBitLevels];
    // return (Models != 0);
  }
  void Init()
  {
    UInt32 numModels = 1 << NumBitLevels;
    for(UInt32 i = 1; i < numModels; i++)
      Models[i].Init();
  }
  void Encode(CEncoder *rangeEncoder, UInt32 symbol)
  {
    UInt32 modelIndex = 1;
    for (UInt32 i = 0; i < NumBitLevels; i++)
    {
      UInt32 bit = symbol & 1;
      Models[modelIndex].Encode(rangeEncoder, bit);
      modelIndex = (modelIndex << 1) | bit;
      symbol >>= 1;
    }
  }
  UInt32 GetPrice(UInt32 symbol) const
  {
    UInt32 price = 0;
    UInt32 modelIndex = 1;
    for (UInt32 i = NumBitLevels; i > 0; i--)
    {
      UInt32 bit = symbol & 1;
      symbol >>= 1;
      price += Models[modelIndex].GetPrice(bit);
      modelIndex = (modelIndex << 1) | bit;
    }
    return price;
  }
};

////////////////////////////////
// CReverseBitTreeDecoder

template <int numMoveBits>
class CReverseBitTreeDecoder2
{
  CBitDecoder<numMoveBits> *Models;
  UInt32 NumBitLevels;
public:
  CReverseBitTreeDecoder2(): Models(0) { }
  ~CReverseBitTreeDecoder2() { delete []Models; }
  void Create(UInt32 numBitLevels)
  {
    NumBitLevels = numBitLevels;
    Models = new CBitDecoder<numMoveBits>[1 << numBitLevels];
    // return (Models != 0);
  }
  void Init()
  {
    UInt32 numModels = 1 << NumBitLevels;
    for(UInt32 i = 1; i < numModels; i++)
      Models[i].Init();
  }
  UInt32 Decode(CDecoder *rangeDecoder)
  {
    UInt32 modelIndex = 1;
    UInt32 symbol = 0;
    RC_INIT_VAR
    for(UInt32 bitIndex = 0; bitIndex < NumBitLevels; bitIndex++)
    {
      // UInt32 bit = Models[modelIndex].Decode(rangeDecoder);
      // modelIndex <<= 1;
      // modelIndex += bit;
      // symbol |= (bit << bitIndex);
      RC_GETBIT2(numMoveBits, Models[modelIndex].Prob, modelIndex, ; , symbol |= (1 << bitIndex))
    }
    RC_FLUSH_VAR
    return symbol;
  };
};
////////////////////////////
// CReverseBitTreeDecoder2

template <int numMoveBits>
UInt32 ReverseBitTreeDecode(CBitDecoder<numMoveBits> *Models, 
    CDecoder *rangeDecoder, UInt32 NumBitLevels)
{
  UInt32 modelIndex = 1;
  UInt32 symbol = 0;
  RC_INIT_VAR
  for(UInt32 bitIndex = 0; bitIndex < NumBitLevels; bitIndex++)
  {
    // UInt32 bit = Models[modelIndex].Decode(rangeDecoder);
    // modelIndex <<= 1;
    // modelIndex += bit;
    // symbol |= (bit << bitIndex);
    RC_GETBIT2(numMoveBits, Models[modelIndex].Prob, modelIndex, ; , symbol |= (1 << bitIndex))
  }
  RC_FLUSH_VAR
  return symbol;
}

template <int numMoveBits, UInt32 NumBitLevels>
class CReverseBitTreeDecoder
{
  CBitDecoder<numMoveBits> Models[1 << NumBitLevels];
public:
  void Init()
  {
    for(UInt32 i = 1; i < (1 << NumBitLevels); i++)
      Models[i].Init();
  }
  UInt32 Decode(CDecoder *rangeDecoder)
  {
    UInt32 modelIndex = 1;
    UInt32 symbol = 0;
    RC_INIT_VAR
    for(UInt32 bitIndex = 0; bitIndex < NumBitLevels; bitIndex++)
    {
      // UInt32 bit = Models[modelIndex].Decode(rangeDecoder);
      // modelIndex <<= 1;
      // modelIndex += bit;
      // symbol |= (bit << bitIndex);
      RC_GETBIT2(numMoveBits, Models[modelIndex].Prob, modelIndex, ; , symbol |= (1 << bitIndex))
    }
    RC_FLUSH_VAR
    return symbol;
  }
};

/*
//////////////////////////
// CBitTreeEncoder2

template <int numMoveBits>
class CBitTreeEncoder2
{
  NCompression::NArithmetic::CBitEncoder<numMoveBits> *Models;
  UInt32 NumBitLevels;
public:
  bool Create(UInt32 numBitLevels)
  { 
    NumBitLevels = numBitLevels;
    Models = new NCompression::NArithmetic::CBitEncoder<numMoveBits>[1 << numBitLevels];
    return (Models != 0);
  }
  void Init()
  {
    UInt32 numModels = 1 << NumBitLevels;
    for(UInt32 i = 1; i < numModels; i++)
      Models[i].Init();
  }
  void Encode(CMyRangeEncoder *rangeEncoder, UInt32 symbol)
  {
    UInt32 modelIndex = 1;
    for (UInt32 bitIndex = NumBitLevels; bitIndex > 0 ;)
    {
      bitIndex--;
      UInt32 bit = (symbol >> bitIndex ) & 1;
      Models[modelIndex].Encode(rangeEncoder, bit);
      modelIndex = (modelIndex << 1) | bit;
    }
  }
  UInt32 GetPrice(UInt32 symbol) const
  {
    UInt32 price = 0;
    UInt32 modelIndex = 1;
    for (UInt32 bitIndex = NumBitLevels; bitIndex > 0 ;)
    {
      bitIndex--;
      UInt32 bit = (symbol >> bitIndex ) & 1;
      price += Models[modelIndex].GetPrice(bit);
      modelIndex = (modelIndex << 1) + bit;
    }
    return price;
  }
};


//////////////////////////
// CBitTreeDecoder2

template <int numMoveBits>
class CBitTreeDecoder2
{
  NCompression::NArithmetic::CBitDecoder<numMoveBits> *Models;
  UInt32 NumBitLevels;
public:
  bool Create(UInt32 numBitLevels)
  { 
    NumBitLevels = numBitLevels;
    Models = new NCompression::NArithmetic::CBitDecoder<numMoveBits>[1 << numBitLevels];
    return (Models != 0);
  }
  void Init()
  {
    UInt32 numModels = 1 << NumBitLevels;
    for(UInt32 i = 1; i < numModels; i++)
      Models[i].Init();
  }
  UInt32 Decode(CMyRangeDecoder *rangeDecoder)
  {
    UInt32 modelIndex = 1;
    RC_INIT_VAR
    for(UInt32 bitIndex = NumBitLevels; bitIndex > 0; bitIndex--)
    {
      // modelIndex = (modelIndex << 1) + Models[modelIndex].Decode(rangeDecoder);
      RC_GETBIT(numMoveBits, Models[modelIndex].Prob, modelIndex)
    }
    RC_FLUSH_VAR
    return modelIndex - (1 << NumBitLevels);
  }
};
*/

}}

#endif
