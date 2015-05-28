/*****************************************************************************/
/* SFileAddFile.c                         Copyright (c) Ladislav Zezula 2010 */
/*---------------------------------------------------------------------------*/
/* MPQ Editing functions                                                     */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 27.03.10  1.00  Lad  Splitted from SFileCreateArchiveEx.cpp               */
/* 21.04.13  1.01  Dea  AddFile callback now part of TMPQArchive             */
/* 24.03.15  1.00  Ayr  Ported to plain c                                    */
/*****************************************************************************/

#include "thunderStorm.h"
#include "StormCommon.h"

/*-----------------------------------------------------------------------------
 * Local variables
 */

/* Mask for lossy compressions */
#define MPQ_LOSSY_COMPRESSION_MASK (MPQ_COMPRESSION_ADPCM_MONO | MPQ_COMPRESSION_ADPCM_STEREO | MPQ_COMPRESSION_HUFFMANN)

/* Data compression for SFileAddFile */
/* Kept here for compatibility with code that was created with StormLib version < 6.50 */
static uint32_t DefaultDataCompression = MPQ_COMPRESSION_PKWARE;

/*-----------------------------------------------------------------------------
 * WAVE verification
 */

#define FILE_SIGNATURE_RIFF     0x46464952
#define FILE_SIGNATURE_WAVE     0x45564157
#define FILE_SIGNATURE_FMT      0x20746D66
#define AUDIO_FORMAT_PCM                 1

typedef struct _WAVE_FILE_HEADER
{
    uint32_t dwChunkId;                        /* 0x52494646 ("RIFF") */
    uint32_t dwChunkSize;                      /* Size of that chunk, in bytes */
    uint32_t dwFormat;                         /* Must be 0x57415645 ("WAVE") */
   
    /* Format sub-chunk */
    uint32_t dwSubChunk1Id;                    /* 0x666d7420 ("fmt ") */
    uint32_t dwSubChunk1Size;                  /* 0x16 for PCM */
    uint16_t wAudioFormat;                    /* 1 = PCM. Other value means some sort of compression */
    uint16_t wChannels;                       /* Number of channels */
    uint32_t dwSampleRate;                     /* 8000, 44100, etc. */
    uint32_t dwBytesRate;                      /* SampleRate * NumChannels * BitsPerSample/8 */
    uint16_t wBlockAlign;                     /* NumChannels * BitsPerSample/8 */
    uint16_t wBitsPerSample;                  /* 8 bits = 8, 16 bits = 16, etc. */

    /* Followed by "data" sub-chunk (we don't care) */
} WAVE_FILE_HEADER, *PWAVE_FILE_HEADER;

static int IsWaveFile_16BitsPerAdpcmSample(
    unsigned char * pbFileData,
    uint32_t cbFileData,
    uint32_t * pdwChannels)
{
    PWAVE_FILE_HEADER pWaveHdr = (PWAVE_FILE_HEADER)pbFileData;

    /* The amount of file data must be at least size of WAVE header */
    if(cbFileData > sizeof(WAVE_FILE_HEADER))
    {
        /* Check for the RIFF header */
        if(pWaveHdr->dwChunkId == FILE_SIGNATURE_RIFF && pWaveHdr->dwFormat == FILE_SIGNATURE_WAVE)
        {
            /* Check for ADPCM format */
            if(pWaveHdr->dwSubChunk1Id == FILE_SIGNATURE_FMT && pWaveHdr->wAudioFormat == AUDIO_FORMAT_PCM)
            {
                /* Now the number of bits per sample must be at least 16. */
                /* If not, the WAVE file gets corrupted by the ADPCM compression */
                if(pWaveHdr->wBitsPerSample >= 0x10)
                {
                    *pdwChannels = pWaveHdr->wChannels;
                    return 1;
                }
            }
        }
    }

    return 0;
}

/*-----------------------------------------------------------------------------
 * MPQ write data functions
 */

