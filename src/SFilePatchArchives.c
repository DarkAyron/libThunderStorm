/*****************************************************************************/
/* SFilePatchArchives.cpp                 Copyright (c) Ladislav Zezula 2010 */
/*---------------------------------------------------------------------------*/
/* Description:                                                              */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 18.08.10  1.00  Lad  The first version of SFilePatchArchives.cpp          */
/* 01.04.15  1.00  Ayr  Ported to plain c                                    */
/*****************************************************************************/

#include "thunderStorm.h"
#include "StormCommon.h"

/*-----------------------------------------------------------------------------
 * Local structures
 */

#define PATCH_SIGNATURE_HEADER 0x48435450
#define PATCH_SIGNATURE_MD5    0x5f35444d
#define PATCH_SIGNATURE_XFRM   0x4d524658

#define SIZE_OF_XFRM_HEADER  0x0C

/* Header for incremental patch files */
typedef struct _MPQ_PATCH_HEADER
{
    /*-- PATCH header -----------------------------------*/
    uint32_t dwSignature;                          /* 'PTCH' */
    uint32_t dwSizeOfPatchData;                    /* Size of the entire patch (decompressed) */
    uint32_t dwSizeBeforePatch;                    /* Size of the file before patch */
    uint32_t dwSizeAfterPatch;                     /* Size of file after patch */
    
    /*-- MD5 block --------------------------------------*/
    uint32_t dwMD5;                                /* 'MD5_' */
    uint32_t dwMd5BlockSize;                       /* Size of the MD5 block, including the signature and size itself */
    uint8_t md5_before_patch[0x10];                /* MD5 of the original (unpached) file */
    uint8_t md5_after_patch[0x10];                 /* MD5 of the patched file */

    /*-- XFRM block -------------------------------------*/
    uint32_t dwXFRM;                               /* 'XFRM' */
    uint32_t dwXfrmBlockSize;                      /* Size of the XFRM block, includes XFRM header and patch data */
    uint32_t dwPatchType;                          /* Type of patch ('BSD0' or 'COPY') */

    /* Followed by the patch data */
} MPQ_PATCH_HEADER, *PMPQ_PATCH_HEADER;

typedef struct _BLIZZARD_BSDIFF40_FILE
{
    uint64_t Signature;
    uint64_t CtrlBlockSize;
    uint64_t DataBlockSize;
    uint64_t NewFileSize;
} BLIZZARD_BSDIFF40_FILE, *PBLIZZARD_BSDIFF40_FILE;

typedef struct _BSDIFF_CTRL_BLOCK
{
    uint32_t dwAddDataLength;
    uint32_t dwMovDataLength;
    uint32_t dwOldMoveLength;

} BSDIFF_CTRL_BLOCK, *PBSDIFF_CTRL_BLOCK;

typedef struct _LOCALIZED_MPQ_INFO
{
    const char * szNameTemplate;            /* Name template */
    size_t nLangOffset;                     /* Offset of the language */
    size_t nLength;                         /* Length of the name template */
} LOCALIZED_MPQ_INFO, *PLOCALIZED_MPQ_INFO;

/*-----------------------------------------------------------------------------
 * Local variables
 */

/* 4-byte groups for all languages */
static const char * LanguageList = "baseteenenUSenGBenCNenTWdeDEesESesMXfrFRitITkoKRptBRptPTruRUzhCNzhTW";

/* List of localized MPQs for World of Warcraft */
static LOCALIZED_MPQ_INFO LocaleMpqs_WoW[] =
{
    {"expansion1-locale-####", 18, 22}, 
    {"expansion1-speech-####", 18, 22}, 
    {"expansion2-locale-####", 18, 22}, 
    {"expansion2-speech-####", 18, 22}, 
    {"expansion3-locale-####", 18, 22}, 
    {"expansion3-speech-####", 18, 22}, 
    {"locale-####",             7, 11}, 
    {"speech-####",             7, 11}, 
    {NULL, 0, 0}
};

/*-----------------------------------------------------------------------------
 * Local functions
 */

static inline int IsPatchMetadataFile(TFileEntry * pFileEntry)
{
    /* The file must ave a namet */
    if(pFileEntry->szFileName != NULL && (pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) == 0)
    {
        /* The file must be small */
        if(0 < pFileEntry->dwFileSize && pFileEntry->dwFileSize < 0x40)
        {
            /* Compare the plain name */
            return (strcasecmp(GetPlainFileName(pFileEntry->szFileName), PATCH_METADATA_NAME) == 0);
        }
    }

    /* Not a patch_metadata */
    return 0;
}

