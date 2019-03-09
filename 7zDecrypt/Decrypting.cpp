#include "pch.h"

#include "7zDecrypt.h"


#include "../CPP/Common/MyWindows.h"
//
//#ifdef _WIN32
//#include <Psapi.h>
//#endif
//
//#include "../C/CpuArch.h"
//
//#include "../CPP/Common/Common.h"
//#include "../CPP/Common/MyInitGuid.h"
//
//#include "../CPP/Common/CommandLineParser.h"
//#include "../CPP/Common/IntToString.h"
//#include "../CPP/Common/MyException.h"
//#include "../CPP/Common/StringConvert.h"
//#include "../CPP/Common/StringToInt.h"
//#include "../CPP/Common/UTFConvert.h"
//
//#include "../CPP/Windows/ErrorMsg.h"
//
//#include "../CPP/Windows/TimeUtils.h"
//
//#include "../CPP/7zip/UI/Common/ArchiveCommandLine.h"
//#include "../CPP/7zip/UI/Common/Bench.h"
//#include "../CPP/7zip/UI/Common/ExitCode.h"
//#include "../CPP/7zip/UI/Common/Extract.h"
//
//#ifdef EXTERNAL_CODECS
//#include "../CPP/7zip/UI/Common/LoadCodecs.h"
//#endif
//
//#include "../CPP/7zip/Common/RegisterCodec.h"
//
//#include "../CPP/7zip/UI/Console/BenchCon.h"
//#include "../CPP/7zip/UI/Console/ConsoleClose.h"
//#include "../CPP/7zip/UI/Console/ExtractCallbackConsole.h"
//#include "../CPP/7zip/UI/Console/List.h"
//#include "../CPP/7zip/UI/Console/OpenCallbackConsole.h"
//#include "../CPP/7zip/UI/Console/UpdateCallbackConsole.h"
//
//#include "../CPP/7zip/UI/Console/HashCon.h"
//
//#ifdef PROG_VARIANT_R
//#include "../C/7zVersion.h"
//#else
//#include "../CPP/7zip/MyVersion.h"
//#endif

////

//#include "../CPP/Windows/FileFind.h"
//
//#include "../CPP/7zip/Archive/IArchive.h"
//
//#include "../CPP/7zip/UI/Common/ArchiveExtractCallback.h"
//#include "../CPP/7zip/UI/Common/ArchiveOpenCallback.h"
//#include "../CPP/7zip/UI/Common/ExtractMode.h"
//#include "../CPP/7zip/UI/Common/Property.h"
//
///

//#include "../C/Sort.h"
//
//#include "../CPP/Common/StringConvert.h"
//
#include "../CPP/Windows/FileDir.h"
#include "../CPP/Windows/PropVariant.h"
#include "../CPP/Windows/PropVariantConv.h"
//
#include "../CPP/7zip/UI/Common/ExtractingFilePath.h"
//#include "../CPP/7zip/UI/Common/SetProperties.h"
//////

#include "DecryptCbConsole.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;
using namespace NCommandLineParser;

//char Default_Dict[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

char Default_Dict[] = "0123456789";

//char CurrPasswd[64];
//unsigned int curr_len = 1; //current password length

/* v9.31: BUG was fixed:
   Sorted list for file paths was sorted with case insensitive compare function.
   But FindInSorted function did binary search via case sensitive compare function */
int Find_FileName_InSortedVector(const UStringVector &fileName, const UString &name);


class PasswordGenerator
{
public:
    PasswordGenerator() {}
    ~PasswordGenerator() { Destroy(); }

