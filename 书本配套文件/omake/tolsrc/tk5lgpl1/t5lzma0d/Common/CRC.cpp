// Common/CRC.cpp

#include "StdAfx.h"

#include "CRC.h"

static const UInt32 kCRCPoly = 0xEDB88320;

UInt32 CCRC::Table[256];

void CCRC::InitTable()
{
  for (UInt32 i = 0; i < 256; i++)
  {
    UInt32 r = i;
    for (int j = 0; j < 8; j++)
      if (r & 1) 
        r = (r >> 1) ^ kCRCPoly;
      else     
        r >>= 1;
    CCRC::Table[i] = r;
  }
}

class CCRCTableInit
{
public:
  CCRCTableInit() { CCRC::InitTable(); }
} g_CRCTableInit;

void CCRC::Update(Byte b)
{
  _value = Table[((Byte)(_value)) ^ b] ^ (_value >> 8);
}

void CCRC::Update(UInt32 v)
{
  for (int i = 0; i < 4; i++)
    Update((Byte)(v >> (8 * i)));
}

void CCRC::Update(const UInt64 &v)
{
  for (int i = 0; i < 8; i++)
    Update((Byte)(v >> (8 * i)));
}

void CCRC::Update(const void *data, UInt32 size)
{
  UInt32 v = _value;
  const Byte *p = (const Byte *)data;
  for (; size > 0 ; size--, p++)
    v = Table[((Byte)(v)) ^ *p] ^ (v >> 8);
  _value = v;
}
