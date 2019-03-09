#pragma once
// ExtractCallbackConsole.h

#ifndef __EXTRACT_CALLBACK_CONSOLE_DECRYPT_H
#define __EXTRACT_CALLBACK_CONSOLE_DECRYPT_H

#include "../CPP/Common/Common.h"

#include "../CPP/Common/StdOutStream.h"

#include "../CPP/7zip/IPassword.h"

#include "../CPP/7zip/Archive/IArchive.h"

#include "../CPP/7zip/UI/Common/ArchiveExtractCallback.h"

#include "../CPP/7zip/UI/Console/PercentPrinter.h"

#include "../CPP/7zip/UI/Console/OpenCallbackConsole.h"
#include "../CPP/7zip/UI/Console/ExtractCallbackConsole.h"
//
//#include "../CPP/Common/MyWindows.h"
//
//
//#include "../CPP/Common/IntToString.h"
//#include "../CPP/Common/Wildcard.h"
//
//#include "../CPP/Windows/FileDir.h"
//#include "../CPP/Windows/FileFind.h"
//#include "../CPP/Windows/TimeUtils.h"
//#include "../CPP/Windows/ErrorMsg.h"
//#include "../CPP/Windows/PropVariantConv.h"
//
//#ifndef _7ZIP_ST
//#include "../CPP/Windows/Synchronization.h"
//#endif
//
//#include "../CPP/7zip/Common/FilePathAutoRename.h"
//
//#include "../CPP/7zip/UI/Common/ExtractingFilePath.h"
//
//#include "../CPP/7zip/UI/Console/ConsoleClose.h"
//#include "../CPP/7zip/UI/Console/ExtractCallbackConsole.h"
//#include "../CPP/7zip/UI/Console/UserInputUtils.h"
//
//#include "../C/7zTypes.h"

class CDecryptCallbackConsole : public CExtractCallbackConsole
{
    Int32 OpResult;

    AString _tempA;
    UString _tempU;

    UString _currentName;

    void ClosePercents_for_so()
    {
        if (NeedPercents() && _so == _percent._so)
            _percent.ClosePrint(false);
    }

    void ClosePercentsAndFlush()
    {
        if (NeedPercents())
            _percent.ClosePrint(true);
        if (_so)
            _so->Flush();
    }

public:

    CDecryptCallbackConsole() : CExtractCallbackConsole()
    {
    }

    bool IsWrongPassword()
    {
        return
            ((OpResult == NArchive::NExtract::NOperationResult::kDataError)||
             (OpResult == NArchive::NExtract::NOperationResult::kWrongPassword));
    }

    Int32 GetOpResult()
    {
        return OpResult;
    }

    STDMETHODIMP SetOperationResult(Int32 opRes, Int32 encrypted) override;
    STDMETHODIMP SetTotal(UInt64 size) override;
    STDMETHODIMP SetCompleted(const UInt64 *completeValue) override;
    STDMETHODIMP PrepareOperation(const wchar_t *name, Int32 /* isFolder */, Int32 askExtractMode, const UInt64 *position) override;
};

#if 0
class CExtractCallbackConsole :
    public IExtractCallbackUI,
    // public IArchiveExtractCallbackMessage,
    public IFolderArchiveExtractCallback2,
#ifndef _NO_CRYPTO
    public ICryptoGetTextPassword,
#endif
    public COpenCallbackConsole,
    public CMyUnknownImp
{
    AString _tempA;
    UString _tempU;

    UString _currentName;

    void ClosePercents_for_so()
    {
        if (NeedPercents() && _so == _percent._so)
            _percent.ClosePrint(false);
    }

    void ClosePercentsAndFlush()
    {
        if (NeedPercents())
            _percent.ClosePrint(true);
        if (_so)
            _so->Flush();
    }

public:
    MY_QUERYINTERFACE_BEGIN2(IFolderArchiveExtractCallback)
        // MY_QUERYINTERFACE_ENTRY(IArchiveExtractCallbackMessage)
        MY_QUERYINTERFACE_ENTRY(IFolderArchiveExtractCallback2)
#ifndef _NO_CRYPTO
        MY_QUERYINTERFACE_ENTRY(ICryptoGetTextPassword)
#endif
        MY_QUERYINTERFACE_END
        MY_ADDREF_RELEASE

        STDMETHOD(SetTotal)(UInt64 total);
    STDMETHOD(SetCompleted)(const UInt64 *completeValue);

    INTERFACE_IFolderArchiveExtractCallback(;)

        INTERFACE_IExtractCallbackUI(;)
        // INTERFACE_IArchiveExtractCallbackMessage(;)
        INTERFACE_IFolderArchiveExtractCallback2(;)

#ifndef _NO_CRYPTO

        STDMETHOD(CryptoGetTextPassword)(BSTR *password);

#endif

    UInt64 NumTryArcs;

    bool ThereIsError_in_Current;
    bool ThereIsWarning_in_Current;

    UInt64 NumOkArcs;
    UInt64 NumCantOpenArcs;
    UInt64 NumArcsWithError;
    UInt64 NumArcsWithWarnings;

    UInt64 NumOpenArcErrors;
    UInt64 NumOpenArcWarnings;

    UInt64 NumFileErrors;
    UInt64 NumFileErrors_in_Current;

    bool NeedFlush;
    unsigned PercentsNameLevel;
    unsigned LogLevel;

    CExtractCallbackConsole() :
        NeedFlush(false),
        PercentsNameLevel(1),
        LogLevel(0)
    {}

    void SetWindowWidth(unsigned width) { _percent.MaxLen = width - 1; }

    void Init(CStdOutStream *outStream, CStdOutStream *errorStream, CStdOutStream *percentStream)
    {
        COpenCallbackConsole::Init(outStream, errorStream, percentStream);

        NumTryArcs = 0;

        ThereIsError_in_Current = false;
        ThereIsWarning_in_Current = false;

        NumOkArcs = 0;
        NumCantOpenArcs = 0;
        NumArcsWithError = 0;
        NumArcsWithWarnings = 0;

        NumOpenArcErrors = 0;
        NumOpenArcWarnings = 0;

        NumFileErrors = 0;
        NumFileErrors_in_Current = 0;
    }
};
#endif


#endif
