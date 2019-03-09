#pragma once

#include "pch.h"

#ifdef _WIN32
#include "../C/DllSecur.h"
#endif

#include "../CPP/Common/Common.h"

#include "../CPP/Common/MyException.h"
#include "../CPP/Common/StdOutStream.h"

#include "../CPP/Windows/ErrorMsg.h"
#include "../CPP/Windows/NtCheck.h"

#include "../CPP/7zip/UI/Common/ArchiveCommandLine.h"
#include "../CPP/7zip/UI/Common/ExitCode.h"

#include "../CPP/7zip/UI/Console/ConsoleClose.h"


extern CStdOutStream *g_StdStream;
extern CStdOutStream *g_ErrStream;

int Release(CCodecs* codecs);

int Init(UStringVector& cmdstrings,
    CArcCmdLineOptions& options,
    CCodecs* &outcodecs,
    CObjectVector<COpenType>& types,
    CIntVector& excludedFormats);

int Execute(CArcCmdLineOptions& options,
    CCodecs* codecs,
    CObjectVector<COpenType>& types,
    CIntVector& excludedFormats);

int DecryptingExecute(CArcCmdLineOptions& options,
    CCodecs* codecs,
    CObjectVector<COpenType>& types,
    CIntVector& excludedFormats,
    wchar_t* PatternFile);