static int WriteDataToMpqFile(
    TMPQArchive * ha,
    TMPQFile * hf,
    unsigned char * pbFileData,
    uint32_t dwDataSize,
    uint32_t dwCompression)
{
    TFileEntry * pFileEntry = hf->pFileEntry;
    uint64_t ByteOffset;
    unsigned char * pbCompressed = NULL;             /* Compressed (target) data */
    unsigned char * pbToWrite = hf->pbFileSector;    /* Data to write to the file */
    int nCompressionLevel;                  /* ADPCM compression level (only used for wave files) */
    int nError = ERROR_SUCCESS;

    /* Make sure that the caller won't overrun the previously initiated file size */
    assert(hf->dwFilePos + dwDataSize <= pFileEntry->dwFileSize);
    assert(hf->dwSectorCount != 0);
    assert(hf->pbFileSector != NULL);
    if((hf->dwFilePos + dwDataSize) > pFileEntry->dwFileSize)
        return ERROR_DISK_FULL;

    /* Now write all data to the file sector buffer */
    if(nError == ERROR_SUCCESS)
    {
        uint32_t dwBytesInSector = hf->dwFilePos % hf->dwSectorSize;
        uint32_t dwSectorIndex = hf->dwFilePos / hf->dwSectorSize;
        uint32_t dwBytesToCopy;

        /* Process all data. */
        while(dwDataSize != 0)
        {
            dwBytesToCopy = dwDataSize;
                
            /* Check for sector overflow */
            if(dwBytesToCopy > (hf->dwSectorSize - dwBytesInSector))
                dwBytesToCopy = (hf->dwSectorSize - dwBytesInSector);

            /* Copy the data to the file sector */
            memcpy(hf->pbFileSector + dwBytesInSector, pbFileData, dwBytesToCopy);
            dwBytesInSector += dwBytesToCopy;
            pbFileData += dwBytesToCopy;
            dwDataSize -= dwBytesToCopy;

            /* Update the file position */
            hf->dwFilePos += dwBytesToCopy;

            /* If the current sector is full, or if the file is already full, */
            /* then write the data to the MPQ */
            if(dwBytesInSector >= hf->dwSectorSize || hf->dwFilePos >= pFileEntry->dwFileSize)
            {
                /* Set the position in the file */
                ByteOffset = hf->RawFilePos + pFileEntry->dwCmpSize;

                /* Update CRC32 and MD5 of the file */
                md5_process((hash_state *)hf->hctx, hf->pbFileSector, dwBytesInSector);
                hf->dwCrc32 = crc32(hf->dwCrc32, hf->pbFileSector, dwBytesInSector);

                /* Compress the file sector, if needed */
                if(pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK)
                {
                    int nOutBuffer = (int)dwBytesInSector;
                    int nInBuffer = (int)dwBytesInSector;

                    /* If the file is compressed, allocate buffer for the compressed data. */
                    /* Note that we allocate buffer that is a bit longer than sector size, */
                    /* for case if the compression method performs a buffer overrun */
                    if(pbCompressed == NULL)
                    {
                        pbToWrite = pbCompressed = STORM_ALLOC(uint8_t, hf->dwSectorSize + 0x100);
                        if(pbCompressed == NULL)
                        {
                            nError = ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }
                    }

                    /*
                     * Note that both SCompImplode and SCompCompress copy data as-is,
                     * if they are unable to compress the data.
                     */

                    if(pFileEntry->dwFlags & MPQ_FILE_IMPLODE)
                    {
                        SCompImplode(pbCompressed, &nOutBuffer, hf->pbFileSector, nInBuffer);
                    }

                    if(pFileEntry->dwFlags & MPQ_FILE_COMPRESS)
                    {
                        /* If this is the first sector, we need to override the given compression
                         * by the first sector compression. This is because the entire sector must
                         * be compressed by the same compression.
                         *
                         * Test case:                        
                         *
                         * WRITE_FILE(hFile, pvBuffer, 0x10, MPQ_COMPRESSION_PKWARE)       * Write 0x10 bytes (sector 0)
                         * WRITE_FILE(hFile, pvBuffer, 0x10, MPQ_COMPRESSION_ADPCM_MONO)   * Write 0x10 bytes (still sector 0)
                         * WRITE_FILE(hFile, pvBuffer, 0x10, MPQ_COMPRESSION_ADPCM_MONO)   * Write 0x10 bytes (still sector 0)
                         * WRITE_FILE(hFile, pvBuffer, 0x10, MPQ_COMPRESSION_ADPCM_MONO)   * Write 0x10 bytes (still sector 0)
                         */
                        dwCompression = (dwSectorIndex == 0) ? hf->dwCompression0 : dwCompression;

                        /* If the caller wants ADPCM compression, we will set wave compression level to 4, */
                        /* which corresponds to medium quality */
                        nCompressionLevel = (dwCompression & MPQ_LOSSY_COMPRESSION_MASK) ? 4 : -1;
                        SCompCompress(pbCompressed, &nOutBuffer, hf->pbFileSector, nInBuffer, (unsigned)dwCompression, 0, nCompressionLevel);
                    }

                    /* Update sector positions */
                    dwBytesInSector = nOutBuffer;
                    if(hf->SectorOffsets != NULL)
                        hf->SectorOffsets[dwSectorIndex+1] = hf->SectorOffsets[dwSectorIndex] + dwBytesInSector;

                    /* We have to calculate sector CRC, if enabled */
                    if(hf->SectorChksums != NULL)
                        hf->SectorChksums[dwSectorIndex] = adler32(0, pbCompressed, nOutBuffer);
                }

                /* Pad the sector, if necessary */
                if((pFileEntry->dwFlags & (MPQ_FILE_ENCRYPT_ANUBIS | MPQ_FILE_ENCRYPT_SERPENT)) && dwBytesInSector < 16)
                {
                    uint32_t padBytes = 16 - dwBytesInSector;
                    memset(pbToWrite + dwBytesInSector, 0, padBytes);
                    
                    dwBytesInSector += padBytes;
                    if(hf->SectorOffsets != NULL)
                        hf->SectorOffsets[dwSectorIndex+1] = hf->SectorOffsets[dwSectorIndex] + dwBytesInSector;
                }

                /* Encrypt the sector, if necessary */
                if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED)
                {
                    BSWAP_ARRAY32_UNSIGNED(pbToWrite, dwBytesInSector);
                    EncryptMpqBlock(pbToWrite, dwBytesInSector, hf->dwFileKey + dwSectorIndex);
                    BSWAP_ARRAY32_UNSIGNED(pbToWrite, dwBytesInSector);
                    
                    /* Encrypt with anubis, if necessary */
                    if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPT_ANUBIS)
                         EncryptMpqBlockAnubis(pbToWrite, dwBytesInSector, &(ha->keyScheduleAnubis));

                    /* Encrypt with serpent, if necessary */
                    if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPT_SERPENT)
                         EncryptMpqBlockSerpent(pbToWrite, dwBytesInSector, &(ha->keyScheduleSerpent));
                }

                /* Write the file sector */
                if(!FileStream_Write(ha->pStream, &ByteOffset, pbToWrite, dwBytesInSector))
                {
                    nError = GetLastError();
                    break;
                }

                /* Call the compact callback, if any */
                if(ha->pfnAddFileCB != NULL)
                    ha->pfnAddFileCB(ha->pvAddFileUserData, hf->dwFilePos, hf->dwDataSize, 0);

                /* Update the compressed file size */
                pFileEntry->dwCmpSize += dwBytesInSector;
                dwBytesInSector = 0;
                dwSectorIndex++;
            }
        }
    }

    /* Cleanup */
    if(pbCompressed != NULL)
        STORM_FREE(pbCompressed);
    return nError;
}

/*-----------------------------------------------------------------------------
 * Recrypts file data for file renaming
 */

