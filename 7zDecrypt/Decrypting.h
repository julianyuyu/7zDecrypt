#pragma once

#include "pch.h"

#define MAX_PASSWD_LEGTH    32

const wchar_t Default_Dict[] = L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ<>,.?:;{}[]()|=_+-*&^%$#@!~ ";

//const wchar_t Default_Dict[] = L"0123456789";
//const wchar_t Default_Dict[] = L"abcms1ft547[]()";

struct DECRYPT_ARGS
{
    // password args.
    int MinPasswdLength;
    int MaxPasswdLength;
    wchar_t* PatternFile;
    wchar_t* Dict;
    wchar_t* PasswdResult;

    // thread args.
    int ThreadCount;
    int ThreadIndex;
    bool* pDecryptState; /*close flag, and also return true if success to decrypt*/

    // app settings.
    int PrintRate; // frequence of message printint.
    int MaxTryCount;
};

HRESULT DecryptingExtract(
    CCodecs *codecs,
    const CObjectVector<COpenType> &types,
    const CIntVector &excludedFormats,
    UStringVector &arcPaths, UStringVector &arcPathsFull,
    const NWildcard::CCensorNode &wildcardCensor,
    const CExtractOptions &options,
    IOpenCallbackUI *openCallback,
    IExtractCallbackUI *extractCallback,
#ifndef _SFX
    IHashCalc *hash,
#endif
    UString &errorMessage,
    CDecompressStat &st,
    DECRYPT_ARGS& DecryptArgs);