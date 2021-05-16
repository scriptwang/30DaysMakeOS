// LzmaAlone.cpp

#include "StdAfx.h"

#ifdef WIN32
#include <initguid.h>
#else
#define INITGUID
#endif

#include "../../../Common/MyWindows.h"

// #include <limits.h>
#include <stdio.h>

#if defined(WIN32) || defined(OS2) || defined(MSDOS)
#include <fcntl.h>
#include <io.h>
#define MY_SET_BINARY_MODE(file) setmode(fileno(file),O_BINARY)
#else
#define MY_SET_BINARY_MODE(file)
#endif

#include "../../../Common/CommandLineParser.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/StringToInt.h"

// #include "Windows/PropVariant.h"

#include "../../Common/FileStreams.h"

#include "../LZMA/LZMADecoder.h"
#include "../LZMA/LZMAEncoder.h"

#include "LzmaBench.h"

using namespace NCommandLineParser;

namespace NKey {
enum Enum
{
  kHelp1 = 0,
  kHelp2,
  kMode,
  kDictionary,
  kFastBytes,
  kLitContext,
  kLitPos,
  kPosBits,
  kMatchFinder,
  kEOS,
  kStdIn,
  kStdOut
};
}

static const CSwitchForm kSwitchForms[] = 
{
  { L"?",  NSwitchType::kSimple, false },
  { L"H",  NSwitchType::kSimple, false },
  { L"A", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"D", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"FB", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"LC", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"LP", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"PB", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"MF", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"EOS", NSwitchType::kSimple, false },
  { L"SI",  NSwitchType::kSimple, false },
  { L"SO",  NSwitchType::kSimple, false }
};

static const int kNumSwitches = sizeof(kSwitchForms) / sizeof(kSwitchForms[0]);

static void PrintHelp()
{
  fprintf(stderr, "\nUsage:  t5lzma <e|d> inputFile outputFile [<switches>...]\n"
             "  e: encode file\n"
             "  d: decode file\n"
             "  b: Benchmark\n"
    "<Switches>\n"
    "  -a{N}:  set compression mode - [0, 2], default: 2 (max)\n"
    "  -d{N}:  set dictionary - [0,28], default: 23 (8MB)\n"
    "  -fb{N}: set number of fast bytes - [5, 255], default: 128\n"
    "  -lc{N}: set number of literal context bits - [0, 8], default: 0\n"
    "  -lp{N}: set number of literal pos bits - [0, 4], default: 0\n"
    "  -pb{N}: set number of pos bits - [0, 4], default: 0\n"
    "  -mf{MF_ID}: set Match Finder: [bt2, bt3, bt4, bt4b, pat2r, pat2,\n"
    "              pat2h, pat3h, pat4h, hc3, hc4], default: bt4\n"
    "  -eos:   write End Of Stream marker\n"
    "  -si:    Read data from stdin\n"
    "  -so:    Write data to stdout\n"
    );
}

static void PrintHelpAndExit(const char *s)
{
  fprintf(stderr, "\nError: %s\n\n", s);
  PrintHelp();
  throw -1;
}

static void IncorrectCommand()
{
  PrintHelpAndExit("Incorrect command");
}

static void WriteArgumentsToStringList(int numArguments, const char *arguments[], 
    UStringVector &strings)
{
  for(int i = 1; i < numArguments; i++)
    strings.Add(MultiByteToUnicodeString(arguments[i]));
}

static bool GetNumber(const wchar_t *s, UInt32 &value)
{
  value = 0;
  if (MyStringLen(s) == 0)
    return false;
  const wchar_t *end;
  UInt64 res = ConvertStringToUINT64(s, &end);
  if (*end != L'\0')
    return false;
  if (res > 0xFFFFFFFF)
    return false;
  value = UInt32(res);
  return true;
}