    bool Initialize(int minLen, int MaxLen, char* dict,
        int partNum=1/*how many part will the dict devided to*/,
        int partIdx=0/*which part this generator will handle*/)
    {
        m_PasDict = dict;
        m_DictLen = strlen(m_PasDict);

        m_MaxPasLen = MaxLen;
        if (m_MaxPasLen > m_DictLen)
        {
            m_MaxPasLen = m_DictLen;
        }

        //init port devidtion
        m_PartNum = partNum;
        m_PartIdx = partIdx;
        if (m_PartNum > m_DictLen)
            m_PartNum = m_DictLen;
        if (partIdx >= m_PartNum)
            return false;

        m_Char1IdxStart = 0;
        m_Char1IdxEnd = m_DictLen;

        // set range for first char, 
        // PasGenerator of current PortIdx will only handle in this range.
        if (m_PartNum > 1)
        {
            int portlen = (m_DictLen + m_PartNum - 1) / m_PartNum;
            int firstChar_IdxStart = portlen * m_PartIdx;
            int firstChar_idxEnd = firstChar_IdxStart + portlen;

            if (firstChar_IdxStart > m_DictLen)
                firstChar_IdxStart = 0; // should not be here.
            if (firstChar_idxEnd > m_DictLen)
                firstChar_idxEnd = m_DictLen;
            m_Char1IdxStart = firstChar_IdxStart;
            m_Char1IdxEnd = firstChar_idxEnd;
        }

        m_PasLen = minLen;
        m_CharIndices = new int[m_MaxPasLen];
        m_CurrPas = new char[m_MaxPasLen + 1];

        //memset(m_CharIndices, 0, sizeof(int) * m_MaxPasLen);
        //memset(m_CurrPas, 0, sizeof(char) * (m_MaxPasLen + 1));
        for (int i = 1; i < m_PasLen; ++i)
        {
            m_CharIndices[i] = 0;
            m_CurrPas[i] = m_PasDict[0];
        }

        // init idx for first char
        m_CharIndices[0] = m_Char1IdxStart;
        m_CurrPas[0] = m_PasDict[m_Char1IdxStart];
        return true;
    }

    void Destroy()
    {
        delete[] m_CurrPas;
        delete[] m_CharIndices;
    }

    char* UpdatePassword()
    {
        //for (int i = 0; i < PasLen; ++i)
        //{
        //    CurrPas
        //}
        int NowIdx = m_PasLen - 1;

        //CurrPas[NowIdx] = m_PasDict[CharIndices[NowIdx]];
        //CharIndices[NowIdx]++;

        // if all char in pas is last char in dict, then expand pas len.
        int checkIdx = NowIdx;
        bool NeedExpend = true;
        while (checkIdx >= 0)
        {
            if (checkIdx!=0)
            {
                if (m_CharIndices[checkIdx] >= m_DictLen)
                {
                    m_CharIndices[checkIdx] = 0;
                    m_CurrPas[checkIdx] = m_PasDict[0];
                    checkIdx--;

                    if (checkIdx < 0)
                        break;
                    m_CharIndices[checkIdx]++;
                }
                else
                {
                    NeedExpend = false;
                    m_CurrPas[checkIdx] = m_PasDict[m_CharIndices[checkIdx]];
                    m_CharIndices[NowIdx]++; // update last bit.(dont use checkIdx here, in case it has been '--')
                    break;
                }
            }
            else
            {
                // handle char 1, which will control the part.

                if (m_CharIndices[checkIdx] >= m_Char1IdxEnd)
                {
                    m_CharIndices[checkIdx] = m_Char1IdxStart;
                    m_CurrPas[checkIdx] = m_PasDict[m_Char1IdxStart];
                    //checkIdx--;

                    //if (checkIdx < 0)
                    break;
                    //m_CharIndices[checkIdx]++;
                }
                else
                {
                    NeedExpend = false;
                    m_CurrPas[checkIdx] = m_PasDict[m_CharIndices[checkIdx]];
                    m_CharIndices[NowIdx]++; // update last bit.
                    break;
                }
            }
        }

        if (NeedExpend)
        {
            m_PasLen++;
            if (m_PasLen > m_MaxPasLen)
                return nullptr;

            for (int i = 1; i < m_PasLen; ++i)
            {
                m_CharIndices[i] = 0;
                m_CurrPas[i] = m_PasDict[0];
            }
            m_CharIndices[0] = m_Char1IdxStart;
            m_CurrPas[0] = m_PasDict[m_Char1IdxStart];

            m_CharIndices[m_PasLen - 1]++;
        }

        m_CurrPas[m_PasLen] = '\0';

        //m_ConsoleCb->Password = UString(CurrPas);

        return m_CurrPas;
    }
protected:
    char* m_PasDict = nullptr;
    int m_DictLen = 0;
    int m_PasLen = 1;
    int m_MaxPasLen = 0;
    int* m_CharIndices = nullptr;
    char* m_CurrPas = nullptr;