static void Decompress_RLE(unsigned char * pbDecompressed, uint32_t cbDecompressed, unsigned char * pbCompressed, uint32_t cbCompressed)
{
    unsigned char * pbDecompressedEnd = pbDecompressed + cbDecompressed;
    unsigned char * pbCompressedEnd = pbCompressed + cbCompressed;
    uint8_t RepeatCount; 
    uint8_t OneByte;

    /* Cut the initial uint32_t from the compressed chunk */
    pbCompressed += sizeof(uint32_t);

    /* Pre-fill decompressed buffer with zeros */
    memset(pbDecompressed, 0, cbDecompressed);

    /* Unpack */
    while(pbCompressed < pbCompressedEnd && pbDecompressed < pbDecompressedEnd)
    {
        OneByte = *pbCompressed++;
        
        /* Is it a repetition byte ? */
        if(OneByte & 0x80)
        {
            uint8_t i;
            
            RepeatCount = (OneByte & 0x7F) + 1;
            for(i = 0; i < RepeatCount; i++)
            {
                if(pbDecompressed == pbDecompressedEnd || pbCompressed == pbCompressedEnd)
                    break;

                *pbDecompressed++ = *pbCompressed++;
            }
        }
        else
        {
            pbDecompressed += (OneByte + 1);
        }
    }
}

static int LoadFilePatch_COPY(TMPQFile * hf, PMPQ_PATCH_HEADER pFullPatch)
{
    size_t cbBytesToRead = pFullPatch->dwSizeOfPatchData - sizeof(MPQ_PATCH_HEADER);
    size_t cbBytesRead = 0;

    /* Simply load the rest of the patch */
    SFileReadFile((void *)hf, (pFullPatch + 1), cbBytesToRead, &cbBytesRead);
    return (cbBytesRead == cbBytesToRead) ? ERROR_SUCCESS : ERROR_FILE_CORRUPT;
}

