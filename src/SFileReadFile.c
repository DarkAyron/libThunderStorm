/*****************************************************************************/
/* SFileReadFile.c                        Copyright (c) Ladislav Zezula 2003 */
/*---------------------------------------------------------------------------*/
/* Description :                                                             */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* xx.xx.99  1.00  Lad  The first version of SFileReadFile.cpp               */
/* 24.03.99  1.00  Lad  Added the SFileGetFileInfo function                  */
/* 02.04.15  1.00  Ayr  Ported to plain c                                    */
/*****************************************************************************/

#include "thunderStorm.h"
#include "StormCommon.h"

/*-----------------------------------------------------------------------------
 * Local functions
 *
 *  hf            - MPQ File handle.
 *  pbBuffer      - Pointer to target buffer to store sectors.
 *  dwByteOffset  - Position of sector in the file (relative to file begin)
 *  dwBytesToRead - Number of bytes to read. Must be multiplier of sector size.
 *  pdwBytesRead  - Stored number of bytes loaded
 */
static int ReadMpqSectors(TMPQFile * hf, unsigned char * pbBuffer, uint32_t dwByteOffset, uint32_t dwBytesToRead, uint32_t * pdwBytesRead)
{
    uint64_t RawFilePos;
    TMPQArchive * ha = hf->ha;
    TFileEntry * pFileEntry = hf->pFileEntry;
    unsigned char * pbRawSector = NULL;
    unsigned char * pbOutSector = pbBuffer;
    unsigned char * pbInSector = pbBuffer;
    uint32_t dwRawBytesToRead;
    uint32_t dwRawSectorOffset = dwByteOffset;
    uint32_t dwSectorsToRead = dwBytesToRead / ha->dwSectorSize;
    uint32_t dwSectorIndex = dwByteOffset / ha->dwSectorSize;
    uint32_t dwSectorsDone = 0;
    uint32_t dwBytesRead = 0;
    int nError = ERROR_SUCCESS;

    /* Note that dwByteOffset must be aligned to size of one sector */
    /* Note that dwBytesToRead must be a multiplier of one sector size */
    /* This is local function, so we won't check if that's true. */
    /* Note that files stored in single units are processed by a separate function */

    /* If there is not enough bytes remaining, cut dwBytesToRead */
    if((dwByteOffset + dwBytesToRead) > hf->dwDataSize)
        dwBytesToRead = hf->dwDataSize - dwByteOffset;
    dwRawBytesToRead = dwBytesToRead;

    /* Perform all necessary work to do with compressed files */
    if(pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK)
    {
        /* If the sector positions are not loaded yet, do it */
        if(hf->SectorOffsets == NULL)
        {
            nError = AllocateSectorOffsets(hf, 1);
            if(nError != ERROR_SUCCESS)
                return nError;
        }

        /* If the sector checksums are not loaded yet, load them now. */
        if(hf->SectorChksums == NULL && (pFileEntry->dwFlags & MPQ_FILE_SECTOR_CRC) && hf->bLoadedSectorCRCs == 0)
        {
            /*
             * Sector CRCs is plain crap feature. It is almost never present,
             * often it's empty, or the end offset of sector CRCs is zero.
             * We only try to load sector CRCs once, and regardless if it fails
             * or not, we won't try that again for the given file.
             */

            AllocateSectorChecksums(hf, 1);
            hf->bLoadedSectorCRCs = 1;
        }

        /* TODO: If the raw data MD5s are not loaded yet, load them now */
        /* Only do it if the MPQ is of format 4.0 */
/*      if(ha->pHeader->wFormatVersion >= MPQ_FORMAT_VERSION_4 && ha->pHeader->dwRawChunkSize != 0)
        {
            nError = AllocateRawMD5s(hf, 1);
            if(nError != ERROR_SUCCESS)
                return nError;
        } */

        /* Assign the temporary buffer as target for read operation */
        dwRawSectorOffset = hf->SectorOffsets[dwSectorIndex];
        dwRawBytesToRead = hf->SectorOffsets[dwSectorIndex + dwSectorsToRead] - dwRawSectorOffset;
        
        /* If the file is compressed, also allocate secondary buffer */
        pbInSector = pbRawSector = STORM_ALLOC(uint8_t, dwRawBytesToRead);
        if(pbRawSector == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Calculate raw file offset where the sector(s) are stored. */
    RawFilePos = CalculateRawSectorOffset(hf, dwRawSectorOffset);

    /* Set file pointer and read all required sectors */
    if(FileStream_Read(ha->pStream, &RawFilePos, pbInSector, dwRawBytesToRead))
    {
        uint32_t i;
        
        /* Now we have to decrypt and decompress all file sectors that have been loaded */
        for(i = 0; i < dwSectorsToRead; i++)
        {
            uint32_t dwRawBytesInThisSector = ha->dwSectorSize;
            uint32_t dwBytesInThisSector = ha->dwSectorSize;
            uint32_t dwIndex = dwSectorIndex + i;

            /* If there is not enough bytes in the last sector, */
            /* cut the number of bytes in this sector */
            if(dwRawBytesInThisSector > dwBytesToRead)
                dwRawBytesInThisSector = dwBytesToRead;
            if(dwBytesInThisSector > dwBytesToRead)
                dwBytesInThisSector = dwBytesToRead;

            /* If the file is compressed, we have to adjust the raw sector size */
            if(pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK)
                dwRawBytesInThisSector = hf->SectorOffsets[dwIndex + 1] - hf->SectorOffsets[dwIndex];

            /* If the file is encrypted, we have to decrypt the sector */
            if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED)
            {
                if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPT_ANUBIS)
                {
                    DecryptMpqBlockAnubis(pbInSector, dwRawBytesInThisSector, &(ha->keySchedule));
                }
                
                BSWAP_ARRAY32_UNSIGNED(pbInSector, dwRawBytesInThisSector);

                /* If we don't know the key, try to detect it by file content */
                if(hf->dwFileKey == 0)
                {
                    hf->dwFileKey = DetectFileKeyByContent(pbInSector, dwBytesInThisSector, hf->dwDataSize);
                    if(hf->dwFileKey == 0)
                    {
                        nError = ERROR_UNKNOWN_FILE_KEY;
                        break;
                    }
                }

                DecryptMpqBlock(pbInSector, dwRawBytesInThisSector, hf->dwFileKey + dwIndex);
                BSWAP_ARRAY32_UNSIGNED(pbInSector, dwRawBytesInThisSector);
            }

            /* If the file has sector CRC check turned on, perform it */
            if(hf->bCheckSectorCRCs && hf->SectorChksums != NULL)
            {
                uint32_t dwAdlerExpected = hf->SectorChksums[dwIndex];
                uint32_t dwAdlerValue = 0;

                /* We can only check sector CRC when it's not zero */
                /* Neither can we check it if it's 0xFFFFFFFF. */
                if(dwAdlerExpected != 0 && dwAdlerExpected != 0xFFFFFFFF)
                {
                    dwAdlerValue = adler32(0, pbInSector, dwRawBytesInThisSector);
                    if(dwAdlerValue != dwAdlerExpected)
                    {
                        nError = ERROR_CHECKSUM_ERROR;
                        break;
                    }
                }
            }

            /* If the sector is really compressed, decompress it. */
            /* WARNING : Some sectors may not be compressed, it can be determined only */
            /* by comparing uncompressed and compressed size !!! */
            if(dwRawBytesInThisSector < dwBytesInThisSector)
            {
                int cbOutSector = dwBytesInThisSector;
                int cbInSector = dwRawBytesInThisSector;
                int nResult = 0;

                /* Is the file compressed by Blizzard's multiple compression ? */
                if(pFileEntry->dwFlags & MPQ_FILE_COMPRESS)
                {
                    if(ha->pHeader->wFormatVersion >= MPQ_FORMAT_VERSION_2)
                        nResult = SCompDecompress2(pbOutSector, &cbOutSector, pbInSector, cbInSector);
                    else
                        nResult = SCompDecompress(pbOutSector, &cbOutSector, pbInSector, cbInSector);
                }

                /* Is the file compressed by PKWARE Data Compression Library ? */
                else if(pFileEntry->dwFlags & MPQ_FILE_IMPLODE)
                {
                    nResult = SCompExplode(pbOutSector, &cbOutSector, pbInSector, cbInSector);
                }

                /* Did the decompression fail ? */
                if(nResult == 0)
                {
                    nError = ERROR_FILE_CORRUPT;
                    break;
                }
            }
            else
            {
                if(pbOutSector != pbInSector)
                    memcpy(pbOutSector, pbInSector, dwBytesInThisSector);
            }

            /* Move pointers */
            dwBytesToRead -= dwBytesInThisSector;
            dwByteOffset += dwBytesInThisSector;
            dwBytesRead += dwBytesInThisSector;
            pbOutSector += dwBytesInThisSector;
            pbInSector += dwRawBytesInThisSector;
            dwSectorsDone++;
        }
    }
    else
    {
        nError = GetLastError();
    }

    /* Free all used buffers */
    if(pbRawSector != NULL)
        STORM_FREE(pbRawSector);
    
    /* Give the caller thenumber of bytes read */
    *pdwBytesRead = dwBytesRead;
    return nError; 
}