    int m_PartNum = 1;/*how many part will the dict divided to*/
    int m_PartIdx = 0;/*which part this generator will handle*/
    int m_Char1IdxStart;
    int m_Char1IdxEnd;
};
//
//char* PasDict = nullptr;
//int DictLen = 0;
//int PasLen = 1;
//int MaxPasLen = 0;
//int* CharIndices = nullptr;
//char* CurrPas = nullptr;
//
//void PasswdInit(int minLen, int MaxLen, char* dict)
//{
//    PasDict = dict;
//    DictLen = strlen(PasDict);
//    MaxPasLen = MaxLen;
//
//    if (MaxPasLen > DictLen)
//    {
//        MaxPasLen = DictLen;
//    }
//
//    PasLen = minLen;
//    CharIndices = new int[MaxPasLen];
//    CurrPas = new char[MaxPasLen+1];
//    memset(CharIndices, 0, sizeof(int) * MaxPasLen);
//    memset(CurrPas, 0, sizeof(char) * (MaxPasLen+1));
//
//    for (int i = 0; i < PasLen; ++i)
//    {
//        CurrPas[i] = PasDict[0];
//    }
//}
//
//void PasswdExit()
//{
//    delete[] CurrPas;
//    delete[] CharIndices;
//}
//
//bool UpdatePassword(CDecryptCallbackConsole* consoleCb)
//{
//    //for (int i = 0; i < PasLen; ++i)
//    //{
//    //    CurrPas
//    //}
//    int NowIdx = PasLen - 1;
//
//    //CurrPas[NowIdx] = PasDict[CharIndices[NowIdx]];
//    //CharIndices[NowIdx]++;
//
//    // if all char in pas is last char in dict, then expand pas len.
//    int checkIdx = NowIdx;
//    bool NeedExpend = true;
//    while (checkIdx >= 0)
//    {
//        if (CharIndices[checkIdx] >= DictLen)
//        {
//            CharIndices[checkIdx] = 0;
//            CurrPas[checkIdx] = PasDict[0];
//            checkIdx--;
//
//            if (checkIdx < 0)
//                break;
//            CharIndices[checkIdx]++;
//        }
//        else
//        {
//            NeedExpend = false;
//            CurrPas[checkIdx] = PasDict[CharIndices[checkIdx]];
//            CharIndices[NowIdx]++; // update last bit.
//            break;
//        }
//    }
//
//    if (NeedExpend)
//    {
//        PasLen++;
//        if (PasLen > MaxPasLen)
//            return false;
//
//        for (int i = 0; i < PasLen; ++i)
//        {
//            CharIndices[i] = 0;
//            CurrPas[i] = PasDict[0];
//        }
//
//        CharIndices[PasLen - 1]++;
//    }
//
//    CurrPas[PasLen] = '\0';
//    
//    consoleCb->Password = UString(CurrPas);
//
//    return true;
//}

// JULIAN
HRESULT DecryptingLoop(
    IInArchive *archive,
    Int32 testMode, 
    IArchiveExtractCallback *extractCallback,
    IExtractCallbackUI* consoleCallback)
{
    HRESULT result = E_FAIL;

    CStdOutStream *so = g_StdStream;
    UInt32 index = 0; // always test the first file in the archive.

    CDecryptCallbackConsole* consoleCb = static_cast<CDecryptCallbackConsole *>(consoleCallback);

    int maxcount = 200;
    int printuint = 100;
    int totalcount = 0;
    bool correct = false;

    //PasswdInit(4, 10, Default_Dict);
    PasswordGenerator pasGen;
    pasGen.Initialize(1, 10, Default_Dict, 2, 1);
    char* curr;
    if (archive && so)
    {
        do {

            result = archive->Extract(&index, 1, testMode, extractCallback);

            // update password
            //if (totalcount == 180)
            //    consoleCb->Password = UString("b");
            //if (!UpdatePassword(consoleCb))
            curr = pasGen.UpdatePassword();
            if (!curr)
                break;

            consoleCb->Password = UString(curr);
            correct = !consoleCb->IsWrongPassword();

            totalcount++;

            //if (so)
            {
                *so << endl << "Passwd:" << consoleCb->Password;
                //if ((totalcount % printuint) == 0)
                //{
                //    *so << endl << "Tried Times : " << totalcount;
                //    *so << endl << "Passwd:" << consoleCb->Password;

                //}

            }
        } while (!correct && totalcount < maxcount);
    }

    //PasswdExit();

    if (so)
    {
        *so << endl << endl;
        if (correct)
        {
            *so << "Decryting Success!" << endl;
            *so << "------------------------------" << endl;
            *so << " password is \"" << consoleCb->Password << "\"" << endl;
            *so << "------------------------------" << endl;
        }
        else
        {
            *so << "Done decryting, password not found!" << endl;
        }

        *so << "try decryting for " << totalcount << " times!" << endl;

        if (FAILED(result))
            *so << "DecrytingLoop error!" << endl;
    }
    return result;
}



