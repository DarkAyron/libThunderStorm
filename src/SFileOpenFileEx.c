/*****************************************************************************/
/* SFileOpenFileEx.c                      Copyright (c) Ladislav Zezula 2003 */
/*---------------------------------------------------------------------------*/
/* Description :                                                             */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* xx.xx.99  1.00  Lad  The first version of SFileOpenFileEx.cpp             */
/* 01.04.15  1.00  Ayr  Ported to plain c                                    */
/*****************************************************************************/

#include "thunderStorm.h"
#include "StormCommon.h"

/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

static const char * GetPatchFileName(TMPQArchive * ha, const char * szFileName, char * szBuffer)
{
    TMPQNamePrefix * pPrefix;

    /* Are there patches in the current MPQ? */
    if(ha->dwFlags & MPQ_FLAG_PATCH)
    {
        /* The patch prefix must be already known here */
        assert(ha->pPatchPrefix != NULL);
        pPrefix = ha->pPatchPrefix;

        /* The patch name for "OldWorld\\XXX\\YYY" is "Base\\XXX\YYY" */
        /* We need to remove the "OldWorld\\" prefix */
        if(!strncasecmp(szFileName, "OldWorld\\", 9))
            szFileName += 9;

        /* Create the file name from the known patch entry */
        memcpy(szBuffer, pPrefix->szPatchPrefix, pPrefix->nLength);
        strcpy(szBuffer + pPrefix->nLength, szFileName);
        szFileName = szBuffer;
    }

    return szFileName;
}