static int ReadMpqFileSingleUnit(TMPQFile * hf, void * pvBuffer, uint32_t dwFilePos, uint32_t dwToRead, uint32_t * pdwBytesRead)
{
    uint64_t RawFilePos = hf->RawFilePos;
    TMPQArchive * ha = hf->ha;
    TFileEntry * pFileEntry = hf->pFileEntry;
    unsigned char * pbCompressed = NULL;
    unsigned char * pbRawData = NULL;
    int nError = ERROR_SUCCESS;

    /* If the file buffer is not allocated yet, do it. */
    if(hf->pbFileSector == NULL)
    {
        nError = AllocateSectorBuffer(hf);
        if(nError != ERROR_SUCCESS)
            return nError;
        pbRawData = hf->pbFileSector;
    }

    /* If the file is a patch file, adjust raw data offset */
    if(hf->pPatchInfo != NULL)
        RawFilePos += hf->pPatchInfo->dwLength;

    /* If the file sector is not loaded yet, do it */
    if(hf->dwSectorOffs != 0)
    {
        /* Is the file compressed? */
        if(pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK)
        {
            /* Allocate space for compressed data */
            pbCompressed = STORM_ALLOC(uint8_t, pFileEntry->dwCmpSize);
            if(pbCompressed == NULL)
                return ERROR_NOT_ENOUGH_MEMORY;
            pbRawData = pbCompressed;
        }
        
        /* Load the raw (compressed, encrypted) data */
        if(!FileStream_Read(ha->pStream, &RawFilePos, pbRawData, pFileEntry->dwCmpSize))
        {
            STORM_FREE(pbCompressed);
            return GetLastError();
        }

        /* If the file is encrypted, we have to decrypt the data first */
        if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED)
        {
            BSWAP_ARRAY32_UNSIGNED(pbRawData, pFileEntry->dwCmpSize);
            DecryptMpqBlock(pbRawData, pFileEntry->dwCmpSize, hf->dwFileKey);
            BSWAP_ARRAY32_UNSIGNED(pbRawData, pFileEntry->dwCmpSize);
        }

        /* If the file is compressed, we have to decompress it now */
        if(pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK)
        {
            int cbOutBuffer = (int)hf->dwDataSize;
            int cbInBuffer = (int)pFileEntry->dwCmpSize;
            int nResult = 0;

            /*
             * If the file is an incremental patch, the size of compressed data
             * is determined as pFileEntry->dwCmpSize - sizeof(TPatchInfo)
             *
             * In "wow-update-12694.MPQ" from Wow-Cataclysm BETA:
             *
             * File                                    CmprSize   DcmpSize DataSize Compressed?
             * --------------------------------------  ---------- -------- -------- ---------------
             * esES\DBFilesClient\LightSkyBox.dbc      0xBE->0xA2  0xBC     0xBC     Yes
             * deDE\DBFilesClient\MountCapability.dbc  0x93->0x77  0x77     0x77     No
             */

            if(pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE)
                cbInBuffer = cbInBuffer - sizeof(TPatchInfo);

            /* Is the file compressed by Blizzard's multiple compression ? */
            if(pFileEntry->dwFlags & MPQ_FILE_COMPRESS)
            {
                if(ha->pHeader->wFormatVersion >= MPQ_FORMAT_VERSION_2)
                    nResult = SCompDecompress2(hf->pbFileSector, &cbOutBuffer, pbRawData, cbInBuffer);
                else
                    nResult = SCompDecompress(hf->pbFileSector, &cbOutBuffer, pbRawData, cbInBuffer);
            }

            /* Is the file compressed by PKWARE Data Compression Library ? */
            /* Note: Single unit files compressed with IMPLODE are not supported by Blizzard */
            else if(pFileEntry->dwFlags & MPQ_FILE_IMPLODE)
                nResult = SCompExplode(hf->pbFileSector, &cbOutBuffer, pbRawData, cbInBuffer);

            nError = (nResult != 0) ? ERROR_SUCCESS : ERROR_FILE_CORRUPT;
        }
        else
        {
            if(hf->pbFileSector != NULL && pbRawData != hf->pbFileSector)
                memcpy(hf->pbFileSector, pbRawData, hf->dwDataSize);
        }

        /* Free the decompression buffer. */
        if(pbCompressed != NULL)
            STORM_FREE(pbCompressed);

        /* The file sector is now properly loaded */
        hf->dwSectorOffs = 0;
    }

    /* At this moment, we have the file loaded into the file buffer. */
    /* Copy as much as the caller wants */
    if(nError == ERROR_SUCCESS && hf->dwSectorOffs == 0)
    {
        /* File position is greater or equal to file size ? */
        if(dwFilePos >= hf->dwDataSize)
        {
            *pdwBytesRead = 0;
            return ERROR_SUCCESS;
        }

        /* If not enough bytes remaining in the file, cut them */
        if((hf->dwDataSize - dwFilePos) < dwToRead)
            dwToRead = (hf->dwDataSize - dwFilePos);

        /* Copy the bytes */
        memcpy(pvBuffer, hf->pbFileSector + dwFilePos, dwToRead);

        /* Give the number of bytes read */
        *pdwBytesRead = dwToRead;
        return ERROR_SUCCESS;
    }

    /* An error, sorry */
    return ERROR_CAN_NOT_COMPLETE;
}

