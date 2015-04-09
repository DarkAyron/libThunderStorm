/*****************************************************************************/
/* SFileExtractFile.c                     Copyright (c) Ladislav Zezula 2003 */
/*---------------------------------------------------------------------------*/
/* Simple extracting utility                                                 */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 20.06.03  1.00  Lad  The first version of SFileExtractFile.cpp            */
/* 27.03.15  1.00  Ayr  Ported to plain c                                    */
/*****************************************************************************/

#include "thunderStorm.h"
#include "StormCommon.h"

int EXPORT_SYMBOL SFileExtractFile(void * hMpq, const char * szToExtract, const char * szExtracted, uint32_t dwSearchScope)
{
    TFileStream * pLocalFile = NULL;
    void * hMpqFile = NULL;
    int nError = ERROR_SUCCESS;

    /* Open the MPQ file */
    if(nError == ERROR_SUCCESS)
    {
        if(!SFileOpenFileEx(hMpq, szToExtract, dwSearchScope, &hMpqFile))
            nError = GetLastError();
    }

    /* Create the local file */
    if(nError == ERROR_SUCCESS)
    {
        pLocalFile = FileStream_CreateFile(szExtracted, 0);
        if(pLocalFile == NULL)
            nError = GetLastError();
    }

    /* Copy the file's content */
    while(nError == ERROR_SUCCESS)
    {
        char  szBuffer[0x1000];
        size_t dwTransferred = 0;

        /* dwTransferred is only set to nonzero if something has been read. */
        /* nError can be ERROR_SUCCESS or ERROR_HANDLE_EOF */
        if(!SFileReadFile(hMpqFile, szBuffer, sizeof(szBuffer), &dwTransferred))
            nError = GetLastError();
        if(nError == ERROR_HANDLE_EOF)
            nError = ERROR_SUCCESS;
        if(dwTransferred == 0)
            break;

        /* If something has been actually read, write it */
        if(!FileStream_Write(pLocalFile, NULL, szBuffer, dwTransferred))
            nError = GetLastError();
    }

    /* Close the files */
    if(hMpqFile != NULL)
        SFileCloseFile(hMpqFile);
    if(pLocalFile != NULL)
        FileStream_Close(pLocalFile);
    if(nError != ERROR_SUCCESS)
        SetLastError(nError);
    return (nError == ERROR_SUCCESS);
}
