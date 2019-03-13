#pragma once

#include "pch.h"


//const char Default_Dict[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

const char Default_Dict[] = "0123456789";

struct DECRYPT_ARGS
{
    // password args.
    int MinPasswdLength;
    int MaxPasswdLength;
    wchar_t* PatternFile;
    char* Dict;

    // thread args.
    int ThreadCount;
    int ThreadIndex;
    bool* pDecryptState; /*close flag, and also return true if success to decrypt*/
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