static int ReadMpkFileSingleUnit(TMPQFile * hf, void * pvBuffer, uint32_t dwFilePos, uint32_t dwToRead, uint32_t * pdwBytesRead)
{
    uint64_t RawFilePos = hf->RawFilePos + 0x0C;   /* For some reason, MPK files start at position (hf->RawFilePos + 0x0C) */
    TMPQArchive * ha = hf->ha;
    TFileEntry * pFileEntry = hf->pFileEntry;
    unsigned char * pbCompressed = NULL;
    unsigned char * pbRawData = hf->pbFileSector;
    int nError = ERROR_SUCCESS;

    /* We do not support patch files in MPK archives */
    assert(hf->pPatchInfo == NULL);

    /* If the file buffer is not allocated yet, do it. */
    if(hf->pbFileSector == NULL)
    {
        nError = AllocateSectorBuffer(hf);
        if(nError != ERROR_SUCCESS)
            return nError;
        pbRawData = hf->pbFileSector;
    }

    /* If the file sector is not loaded yet, do it */
    if(hf->dwSectorOffs != 0)
    {
        /* Is the file compressed? */
        if(pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK)
        {
            /* Allocate space for compressed data */
            pbCompressed = STORM_ALLOC(uint8_t, pFileEntry->dwCmpSize);
            if(pbCompressed == NULL)
                return ERROR_NOT_ENOUGH_MEMORY;
            pbRawData = pbCompressed;
        }
        
        /* Load the raw (compressed, encrypted) data */
        if(!FileStream_Read(ha->pStream, &RawFilePos, pbRawData, pFileEntry->dwCmpSize))
        {
            STORM_FREE(pbCompressed);
            return GetLastError();
        }

        /* If the file is encrypted, we have to decrypt the data first */
        if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED)
        {
            DecryptMpkTable(pbRawData, pFileEntry->dwCmpSize);
        }

        /* If the file is compressed, we have to decompress it now */
        if(pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK)
        {
            int cbOutBuffer = (int)hf->dwDataSize;

            if(!SCompDecompressMpk(hf->pbFileSector, &cbOutBuffer, pbRawData, (int)pFileEntry->dwCmpSize))
                nError = ERROR_FILE_CORRUPT;
        }
        else
        {
            if(pbRawData != hf->pbFileSector)
                memcpy(hf->pbFileSector, pbRawData, hf->dwDataSize);
        }

        /* Free the decompression buffer. */
        if(pbCompressed != NULL)
            STORM_FREE(pbCompressed);

        /* The file sector is now properly loaded */
        hf->dwSectorOffs = 0;
    }

    /* At this moment, we have the file loaded into the file buffer. */
    /* Copy as much as the caller wants */
    if(nError == ERROR_SUCCESS && hf->dwSectorOffs == 0)
    {
        /* File position is greater or equal to file size ? */
        if(dwFilePos >= hf->dwDataSize)
        {
            *pdwBytesRead = 0;
            return ERROR_SUCCESS;
        }

        /* If not enough bytes remaining in the file, cut them */
        if((hf->dwDataSize - dwFilePos) < dwToRead)
            dwToRead = (hf->dwDataSize - dwFilePos);

        /* Copy the bytes */
        memcpy(pvBuffer, hf->pbFileSector + dwFilePos, dwToRead);

        /* Give the number of bytes read */
        *pdwBytesRead = dwToRead;
        return ERROR_SUCCESS;
    }

    /* An error, sorry */
    return ERROR_CAN_NOT_COMPLETE;
}