static int RecryptFileData(
    TMPQArchive * ha,
    TMPQFile * hf,
    const char * szFileName,
    const char * szNewFileName)
{
    uint64_t RawFilePos;
    TFileEntry * pFileEntry = hf->pFileEntry;
    uint32_t dwBytesToRecrypt = pFileEntry->dwCmpSize;
    uint32_t dwOldKey;
    uint32_t dwNewKey;
    int nError = ERROR_SUCCESS;

    /* The file must be encrypted */
    assert(pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED);

    /* File decryption key is calculated from the plain name */
    szNewFileName = GetPlainFileName(szNewFileName);
    szFileName = GetPlainFileName(szFileName);

    /* Calculate both file keys */
    dwOldKey = DecryptFileKey(szFileName,    pFileEntry->ByteOffset, pFileEntry->dwFileSize, pFileEntry->dwFlags);
    dwNewKey = DecryptFileKey(szNewFileName, pFileEntry->ByteOffset, pFileEntry->dwFileSize, pFileEntry->dwFlags);

    /* Incase the keys are equal, don't recrypt the file */
    if(dwNewKey == dwOldKey)
        return ERROR_SUCCESS;
    hf->dwFileKey = dwOldKey;

    /* Calculate the raw position of the file in the archive */
    hf->MpqFilePos = pFileEntry->ByteOffset;
    hf->RawFilePos = ha->MpqPos + hf->MpqFilePos;

    /* Allocate buffer for file transfer */
    nError = AllocateSectorBuffer(hf);
    if(nError != ERROR_SUCCESS)
        return nError;

    /* Also allocate buffer for sector offsets */
    /* Note: Don't load sector checksums, we don't need to recrypt them */
    nError = AllocateSectorOffsets(hf, 1);
    if(nError != ERROR_SUCCESS)
        return nError;

    /* If we have sector offsets, recrypt these as well */
    if(hf->SectorOffsets != NULL)
    {
        /* Allocate secondary buffer for sectors copy */
        uint32_t * SectorOffsetsCopy = STORM_ALLOC(uint32_t, hf->SectorOffsets[0] / sizeof(uint32_t));
        uint32_t dwSectorOffsLen = hf->SectorOffsets[0];

        if(SectorOffsetsCopy == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        /* Recrypt the array of sector offsets */
        memcpy(SectorOffsetsCopy, hf->SectorOffsets, dwSectorOffsLen);
        EncryptMpqBlock(SectorOffsetsCopy, dwSectorOffsLen, dwNewKey - 1);
        BSWAP_ARRAY32_UNSIGNED(SectorOffsetsCopy, dwSectorOffsLen);

        /* Write the recrypted array back */
        if(!FileStream_Write(ha->pStream, &hf->RawFilePos, SectorOffsetsCopy, dwSectorOffsLen))
            nError = GetLastError();
        STORM_FREE(SectorOffsetsCopy);
    }

    /* Now we have to recrypt all file sectors. We do it without */
    /* recompression, because recompression is not necessary in this case */
    if(nError == ERROR_SUCCESS)
    {
        uint32_t dwSector;
        
        for(dwSector = 0; dwSector < hf->dwSectorCount; dwSector++)
        {
            uint32_t dwRawDataInSector = hf->dwSectorSize;
            uint32_t dwRawByteOffset = dwSector * hf->dwSectorSize;

            /* Last sector: If there is not enough bytes remaining in the file, cut the raw size */
            if(dwRawDataInSector > dwBytesToRecrypt)
                dwRawDataInSector = dwBytesToRecrypt;

            /* Fix the raw data length if the file is compressed */
            if(hf->SectorOffsets != NULL)
            {
                dwRawDataInSector = hf->SectorOffsets[dwSector+1] - hf->SectorOffsets[dwSector];
                dwRawByteOffset = hf->SectorOffsets[dwSector];
            }

            /* Calculate the raw file offset of the file sector */
            RawFilePos = CalculateRawSectorOffset(hf, dwRawByteOffset);

            /* Read the file sector */
            if(!FileStream_Read(ha->pStream, &RawFilePos, hf->pbFileSector, dwRawDataInSector))
            {
                nError = GetLastError();
                break;
            }

            /* If necessary, re-encrypt the sector */
            /* Note: Recompression is not necessary here. Unlike encryption, */
            /* the compression does not depend on the position of the file in MPQ. */
            if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPT_SERPENT)
                DecryptMpqBlockSerpent(hf->pbFileSector, dwRawDataInSector, &(ha->keyScheduleSerpent));

            if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPT_ANUBIS)
                DecryptMpqBlockAnubis(hf->pbFileSector, dwRawDataInSector, &(ha->keyScheduleAnubis));
            
            BSWAP_ARRAY32_UNSIGNED(hf->pbFileSector, dwRawDataInSector);
            DecryptMpqBlock(hf->pbFileSector, dwRawDataInSector, dwOldKey + dwSector);
            EncryptMpqBlock(hf->pbFileSector, dwRawDataInSector, dwNewKey + dwSector);
            BSWAP_ARRAY32_UNSIGNED(hf->pbFileSector, dwRawDataInSector);
            
            if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPT_ANUBIS)
                EncryptMpqBlockAnubis(hf->pbFileSector, dwRawDataInSector, &(ha->keyScheduleAnubis));

            if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPT_SERPENT)
                EncryptMpqBlockSerpent(hf->pbFileSector, dwRawDataInSector, &(ha->keyScheduleSerpent));

            /* Write the sector back */
            if(!FileStream_Write(ha->pStream, &RawFilePos, hf->pbFileSector, dwRawDataInSector))
            {
                nError = GetLastError();
                break;
            }

            /* Decrement number of bytes remaining */
            dwBytesToRecrypt -= hf->dwSectorSize;
        }
    }

    return nError;
}

/*-----------------------------------------------------------------------------
 * Internal support for MPQ modifications
 */

