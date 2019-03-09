
#include "pch.h"
#include "7zDecrypt.h"
#include "DecryptCbConsole.h"
//
//#include "../CPP/Common/IntToString.h"
//#include "../CPP/Common/Wildcard.h"
//
//#include "../CPP/Windows/FileDir.h"
//#include "../CPP/Windows/FileFind.h"
//#include "../CPP/Windows/TimeUtils.h"
//#include "../CPP/Windows/ErrorMsg.h"
//#include "../CPP/Windows/PropVariantConv.h"

#ifndef _7ZIP_ST
#include "../CPP/Windows/Synchronization.h"
#endif
//
//#include "../CPP/7zip/Common/FilePathAutoRename.h"
//
//#include "../CPP/7zip/UI/Common/ExtractingFilePath.h"
//
//#include "../CPP/7zip/UI/Console/ConsoleClose.h"
//#include "../CPP/7zip/UI/Console/ExtractCallbackConsole.h"
//#include "../CPP/7zip/UI/Console/UserInputUtils.h"

using namespace NWindows;

#ifndef _7ZIP_ST
static NSynchronization::CCriticalSection g_CriticalSection;
#define MT_LOCK NSynchronization::CCriticalSectionLock lock(g_CriticalSection);
#else
#define MT_LOCK
#endif

static const char * const kTestString = "T";
static const char * const kExtractString = "-";
static const char * const kSkipString = ".";

//
//static const char * const kError = "ERROR: ";
//
//static const char * const kUnsupportedMethod = "Unsupported Method";
//static const char * const kCrcFailed = "CRC Failed";
//static const char * const kCrcFailedEncrypted = "CRC Failed in encrypted file. Wrong password?";
//static const char * const kDataError = "Data Error";
//static const char * const kDataErrorEncrypted = "Data Error in encrypted file. Wrong password?";
//static const char * const kUnavailableData = "Unavailable data";
//static const char * const kUnexpectedEnd = "Unexpected end of data";
//static const char * const kDataAfterEnd = "There are some data after the end of the payload data";
//static const char * const kIsNotArc = "Is not archive";
//static const char * const kHeadersError = "Headers Error";
//static const char * const kWrongPassword = "Wrong password";
//
//void SetExtractErrorMessage2(Int32 opRes, Int32 encrypted, AString &dest)
//{
//    dest.Empty();
//    const char *s = NULL;
//
//    switch (opRes)
//    {
//    case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
//        s = kUnsupportedMethod;
//        break;
//    case NArchive::NExtract::NOperationResult::kCRCError:
//        s = (encrypted ? kCrcFailedEncrypted : kCrcFailed);
//        break;
//    case NArchive::NExtract::NOperationResult::kDataError:
//        s = (encrypted ? kDataErrorEncrypted : kDataError);
//        break;
//    case NArchive::NExtract::NOperationResult::kUnavailable:
//        s = kUnavailableData;
//        break;
//    case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
//        s = kUnexpectedEnd;
//        break;
//    case NArchive::NExtract::NOperationResult::kDataAfterEnd:
//        s = kDataAfterEnd;
//        break;
//    case NArchive::NExtract::NOperationResult::kIsNotArc:
//        s = kIsNotArc;
//        break;
//    case NArchive::NExtract::NOperationResult::kHeadersError:
//        s = kHeadersError;
//        break;
//    case NArchive::NExtract::NOperationResult::kWrongPassword:
//        s = kWrongPassword;
//        break;
//    }
//
//    dest += kError;
//    if (s)
//        dest += s;
//    else
//    {
//        dest += "Error #";
//        dest.Add_UInt32(opRes);
//    }
//}

STDMETHODIMP CDecryptCallbackConsole::SetOperationResult(Int32 opRes, Int32 encrypted)
{
    MT_LOCK

    OpResult = opRes;

    if (opRes == NArchive::NExtract::NOperationResult::kOK)
    {
        if (NeedPercents())
        {
            _percent.Command.Empty();
            _percent.FileName.Empty();
            _percent.Files++;
        }
    }
    else
    {

        //NumFileErrors_in_Current++;
        //NumFileErrors++;

        //if (_se)
        //{
        //    ClosePercentsAndFlush();

        //    AString s;
        //    SetExtractErrorMessage2(opRes, encrypted, s);

        //    *_se << s;
        //    if (!_currentName.IsEmpty())
        //    {
        //        *_se << " : ";
        //        _se->NormalizePrint_UString(_currentName);
        //    }
        //    *_se << endl;
        //    _se->Flush();
        //}
    }

    //return CheckBreak2();
    return S_OK;
}

STDMETHODIMP CDecryptCallbackConsole::SetTotal(UInt64 size)
{
    MT_LOCK

    if (NeedPercents())
    {
        _percent.Total = size;

        //JULIAN
        _percent.Print();
    }
    //return CheckBreak2();
    return S_OK;
}

STDMETHODIMP CDecryptCallbackConsole::SetCompleted(const UInt64 *completeValue)
{
    MT_LOCK

    if (NeedPercents())
    {
        if (completeValue)
            _percent.Completed = *completeValue;
        //_percent.Print();
    }
    //return CheckBreak2();
    return S_OK;
}

STDMETHODIMP CDecryptCallbackConsole::PrepareOperation(const wchar_t *name, Int32 /* isFolder */, Int32 askExtractMode, const UInt64 *position)
{
    MT_LOCK

        _currentName = name;

    const char *s;
    unsigned requiredLevel = 1;

    switch (askExtractMode)
    {
    case NArchive::NExtract::NAskMode::kExtract: s = kExtractString; break;
    case NArchive::NExtract::NAskMode::kTest:    s = kTestString; break;
    case NArchive::NExtract::NAskMode::kSkip:    s = kSkipString; requiredLevel = 2; break;
    default: s = "???"; requiredLevel = 2;
    };

    bool show2 = (LogLevel >= requiredLevel && _so);

    if (show2)
    {
        ClosePercents_for_so();

        _tempA = s;
        if (name)
            _tempA.Add_Space();
        *_so << _tempA;

        _tempU.Empty();
        if (name)
        {
            _tempU = name;
            _so->Normalize_UString(_tempU);
        }
        _so->PrintUString(_tempU, _tempA);
        if (position)
            *_so << " <" << *position << ">";
        *_so << endl;

        if (NeedFlush)
            _so->Flush();
    }

    if (NeedPercents())
    {
        if (PercentsNameLevel >= 1)
        {
            _percent.FileName.Empty();
            _percent.Command.Empty();
            if (PercentsNameLevel > 1 || !show2)
            {
                _percent.Command = s;
                if (name)
                    _percent.FileName = name;
            }
        }

        //JULIAN
        //_percent.Print();
    }

    //return CheckBreak2();
    return S_OK;
}