static int ReadMpqFileSectorFile(TMPQFile * hf, void * pvBuffer, uint32_t dwFilePos, uint32_t dwBytesToRead, uint32_t * pdwBytesRead)
{
    TMPQArchive * ha = hf->ha;
    unsigned char * pbBuffer = (uint8_t *)pvBuffer;
    uint32_t dwTotalBytesRead = 0;                         /* Total bytes read in all three parts */
    uint32_t dwSectorSizeMask = ha->dwSectorSize - 1;      /* Mask for block size, usually 0x0FFF */
    uint32_t dwFileSectorPos;                              /* File offset of the loaded sector */
    uint32_t dwBytesRead;                                  /* Number of bytes read (temporary variable) */
    int nError;

    /* If the file position is at or beyond end of file, do nothing */
    if(dwFilePos >= hf->dwDataSize)
    {
        *pdwBytesRead = 0;
        return ERROR_SUCCESS;
    }

    /* If not enough bytes in the file remaining, cut them */
    if(dwBytesToRead > (hf->dwDataSize - dwFilePos))
        dwBytesToRead = (hf->dwDataSize - dwFilePos);

    /* Compute sector position in the file */
    dwFileSectorPos = dwFilePos & ~dwSectorSizeMask;  /* Position in the block */

    /* If the file sector buffer is not allocated yet, do it now */
    if(hf->pbFileSector == NULL)
    {
        nError = AllocateSectorBuffer(hf);
        if(nError != ERROR_SUCCESS)
            return nError;
    }

    /* Load the first (incomplete) file sector */
    if(dwFilePos & dwSectorSizeMask)
    {
        uint32_t dwBytesInSector = ha->dwSectorSize;
        uint32_t dwBufferOffs = dwFilePos & dwSectorSizeMask;
        uint32_t dwToCopy;                                     

        /* Is the file sector already loaded ? */
        if(hf->dwSectorOffs != dwFileSectorPos)
        {
            /* Load one MPQ sector into archive buffer */
            nError = ReadMpqSectors(hf, hf->pbFileSector, dwFileSectorPos, ha->dwSectorSize, &dwBytesInSector);
            if(nError != ERROR_SUCCESS)
                return nError;

            /* Remember that the data loaded to the sector have new file offset */
            hf->dwSectorOffs = dwFileSectorPos;
        }
        else
        {
            if((dwFileSectorPos + dwBytesInSector) > hf->dwDataSize)
                dwBytesInSector = hf->dwDataSize - dwFileSectorPos;
        }

        /* Copy the data from the offset in the loaded sector to the end of the sector */
        dwToCopy = dwBytesInSector - dwBufferOffs;
        if(dwToCopy > dwBytesToRead)
            dwToCopy = dwBytesToRead;

        /* Copy data from sector buffer into target buffer */
        memcpy(pbBuffer, hf->pbFileSector + dwBufferOffs, dwToCopy);

        /* Update pointers and byte counts */
        dwTotalBytesRead += dwToCopy;
        dwFileSectorPos  += dwBytesInSector;
        pbBuffer         += dwToCopy;
        dwBytesToRead    -= dwToCopy;
    }

    /* Load the whole ("middle") sectors only if there is at least one full sector to be read */
    if(dwBytesToRead >= ha->dwSectorSize)
    {
        uint32_t dwBlockBytes = dwBytesToRead & ~dwSectorSizeMask;

        /* Load all sectors to the output buffer */
        nError = ReadMpqSectors(hf, pbBuffer, dwFileSectorPos, dwBlockBytes, &dwBytesRead);
        if(nError != ERROR_SUCCESS)
            return nError;

        /* Update pointers */
        dwTotalBytesRead += dwBytesRead;
        dwFileSectorPos  += dwBytesRead;
        pbBuffer         += dwBytesRead;
        dwBytesToRead    -= dwBytesRead;
    }

    /* Read the terminating sector */
    if(dwBytesToRead > 0)
    {
        uint32_t dwToCopy = ha->dwSectorSize;

        /* Is the file sector already loaded ? */
        if(hf->dwSectorOffs != dwFileSectorPos)
        {
            /* Load one MPQ sector into archive buffer */
            nError = ReadMpqSectors(hf, hf->pbFileSector, dwFileSectorPos, ha->dwSectorSize, &dwBytesRead);
            if(nError != ERROR_SUCCESS)
                return nError;

            /* Remember that the data loaded to the sector have new file offset */
            hf->dwSectorOffs = dwFileSectorPos;
        }

        /* Check number of bytes read */
        if(dwToCopy > dwBytesToRead)
            dwToCopy = dwBytesToRead;

        /* Copy the data from the cached last sector to the caller's buffer */
        memcpy(pbBuffer, hf->pbFileSector, dwToCopy);
        
        /* Update pointers */
        dwTotalBytesRead += dwToCopy;
    }

    /* Store total number of bytes read to the caller */
    *pdwBytesRead = dwTotalBytesRead;
    return ERROR_SUCCESS;
}

