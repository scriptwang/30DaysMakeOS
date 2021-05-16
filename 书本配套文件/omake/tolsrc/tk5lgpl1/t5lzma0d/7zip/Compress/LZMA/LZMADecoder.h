// LZMA/Decoder.h

#ifndef __LZMA_DECODER_H
#define __LZMA_DECODER_H

#include "../../../Common/MyCom.h"
#include "../../ICoder.h"
#include "../LZ/LZOutWindow.h"
#include "../RangeCoder/RangeCoderBitTree.h"

#include "LZMA.h"

namespace NCompress {
namespace NLZMA {

typedef NRangeCoder::CBitDecoder<kNumMoveBits> CMyBitDecoder;

class CLiteralDecoder2
{
  CMyBitDecoder _decoders[0x300];
public:
  void Init()
  {
    for (int i = 0; i < 0x300; i++)
      _decoders[i].Init();
  }
  Byte DecodeNormal(NRangeCoder::CDecoder *rangeDecoder)
  {
    UInt32 symbol = 1;
    RC_INIT_VAR
    do
    {
      // symbol = (symbol << 1) | _decoders[0][symbol].Decode(rangeDecoder);
      RC_GETBIT(kNumMoveBits, _decoders[symbol].Prob, symbol)
    }
    while (symbol < 0x100);
    RC_FLUSH_VAR
    return (Byte)symbol;
  }
  Byte DecodeWithMatchByte(NRangeCoder::CDecoder *rangeDecoder, Byte matchByte)
  {
    UInt32 symbol = 1;
    RC_INIT_VAR
    do
    {
      UInt32 matchBit = (matchByte >> 7) & 1;
      matchByte <<= 1;
      // UInt32 bit = _decoders[1 + matchBit][symbol].Decode(rangeDecoder);
      // symbol = (symbol << 1) | bit;
      UInt32 bit;
      RC_GETBIT2(kNumMoveBits, _decoders[((1 + matchBit) << 8) + symbol].Prob, symbol, 
          bit = 0, bit = 1)
      if (matchBit != bit)
      {
        while (symbol < 0x100)
        {
          // symbol = (symbol << 1) | _decoders[0][symbol].Decode(rangeDecoder);
          RC_GETBIT(kNumMoveBits, _decoders[symbol].Prob, symbol)
        }
        break;
      }
    }
    while (symbol < 0x100);
    RC_FLUSH_VAR
    return (Byte)symbol;
  }
};

class CLiteralDecoder
{
  CLiteralDecoder2 *_coders;
  int _numPrevBits;
  int _numPosBits;
  UInt32 _posMask;
public:
  CLiteralDecoder(): _coders(0) {}
  ~CLiteralDecoder()  { Free(); }
  void Free()
  { 
    delete []_coders;
    _coders = 0;
  }
  void Create(int numPosBits, int numPrevBits)
  {
    if (_coders == 0 || (numPosBits + numPrevBits) != 
        (_numPrevBits + _numPosBits) )
    {
      Free();
      UInt32 numStates = 1 << (numPosBits + numPrevBits);
      _coders = new CLiteralDecoder2[numStates];
    }
    _numPosBits = numPosBits;
    _posMask = (1 << numPosBits) - 1;
    _numPrevBits = numPrevBits;
  }
  void Init()
  {
    UInt32 numStates = 1 << (_numPrevBits + _numPosBits);
    for (UInt32 i = 0; i < numStates; i++)
      _coders[i].Init();
  }
  UInt32 GetState(UInt32 pos, Byte prevByte) const
    { return ((pos & _posMask) << _numPrevBits) + (prevByte >> (8 - _numPrevBits)); }
  Byte DecodeNormal(NRangeCoder::CDecoder *rangeDecoder, UInt32 pos, Byte prevByte)
    { return _coders[GetState(pos, prevByte)].DecodeNormal(rangeDecoder); }
  Byte DecodeWithMatchByte(NRangeCoder::CDecoder *rangeDecoder, UInt32 pos, Byte prevByte, Byte matchByte)
    { return _coders[GetState(pos, prevByte)].DecodeWithMatchByte(rangeDecoder, matchByte); }
};

namespace NLength {

class CDecoder
{
  CMyBitDecoder _choice;
  NRangeCoder::CBitTreeDecoder<kNumMoveBits, kNumLowBits>  _lowCoder[kNumPosStatesMax];
  CMyBitDecoder _choice2;
  NRangeCoder::CBitTreeDecoder<kNumMoveBits, kNumMidBits>  _midCoder[kNumPosStatesMax];
  NRangeCoder::CBitTreeDecoder<kNumMoveBits, kNumHighBits> _highCoder; 
public:
  void Init(UInt32 numPosStates)
  {
    _choice.Init();
    for (UInt32 posState = 0; posState < numPosStates; posState++)
    {
      _lowCoder[posState].Init();
      _midCoder[posState].Init();
    }
    _choice2.Init();
    _highCoder.Init();
  }
	/* !!! */
	UInt32 Decode(NRangeCoder::CDecoder *rangeDecoder, UInt32 posState)
	{
		int i;
		if (_choice.Decode(rangeDecoder) == 0)
			return _lowCoder[posState].Decode(rangeDecoder);
		if (_choice2.Decode(rangeDecoder) == 0)
			return kNumLowSymbols + _midCoder[posState].Decode(rangeDecoder);
		i = _highCoder.Decode(rangeDecoder) - (256 - 8);
		if (i > 0) {
			i = (1 << i | rangeDecoder->DecodeDirectBits(i)) - 1;
			i = (1 << i | rangeDecoder->DecodeDirectBits(i)) - 1;
		}
		return (kNumLowSymbols + kNumMidSymbols + (256 - 8)) + i;
	}
};

}

class CDecoder: 
  public ICompressCoder,
  public ICompressSetDecoderProperties,
  public CMyUnknownImp
{
  CLZOutWindow _outWindowStream;
  NRangeCoder::CDecoder _rangeDecoder;

  CMyBitDecoder _isMatch[kNumStates][NLength::kNumPosStatesMax];
  CMyBitDecoder _isRep[kNumStates];
  CMyBitDecoder _isRepG0[kNumStates];
  CMyBitDecoder _isRepG1[kNumStates];
  CMyBitDecoder _isRepG2[kNumStates];
  CMyBitDecoder _isRep0Long[kNumStates][NLength::kNumPosStatesMax];

  NRangeCoder::CBitTreeDecoder<kNumMoveBits, kNumPosSlotBits> _posSlotDecoder[kNumLenToPosStates];

  CMyBitDecoder _posDecoders[kNumFullDistances - kEndPosModelIndex];
  NRangeCoder::CReverseBitTreeDecoder<kNumMoveBits, kNumAlignBits> _posAlignDecoder;
  
  NLength::CDecoder _lenDecoder;
  NLength::CDecoder _repMatchLenDecoder;

  CLiteralDecoder _literalDecoder;

  UInt32 _dictionarySizeCheck;
  
  UInt32 _posStateMask;

public:
  MY_UNKNOWN_IMP1(ICompressSetDecoderProperties)

  HRESULT Init(ISequentialInStream *inStream, 
      ISequentialOutStream *outStream);
  /*
  void ReleaseStreams()
  {
    _outWindowStream.ReleaseStream();
    _rangeDecoder.ReleaseStream();
  }
  */

  class CDecoderFlusher
  {
    CDecoder *_decoder;
  public:
    bool NeedFlush;
    CDecoderFlusher(CDecoder *decoder): 
          _decoder(decoder), NeedFlush(true) {}
    ~CDecoderFlusher() 
    { 
      if (NeedFlush)
        _decoder->Flush();
      // _decoder->ReleaseStreams(); 
    }
  };

  HRESULT Flush() {  return _outWindowStream.Flush(); }  

  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  // ICompressSetDecoderProperties
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream);
};

}}

#endif