int SFileAddFile_Init(
    TMPQArchive * ha,
    const char * szFileName,
    uint64_t FileTime,
    uint32_t dwFileSize,
    uint32_t lcLocale,
    uint32_t dwFlags,
    TMPQFile ** phf)
{
    TFileEntry * pFileEntry = NULL;
    uint64_t TempPos;                  /* For various file offset calculations */
    TMPQFile * hf = NULL;               /* File structure for newly added file */
    uint32_t dwHashIndex = HASH_ENTRY_FREE;
    int nError = ERROR_SUCCESS;

    /*
     * Note: This is an internal function so no validity checks are done.
     * It is the caller's responsibility to make sure that no invalid
     * flags get to this point
     */

    /* Sestor CRC is not allowed with single unit files */
    if(dwFlags & MPQ_FILE_SINGLE_UNIT)
        dwFlags &= ~MPQ_FILE_SECTOR_CRC;

    /* Sector CRC is not allowed if the file is not compressed */
    if(!(dwFlags & MPQ_FILE_COMPRESS_MASK))
        dwFlags &= ~MPQ_FILE_SECTOR_CRC;
    
    /* Fix Key is not allowed if the file is not enrypted */
    if(!(dwFlags & MPQ_FILE_ENCRYPTED))
        dwFlags &= ~MPQ_FILE_FIX_KEY;

    /* If the MPQ is of version 3.0 or higher, we ignore file locale. */
    /* This is because HET and BET tables have no known support for it */
    if(ha->pHeader->wFormatVersion >= MPQ_FORMAT_VERSION_3)
        lcLocale = 0;

    /* Allocate the TMPQFile entry for newly added file */
    hf = CreateFileHandle(ha, NULL);
    if(hf == NULL)
        nError = ERROR_NOT_ENOUGH_MEMORY;

    /* Find a free space in the MPQ and verify if it's not over 4 GB on MPQs v1 */
    if(nError == ERROR_SUCCESS)
    {
        /* Find the position where the file will be stored */
        hf->MpqFilePos = FindFreeMpqSpace(ha);
        hf->RawFilePos = ha->MpqPos + hf->MpqFilePos;
        hf->bIsWriteHandle = 1;

        /* When format V1, the size of the archive cannot exceed 4 GB */
        if(ha->pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1)
        {
            TempPos  = hf->MpqFilePos + dwFileSize;
            TempPos += ha->pHeader->dwHashTableSize * sizeof(TMPQHash);
            TempPos += ha->dwFileTableSize * sizeof(TMPQBlock);
            if((TempPos >> 32) != 0)
                nError = ERROR_DISK_FULL;
        }
    }

    /* Allocate file entry in the MPQ */
    if(nError == ERROR_SUCCESS)
    {
        /* Check if the file already exists in the archive */
        pFileEntry = GetFileEntryExact(ha, szFileName, lcLocale, &dwHashIndex);
        if(pFileEntry != NULL)
        {
            /* If the caller didn't set MPQ_FILE_REPLACEEXISTING, fail it */
            if(dwFlags & MPQ_FILE_REPLACEEXISTING)
                InvalidateInternalFiles(ha);
            else
                nError = ERROR_ALREADY_EXISTS;
        }
        else
        {
            /* Free all internal files - (listfile), (attributes), (signature) */
            InvalidateInternalFiles(ha);
            
            /* Attempt to allocate new file entry */
            pFileEntry = AllocateFileEntry(ha, szFileName, lcLocale, &dwHashIndex);
            if(pFileEntry == NULL)
                nError = ERROR_DISK_FULL;   
        }
    }

    /* Fill the file entry and TMPQFile structure */
    if(nError == ERROR_SUCCESS)
    {
        /* At this point, the file name in the file entry must be set */
        assert(pFileEntry->szFileName != NULL);
        assert(strcasecmp(pFileEntry->szFileName, szFileName) == 0);

        /* Initialize the hash entry for the file */
        hf->pFileEntry = pFileEntry;
        hf->dwDataSize = dwFileSize;

        /* Set the hash table entry */
        if(ha->pHashTable != NULL && dwHashIndex < ha->pHeader->dwHashTableSize)
        {
            hf->pHashEntry = ha->pHashTable + dwHashIndex;
            hf->pHashEntry->lcLocale = (uint16_t)lcLocale;
        }

        /* Decrypt the file key */
        if(dwFlags & MPQ_FILE_ENCRYPTED)
            hf->dwFileKey = DecryptFileKey(szFileName, hf->MpqFilePos, dwFileSize, dwFlags);

        /* Initialize the block table entry for the file */
        pFileEntry->ByteOffset = hf->MpqFilePos;
        pFileEntry->dwFileSize = dwFileSize;
        pFileEntry->dwCmpSize = 0;
        pFileEntry->dwFlags  = dwFlags | MPQ_FILE_EXISTS;

        /* Initialize the file time, CRC32 and MD5 */
        assert(sizeof(hf->hctx) >= sizeof(hash_state));
        memset(pFileEntry->md5, 0, MD5_DIGEST_SIZE);
        md5_init((hash_state *)hf->hctx);
        pFileEntry->dwCrc32 = crc32(0, Z_NULL, 0);

        /* If the caller gave us a file time, use it. */
        pFileEntry->FileTime = FileTime;

        /* Mark the archive as modified */
        ha->dwFlags |= MPQ_FLAG_CHANGED;

        /* Call the callback, if needed */
        if(ha->pfnAddFileCB != NULL)
            ha->pfnAddFileCB(ha->pvAddFileUserData, 0, hf->dwDataSize, 0);
        hf->nAddFileError = ERROR_SUCCESS;               
    }

    /* Fre the file handle if failed */
    if(nError != ERROR_SUCCESS && hf != NULL)
        FreeFileHandle(&hf);

    /* Give the handle to the caller */
    *phf = hf;
    return nError;
}