static int ReadMpqFilePatchFile(TMPQFile * hf, void * pvBuffer, uint32_t dwFilePos, uint32_t dwToRead, uint32_t * pdwBytesRead)
{
    uint32_t dwBytesToRead = dwToRead;
    uint32_t dwBytesRead = 0;
    int nError = ERROR_SUCCESS;

    /* Make sure that the patch file is loaded completely */
    if(nError == ERROR_SUCCESS && hf->pbFileData == NULL)
    {
        /* Load the original file and store its content to "pbOldData" */
        hf->pbFileData = STORM_ALLOC(uint8_t, hf->pFileEntry->dwFileSize);
        hf->cbFileData = hf->pFileEntry->dwFileSize;
        if(hf->pbFileData == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        /* Read the file data */
        if(hf->pFileEntry->dwFlags & MPQ_FILE_SINGLE_UNIT)
            nError = ReadMpqFileSingleUnit(hf, hf->pbFileData, 0, hf->cbFileData, &dwBytesRead);
        else
            nError = ReadMpqFileSectorFile(hf, hf->pbFileData, 0, hf->cbFileData, &dwBytesRead);

        /* Fix error code */
        if(nError == ERROR_SUCCESS && dwBytesRead != hf->cbFileData)
            nError = ERROR_FILE_CORRUPT;

        /* Patch the file data */
        if(nError == ERROR_SUCCESS)
            nError = PatchFileData(hf);

        /* Reset number of bytes read to zero */
        dwBytesRead = 0;
    }

    /* If there is something to read, do it */
    if(nError == ERROR_SUCCESS)
    {
        if(dwFilePos < hf->cbFileData)
        {
            /* Make sure we don't copy more than file size */
            if((dwFilePos + dwToRead) > hf->cbFileData)
                dwToRead = hf->cbFileData - dwFilePos;

            /* Copy the appropriate amount of the file data to the caller's buffer */
            memcpy(pvBuffer, hf->pbFileData + dwFilePos, dwToRead);
            dwBytesRead = dwToRead;
        }

        /* Set the proper error code */
        nError = (dwBytesRead == dwBytesToRead) ? ERROR_SUCCESS : ERROR_HANDLE_EOF;
    }

    /* Give the result to the caller */
    if(pdwBytesRead != NULL)
        *pdwBytesRead = dwBytesRead;
    return nError;
}

static int ReadMpqFileLocalFile(TMPQFile * hf, void * pvBuffer, uint32_t dwFilePos, uint32_t dwToRead, uint32_t * pdwBytesRead)
{
    uint64_t FilePosition1 = dwFilePos;
    uint64_t FilePosition2;
    uint32_t dwBytesRead = 0;
    int nError = ERROR_SUCCESS;

    assert(hf->pStream != NULL);

    /* Because stream I/O functions are designed to read */
    /* "all or nothing", we compare file position before and after, */
    /* and if they differ, we assume that number of bytes read */
    /* is the difference between them */

    if(!FileStream_Read(hf->pStream, &FilePosition1, pvBuffer, dwToRead))
    {
        /* If not all bytes have been read, then return the number of bytes read */
        if((nError = GetLastError()) == ERROR_HANDLE_EOF)
        {
            FileStream_GetPos(hf->pStream, &FilePosition2);
            dwBytesRead = (uint32_t)(FilePosition2 - FilePosition1);
        }
    }
    else
    {
        dwBytesRead = dwToRead;
    }

    *pdwBytesRead = dwBytesRead;
    return nError;
}

/*-----------------------------------------------------------------------------
 * SFileReadFile
 */

int EXPORT_SYMBOL SFileReadFile(void * hFile, void * pvBuffer, size_t dwToRead, size_t * pdwRead)
{
    TMPQFile * hf = (TMPQFile *)hFile;
    uint32_t dwBytesRead = 0;                      /* Number of bytes read */
    int nError = ERROR_SUCCESS;

    /* Always zero the result */
    if(pdwRead != NULL)
        *pdwRead = 0;

    /* Check valid parameters */
    if(!IsValidFileHandle(hFile))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if(pvBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* If we didn't load the patch info yet, do it now */
    if(hf->pFileEntry != NULL && (hf->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) && hf->pPatchInfo == NULL)
    {
        nError = AllocatePatchInfo(hf, 1);
        if(nError != ERROR_SUCCESS)
        {
            SetLastError(nError);
            return 0;
        }
    }

    /* If the file is local file, read the data directly from the stream */
    if(hf->pStream != NULL)
    {
        nError = ReadMpqFileLocalFile(hf, pvBuffer, hf->dwFilePos, dwToRead, &dwBytesRead);
    }

    /* If the file is a patch file, we have to read it special way */
    else if(hf->hfPatch != NULL && (hf->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE) == 0)
    {
        nError = ReadMpqFilePatchFile(hf, pvBuffer, hf->dwFilePos, dwToRead, &dwBytesRead);
    }

    /* If the archive is a MPK archive, we need special way to read the file */
    else if(hf->ha->dwSubType == MPQ_SUBTYPE_MPK)
    {
        nError = ReadMpkFileSingleUnit(hf, pvBuffer, hf->dwFilePos, dwToRead, &dwBytesRead);
    }

    /* If the file is single unit file, redirect it to read file */
    else if(hf->pFileEntry->dwFlags & MPQ_FILE_SINGLE_UNIT)
    {
        nError = ReadMpqFileSingleUnit(hf, pvBuffer, hf->dwFilePos, dwToRead, &dwBytesRead);
    }

    /* Otherwise read it as sector based MPQ file */
    else
    {                                                                   
        nError = ReadMpqFileSectorFile(hf, pvBuffer, hf->dwFilePos, dwToRead, &dwBytesRead);
    }

    /* Increment the file position */
    hf->dwFilePos += dwBytesRead;

    /* Give the caller the number of bytes read */
    if(pdwRead != NULL)
        *pdwRead = dwBytesRead;

    /* If the read operation succeeded, but not full number of bytes was read, */
    /* set the last error to ERROR_HANDLE_EOF */
    if(nError == ERROR_SUCCESS && (dwBytesRead < dwToRead))
        nError = ERROR_HANDLE_EOF;

    /* If something failed, set the last error value */
    if(nError != ERROR_SUCCESS)
        SetLastError(nError);
    return (nError == ERROR_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * SFileGetFileSize
 */

size_t EXPORT_SYMBOL SFileGetFileSize(void * hFile, uint32_t * pdwFileSizeHigh)
{
    uint64_t FileSize;
    TMPQFile * hf = (TMPQFile *)hFile;

    /* Validate the file handle before we go on */
    if(IsValidFileHandle(hFile))
    {
        /* Make sure that the variable is initialized */
        FileSize = 0;

        /* If the file is patched file, we have to get the size of the last version */
        if(hf->hfPatch != NULL)
        {
            /* Walk through the entire patch chain, take the last version */
            while(hf != NULL)
            {
                /* Get the size of the currently pointed version */
                FileSize = hf->pFileEntry->dwFileSize;

                /* Move to the next patch file in the hierarchy */
                hf = hf->hfPatch;
            }
        }
        else
        {
            /* Is it a local file ? */
            if(hf->pStream != NULL)
            {
                FileStream_GetSize(hf->pStream, &FileSize);
            }
            else
            {
                FileSize = hf->dwDataSize;
            }
        }

        /* If opened from archive, return file size */
        if(pdwFileSizeHigh != NULL)
            *pdwFileSizeHigh = (uint32_t)(FileSize >> 32);
        return (size_t)FileSize;
    }

    SetLastError(ERROR_INVALID_HANDLE);
    return SFILE_INVALID_SIZE;
}

size_t EXPORT_SYMBOL SFileSetFilePointer(void * hFile, off_t lFilePos, int * plFilePosHigh, uint32_t dwMoveMethod)
{
    TMPQFile * hf = (TMPQFile *)hFile;
    uint64_t FilePosition;
    uint64_t MoveOffset;
    uint32_t dwFilePosHi;

    /* If the hFile is not a valid file handle, return an error. */
    if(!IsValidFileHandle(hFile))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return SFILE_INVALID_POS;
    }

    /* Get the relative point where to move from */
    switch(dwMoveMethod)
    {
        case SEEK_SET:
            FilePosition = 0;
            break;

        case SEEK_CUR:
            if(hf->pStream != NULL)
            {
                FileStream_GetPos(hf->pStream, &FilePosition);
            }
            else
            {
                FilePosition = hf->dwFilePos;
            }
            break;

        case SEEK_END:
            if(hf->pStream != NULL)
            {
                FileStream_GetSize(hf->pStream, &FilePosition);
            }
            else
            {
                FilePosition = SFileGetFileSize(hFile, NULL);
            }
            break;

        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return SFILE_INVALID_POS;
    }

    /* Now get the move offset. Note that both values form */
    /* a signed 64-bit value (a file pointer can be moved backwards) */
    if(plFilePosHigh != NULL)
        dwFilePosHi = *plFilePosHigh;
    else
        dwFilePosHi = (lFilePos & 0x80000000) ? 0xFFFFFFFF : 0;
    if (sizeof(off_t) == 8)
        MoveOffset = (uint64_t)lFilePos;
    else
        MoveOffset = MAKE_OFFSET64(dwFilePosHi, lFilePos);

    /* Now calculate the new file pointer */
    /* Do not allow the file pointer to overflow */
    FilePosition = ((FilePosition + MoveOffset) >= FilePosition) ? (FilePosition + MoveOffset) : 0;

    /* Now apply the file pointer to the file */
    if(hf->pStream != NULL)
    {
        /* Apply the new file position */
        if(!FileStream_Read(hf->pStream, &FilePosition, NULL, 0))
            return SFILE_INVALID_POS;

        /* Return the new file position */
        if(plFilePosHigh != NULL)
            *plFilePosHigh = (int)(FilePosition >> 32);
        return (size_t)FilePosition;
    }
    else
    {
        /* Files in MPQ can't be bigger than 4 GB. */
        /* We don't allow to go past 4 GB */
        if(FilePosition >> 32)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return SFILE_INVALID_POS;
        }

        /* Change the file position */
        hf->dwFilePos = (uint32_t)FilePosition;

        /* Return the new file position */
        if(plFilePosHigh != NULL)
            *plFilePosHigh = 0;
        return (size_t)FilePosition;
    }
}