static int LoadFilePatch_BSD0(TMPQFile * hf, PMPQ_PATCH_HEADER pFullPatch)
{
    unsigned char * pbDecompressed = (unsigned char *)(pFullPatch + 1);
    unsigned char * pbCompressed = NULL;
    size_t cbDecompressed = 0;
    size_t cbCompressed = 0;
    size_t dwBytesRead = 0;
    int nError = ERROR_SUCCESS;

    /* Calculate the size of compressed data */
    cbDecompressed = pFullPatch->dwSizeOfPatchData - sizeof(MPQ_PATCH_HEADER);
    cbCompressed = pFullPatch->dwXfrmBlockSize - SIZE_OF_XFRM_HEADER;

    /* Is that file compressed? */
    if(cbCompressed < cbDecompressed)
    {
        pbCompressed = STORM_ALLOC(uint8_t, cbCompressed);
        if(pbCompressed == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;

        /* Read the compressed patch data */
        if(nError == ERROR_SUCCESS)
        {
            SFileReadFile((void *)hf, pbCompressed, cbCompressed, &dwBytesRead);
            if(dwBytesRead != cbCompressed)
                nError = ERROR_FILE_CORRUPT;
        }

        /* Decompress the data */
        if(nError == ERROR_SUCCESS)
            Decompress_RLE(pbDecompressed, cbDecompressed, pbCompressed, cbCompressed);

        if(pbCompressed != NULL)
            STORM_FREE(pbCompressed);
    }
    else
    {
        SFileReadFile((void *)hf, pbDecompressed, cbDecompressed, &dwBytesRead);
        if(dwBytesRead != cbDecompressed)
            nError = ERROR_FILE_CORRUPT;
    }

    return nError;
}

static int ApplyFilePatch_COPY(
    TMPQPatcher * pPatcher,
    PMPQ_PATCH_HEADER pFullPatch,
    unsigned char * pbTarget,
    unsigned char * pbSource)
{
    /* Sanity checks */
    assert(pPatcher->cbMaxFileData >= pPatcher->cbFileData);
    pFullPatch = pFullPatch;

    /* Copy the patch data as-is */
    memcpy(pbTarget, pbSource, pPatcher->cbFileData);
    return ERROR_SUCCESS;
}

static int ApplyFilePatch_BSD0(
    TMPQPatcher * pPatcher,
    PMPQ_PATCH_HEADER pFullPatch,
    unsigned char * pbTarget,
    unsigned char * pbSource)
{
    PBLIZZARD_BSDIFF40_FILE pBsdiff;
    PBSDIFF_CTRL_BLOCK pCtrlBlock;
    unsigned char * pbPatchData = (unsigned char *)(pFullPatch + 1);
    unsigned char * pDataBlock;
    unsigned char * pExtraBlock;
    unsigned char * pbOldData = pbSource;
    unsigned char * pbNewData = pbTarget;
    uint32_t dwCombineSize;
    uint32_t dwNewOffset = 0;                          /* Current position to patch */
    uint32_t dwOldOffset = 0;                          /* Current source position */
    uint32_t dwNewSize;                                /* Patched file size */
    uint32_t dwOldSize = pPatcher->cbFileData;         /* File size before patch */

    /* Get pointer to the patch header */
    /* Format of BSDIFF header corresponds to original BSDIFF, which is: */
    /* 0000   8 bytes   signature "BSDIFF40" */
    /* 0008   8 bytes   size of the control block */
    /* 0010   8 bytes   size of the data block */
    /* 0018   8 bytes   new size of the patched file */
    pBsdiff = (PBLIZZARD_BSDIFF40_FILE)pbPatchData;
    pbPatchData += sizeof(BLIZZARD_BSDIFF40_FILE);

    /* Get pointer to the 32-bit BSDIFF control block */
    /* The control block follows immediately after the BSDIFF header */
    /* and consists of three 32-bit integers */
    /* 0000   4 bytes   Length to copy from the BSDIFF data block the new file */
    /* 0004   4 bytes   Length to copy from the BSDIFF extra block */
    /* 0008   4 bytes   Size to increment source file offset */
    pCtrlBlock = (PBSDIFF_CTRL_BLOCK)pbPatchData;
    pbPatchData += (size_t)BSWAP_INT64_UNSIGNED(pBsdiff->CtrlBlockSize);

    /* Get the pointer to the data block */
    pDataBlock = (unsigned char *)pbPatchData;
    pbPatchData += (size_t)BSWAP_INT64_UNSIGNED(pBsdiff->DataBlockSize);

    /* Get the pointer to the extra block */
    pExtraBlock = (unsigned char *)pbPatchData;
    dwNewSize = (uint32_t)BSWAP_INT64_UNSIGNED(pBsdiff->NewFileSize);

    /* Now patch the file */
    while(dwNewOffset < dwNewSize)
    {
        uint32_t dwAddDataLength = BSWAP_INT32_UNSIGNED(pCtrlBlock->dwAddDataLength);
        uint32_t dwMovDataLength = BSWAP_INT32_UNSIGNED(pCtrlBlock->dwMovDataLength);
        uint32_t dwOldMoveLength = BSWAP_INT32_UNSIGNED(pCtrlBlock->dwOldMoveLength);
        uint32_t i;

        /* Sanity check */
        if((dwNewOffset + dwAddDataLength) > dwNewSize)
            return ERROR_FILE_CORRUPT;

        /* Read the diff string to the target buffer */
        memcpy(pbNewData + dwNewOffset, pDataBlock, dwAddDataLength);
        pDataBlock += dwAddDataLength;

        /* Get the longest block that we can combine */
        dwCombineSize = ((dwOldOffset + dwAddDataLength) >= dwOldSize) ? (dwOldSize - dwOldOffset) : dwAddDataLength;

        /* Now combine the patch data with the original file */
        for(i = 0; i < dwCombineSize; i++)
            pbNewData[dwNewOffset + i] = pbNewData[dwNewOffset + i] + pbOldData[dwOldOffset + i];
        
        /* Move the offsets */
        dwNewOffset += dwAddDataLength;
        dwOldOffset += dwAddDataLength;

        /* Sanity check */
        if((dwNewOffset + dwMovDataLength) > dwNewSize)
            return ERROR_FILE_CORRUPT;

        /* Copy the data from the extra block in BSDIFF patch */
        memcpy(pbNewData + dwNewOffset, pExtraBlock, dwMovDataLength);
        pExtraBlock += dwMovDataLength;
        dwNewOffset += dwMovDataLength;

        /* Move the old offset */
        if(dwOldMoveLength & 0x80000000)
            dwOldMoveLength = 0x80000000 - dwOldMoveLength;
        dwOldOffset += dwOldMoveLength;
        pCtrlBlock++;
    }

    /* The size after patch must match */
    if(dwNewOffset != pFullPatch->dwSizeAfterPatch)
        return ERROR_FILE_CORRUPT;

    /* Update the new data size */
    pPatcher->cbFileData = dwNewOffset;
    return ERROR_SUCCESS;
}

static PMPQ_PATCH_HEADER LoadFullFilePatch(TMPQFile * hf, MPQ_PATCH_HEADER * PatchHeader)
{
    PMPQ_PATCH_HEADER pFullPatch;
    int nError = ERROR_SUCCESS;

    /* BSWAP the entire header, if needed */
    BSWAP_ARRAY32_UNSIGNED(PatchHeader, sizeof(uint32_t) * 6);
    BSWAP_ARRAY32_UNSIGNED(PatchHeader->dwXFRM, sizeof(uint32_t) * 3);

    /* Verify the signatures in the patch header */
    if(PatchHeader->dwSignature != PATCH_SIGNATURE_HEADER || PatchHeader->dwMD5 != PATCH_SIGNATURE_MD5 || PatchHeader->dwXFRM != PATCH_SIGNATURE_XFRM)
        return NULL;

    /* Allocate space for patch header and compressed data */
    pFullPatch = (PMPQ_PATCH_HEADER)STORM_ALLOC(uint8_t, PatchHeader->dwSizeOfPatchData);
    if(pFullPatch != NULL)
    {
        /* Copy the patch header */
        memcpy(pFullPatch, PatchHeader, sizeof(MPQ_PATCH_HEADER));

        /* Read the patch, depending on patch type */
        if(nError == ERROR_SUCCESS)
        {
            switch(PatchHeader->dwPatchType)
            {
                case 0x59504f43:    /* 'COPY' */
                    nError = LoadFilePatch_COPY(hf, pFullPatch);
                    break;

                case 0x30445342:    /* 'BSD0' */
                    nError = LoadFilePatch_BSD0(hf, pFullPatch);
                    break;

                default:
                    nError = ERROR_FILE_CORRUPT;
                    break;
            }
        }

        /* If something failed, free the patch buffer */
        if(nError != ERROR_SUCCESS)
        {
            STORM_FREE(pFullPatch);
            pFullPatch = NULL;
        }
    }

    /* Give the result to the caller */
    return pFullPatch;
}

static int ApplyFilePatch(
    TMPQPatcher * pPatcher,
    PMPQ_PATCH_HEADER pFullPatch)
{
    unsigned char * pbSource = (pPatcher->nCounter & 0x1) ? pPatcher->pbFileData2 : pPatcher->pbFileData1;
    unsigned char * pbTarget = (pPatcher->nCounter & 0x1) ? pPatcher->pbFileData1 : pPatcher->pbFileData2;
    int nError;

    /* Sanity checks */
    assert(pFullPatch->dwSizeAfterPatch <= pPatcher->cbMaxFileData);

    /* Apply the patch according to the type */
    switch(pFullPatch->dwPatchType)
    {
        case 0x59504f43:    /* 'COPY' */
            nError = ApplyFilePatch_COPY(pPatcher, pFullPatch, pbTarget, pbSource);
            break;

        case 0x30445342:    /* 'BSD0' */
            nError = ApplyFilePatch_BSD0(pPatcher, pFullPatch, pbTarget, pbSource);
            break;

        default:
            nError = ERROR_FILE_CORRUPT;
            break;
    }

    /* Verify MD5 after patch */
    if(nError == ERROR_SUCCESS && pFullPatch->dwSizeAfterPatch != 0)
    {
        /* Verify the patched file */
        if(!VerifyDataBlockHash(pbTarget, pFullPatch->dwSizeAfterPatch, pFullPatch->md5_after_patch))
            nError = ERROR_FILE_CORRUPT;
        
        /* Copy the MD5 of the new block */
        memcpy(pPatcher->this_md5, pFullPatch->md5_after_patch, MD5_DIGEST_SIZE);
    }

    return nError;
}

/*-----------------------------------------------------------------------------
 * Local functions (patch prefix matching)
 */

static int CreatePatchPrefix(TMPQArchive * ha, const char * szFileName, size_t nLength)
{
    TMPQNamePrefix * pNewPrefix;

    /* If the end of the patch prefix was not entered, find it */
    if(szFileName != NULL && nLength == 0)
        nLength = strlen(szFileName);

    /* Create the patch prefix */
    pNewPrefix = (TMPQNamePrefix *)STORM_ALLOC(uint8_t, sizeof(TMPQNamePrefix) + nLength);
    if(pNewPrefix != NULL)
    {
        /* Fill the name prefix */
        pNewPrefix->nLength = nLength;
        pNewPrefix->szPatchPrefix[0] = 0;
        
        /* Fill the name prefix. Also add the backslash */
        if(szFileName && nLength)
        {
            memcpy(pNewPrefix->szPatchPrefix, szFileName, nLength);
            pNewPrefix->szPatchPrefix[nLength] = 0;
        }
    }

    ha->pPatchPrefix = pNewPrefix;
    return (pNewPrefix != NULL);
}

static int IsMatchingPatchFile(
    TMPQArchive * ha,
    const char * szFileName,
    unsigned char * pbFileMd5)
{
    MPQ_PATCH_HEADER PatchHeader = {0};
    void * hFile = NULL;
    size_t dwTransferred = 0;
    int bResult = 0;

    /* Open the file and load the patch header */
    if(SFileOpenFileEx((void *)ha, szFileName, SFILE_OPEN_BASE_FILE, &hFile))
    {
        /* Load the patch header */
        SFileReadFile(hFile, &PatchHeader, sizeof(MPQ_PATCH_HEADER), &dwTransferred);
        BSWAP_ARRAY32_UNSIGNED(pPatchHeader, sizeof(uint32_t) * 6);

        /* If the file contains an incremental patch, */
        /* compare the "MD5 before patching" with the base file MD5 */
        if(dwTransferred == sizeof(MPQ_PATCH_HEADER) && PatchHeader.dwSignature == PATCH_SIGNATURE_HEADER)
            bResult = (!memcmp(PatchHeader.md5_before_patch, pbFileMd5, MD5_DIGEST_SIZE));

        /* Close the file */
        SFileCloseFile(hFile);
    }

    return bResult;
}

static const char * FindArchiveLanguage(TMPQArchive * ha, PLOCALIZED_MPQ_INFO pMpqInfo)
{
    TFileEntry * pFileEntry;
    const char * szLanguage = LanguageList;
    char szFileName[0x40];

    /* Iterate through all localized languages */
    while(pMpqInfo->szNameTemplate != NULL)
    {
        /* Iterate through all languages */
        for(szLanguage = LanguageList; szLanguage[0] != 0; szLanguage += 4)
        {
            /* Construct the file name */
            memcpy(szFileName, pMpqInfo->szNameTemplate, pMpqInfo->nLength);
            szFileName[pMpqInfo->nLangOffset + 0] = szLanguage[0];
            szFileName[pMpqInfo->nLangOffset + 1] = szLanguage[1];
            szFileName[pMpqInfo->nLangOffset + 2] = szLanguage[2];
            szFileName[pMpqInfo->nLangOffset + 3] = szLanguage[3];

            /* Append the suffix */
            memcpy(szFileName + pMpqInfo->nLength, "-md5.lst", 9);

            /* Check whether the name exists */
            pFileEntry = GetFileEntryLocale(ha, szFileName, 0);
            if(pFileEntry != NULL)
                return szLanguage;
        }

        /* Move to the next language name */
        pMpqInfo++;
    }

    /* Not found */
    return NULL;
}

static TFileEntry * FindBaseLstFile(TMPQArchive * ha)
{
    TFileEntry * pFileEntry;
    const char * szLanguage;
    char szFileName[0x40];

    /* Prepare the file name tenplate */
    memcpy(szFileName, "####-md5.lst", 13);

    /* Try all languages */
    for(szLanguage = LanguageList; szLanguage[0] != 0; szLanguage++)
    {
        /* Copy the language name */
        szFileName[0] = szLanguage[0];
        szFileName[1] = szLanguage[1];
        szFileName[2] = szLanguage[2];
        szFileName[3] = szLanguage[3];

        /* Check whether this file exists */
        pFileEntry = GetFileEntryLocale(ha, szFileName, 0);
        if(pFileEntry != NULL)
            return pFileEntry;
    }

    return NULL;
}

static int FindPatchPrefix_WoW_13164_13623(TMPQArchive * haBase, TMPQArchive * haPatch)
{
    const char * szPatchPrefix;
    char szNamePrefix[0x08];

    /* Try to find the language of the MPQ archive */
    szPatchPrefix = FindArchiveLanguage(haBase, LocaleMpqs_WoW);
    if(szPatchPrefix == NULL)
        szPatchPrefix = "Base";

    /* Format the patch prefix */
    szNamePrefix[0] = szPatchPrefix[0];
    szNamePrefix[1] = szPatchPrefix[1];
    szNamePrefix[2] = szPatchPrefix[2];
    szNamePrefix[3] = szPatchPrefix[3];
    szNamePrefix[4] = '\\';
    szNamePrefix[5] = 0;
    return CreatePatchPrefix(haPatch, szNamePrefix, 5);
}

/*
 * Find match in Starcraft II patch MPQs
 * Match a LST file in the root directory if the MPQ with any of the file in subdirectories
 *
 * The problem:
 * Base:  enGB-md5.lst
 * Patch: Campaigns\Liberty.SC2Campaign\enGB.SC2Assets\enGB-md5.lst
 *        Campaigns\Liberty.SC2Campaign\enGB.SC2Data\enGB-md5.lst
 *        Campaigns\LibertyStory.SC2Campaign\enGB.SC2Data\enGB-md5.lst
 *        Campaigns\LibertyStory.SC2Campaign\enGB.SC2Data\enGB-md5.lst Mods\Core.SC2Mod\enGB.SC2Assets\enGB-md5.lst
 *        Mods\Core.SC2Mod\enGB.SC2Data\enGB-md5.lst
 *        Mods\Liberty.SC2Mod\enGB.SC2Assets\enGB-md5.lst
 *        Mods\Liberty.SC2Mod\enGB.SC2Data\enGB-md5.lst
 *        Mods\LibertyMulti.SC2Mod\enGB.SC2Data\enGB-md5.lst
 *
 * Solution:
 * We need to match the file by its MD5
 */

static int FindPatchPrefix_SC2(TMPQArchive * haBase, TMPQArchive * haPatch)
{
    TMPQNamePrefix * pPatchPrefix;
    TFileEntry * pBaseEntry;
    char * szLstFileName;
    char * szPlainName;
    size_t cchWorkBuffer = 0x400;
    int bResult = 0;

    /* First-level patches: Find the same file within the patch archive */
    /* and verify by MD5-before-patch */
    if(haBase->haPatch == NULL)
    {
        TFileEntry * pFileTableEnd = haPatch->pFileTable + haPatch->dwFileTableSize;
        TFileEntry * pFileEntry;

        /* Allocate working buffer for merging LST file */
        szLstFileName = STORM_ALLOC(char, cchWorkBuffer);
        if(szLstFileName != NULL)
        {
            /* Find a *-md5.lst file in the base archive */
            pBaseEntry = FindBaseLstFile(haBase);
            if(pBaseEntry == NULL)
                return 0;

            /* Parse the entire file table */
            for(pFileEntry = haPatch->pFileTable; pFileEntry < pFileTableEnd; pFileEntry++)
            {
                /* Look for "patch_metadata" file */
                if(IsPatchMetadataFile(pFileEntry))
                {
                    /* Construct the name of the MD5 file */
                    strcpy(szLstFileName, pFileEntry->szFileName);
                    szPlainName = (char *)GetPlainFileName(szLstFileName);
                    strcpy(szPlainName, pBaseEntry->szFileName);

                    /* Check for matching MD5 file */
                    if(IsMatchingPatchFile(haPatch, szLstFileName, pBaseEntry->md5))
                    {
                        bResult = CreatePatchPrefix(haPatch, szLstFileName, (size_t)(szPlainName - szLstFileName));
                        break;
                    }
                }
            }

            /* Delete the merge buffer */
            STORM_FREE(szLstFileName);
        }
    }

    // For second-level patches, just take the patch prefix from the lower level patch */
    else
    {
        // There must be at least two patches in the chain */
        assert(haBase->haPatch->pPatchPrefix != NULL);
        pPatchPrefix = haBase->haPatch->pPatchPrefix;

        // Copy the patch prefix */
        bResult = CreatePatchPrefix(haPatch,
                                    pPatchPrefix->szPatchPrefix,
                                    pPatchPrefix->nLength);
    }

    return bResult;
}

static int FindPatchPrefix(TMPQArchive * haBase, TMPQArchive * haPatch, const char * szPatchPathPrefix)
{
    /* If the patch prefix was explicitly entered, we use that one */
    if(szPatchPathPrefix != NULL)
        return CreatePatchPrefix(haPatch, szPatchPathPrefix, 0);

    /* Patches for World of Warcraft - mostly the do not use prefix. */
    /* All patches that use patch prefix have the "base\\(patch_metadata) file present */
    if(GetFileEntryLocale(haPatch, "base\\" PATCH_METADATA_NAME, 0))
        return FindPatchPrefix_WoW_13164_13623(haBase, haPatch);

    /* Updates for Starcraft II */
    /* Match: LocalizedData\GameHotkeys.txt <==> Campaigns\Liberty.SC2Campaign\enGB.SC2Data\LocalizedData\GameHotkeys.txt */
    /* All Starcraft II base archives seem to have the file "StreamingBuckets.txt" present */
    if(GetFileEntryLocale(haBase, "StreamingBuckets.txt", 0))
        return FindPatchPrefix_SC2(haBase, haPatch);

    /* Diablo III patch MPQs don't use patch prefix */
    /* Hearthstone MPQs don't use patch prefix */
    CreatePatchPrefix(haPatch, NULL, 0);
    return 1;
}

/*-----------------------------------------------------------------------------
 * Public functions (StormLib internals)
 */

int IsIncrementalPatchFile(const void * pvData, uint32_t cbData, uint32_t * pdwPatchedFileSize)
{
    PMPQ_PATCH_HEADER pPatchHeader = (PMPQ_PATCH_HEADER)pvData;
    BLIZZARD_BSDIFF40_FILE DiffFile;
    uint32_t dwPatchType;

    if(cbData >= sizeof(MPQ_PATCH_HEADER) + sizeof(BLIZZARD_BSDIFF40_FILE))
    {
        dwPatchType = BSWAP_INT32_UNSIGNED(pPatchHeader->dwPatchType);
        if(dwPatchType == 0x30445342)
        {
            /* Give the caller the patch file size */
            if(pdwPatchedFileSize != NULL)
            {
                Decompress_RLE((unsigned char *)&DiffFile, sizeof(BLIZZARD_BSDIFF40_FILE), (unsigned char *)(pPatchHeader + 1), sizeof(BLIZZARD_BSDIFF40_FILE));
                DiffFile.NewFileSize = BSWAP_INT64_UNSIGNED(DiffFile.NewFileSize);
                *pdwPatchedFileSize = (uint32_t)DiffFile.NewFileSize;
                return 1;
            }
        }
    }

    return 0;
}

int Patch_InitPatcher(TMPQPatcher * pPatcher, TMPQFile * hf)
{
    uint32_t cbMaxFileData = 0;

    /* Overflow check */
    if((sizeof(MPQ_PATCH_HEADER) + cbMaxFileData) < cbMaxFileData)
        return ERROR_NOT_ENOUGH_MEMORY;
    if(hf->hfPatch == NULL)
        return ERROR_INVALID_PARAMETER;

    /* Initialize the entire structure with zeros */
    memset(pPatcher, 0, sizeof(TMPQPatcher));

    /* Copy the MD5 of the current file */
    memcpy(pPatcher->this_md5, hf->pFileEntry->md5, MD5_DIGEST_SIZE);

    /* Find out the biggest data size needed during the patching process */
    while(hf != NULL)
    {
        if(hf->pFileEntry->dwFileSize > cbMaxFileData)
            cbMaxFileData = hf->pFileEntry->dwFileSize;
        hf = hf->hfPatch;
    }

    /* Allocate primary and secondary buffer */
    pPatcher->pbFileData1 = STORM_ALLOC(uint8_t, cbMaxFileData);
    pPatcher->pbFileData2 = STORM_ALLOC(uint8_t, cbMaxFileData);
    if(!pPatcher->pbFileData1 || !pPatcher->pbFileData2)
        return ERROR_NOT_ENOUGH_MEMORY;

    pPatcher->cbMaxFileData = cbMaxFileData;
    return ERROR_SUCCESS;
}

/*
 * Note: The patch may either be applied to the base file or to the previous version
 * In Starcraft II, Mods\Core.SC2Mod\Base.SC2Data, file StreamingBuckets.txt:
 *
 * Base file MD5: 31376b0344b6df59ad009d4296125539
 *
 * s2-update-base-23258: from 31376b0344b6df59ad009d4296125539 to 941a82683452e54bf024a8d491501824
 * s2-update-base-24540: from 31376b0344b6df59ad009d4296125539 to 941a82683452e54bf024a8d491501824
 * s2-update-base-26147: from 31376b0344b6df59ad009d4296125539 to d5d5253c762fac6b9761240288a0771a
 * s2-update-base-28522: from 31376b0344b6df59ad009d4296125539 to 5a76c4b356920aab7afd22e0e1913d7a
 * s2-update-base-30508: from 31376b0344b6df59ad009d4296125539 to 8cb0d4799893fe801cc78ae4488a3671
 * s2-update-base-32283: from 31376b0344b6df59ad009d4296125539 to 8cb0d4799893fe801cc78ae4488a3671
 *
 * We don't keep all intermediate versions in memory, as it would cause massive
 * memory usage during patching process. A prime example is the file
 * DBFilesClient\\Item-Sparse.db2 from locale-enGB.MPQ (WoW 16965), which has
 * 9 patches in a row, each requiring 70 MB memory (35 MB patch data + 35 MB work buffer)
 */

int Patch_Process(TMPQPatcher * pPatcher, TMPQFile * hf)
{
    PMPQ_PATCH_HEADER pFullPatch;
    MPQ_PATCH_HEADER PatchHeader1;
    MPQ_PATCH_HEADER PatchHeader2 = {0};
    TMPQFile * hfBase = hf;
    size_t cbBytesRead = 0;
    int nError = ERROR_SUCCESS;

    /* Move to the first patch */
    assert(hfBase->pbFileData == NULL);
    assert(hfBase->cbFileData == 0);
    hf = hf->hfPatch;

    /* Read the header of the current patch */
    SFileReadFile((void *)hf, &PatchHeader1, sizeof(MPQ_PATCH_HEADER), &cbBytesRead);
    if(cbBytesRead != sizeof(MPQ_PATCH_HEADER))
        return ERROR_FILE_CORRUPT;

    /* Perform the patching process */
    while(nError == ERROR_SUCCESS && hf != NULL)
    {
        /* Try to read the next patch header. If the md5_before_patch */
        /* still matches we go directly to the next one and repeat */
        while(hf->hfPatch != NULL)
        {
            /* Attempt to read the patch header */
            SFileReadFile((void *)hf->hfPatch, &PatchHeader2, sizeof(MPQ_PATCH_HEADER), &cbBytesRead);
            if(cbBytesRead != sizeof(MPQ_PATCH_HEADER))
                return ERROR_FILE_CORRUPT;

            /* Compare the md5_before_patch */
            if(memcmp(PatchHeader2.md5_before_patch, pPatcher->this_md5, MD5_DIGEST_SIZE))
                break;

            /* Move one patch fuhrter */
            PatchHeader1 = PatchHeader2;
            hf = hf->hfPatch;
        }

        /* Allocate memory for the patch data */
        pFullPatch = LoadFullFilePatch(hf, &PatchHeader1);
        if(pFullPatch != NULL)
        {
            /* Apply the patch */
            nError = ApplyFilePatch(pPatcher, pFullPatch);
            STORM_FREE(pFullPatch);
        }
        else
        {
            nError = ERROR_FILE_CORRUPT;
        }

        /* Move to the next patch */
        PatchHeader1 = PatchHeader2;
        pPatcher->nCounter++;
        hf = hf->hfPatch;
    }

    /* Put the result data to the file structure */
    if(nError == ERROR_SUCCESS)
    {
        /* Swap the pointer to the file data structure */
        if(pPatcher->nCounter & 0x01)
        {
            hfBase->pbFileData = pPatcher->pbFileData2;
            pPatcher->pbFileData2 = NULL;
        }
        else
        {
            hfBase->pbFileData = pPatcher->pbFileData1;
            pPatcher->pbFileData1 = NULL;
        }

        /* Also supply the data size */
        hfBase->cbFileData = pPatcher->cbFileData;
    }

    return ERROR_SUCCESS;
}

void Patch_Finalize(TMPQPatcher * pPatcher)
{
    if(pPatcher != NULL)
    {
        if(pPatcher->pbFileData1 != NULL)
            STORM_FREE(pPatcher->pbFileData1);
        if(pPatcher->pbFileData2 != NULL)
            STORM_FREE(pPatcher->pbFileData2);
        
        memset(pPatcher, 0, sizeof(TMPQPatcher));
    }
}


/*-----------------------------------------------------------------------------
 * Public functions
 *
 *
 * Patch prefix is the path subdirectory where the patched files are within MPQ.
 *
 * Example 1:
 * Main MPQ:  locale-enGB.MPQ
 * Patch MPQ: wow-update-12694.MPQ
 * File in main MPQ: DBFilesClient\Achievement.dbc
 * File in patch MPQ: enGB\DBFilesClient\Achievement.dbc
 * Path prefix: enGB
 *
 * Example 2:
 * Main MPQ:  expansion1.MPQ
 * Patch MPQ: wow-update-12694.MPQ
 * File in main MPQ: DBFilesClient\Achievement.dbc
 * File in patch MPQ: Base\DBFilesClient\Achievement.dbc
 * Path prefix: Base
 */

int EXPORT_SYMBOL SFileOpenPatchArchive(
    void * hMpq,
    const char * szPatchMpqName,
    const char * szPatchPathPrefix,
    uint32_t dwFlags)
{
    TMPQArchive * haPatch;
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    void * hPatchMpq = NULL;
    int nError = ERROR_SUCCESS;

    /* Keep compiler happy */
    dwFlags = dwFlags;

    /* Verify input parameters */
    if(!IsValidMpqHandle(hMpq))
        nError = ERROR_INVALID_HANDLE;
    if(szPatchMpqName == NULL || *szPatchMpqName == 0)
        nError = ERROR_INVALID_PARAMETER;

    /*
     * We don't allow adding patches to archives that have been open for write
     *
     * Error scenario:
     *
     * 1) Open archive for writing
     * 2) Modify or replace a file
     * 3) Add patch archive to the opened MPQ
     * 4) Read patched file
     * 5) Now what ?
     */

    if(nError == ERROR_SUCCESS)
    {
        if(!(ha->dwFlags & MPQ_FLAG_READ_ONLY))
            nError = ERROR_ACCESS_DENIED;
    }

    /* Open the archive like it is normal archive */
    if(nError == ERROR_SUCCESS)
    {
        if(!SFileOpenArchive(szPatchMpqName, 0, MPQ_OPEN_READ_ONLY | MPQ_OPEN_PATCH, &hPatchMpq))
            return 0;
        haPatch = (TMPQArchive *)hPatchMpq;

        /* We need to remember the proper patch prefix to match names of patched files */
        FindPatchPrefix(ha, (TMPQArchive *)hPatchMpq, szPatchPathPrefix);

        /* Now add the patch archive to the list of patches to the original MPQ */
        while(ha != NULL)
        {
            if(ha->haPatch == NULL)
            {
                haPatch->haBase = ha;
                ha->haPatch = haPatch;
                return 1;
            }

            /* Move to the next archive */
            ha = ha->haPatch;
        }

        /* Should never happen */
        nError = ERROR_CAN_NOT_COMPLETE;
    }

    SetLastError(nError);
    return 0;
}

int EXPORT_SYMBOL SFileIsPatchedArchive(void * hMpq)
{
    TMPQArchive * ha = (TMPQArchive *)hMpq;

    /* Verify input parameters */
    if(!IsValidMpqHandle(hMpq))
        return 0;

    return (ha->haPatch != NULL);
}