int SFileAddFile_Write(TMPQFile * hf, const void * pvData, uint32_t dwSize, uint32_t dwCompression)
{
    TMPQArchive * ha;
    TFileEntry * pFileEntry;
    int nError = ERROR_SUCCESS;

    /* Don't bother if the caller gave us zero size */
    if(pvData == NULL || dwSize == 0)
        return ERROR_SUCCESS;

    /* Get pointer to the MPQ archive */
    pFileEntry = hf->pFileEntry;
    ha = hf->ha;

    /* Allocate file buffers */
    if(hf->pbFileSector == NULL)
    {
        uint64_t RawFilePos = hf->RawFilePos;

        /* Allocate buffer for file sector */
        hf->nAddFileError = nError = AllocateSectorBuffer(hf);
        if(nError != ERROR_SUCCESS)
            return nError;

        /* Allocate patch info, if the data is patch */
        if(hf->pPatchInfo == NULL && IsIncrementalPatchFile(pvData, dwSize, &hf->dwPatchedFileSize))
        {
            /* Set the MPQ_FILE_PATCH_FILE flag */
            hf->pFileEntry->dwFlags |= MPQ_FILE_PATCH_FILE;

            /* Allocate the patch info */
            hf->nAddFileError = nError = AllocatePatchInfo(hf, 0);
            if(nError != ERROR_SUCCESS)
                return nError;
        }

        /* Allocate sector offsets */
        if(hf->SectorOffsets == NULL)
        {
            hf->nAddFileError = nError = AllocateSectorOffsets(hf, 0);
            if(nError != ERROR_SUCCESS)
                return nError;
        }

        /* Create array of sector checksums */
        if(hf->SectorChksums == NULL && (pFileEntry->dwFlags & MPQ_FILE_SECTOR_CRC))
        {
            hf->nAddFileError = nError = AllocateSectorChecksums(hf, 0);
            if(nError != ERROR_SUCCESS)
                return nError;
        }

        /* Pre-save the patch info, if any */
        if(hf->pPatchInfo != NULL)
        {
            if(!FileStream_Write(ha->pStream, &RawFilePos, hf->pPatchInfo, hf->pPatchInfo->dwLength))
                nError = GetLastError();
  
            pFileEntry->dwCmpSize += hf->pPatchInfo->dwLength;
            RawFilePos += hf->pPatchInfo->dwLength;
        }

        /* Pre-save the sector offset table, just to reserve space in the file. */
        /* Note that we dont need to swap the sector positions, nor encrypt the table */
        /* at the moment, as it will be written again after writing all file sectors. */
        if(hf->SectorOffsets != NULL)
        {
            if(!FileStream_Write(ha->pStream, &RawFilePos, hf->SectorOffsets, hf->SectorOffsets[0]))
                nError = GetLastError();

            pFileEntry->dwCmpSize += hf->SectorOffsets[0];
            RawFilePos += hf->SectorOffsets[0];
        }
    }

    /* Write the MPQ data to the file */
    if(nError == ERROR_SUCCESS)
    {
        /* Save the first sector compression to the file structure */
        /* Note that the entire first file sector will be compressed */
        /* by compression that was passed to the first call of SFileAddFile_Write */
        if(hf->dwFilePos == 0)
            hf->dwCompression0 = dwCompression;

        /* Write the data to the MPQ */
        nError = WriteDataToMpqFile(ha, hf, (unsigned char *)pvData, dwSize, dwCompression);
    }

    /* If it succeeded and we wrote all the file data, */
    /* we need to re-save sector offset table */
    if(nError == ERROR_SUCCESS)
    {
        if(hf->dwFilePos >= pFileEntry->dwFileSize)
        {
            /* Finish calculating CRC32 */
            hf->pFileEntry->dwCrc32 = hf->dwCrc32;

            /* Finish calculating MD5 */
            md5_done((hash_state *)hf->hctx, hf->pFileEntry->md5);

            /* If we also have sector checksums, write them to the file */
            if(hf->SectorChksums != NULL)
            {
                nError = WriteSectorChecksums(hf);
            }

            /* Now write patch info */
            if(hf->pPatchInfo != NULL)
            {
                memcpy(hf->pPatchInfo->md5, hf->pFileEntry->md5, MD5_DIGEST_SIZE);
                hf->pPatchInfo->dwDataSize  = hf->pFileEntry->dwFileSize;
                hf->pFileEntry->dwFileSize = hf->dwPatchedFileSize;
                nError = WritePatchInfo(hf);
            }

            /* Now write sector offsets to the file */
            if(hf->SectorOffsets != NULL)
            {
                nError = WriteSectorOffsets(hf);
            }

            /* Write the MD5 hashes of each file chunk, if required */
            if(ha->pHeader->dwRawChunkSize != 0)
            {
                nError = WriteMpqDataMD5(ha->pStream,
                                         ha->MpqPos + hf->pFileEntry->ByteOffset,
                                         hf->pFileEntry->dwCmpSize,
                                         ha->pHeader->dwRawChunkSize);
            }
        }
    }

    /* Store the error code from the Write File operation */
    hf->nAddFileError = nError;
    return nError;
}

int SFileAddFile_Finish(TMPQFile * hf)
{
    TMPQArchive * ha = hf->ha;
    TFileEntry * pFileEntry = hf->pFileEntry;
    int nError = hf->nAddFileError;

    /* If all previous operations succeeded, we can update the MPQ */
    if(nError == ERROR_SUCCESS)
    {
        /* Verify if the caller wrote the file properly */
        if(hf->pPatchInfo == NULL)
        {
            assert(pFileEntry != NULL);
            if(hf->dwFilePos != pFileEntry->dwFileSize)
                nError = ERROR_CAN_NOT_COMPLETE;
        }
        else
        {
            if(hf->dwFilePos != hf->pPatchInfo->dwDataSize)
                nError = ERROR_CAN_NOT_COMPLETE;
        }
    }

    /* Now we need to recreate the HET table, if exists */
    if(nError == ERROR_SUCCESS && ha->pHetTable != NULL)
    {
        nError = RebuildHetTable(ha);
    }

    /* Update the block table size */
    if(nError == ERROR_SUCCESS)
    {
        /* Call the user callback, if any */
        if(ha->pfnAddFileCB != NULL)
            ha->pfnAddFileCB(ha->pvAddFileUserData, hf->dwDataSize, hf->dwDataSize, 1);
    }
    else
    {
        /* Free the file entry in MPQ tables */
        if(pFileEntry != NULL)
            DeleteFileEntry(ha, hf);
    }

    /* Clear the add file callback */
    FreeFileHandle(&hf);
    return nError;
}

/*-----------------------------------------------------------------------------
 * Adds data as file to the archive 
 */

int EXPORT_SYMBOL SFileCreateFile(
    void * hMpq,
    const char * szArchivedName,
    uint64_t FileTime,
    size_t dwFileSize,
    uint32_t lcLocale,
    uint32_t dwFlags,
    void ** phFile)
{
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    int nError = ERROR_SUCCESS;
    uint32_t uiFileSize = dwFileSize;
    
    if(uiFileSize > dwFileSize)
    {
        nError = EFBIG;
    }

    /* Check valid parameters */
    if(!IsValidMpqHandle(hMpq))
        nError = ERROR_INVALID_HANDLE;
    if(szArchivedName == NULL || *szArchivedName == 0)
        nError = ERROR_INVALID_PARAMETER;
    if(phFile == NULL)
        nError = ERROR_INVALID_PARAMETER;
    
    /* Don't allow to add file if the MPQ is open for read only */
    if(nError == ERROR_SUCCESS)
    {
        if(ha->dwFlags & MPQ_FLAG_READ_ONLY)
            nError = ERROR_ACCESS_DENIED;

        /* Don't allow to add a file under pseudo-file name */
        if(IsPseudoFileName(szArchivedName, NULL))
            nError = ERROR_INVALID_PARAMETER;

        /* Don't allow to add any of the internal files */
        if(IsInternalMpqFileName(szArchivedName))
            nError = ERROR_INTERNAL_FILE;
    }

    /* Perform validity check of the MPQ flags */
    if(nError == ERROR_SUCCESS)
    {
        /* Mask all unsupported flags out */
        dwFlags &= MPQ_FILE_VALID_FLAGS;

        /* Check for valid flag combinations */
        if((dwFlags & (MPQ_FILE_IMPLODE | MPQ_FILE_COMPRESS)) == (MPQ_FILE_IMPLODE | MPQ_FILE_COMPRESS))
            nError = ERROR_INVALID_PARAMETER;
    }

    /* Initiate the add file operation */
    if(nError == ERROR_SUCCESS)
        nError = SFileAddFile_Init(ha, szArchivedName, FileTime, uiFileSize, lcLocale, dwFlags, (TMPQFile **)phFile);

    /* Deal with the errors */
    if(nError != ERROR_SUCCESS)
        SetLastError(nError);
    return (nError == ERROR_SUCCESS);
}