static HRESULT DecompressArchive(
    CCodecs *codecs,
    const CArchiveLink &arcLink,
    UInt64 packSize,
    const NWildcard::CCensorNode &wildcardCensor,
    const CExtractOptions &options,
    bool calcCrc,
    IExtractCallbackUI *callback,
    CArchiveExtractCallback *ecs,
    UString &errorMessage,
    UInt64 &stdInProcessed)
{
    const CArc &arc = arcLink.Arcs.Back();
    stdInProcessed = 0;
    IInArchive *archive = arc.Archive;
    CRecordVector<UInt32> realIndices;

    UStringVector removePathParts;

    FString outDir = options.OutputDir;
    UString replaceName = arc.DefaultName;

    if (arcLink.Arcs.Size() > 1)
    {
        // Most "pe" archives have same name of archive subfile "[0]" or ".rsrc_1".
        // So it extracts different archives to one folder.
        // We will use top level archive name
        const CArc &arc0 = arcLink.Arcs[0];
        if (StringsAreEqualNoCase_Ascii(codecs->Formats[arc0.FormatIndex].Name, "pe"))
            replaceName = arc0.DefaultName;
    }

    outDir.Replace(FString("*"), us2fs(Get_Correct_FsFile_Name(replaceName)));

    bool elimIsPossible = false;
    UString elimPrefix; // only pure name without dir delimiter
    FString outDirReduced = outDir;

    if (options.ElimDup.Val && options.PathMode != NExtract::NPathMode::kAbsPaths)
    {
        UString dirPrefix;
        SplitPathToParts_Smart(fs2us(outDir), dirPrefix, elimPrefix);
        if (!elimPrefix.IsEmpty())
        {
            if (IsPathSepar(elimPrefix.Back()))
                elimPrefix.DeleteBack();
            if (!elimPrefix.IsEmpty())
            {
                outDirReduced = us2fs(dirPrefix);
                elimIsPossible = true;
            }
        }
    }

    bool allFilesAreAllowed = wildcardCensor.AreAllAllowed();

    if (!options.StdInMode)
    {
        UInt32 numItems;
        RINOK(archive->GetNumberOfItems(&numItems));

        CReadArcItem item;

        for (UInt32 i = 0; i < numItems; i++)
        {
            if (elimIsPossible || !allFilesAreAllowed)
            {
                RINOK(arc.GetItem(i, item));
            }
            else
            {
#ifdef SUPPORT_ALT_STREAMS
                item.IsAltStream = false;
                if (!options.NtOptions.AltStreams.Val && arc.Ask_AltStream)
                {
                    RINOK(Archive_IsItem_AltStream(arc.Archive, i, item.IsAltStream));
                }
#endif
            }

#ifdef SUPPORT_ALT_STREAMS
            if (!options.NtOptions.AltStreams.Val && item.IsAltStream)
                continue;
#endif

            if (elimIsPossible)
            {
                const UString &s =
#ifdef SUPPORT_ALT_STREAMS
                    item.MainPath;
#else
                    item.Path;
#endif
                if (!IsPath1PrefixedByPath2(s, elimPrefix))
                    elimIsPossible = false;
                else
                {
                    wchar_t c = s[elimPrefix.Len()];
                    if (c == 0)
                    {
                        if (!item.MainIsDir)
                            elimIsPossible = false;
                    }
                    else if (!IsPathSepar(c))
                        elimIsPossible = false;
                }
            }

            if (!allFilesAreAllowed)
            {
                if (!CensorNode_CheckPath(wildcardCensor, item))
                    continue;
            }

            realIndices.Add(i);
        }

        if (realIndices.Size() == 0)
        {
            callback->ThereAreNoFiles();
            return callback->ExtractResult(S_OK);
        }
    }

    if (elimIsPossible)
    {
        removePathParts.Add(elimPrefix);
        // outDir = outDirReduced;
    }

#ifdef _WIN32
    // GetCorrectFullFsPath doesn't like "..".
    // outDir.TrimRight();
    // outDir = GetCorrectFullFsPath(outDir);
#endif

    if (outDir.IsEmpty())
        outDir = "." STRING_PATH_SEPARATOR;
    /*
    #ifdef _WIN32
    else if (NName::IsAltPathPrefix(outDir)) {}
    #endif
    */
    else if (!CreateComplexDir(outDir))
    {
        HRESULT res = ::GetLastError();
        if (res == S_OK)
            res = E_FAIL;
        errorMessage = "Can not create output directory: ";
        errorMessage += fs2us(outDir);
        return res;
    }

    ecs->Init(
        options.NtOptions,
        options.StdInMode ? &wildcardCensor : NULL,
        &arc,
        callback,
        options.StdOutMode, options.TestMode,
        outDir,
        removePathParts, false,
        packSize);


#ifdef SUPPORT_LINKS

    if (!options.StdInMode &&
        !options.TestMode &&
        options.NtOptions.HardLinks.Val)
    {
        RINOK(ecs->PrepareHardLinks(&realIndices));
    }

#endif

    HRESULT result;
    Int32 testMode = (options.TestMode && !calcCrc) ? 1 : 0;

    CArchiveExtractCallback_Closer ecsCloser(ecs);

    // JULIAN
    CStdOutStream *so = g_StdStream;
    if (so)
    {
        *so << endl;
        *so << "====== Decrypting Begin ======" << endl;
        *so << endl;
    }

    if (options.StdInMode)
    {
        result = archive->Extract(NULL, (UInt32)(Int32)-1, testMode, ecs);
        NCOM::CPropVariant prop;
        if (archive->GetArchiveProperty(kpidPhySize, &prop) == S_OK)
            ConvertPropVariantToUInt64(prop, stdInProcessed);

        //JULIAN
        if (so)
        {
            *so << "long code path!!!" << endl;
        }
    }
    else
    {
        //result = archive->Extract(&realIndices.Front(), realIndices.Size(), testMode, ecs);

        //JULIAN
        result = DecryptingLoop(archive, testMode, ecs, callback);
    }

    //JULIAN
    if (so)
    {
        *so << endl;
        *so << "======= Decrypting End =======" << endl;
        *so << endl;
    }

    HRESULT res2 = ecsCloser.Close();
    if (result == S_OK)
        result = res2;

    return callback->ExtractResult(result);
}

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
    wchar_t* PatternFile)
{
    st.Clear();
    UInt64 totalPackSize = 0;
    CRecordVector<UInt64> arcSizes;

    unsigned numArcs = options.StdInMode ? 1 : arcPaths.Size();

    unsigned i;

    for (i = 0; i < numArcs; i++)
    {
        NFind::CFileInfo fi;
        fi.Size = 0;
        if (!options.StdInMode)
        {
            const FString &arcPath = us2fs(arcPaths[i]);
            if (!fi.Find(arcPath))
                throw "there is no such archive";
            if (fi.IsDir())
                throw "can't decompress folder";
        }
        arcSizes.Add(fi.Size);
        totalPackSize += fi.Size;
    }

    CBoolArr skipArcs(numArcs);
    for (i = 0; i < numArcs; i++)
        skipArcs[i] = false;

    CArchiveExtractCallback *ecs = new CArchiveExtractCallback;
    CMyComPtr<IArchiveExtractCallback> ec(ecs);
    bool multi = (numArcs > 1);
    ecs->InitForMulti(multi, options.PathMode, options.OverwriteMode,
        false // keepEmptyDirParts
    );
#ifndef _SFX
    ecs->SetHashMethods(hash);
#endif

    if (multi)
    {
        RINOK(extractCallback->SetTotal(totalPackSize));
    }

    UInt64 totalPackProcessed = 0;
    bool thereAreNotOpenArcs = false;

    for (i = 0; i < numArcs; i++)
    {
        if (skipArcs[i])
            continue;

        const UString &arcPath = arcPaths[i];
        NFind::CFileInfo fi;
        if (options.StdInMode)
        {
            fi.Size = 0;
            fi.Attrib = 0;
        }
        else
        {
            if (!fi.Find(us2fs(arcPath)) || fi.IsDir())
                throw "there is no such archive";
        }

        /*
        #ifndef _NO_CRYPTO
        openCallback->Open_Clear_PasswordWasAsked_Flag();
        #endif
        */

        RINOK(extractCallback->BeforeOpen(arcPath, options.TestMode));
        CArchiveLink arcLink;

        CObjectVector<COpenType> types2 = types;
        /*
        #ifndef _SFX
        if (types.IsEmpty())
        {
          int pos = arcPath.ReverseFind(L'.');
          if (pos >= 0)
          {
            UString s = arcPath.Ptr(pos + 1);
            int index = codecs->FindFormatForExtension(s);
            if (index >= 0 && s == L"001")
            {
              s = arcPath.Left(pos);
              pos = s.ReverseFind(L'.');
              if (pos >= 0)
              {
                int index2 = codecs->FindFormatForExtension(s.Ptr(pos + 1));
                if (index2 >= 0) // && s.CompareNoCase(L"rar") != 0
                {
                  types2.Add(index2);
                  types2.Add(index);
                }
              }
            }
          }
        }
        #endif
        */

        COpenOptions op;
#ifndef _SFX
        op.props = &options.Properties;
#endif
        op.codecs = codecs;
        op.types = &types2;
        op.excludedFormats = &excludedFormats;
        op.stdInMode = options.StdInMode;
        op.stream = NULL;
        op.filePath = arcPath;

        HRESULT result = arcLink.Open_Strict(op, openCallback);

        if (result == E_ABORT)
            return result;

        // arcLink.Set_ErrorsText();
        RINOK(extractCallback->OpenResult(codecs, arcLink, arcPath, result));

        if (result != S_OK)
        {
            thereAreNotOpenArcs = true;
            if (!options.StdInMode)
            {
                NFind::CFileInfo fi2;
                if (fi2.Find(us2fs(arcPath)))
                    if (!fi2.IsDir())
                        totalPackProcessed += fi2.Size;
            }
            continue;
        }

        if (!options.StdInMode)
        {
            // numVolumes += arcLink.VolumePaths.Size();
            // arcLink.VolumesSize;

            // totalPackSize -= DeleteUsedFileNamesFromList(arcLink, i + 1, arcPaths, arcPathsFull, &arcSizes);
            // numArcs = arcPaths.Size();
            if (arcLink.VolumePaths.Size() != 0)
            {
                Int64 correctionSize = arcLink.VolumesSize;
                FOR_VECTOR(v, arcLink.VolumePaths)
                {
                    int index = Find_FileName_InSortedVector(arcPathsFull, arcLink.VolumePaths[v]);
                    if (index >= 0)
                    {
                        if ((unsigned)index > i)
                        {
                            skipArcs[(unsigned)index] = true;
                            correctionSize -= arcSizes[(unsigned)index];
                        }
                    }
                }
                if (correctionSize != 0)
                {
                    Int64 newPackSize = (Int64)totalPackSize + correctionSize;
                    if (newPackSize < 0)
                        newPackSize = 0;
                    totalPackSize = newPackSize;
                    RINOK(extractCallback->SetTotal(totalPackSize));
                }
            }
        }

        /*
        // Now openCallback and extractCallback use same object. So we don't need to send password.

        #ifndef _NO_CRYPTO
        bool passwordIsDefined;
        UString password;
        RINOK(openCallback->Open_GetPasswordIfAny(passwordIsDefined, password));
        if (passwordIsDefined)
        {
          RINOK(extractCallback->SetPassword(password));
        }
        #endif
        */

        CArc &arc = arcLink.Arcs.Back();
        arc.MTimeDefined = (!options.StdInMode && !fi.IsDevice);
        arc.MTime = fi.MTime;

        UInt64 packProcessed;
        bool calcCrc =
#ifndef _SFX
        (hash != NULL);
#else
            false;
#endif

        RINOK(DecompressArchive(
            codecs,
            arcLink,
            fi.Size + arcLink.VolumesSize,
            wildcardCensor,
            options,
            calcCrc,
            extractCallback, ecs, errorMessage, packProcessed));

        if (!options.StdInMode)
            packProcessed = fi.Size + arcLink.VolumesSize;
        totalPackProcessed += packProcessed;
        ecs->LocalProgressSpec->InSize += packProcessed;
        ecs->LocalProgressSpec->OutSize = ecs->UnpackSize;
        if (!errorMessage.IsEmpty())
            return E_FAIL;
    }

    if (multi || thereAreNotOpenArcs)
    {
        RINOK(extractCallback->SetTotal(totalPackSize));
        RINOK(extractCallback->SetCompleted(&totalPackProcessed));
    }

    st.NumFolders = ecs->NumFolders;
    st.NumFiles = ecs->NumFiles;
    st.NumAltStreams = ecs->NumAltStreams;
    st.UnpackSize = ecs->UnpackSize;
    st.AltStreams_UnpackSize = ecs->AltStreams_UnpackSize;
    st.NumArchives = arcPaths.Size();
    st.PackSize = ecs->LocalProgressSpec->InSize;
    return S_OK;
}