static int OpenLocalFile(const char * szFileName, void * * phFile)
{
    TFileStream * pStream;
    TMPQFile * hf = NULL;
    char szFileNameT[MAX_PATH];

    /* Convert the file name to UNICODE (if needed) */
    CopyFileName(szFileNameT, szFileName, strlen(szFileName));

    /* Open the file and create the TMPQFile structure */
    pStream = FileStream_OpenFile(szFileNameT, STREAM_FLAG_READ_ONLY);
    if(pStream != NULL)
    {
        /* Allocate and initialize file handle */
        hf = CreateFileHandle(NULL, NULL);
        if(hf != NULL)
        {
            hf->pStream = pStream;
            *phFile = hf;
            return 1;
        }
        else
        {
            FileStream_Close(pStream);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    *phFile = NULL;
    return 0;
}

int OpenPatchedFile(void * hMpq, const char * szFileName, void * * phFile)
{
    TMPQArchive * haBase = NULL;
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    TFileEntry * pFileEntry;
    TMPQFile * hfPatch;                     /* Pointer to patch file */
    TMPQFile * hfBase = NULL;               /* Pointer to base open file */
    TMPQFile * hf = NULL;
    void * hPatchFile;
    char szNameBuffer[MAX_PATH];

    /* First of all, find the latest archive where the file is in base version */
    /* (i.e. where the original, unpatched version of the file exists) */
    while(ha != NULL)
    {
        /* If the file is there, then we remember the archive */
        pFileEntry = GetFileEntryExact(ha, GetPatchFileName(ha, szFileName, szNameBuffer), 0);
        if(pFileEntry != NULL && (pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) == 0)
            haBase = ha;

        /* Move to the patch archive */
        ha = ha->haPatch;
    }

    /* If we couldn't find the base file in any of the patches, it doesn't exist */
    if((ha = haBase) == NULL)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return 0;
    }

    /* Now open the base file */
    if(SFileOpenFileEx((void *)ha, GetPatchFileName(ha, szFileName, szNameBuffer), SFILE_OPEN_BASE_FILE, (void * *)&hfBase))
    {
        /* The file must be a base file, i.e. without MPQ_FILE_PATCH_FILE */
        assert((hfBase->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) == 0);
        hf = hfBase;

        /* Now open all patches and attach them on top of the base file */
        for(ha = ha->haPatch; ha != NULL; ha = ha->haPatch)
        {
            /* Prepare the file name with a correct prefix */
            if(SFileOpenFileEx((void *)ha, GetPatchFileName(ha, szFileName, szNameBuffer), SFILE_OPEN_BASE_FILE, &hPatchFile))
            {
                /* Remember the new version */
                hfPatch = (TMPQFile *)hPatchFile;

                /* We should not find patch file */
                assert((hfPatch->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) != 0);

                /* Attach the patch to the base file */
                hf->hfPatch = hfPatch;
                hf = hfPatch;
            }
        }
    }

    /* Give the updated base MPQ */
    if(phFile != NULL)
        *phFile = (void *)hfBase;
    return (hfBase != NULL);
}

/*****************************************************************************/
/* Public functions                                                          */
/*****************************************************************************/

/*-----------------------------------------------------------------------------
 * SFileEnumLocales enums all locale versions within MPQ. 
 * Functions fills all available language identifiers on a file into the buffer
 * pointed by plcLocales. There must be enough entries to copy the localed,
 * otherwise the function returns ERROR_INSUFFICIENT_BUFFER.
 */

int EXPORT_SYMBOL SFileEnumLocales(
    void * hMpq,
    const char * szFileName,
    uint32_t * plcLocales,
    uint32_t * pdwMaxLocales,
    uint32_t dwSearchScope)
{
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    TFileEntry * pFileEntry;
    TMPQHash * pFirstHash;
    TMPQHash * pHash;
    uint32_t dwFileIndex = 0;
    uint32_t dwLocales = 0;

    /* Test the parameters */
    if(!IsValidMpqHandle(hMpq))
        return ERROR_INVALID_HANDLE;
    if(szFileName == NULL || *szFileName == 0)
        return ERROR_INVALID_PARAMETER;
    if(pdwMaxLocales == NULL)
        return ERROR_INVALID_PARAMETER;
    
    /* Keep compiler happy */
    dwSearchScope = dwSearchScope;

    /* Parse hash table entries for all locales */
    if(!IsPseudoFileName(szFileName, &dwFileIndex))
    {
        /* Calculate the number of locales */
        pFirstHash = pHash = GetFirstHashEntry(ha, szFileName);
        while(pHash != NULL)
        {
            dwLocales++;
            pHash = GetNextHashEntry(ha, pFirstHash, pHash);
        }

        /* Test if there is enough space to copy the locales */
        if(*pdwMaxLocales < dwLocales)
        {
            *pdwMaxLocales = dwLocales;
            return ERROR_INSUFFICIENT_BUFFER;
        }

        /* Enum the locales */
        pFirstHash = pHash = GetFirstHashEntry(ha, szFileName);
        while(pHash != NULL)
        {
            *plcLocales++ = pHash->lcLocale;
            pHash = GetNextHashEntry(ha, pFirstHash, pHash);
        }
    }
    else
    {
        /* There must be space for 1 locale */
        if(*pdwMaxLocales < 1)
        {
            *pdwMaxLocales = 1;
            return ERROR_INSUFFICIENT_BUFFER;
        }

        /* For nameless access, always return 1 locale */
        pFileEntry = GetFileEntryByIndex(ha, dwFileIndex);
        pHash = ha->pHashTable + pFileEntry->dwHashIndex;
        *plcLocales = pHash->lcLocale;
        dwLocales = 1;
    }

    /* Give the caller the total number of found locales */
    *pdwMaxLocales = dwLocales;
    return ERROR_SUCCESS;
}

/*-----------------------------------------------------------------------------
 * SFileHasFile
 *
 *   hMpq          - Handle of opened MPQ archive
 *   szFileName    - Name of file to look for
 */

int EXPORT_SYMBOL SFileHasFile(void * hMpq, const char * szFileName)
{
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    TFileEntry * pFileEntry;
    uint32_t dwFlagsToCheck = MPQ_FILE_EXISTS;
    uint32_t dwFileIndex = 0;
    char szPrefixBuffer[MAX_PATH];
    int bIsPseudoName;
    int nError = ERROR_SUCCESS;

    if(!IsValidMpqHandle(hMpq))
        nError = ERROR_INVALID_HANDLE;
    if(szFileName == NULL || *szFileName == 0)
        nError = ERROR_INVALID_PARAMETER;

    /* Prepare the file opening */
    if(nError == ERROR_SUCCESS)
    {
        /* Different processing for pseudo-names */
        bIsPseudoName = IsPseudoFileName(szFileName, &dwFileIndex);

        /* Walk through the MPQ and all patches */
        while(ha != NULL)
        {
            /* Verify presence of the file */
            pFileEntry = (!bIsPseudoName) ? GetFileEntryLocale(ha, GetPatchFileName(ha, szFileName, szPrefixBuffer), lcFileLocale)
                                                  : GetFileEntryByIndex(ha, dwFileIndex);
            /* Verify the file flags */
            if(pFileEntry != NULL && (pFileEntry->dwFlags & dwFlagsToCheck) == MPQ_FILE_EXISTS)
                return 1;

            /* If this is patched archive, go to the patch */
            dwFlagsToCheck = MPQ_FILE_EXISTS | MPQ_FILE_PATCH_FILE;
            ha = ha->haPatch;
        }

        /* Not found, sorry */
        nError = ERROR_FILE_NOT_FOUND;
    }

    /* Cleanup */
    SetLastError(nError);
    return 0;
}


/*-----------------------------------------------------------------------------
 * SFileOpenFileEx
 *
 *   hMpq          - Handle of opened MPQ archive
 *   szFileName    - Name of file to open
 *   dwSearchScope - Where to search
 *   phFile        - Pointer to store opened file handle
 */

int EXPORT_SYMBOL SFileOpenFileEx(void * hMpq, const char * szFileName, uint32_t dwSearchScope, void * * phFile)
{
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    TFileEntry  * pFileEntry = NULL;
    TMPQFile    * hf = NULL;
    uint32_t dwFileIndex = 0;
    int bOpenByIndex = 0;
    int nError = ERROR_SUCCESS;

    /* Don't accept NULL pointer to file handle */
    if(phFile == NULL)
        nError = ERROR_INVALID_PARAMETER;

    /* Prepare the file opening */
    if(nError == ERROR_SUCCESS)
    {
        switch(dwSearchScope)
        {
            case SFILE_OPEN_FROM_MPQ:
            case SFILE_OPEN_BASE_FILE:
                
                if(!IsValidMpqHandle(hMpq))
                {
                    nError = ERROR_INVALID_HANDLE;
                    break;
                }

                if(szFileName == NULL || *szFileName == 0)
                {
                    nError = ERROR_INVALID_PARAMETER;
                    break;
                }

                /* Check the pseudo-file name */
                if(IsPseudoFileName(szFileName, &dwFileIndex))
                {
                    pFileEntry = GetFileEntryByIndex(ha, dwFileIndex);
                    bOpenByIndex = 1;
                    if(pFileEntry == NULL)
                        nError = ERROR_FILE_NOT_FOUND;
                }
                else
                {
                    /* If this MPQ is a patched archive, open the file as patched */
                    if(ha->haPatch == NULL || dwSearchScope == SFILE_OPEN_BASE_FILE)
                    {
                        /* Otherwise, open the file from *this* MPQ */
                        pFileEntry = GetFileEntryLocale(ha, szFileName, lcFileLocale);
                        if(pFileEntry == NULL)
                            nError = ERROR_FILE_NOT_FOUND;
                    }
                    else
                    {
                        return OpenPatchedFile(hMpq, szFileName, phFile);
                    }
                }
                break;

            case SFILE_OPEN_ANY_LOCALE:

                /* This open option is reserved for opening MPQ internal listfile. */
                /* No argument validation. Tries to open file with neutral locale first, */
                /* then any other available. */
                pFileEntry = GetFileEntryAny(ha, szFileName);
                if(pFileEntry == NULL)
                    nError = ERROR_FILE_NOT_FOUND;
                break;

            case SFILE_OPEN_LOCAL_FILE:

                if(szFileName == NULL || *szFileName == 0)
                {
                    nError = ERROR_INVALID_PARAMETER;
                    break;
                }

                return OpenLocalFile(szFileName, phFile); 

            default:

                /* Don't accept any other value */
                nError = ERROR_INVALID_PARAMETER;
                break;
        }

        /* Quick return if something failed */
        if(nError != ERROR_SUCCESS)
        {
            SetLastError(nError);
            *phFile = NULL;
            return 0;
        }
    }

    /* Test if the file was not already deleted. */
    if(nError == ERROR_SUCCESS)
    {
        if((pFileEntry->dwFlags & MPQ_FILE_EXISTS) == 0)
            nError = ERROR_FILE_NOT_FOUND;
        if(pFileEntry->dwFlags & ~MPQ_FILE_VALID_FLAGS)
            nError = ERROR_NOT_SUPPORTED;
    }

    /* Allocate file handle */
    if(nError == ERROR_SUCCESS)
    {
        hf = CreateFileHandle(ha, pFileEntry);
        if(hf == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Initialize file handle */
    if(nError == ERROR_SUCCESS)
    {
        /* If the MPQ has sector CRC enabled, enable if for the file */
        if(ha->dwFlags & MPQ_FLAG_CHECK_SECTOR_CRC)
            hf->bCheckSectorCRCs = 1;

        /* If we know the real file name, copy it to the file entry */
        if(!bOpenByIndex)
        {
            /* If there is no file name yet, allocate it */
            AllocateFileName(ha, pFileEntry, szFileName);

            /* If the file is encrypted, we should detect the file key */
            if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED)
                hf->dwFileKey = DecryptFileKey(szFileName,
                                               pFileEntry->ByteOffset,
                                               pFileEntry->dwFileSize,
                                               pFileEntry->dwFlags);
        }
        else
        {
            /* Try to auto-detect the file name */
            if(!SFileGetFileName(hf, NULL))
                nError = GetLastError();
        }
    }

    /* Cleanup and exit */
    if(nError != ERROR_SUCCESS)
    {
        SetLastError(nError);
        FreeFileHandle(&hf);
        return 0;
    }

    *phFile = hf;
    return 1;
}

/*-----------------------------------------------------------------------------
 * int SFileCloseFile(void * hFile);
 */

int EXPORT_SYMBOL SFileCloseFile(void * hFile)
{
    TMPQFile * hf = (TMPQFile *)hFile;
    
    if(!IsValidFileHandle(hFile))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    /* Free the structure */
    FreeFileHandle(&hf);
    return 1;
}