int EXPORT_SYMBOL SFileWriteFile(
    void * hFile,
    const void * pvData,
    size_t dwSize,
    uint32_t dwCompression)
{
    TMPQFile * hf = (TMPQFile *)hFile;
    int nError = ERROR_SUCCESS;

    /* Check the proper parameters */
    if(!IsValidFileHandle(hFile))
        nError = ERROR_INVALID_HANDLE;
    if(hf->bIsWriteHandle == 0)
        nError = ERROR_INVALID_HANDLE;

    /* Special checks for single unit files */
    if(nError == ERROR_SUCCESS && (hf->pFileEntry->dwFlags & MPQ_FILE_SINGLE_UNIT))
    {
        /*
         * Note: Blizzard doesn't support single unit files
         * that are stored as encrypted or imploded. We will allow them here,
         * the calling application must ensure that such flag combination doesn't get here
         */

/*      if(dwFlags & MPQ_FILE_IMPLODE) */
/*          nError = ERROR_INVALID_PARAMETER; */

/*      if(dwFlags & MPQ_FILE_ENCRYPTED) */
/*          nError = ERROR_INVALID_PARAMETER; */
        
        /* Lossy compression is not allowed on single unit files */
        if(dwCompression & MPQ_LOSSY_COMPRESSION_MASK)
            nError = ERROR_INVALID_PARAMETER;
    }


    /* Write the data to the file */
    if(nError == ERROR_SUCCESS)
        nError = SFileAddFile_Write(hf, pvData, dwSize, dwCompression);
    
    /* Deal with errors */
    if(nError != ERROR_SUCCESS)
        SetLastError(nError);
    return (nError == ERROR_SUCCESS);
}

