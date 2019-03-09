// 7zDecrypt.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "7zDecrypt.h"
#include "avpsync.h"

using namespace NWindows;

CStdOutStream *g_StdStream = NULL;
CStdOutStream *g_ErrStream = NULL;

void ThreadRun(
    CArcCmdLineOptions& options,
    CCodecs* codecs,
    CExternalCodecs& externalCodecs, /*dont edit this name*/
    CObjectVector<COpenType>& types,
    CIntVector& excludedFormats,
    wchar_t* PatternFile);

int wmain(int argc, wchar_t** argv)
{
    g_ErrStream = &g_StdErr;
    g_StdStream = &g_StdOut;
    //std::cout << "Hello World!\n";

    CArcCmdLineOptions options;
    CCodecs* codecs;
    CExternalCodecs externalCodecsDesc;
    CObjectVector<COpenType> types;
    CIntVector excludedFormats;

    // init cmd strings
    bool isDecryptMode = false;
    wchar_t PatternFileName[MAX_PATH] = {};
    UStringVector cmdstrings;

    if (argc == 2)
    {
        // please make sure argv[1] is filename.
        // and will use default password pattern in this case.

        //
        // app.exe archive.ext
        isDecryptMode = true;
        //wcscpy_s(PatternFileName, MAX_PATH, L"default.txt");
    }
    else if (argc == 4)
    {
        // app.exe -f patternfile archive.ext
        // app.exe archive.ext -f patternfile

        int possibleIndex[] = {1, 2}; // -f arg can ben in 1st or 2nd place.

        for (int i = 0; i < ARRAY_SIZE(possibleIndex); ++i)
        {
            // save the pattern file name.
            int idx = possibleIndex[i];
            if (!_wcsicmp(argv[idx], L"-f"))
            {
                wcscpy_s(PatternFileName, MAX_PATH, argv[++idx]);
                isDecryptMode = true;
                break;
            }
        }
    }

    if (isDecryptMode)
    {
        // add "test" command. and set default password as "".
        cmdstrings.Add(UString(L"t"));
        cmdstrings.Add(UString(L"-p"));
    }

    for (int i = 1; i < argc; ++i)
    {
        if (isDecryptMode)
        {
            if (!_wcsicmp(argv[i], L"-f"))
            {
                // save the pattern file name, and skip further parsing.
                ++i;
                continue;
            }
        }
        cmdstrings.Add(UString(argv[i]));
    }

    //if (cmdstrings.Size() > 0)
    //    cmdstrings.Delete(0);

    // parse cmdstrings and build options, init codecs.
    int ret = Init(cmdstrings, options, codecs, externalCodecsDesc, types, excludedFormats);
    if (!ret)
        return ret;

    if (isDecryptMode)
    {
        ThreadRun(options, codecs, externalCodecsDesc, types, excludedFormats, PatternFileName);

        //bool done = false;
        //DecryptingExecute(options, codecs, externalCodecsDesc, types, excludedFormats, PatternFileName, &done);
    }
    else
    {
        Execute(options, codecs, externalCodecsDesc, types, excludedFormats);
    }
    Release(codecs);
    return 0;
}

struct THREAD_CTXT
{
    int Index;
    //bool ThreadDone; /*close flag, and also return true if success to decrypt*/
    CArcCmdLineOptions* options;
    CCodecs* codecs;
    CExternalCodecs* externalCodecs;
    CObjectVector<COpenType>* types;
    CIntVector* excludedFormats;
    wchar_t* PatternFile;
};

static bool GlobalDecryptDoneFlag = false;
int g_ThreadCount = 1;

DWORD WINAPI DecryptingProc(LPVOID param)
{
    THREAD_CTXT* ctxt = (THREAD_CTXT*)param;

    DecryptingExecute(*ctxt->options,
        ctxt->codecs,
        *ctxt->externalCodecs,
        *ctxt->types,
        *ctxt->excludedFormats,
        ctxt->PatternFile,
        &GlobalDecryptDoneFlag,
        ctxt->Index);

    return 0;
}

void ThreadRun(
    CArcCmdLineOptions& options,
    CCodecs* codecs,
    CExternalCodecs& externalCodecs, /*dont edit this name*/
    CObjectVector<COpenType>& types,
    CIntVector& excludedFormats,
    wchar_t* PatternFile)
{
    const int threadNum = 4;
    g_ThreadCount = threadNum;
    AVPThread threads[threadNum];
    THREAD_CTXT ctxt[threadNum];

    // execture;
    for (int i = 0; i < threadNum; i++)
    {
        //ctxt[i].ThreadDone = false;
        ctxt[i].Index = i;
        ctxt[i].options = &options;
        ctxt[i].codecs = codecs;
        ctxt[i].externalCodecs = &externalCodecs;
        ctxt[i].types = &types;
        ctxt[i].excludedFormats = &excludedFormats;
        ctxt[i].PatternFile = PatternFile;

        threads[i].Create(DecryptingProc, &ctxt[i]);

    }

    // wait for close
    // TODO: should close once either thread succeeded to decrypt.
    for (int i = 0; i < threadNum; i++)
    {
        threads[i].WaitClose();
    }

}