int main2(int n, const char *args[])
{
	if (strcmp(args[n - 1], "-notitle") == 0)
		n--; 
	else {
		fprintf(stderr,
			"\nt5lzma 1.00 Copyright (C) 2004 H.Kawai\n"
			" --- based : LZMA 4.03 Copyright (c) 1999-2004 Igor Pavlov  2004-06-18\n"
		);
	}

  if (n == 1)
  {
    PrintHelp();
    return 0;
  }

  if (sizeof(Byte) != 1 || sizeof(UInt32) < 4 || sizeof(UInt64) < 4)
  {
    fprintf(stderr, "Unsupported base types. Edit Common/Types.h and recompile");
    return 1;
  }   

  UStringVector commandStrings;
  WriteArgumentsToStringList(n, args, commandStrings);
  CParser parser(kNumSwitches);
  try
  {
    parser.ParseStrings(kSwitchForms, commandStrings);
  }
  catch(...) 
  {
    IncorrectCommand();
  }

  if(parser[NKey::kHelp1].ThereIs || parser[NKey::kHelp2].ThereIs)
  {
    PrintHelp();
    return 0;
  }
  const UStringVector &nonSwitchStrings = parser.NonSwitchStrings;

  int paramIndex = 0;
  if (paramIndex >= nonSwitchStrings.Size())
    IncorrectCommand();
  const UString &command = nonSwitchStrings[paramIndex++]; 

  if (command.CompareNoCase(L"b") == 0)
  {
    UInt32 dictionary = 1 << 21;
    if(parser[NKey::kDictionary].ThereIs)
    {
      UInt32 dicLog;
      if (!GetNumber(parser[NKey::kDictionary].PostStrings[0], dicLog))
        IncorrectCommand();
      dictionary = 1 << dicLog;
    }
    const UInt32 kNumDefaultItereations = 10;
    UInt32 numIterations = kNumDefaultItereations;
    {
      if (paramIndex < nonSwitchStrings.Size())
        if (!GetNumber(nonSwitchStrings[paramIndex++], numIterations))
          numIterations = kNumDefaultItereations;
    }
    return LzmaBenchmark(numIterations, dictionary);
  }

  bool encodeMode;
  if (command.CompareNoCase(L"e") == 0)
    encodeMode = true;
  else if (command.CompareNoCase(L"d") == 0)
    encodeMode = false;
  else
    IncorrectCommand();

  bool stdInMode = parser[NKey::kStdIn].ThereIs;
  bool stdOutMode = parser[NKey::kStdOut].ThereIs;

  CMyComPtr<ISequentialInStream> inStream;
  CInFileStream *inStreamSpec = 0;
  if (stdInMode)
  {
    inStream = new CStdInFileStream;
    MY_SET_BINARY_MODE(stdin);
  }
  else
  {
    if (paramIndex >= nonSwitchStrings.Size())
      IncorrectCommand();
    const UString &inputName = nonSwitchStrings[paramIndex++]; 
    inStreamSpec = new CInFileStream;
    inStream = inStreamSpec;
    if (!inStreamSpec->Open(GetSystemString(inputName)))
    {
      fprintf(stderr, "\nError: can not open input file %s\n", 
          (const char *)GetOemString(inputName));
      return 1;
    }
  }

  CMyComPtr<ISequentialOutStream> outStream;
  if (stdOutMode)
  {
    outStream = new CStdOutFileStream;
    MY_SET_BINARY_MODE(stdout);
  }
  else
  {
    if (paramIndex >= nonSwitchStrings.Size())
      IncorrectCommand();
    const UString &outputName = nonSwitchStrings[paramIndex++]; 
    COutFileStream *outStreamSpec = new COutFileStream;
    outStream = outStreamSpec;
    if (!outStreamSpec->Open(GetSystemString(outputName)))
    {
      fprintf(stderr, "\nError: can not open output file %s\n", 
        (const char *)GetOemString(outputName));
      return 1;
    }
  }

  UInt64 fileSize;
  if (encodeMode)
  {
    NCompress::NLZMA::CEncoder *encoderSpec = 
      new NCompress::NLZMA::CEncoder;
    CMyComPtr<ICompressCoder> encoder = encoderSpec;

    UInt32 dictionary = 1 << 23;
    UInt32 posStateBits = 0;
    // UInt32 posStateBits = 2;
    // UInt32 litContextBits = 3; // for normal files
    UInt32 litContextBits = 0; // for 32-bit data
    UInt32 litPosBits = 0;
    // UInt32 litPosBits = 2; // for 32-bit data
    UInt32 algorithm = 2;
    UInt32 numFastBytes = 128;
    UString mf = L"BT4";

    bool eos = parser[NKey::kEOS].ThereIs || stdInMode;
 
    if(parser[NKey::kMode].ThereIs)
      if (!GetNumber(parser[NKey::kMode].PostStrings[0], algorithm))
        IncorrectCommand();

    if(parser[NKey::kDictionary].ThereIs)
    {
      UInt32 dicLog;
      if (!GetNumber(parser[NKey::kDictionary].PostStrings[0], dicLog))
        IncorrectCommand();
      dictionary = 1 << dicLog;
    }
    if(parser[NKey::kFastBytes].ThereIs)
      if (!GetNumber(parser[NKey::kFastBytes].PostStrings[0], numFastBytes))
        IncorrectCommand();
    if(parser[NKey::kLitContext].ThereIs)
      if (!GetNumber(parser[NKey::kLitContext].PostStrings[0], litContextBits))
        IncorrectCommand();
    if(parser[NKey::kLitPos].ThereIs)
      if (!GetNumber(parser[NKey::kLitPos].PostStrings[0], litPosBits))
        IncorrectCommand();
    if(parser[NKey::kPosBits].ThereIs)
      if (!GetNumber(parser[NKey::kPosBits].PostStrings[0], posStateBits))
        IncorrectCommand();

    if (parser[NKey::kMatchFinder].ThereIs)
      mf = parser[NKey::kMatchFinder].PostStrings[0];

    PROPID propIDs[] = 
    {
      NCoderPropID::kDictionarySize,
      NCoderPropID::kPosStateBits,
      NCoderPropID::kLitContextBits,
      NCoderPropID::kLitPosBits,
      NCoderPropID::kAlgorithm,
      NCoderPropID::kNumFastBytes,
      NCoderPropID::kMatchFinder,
      NCoderPropID::kEndMarker
    };
    const int kNumProps = sizeof(propIDs) / sizeof(propIDs[0]);
    /*
    NWindows::NCOM::CPropVariant properties[kNumProps];
    properties[0] = UInt32(dictionary);
    properties[1] = UInt32(posStateBits);
    properties[2] = UInt32(litContextBits);
   
    properties[3] = UInt32(litPosBits);
    properties[4] = UInt32(algorithm);
    properties[5] = UInt32(numFastBytes);
    properties[6] = mf;
    properties[7] = eos;
    */
    PROPVARIANT properties[kNumProps];
    for (int p = 0; p < 6; p++)
      properties[p].vt = VT_UI4;
    properties[0].ulVal = UInt32(dictionary);
    properties[1].ulVal = UInt32(posStateBits);
    properties[2].ulVal = UInt32(litContextBits);
    properties[3].ulVal = UInt32(litPosBits);
    properties[4].ulVal = UInt32(algorithm);
    properties[5].ulVal = UInt32(numFastBytes);
    
    properties[6].vt = VT_BSTR;
    properties[6].bstrVal = (BSTR)(const wchar_t *)mf;

    properties[7].vt = VT_BOOL;
    properties[7].boolVal = eos ? VARIANT_TRUE : VARIANT_FALSE;

    if (encoderSpec->SetCoderProperties(propIDs, properties, kNumProps) != S_OK)
      IncorrectCommand();
    encoderSpec->WriteCoderProperties(outStream);

    if (eos || stdInMode)
      fileSize = (UInt64)(Int64)-1;
    else
      inStreamSpec->File.GetLength(fileSize);

    for (int i = 0; i < 8; i++)
    {
      Byte b = Byte(fileSize >> (8 * i));
      if (outStream->Write(&b, sizeof(b), 0) != S_OK)
      {
        fprintf(stderr, "Write error");
        return 1;
      }
    }
    HRESULT result = encoder->Code(inStream, outStream, 0, 0, 0);
    if (result == E_OUTOFMEMORY)
    {
      fprintf(stderr, "\nError: Can not allocate memory\n");
      return 1;
    }   
    else if (result != S_OK)
    {
      fprintf(stderr, "\nEncoder error = %X\n", result);
      return 1;
    }   
  }
  else
  {
    NCompress::NLZMA::CDecoder *decoderSpec = 
        new NCompress::NLZMA::CDecoder;
    CMyComPtr<ICompressCoder> decoder = decoderSpec;
    if (decoderSpec->SetDecoderProperties(inStream) != S_OK)
    {
      fprintf(stderr, "SetDecoderProperties error");
      return 1;
    }
    fileSize = 0;
    for (int i = 0; i < 8; i++)
    {
      Byte b;
      UInt32 processedSize;
      if (inStream->Read(&b, sizeof(b), &processedSize) != S_OK)
      {
        fprintf(stderr, "Read error");
        return 1;
      }
      if (processedSize != 1)
      {
        fprintf(stderr, "Read error");
        return 1;
      }
      fileSize |= ((UInt64)b) << (8 * i);
    }
    if (decoder->Code(inStream, outStream, 0, &fileSize, 0) != S_OK)
    {
      fprintf(stderr, "Decoder error");
      return 1;
    }   
  }
  return 0;
}

int main(int n, const char *args[])
{
  try { return main2(n, args); }
  catch(const char *s) 
  { 
    fprintf(stderr, "\nError: %s\n", s);
    return 1; 
  }
  catch(...) 
  { 
    fprintf(stderr, "\nError\n");
    return 1; 
  }
}