int EXPORT_SYMBOL SFileFinishFile(void * hFile)
{
    TMPQFile * hf = (TMPQFile *)hFile;
    int nError = ERROR_SUCCESS;

    /* Check the proper parameters */
    if(!IsValidFileHandle(hFile))
        nError = ERROR_INVALID_HANDLE;
    if(hf->bIsWriteHandle == 0)
        nError = ERROR_INVALID_HANDLE;

    /* Finish the file */
    if(nError == ERROR_SUCCESS)
        nError = SFileAddFile_Finish(hf);
    
    /* Deal with errors */
    if(nError != ERROR_SUCCESS)
        SetLastError(nError);
    return (nError == ERROR_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * Adds a file to the archive 
 */

int EXPORT_SYMBOL SFileAddFileEx(
    void * hMpq,
    const char * szFileName,
    const char * szArchivedName,
    uint32_t dwFlags,
    uint32_t dwCompression,            /* Compression of the first sector */
    uint32_t dwCompressionNext)        /* Compression of next sectors */
{
    uint64_t FileSize = 0;
    uint64_t FileTime = 0;
    TFileStream * pStream = NULL;
    void * hMpqFile = NULL;
    unsigned char * pbFileData = NULL;
    uint32_t dwBytesRemaining = 0;
    uint32_t dwBytesToRead;
    uint32_t dwSectorSize = 0x1000;
    uint32_t dwChannels = 0;
    int bIsAdpcmCompression = 0;
    int bIsFirstSector = 1;
    int nError = ERROR_SUCCESS;

    /* Check parameters */
    if(hMpq == NULL || szFileName == NULL || *szFileName == 0)
        nError = ERROR_INVALID_PARAMETER;

    /* Open added file */
    if(nError == ERROR_SUCCESS)
    {
        pStream = FileStream_OpenFile(szFileName, STREAM_FLAG_READ_ONLY | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE);
        if(pStream == NULL)
            nError = GetLastError();
    }

    /* Get the file size and file time */
    if(nError == ERROR_SUCCESS)
    {
        FileStream_GetTime(pStream, &FileTime);
        FileStream_GetSize(pStream, &FileSize);
        
        /* Files bigger than 4GB cannot be added to MPQ */
        if(FileSize >> 32)
            nError = ERROR_DISK_FULL;
    }

    /* Allocate data buffer for reading from the source file */
    if(nError == ERROR_SUCCESS)
    {
        dwBytesRemaining = (uint32_t)FileSize;
        pbFileData = STORM_ALLOC(uint8_t, dwSectorSize);
        if(pbFileData == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Deal with various combination of compressions */
    if(nError == ERROR_SUCCESS)
    {
        /* When the compression for next blocks is set to default, */
        /* we will copy the compression for the first sector */
        if(dwCompressionNext == MPQ_COMPRESSION_NEXT_SAME)
            dwCompressionNext = dwCompression;
        
        /* If the caller wants ADPCM compression, we make sure */
        /* that the first sector is not compressed with lossy compression */
        if(dwCompressionNext & (MPQ_COMPRESSION_ADPCM_MONO | MPQ_COMPRESSION_ADPCM_STEREO))
        {
            /* The compression of the first file sector must not be ADPCM */
            /* in order not to corrupt the headers */
            if(dwCompression & (MPQ_COMPRESSION_ADPCM_MONO | MPQ_COMPRESSION_ADPCM_STEREO))
                dwCompression = MPQ_COMPRESSION_PKWARE;
            
            /* Remove both flag mono and stereo flags. */
            /* They will be re-added according to WAVE type */
            dwCompressionNext &= ~(MPQ_COMPRESSION_ADPCM_MONO | MPQ_COMPRESSION_ADPCM_STEREO);
            bIsAdpcmCompression = 1;
        }

        /* Initiate adding file to the MPQ */
        if(!SFileCreateFile(hMpq, szArchivedName, FileTime, (uint32_t)FileSize, lcFileLocale, dwFlags, &hMpqFile))
            nError = GetLastError();
    }

    /* Write the file data to the MPQ */
    while(nError == ERROR_SUCCESS && dwBytesRemaining != 0)
    {
        /* Get the number of bytes remaining in the source file */
        dwBytesToRead = dwBytesRemaining;
        if(dwBytesToRead > dwSectorSize)
            dwBytesToRead = dwSectorSize;

        /* Read data from the local file */
        if(!FileStream_Read(pStream, NULL, pbFileData, dwBytesToRead))
        {
            nError = GetLastError();
            break;
        }

        /* If the file being added is a WAVE file, we check number of channels */
        if(bIsFirstSector && bIsAdpcmCompression)
        {
            /* The file must really be a WAVE file with at least 16 bits per sample, */
            /* otherwise the ADPCM compression will corrupt it */
            if(IsWaveFile_16BitsPerAdpcmSample(pbFileData, dwBytesToRead, &dwChannels))
            {
                /* Setup the compression of next sectors according to number of channels */
                dwCompressionNext |= (dwChannels == 1) ? MPQ_COMPRESSION_ADPCM_MONO : MPQ_COMPRESSION_ADPCM_STEREO;
            }
            else
            {
                /* Setup the compression of next sectors to a lossless compression */
                dwCompressionNext = (dwCompression & MPQ_LOSSY_COMPRESSION_MASK) ? MPQ_COMPRESSION_PKWARE : dwCompression;
            }

            bIsFirstSector = 0;
        }

        /* Add the file sectors to the MPQ */
        if(!SFileWriteFile(hMpqFile, pbFileData, dwBytesToRead, dwCompression))
        {
            nError = GetLastError();
            break;
        }

        /* Set the next data compression */
        dwBytesRemaining -= dwBytesToRead;
        dwCompression = dwCompressionNext;
    }

    /* Finish the file writing */
    if(hMpqFile != NULL)
    {
        if(!SFileFinishFile(hMpqFile))
            nError = GetLastError();
    }

    /* Cleanup and exit */
    if(pbFileData != NULL)
        STORM_FREE(pbFileData);
    if(pStream != NULL)
        FileStream_Close(pStream);
    if(nError != ERROR_SUCCESS)
        SetLastError(nError);
    return (nError == ERROR_SUCCESS);
}
                                                                                                                                 
/* Adds a data file into the archive */
int EXPORT_SYMBOL SFileAddFile(void * hMpq, const char * szFileName, const char * szArchivedName, uint32_t dwFlags)
{
    return SFileAddFileEx(hMpq,
                          szFileName,
                          szArchivedName,
                          dwFlags,
                          DefaultDataCompression,
                          DefaultDataCompression);
}

/* Adds a WAVE file into the archive */
int EXPORT_SYMBOL SFileAddWave(void * hMpq, const char * szFileName, const char * szArchivedName, uint32_t dwFlags, uint32_t dwQuality)
{
    uint32_t dwCompression = 0;

    /*
     * Note to wave compression level:
     * The following conversion table applied:
     * High quality:   WaveCompressionLevel = -1
     * Medium quality: WaveCompressionLevel = 4
     * Low quality:    WaveCompressionLevel = 2
     *
     * Starcraft files are packed as Mono (0x41) on medium quality.
     * Because this compression is not used anymore, our compression functions
     * will default to WaveCompressionLevel = 4 when using ADPCM compression
     */ 

    /* Convert quality to data compression */
    switch(dwQuality)
    {
        case MPQ_WAVE_QUALITY_HIGH:
/*          WaveCompressionLevel = -1; */
            dwCompression = MPQ_COMPRESSION_PKWARE;
            break;

        case MPQ_WAVE_QUALITY_MEDIUM:
/*          WaveCompressionLevel = 4; */
            dwCompression = MPQ_COMPRESSION_ADPCM_STEREO | MPQ_COMPRESSION_HUFFMANN;
            break;

        case MPQ_WAVE_QUALITY_LOW:
/*          WaveCompressionLevel = 2; */
            dwCompression = MPQ_COMPRESSION_ADPCM_STEREO | MPQ_COMPRESSION_HUFFMANN;
            break;
    }

    return SFileAddFileEx(hMpq,
                          szFileName,
                          szArchivedName,
                          dwFlags,
                          MPQ_COMPRESSION_PKWARE,   /* First sector should be compressed as data */
                          dwCompression);           /* Next sectors should be compressed as WAVE */
}

/*-----------------------------------------------------------------------------
 * int SFileRemoveFile(void * hMpq, char * szFileName)
 *
 * This function removes a file from the archive.
 */

int EXPORT_SYMBOL SFileRemoveFile(void * hMpq, const char * szFileName, uint32_t dwSearchScope)
{
    TMPQArchive * ha = IsValidMpqHandle(hMpq);
    TMPQFile * hf = NULL;
    int nError = ERROR_SUCCESS;

    /* Keep compiler happy */
    dwSearchScope = dwSearchScope;

    /* Check the parameters */
    if(ha == NULL)
        nError = ERROR_INVALID_HANDLE;
    if(szFileName == NULL || *szFileName == 0)
        nError = ERROR_INVALID_PARAMETER;
    if(IsInternalMpqFileName(szFileName))
        nError = ERROR_INTERNAL_FILE;

    /* Do not allow to remove files from read-only or patched MPQs */
    if(nError == ERROR_SUCCESS)
    {
        if((ha->dwFlags & MPQ_FLAG_READ_ONLY) || (ha->haPatch != NULL))
            nError = ERROR_ACCESS_DENIED;
    }

    /* If all checks have passed, we can delete the file from the MPQ */
    if(nError == ERROR_SUCCESS)
    {
        /* Open the file from the MPQ */
        if(SFileOpenFileEx(hMpq, szFileName, SFILE_OPEN_BASE_FILE, (void **)&hf))
        {
            /* Delete the file entry */
            nError = DeleteFileEntry(ha, hf);
            FreeFileHandle(&hf);
        }
        else
            nError = GetLastError();
    }

    /* If the file has been deleted, we need to invalidate */
    /* the internal files and recreate HET table */
    if(nError == ERROR_SUCCESS)
    {
        /* Invalidate the entries for internal files */
        /* After we are done with MPQ changes, we need to re-create them anyway */
        InvalidateInternalFiles(ha);

        /*
         * Don't rebuild HET table now; the file's flags indicate
         * that it's been deleted, which is enough
         */
    }

    /* Resolve error and exit */
    if(nError != ERROR_SUCCESS)
        SetLastError(nError);
    return (nError == ERROR_SUCCESS);
}

/* Renames the file within the archive. */
int EXPORT_SYMBOL SFileRenameFile(void * hMpq, const char * szFileName, const char * szNewFileName)
{
    TMPQArchive * ha = IsValidMpqHandle(hMpq);
    TMPQFile * hf;
    int nError = ERROR_SUCCESS;

    /* Test the valid parameters */
    if(ha == NULL)
        nError = ERROR_INVALID_HANDLE;
    if(szFileName == NULL || *szFileName == 0 || szNewFileName == NULL || *szNewFileName == 0)
        nError = ERROR_INVALID_PARAMETER;
    if(IsInternalMpqFileName(szFileName) || IsInternalMpqFileName(szNewFileName))
        nError = ERROR_INTERNAL_FILE;

    /* Do not allow to rename files in MPQ open for read only */
    if(nError == ERROR_SUCCESS)
    {
        if(ha->dwFlags & MPQ_FLAG_READ_ONLY)
            nError = ERROR_ACCESS_DENIED;
    }

    /* Open the new file. If exists, we don't allow rename operation */
    if(nError == ERROR_SUCCESS)
    {
        if(GetFileEntryLocale(ha, szNewFileName, lcFileLocale) != NULL)
            nError = ERROR_ALREADY_EXISTS;
    }

    /* Open the file from the MPQ */
    if(nError == ERROR_SUCCESS)
    {
        /* Attempt to open the file */
        if(SFileOpenFileEx(hMpq, szFileName, SFILE_OPEN_BASE_FILE, (void **)&hf))
        {
            uint64_t RawDataOffs;
            TFileEntry * pFileEntry = hf->pFileEntry;

            /* Invalidate the entries for internal files */
            InvalidateInternalFiles(ha);

            /* Rename the file entry in the table */
            nError = RenameFileEntry(ha, hf, szNewFileName);

            /* If the file is encrypted, we have to re-crypt the file content */
            /* with the new decryption key */
            if((nError == ERROR_SUCCESS) && (pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED))
            {
                /* Recrypt the file data in the MPQ */
                nError = RecryptFileData(ha, hf, szFileName, szNewFileName);
                
                /* Update the MD5 of the raw block */
                if(nError == ERROR_SUCCESS && ha->pHeader->dwRawChunkSize != 0)
                {
                    RawDataOffs = ha->MpqPos + pFileEntry->ByteOffset;
                    WriteMpqDataMD5(ha->pStream,
                                    RawDataOffs,
                                    pFileEntry->dwCmpSize,
                                    ha->pHeader->dwRawChunkSize);
                }
            }

            /* Free the file handle */
            FreeFileHandle(&hf);
        }
        else
        {
            nError = GetLastError();
        }
    }

    /* We also need to rebuild the HET table, if present */
    if(nError == ERROR_SUCCESS && ha->pHetTable != NULL)
        nError = RebuildHetTable(ha);

    /* Resolve error and return */
    if(nError != ERROR_SUCCESS)
        SetLastError(nError);
    return (nError == ERROR_SUCCESS);
}

/*-----------------------------------------------------------------------------
 * Sets default data compression for SFileAddFile
 */

int EXPORT_SYMBOL SFileSetDataCompression(uint32_t DataCompression)
{
    unsigned int uValidMask = (MPQ_COMPRESSION_ZLIB | MPQ_COMPRESSION_PKWARE | MPQ_COMPRESSION_BZIP2 | MPQ_COMPRESSION_SPARSE);

    if((DataCompression & uValidMask) != DataCompression)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    DefaultDataCompression = DataCompression;
    return 1;
}

/*-----------------------------------------------------------------------------
 * Changes locale ID of a file
 */

int EXPORT_SYMBOL SFileSetFileLocale(void * hFile, uint32_t lcNewLocale)
{
    TMPQArchive * ha;
    TFileEntry * pFileEntry;
    TMPQFile * hf = (TMPQFile *)IsValidMpqHandle(hFile);

    /* Invalid handle => do nothing */
    if(hf == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    /* Do not allow to rename files in MPQ open for read only */
    ha = hf->ha;
    if(ha->dwFlags & MPQ_FLAG_READ_ONLY)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return 0;
    }

    /* Do not allow unnamed access */
    if(hf->pFileEntry->szFileName == NULL)
    {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        return 0;
    }

    /* Do not allow to change locale of any internal file */
    if(IsInternalMpqFileName(hf->pFileEntry->szFileName))
    {
        SetLastError(ERROR_INTERNAL_FILE);
        return 0;
    }

    /* Do not allow changing file locales if there is no hash table */
    if(hf->pHashEntry == NULL)
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        return 0;
    }

    /* We have to check if the file+locale is not already there */
    pFileEntry = GetFileEntryExact(ha, hf->pFileEntry->szFileName, lcNewLocale, NULL);
    if(pFileEntry != NULL)
    {
        SetLastError(ERROR_ALREADY_EXISTS);
        return 0;
    }

    /* Update the locale in the hash table entry */
    hf->pHashEntry->lcLocale = (uint16_t)lcNewLocale;
    ha->dwFlags |= MPQ_FLAG_CHANGED;
    return 1;
}

/*-----------------------------------------------------------------------------
 * Sets add file callback
 */

int EXPORT_SYMBOL SFileSetAddFileCallback(void * hMpq, SFILE_ADDFILE_CALLBACK AddFileCB, void * pvUserData)
{
    TMPQArchive * ha = (TMPQArchive *) hMpq;

    if(!IsValidMpqHandle(hMpq))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    ha->pvAddFileUserData = pvUserData;
    ha->pfnAddFileCB = AddFileCB;
    return 1;
}
