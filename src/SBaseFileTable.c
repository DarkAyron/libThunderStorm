/*****************************************************************************/
/* SBaseFileTable.c                       Copyright (c) Ladislav Zezula 2010 */
/*---------------------------------------------------------------------------*/
/* Description: Common handler for classic and new hash&block tables         */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 06.09.10  1.00  Lad  The first version of SBaseFileTable.cpp              */
/*****************************************************************************/

#include "thunderStorm.h"
#include "StormCommon.h"

/*-----------------------------------------------------------------------------
 * Local defines
 */

#define INVALID_FLAG_VALUE 0xCCCCCCCC
#define MAX_FLAG_INDEX     512

/*-----------------------------------------------------------------------------
 * Support for calculating bit sizes
 */

static void InitFileFlagArray(uint32_t * FlagArray)
{
    memset(FlagArray, 0xCC, MAX_FLAG_INDEX * sizeof(uint32_t));
}

static uint32_t GetFileFlagIndex(uint32_t * FlagArray, uint32_t dwFlags)
{
    uint32_t dwFlagIndex;
    
    /* Find free or equal entry in the flag array */
    for(dwFlagIndex = 0; dwFlagIndex < MAX_FLAG_INDEX; dwFlagIndex++)
    {
        if(FlagArray[dwFlagIndex] == INVALID_FLAG_VALUE || FlagArray[dwFlagIndex] == dwFlags)
        {
            FlagArray[dwFlagIndex] = dwFlags;
            return dwFlagIndex;
        }
    }

    /* This should never happen */
    assert(0);
    return 0xFFFFFFFF;
}

static uint32_t GetNecessaryBitCount(uint64_t MaxValue)
{
    uint32_t dwBitCount = 0;

    while(MaxValue > 0)
    {
        MaxValue >>= 1;
        dwBitCount++;
    }
    
    return dwBitCount;
}

/*-----------------------------------------------------------------------------
 * Support functions for BIT_ARRAY
 */

static uint16_t SetBitsMask[] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

static TBitArray * CreateBitArray(
    uint32_t NumberOfBits,
    uint8_t FillValue)
{
    TBitArray * pBitArray;
    size_t nSize = sizeof(TBitArray) + (NumberOfBits + 7) / 8;

    /* Allocate the bit array */
    pBitArray = (TBitArray *)STORM_ALLOC(uint8_t, nSize);
    if(pBitArray != NULL)
    {
        memset(pBitArray, FillValue, nSize);
        pBitArray->NumberOfBytes = (NumberOfBits + 7) / 8;
        pBitArray->NumberOfBits = NumberOfBits;
    }

    return pBitArray;
}

void GetBits(
    TBitArray * pArray,
    unsigned int nBitPosition,
    unsigned int nBitLength,
    void * pvBuffer,
    int nResultByteSize)
{
    unsigned char * pbBuffer = (unsigned char *)pvBuffer;
    unsigned int nBytePosition0 = (nBitPosition / 8);
    unsigned int nBytePosition1 = nBytePosition0 + 1;
    unsigned int nByteLength = (nBitLength / 8);
    unsigned int nBitOffset = (nBitPosition & 0x07);
    unsigned char BitBuffer;

    /* Keep compiler happy for platforms where nResultByteSize is not used */
    nResultByteSize = nResultByteSize;

#ifdef _DEBUG
    /* Check if the target is properly zeroed */
    for(int i = 0; i < nResultByteSize; i++)
        assert(pbBuffer[i] == 0);
#endif

#ifndef PLATFORM_LITTLE_ENDIAN
    /* Adjust the buffer pointer for big endian platforms */
    pbBuffer += (nResultByteSize - 1);
#endif    

    /* Copy whole bytes, if any */
    while(nByteLength > 0)
    {
        /* Is the current position in the Elements byte-aligned? */
        if(nBitOffset != 0)
        {
            BitBuffer = (unsigned char)((pArray->Elements[nBytePosition0] >> nBitOffset) | (pArray->Elements[nBytePosition1] << (0x08 - nBitOffset)));
        }
        else
        {
            BitBuffer = pArray->Elements[nBytePosition0];
        }

#ifdef PLATFORM_LITTLE_ENDIAN
        *pbBuffer++ = BitBuffer;
#else
        *pbBuffer-- = BitBuffer;
#endif

        /* Move byte positions and lengths */
        nBytePosition1++;
        nBytePosition0++;
        nByteLength--;
    }

    /* Get the rest of the bits */
    nBitLength = (nBitLength & 0x07);
    if(nBitLength != 0)
    {
        *pbBuffer = (unsigned char)(pArray->Elements[nBytePosition0] >> nBitOffset);

        if(nBitLength > (8 - nBitOffset))
            *pbBuffer = (unsigned char)((pArray->Elements[nBytePosition1] << (8 - nBitOffset)) | (pArray->Elements[nBytePosition0] >> nBitOffset));

        *pbBuffer &= (0x01 << nBitLength) - 1;
    }
}

void SetBits(
    TBitArray * pArray,
    unsigned int nBitPosition,
    unsigned int nBitLength,
    void * pvBuffer,
    int nResultByteSize)
{
    unsigned char * pbBuffer = (unsigned char *)pvBuffer;
    unsigned int nBytePosition = (nBitPosition / 8);
    unsigned int nBitOffset = (nBitPosition & 0x07);
    unsigned short BitBuffer = 0;
    unsigned short AndMask = 0;
    unsigned short OneByte = 0;

    /* Keep compiler happy for platforms where nResultByteSize is not used */
    nResultByteSize = nResultByteSize;

#ifndef PLATFORM_LITTLE_ENDIAN
    /* Adjust the buffer pointer for big endian platforms */
    pbBuffer += (nResultByteSize - 1);
#endif    

    /* Copy whole bytes, if any */
    while(nBitLength > 8)
    {
        /* Reload the bit buffer */
#ifdef PLATFORM_LITTLE_ENDIAN
        OneByte = *pbBuffer++;
#else
        OneByte = *pbBuffer--;
#endif
        /* Update the BitBuffer and AndMask for the bit array */
        BitBuffer = (BitBuffer >> 0x08) | (OneByte << nBitOffset);
        AndMask = (AndMask >> 0x08) | (0x00FF << nBitOffset);

        /* Update the byte in the array */
        pArray->Elements[nBytePosition] = (uint8_t)((pArray->Elements[nBytePosition] & ~AndMask) | BitBuffer);

        /* Move byte positions and lengths */
        nBytePosition++;
        nBitLength -= 0x08;
    }

    if(nBitLength != 0)
    {
        /* Reload the bit buffer */
        OneByte = *pbBuffer;

        /* Update the AND mask for the last bit */
        BitBuffer = (BitBuffer >> 0x08) | (OneByte << nBitOffset);
        AndMask = (AndMask >> 0x08) | (SetBitsMask[nBitLength] << nBitOffset);

        /* Update the byte in the array */
        pArray->Elements[nBytePosition] = (uint8_t)((pArray->Elements[nBytePosition] & ~AndMask) | BitBuffer);

        /* Update the next byte, if needed */
        if(AndMask & 0xFF00)
        {
            nBytePosition++;
            BitBuffer >>= 0x08;
            AndMask >>= 0x08;

            pArray->Elements[nBytePosition] = (uint8_t)((pArray->Elements[nBytePosition] & ~AndMask) | BitBuffer);
        }
    }
}

/*-----------------------------------------------------------------------------
 * Support for MPQ header
 */

static uint64_t DetermineArchiveSize_V1(
    TMPQArchive * ha,
    TMPQHeader * pHeader,
    uint64_t MpqOffset,
    uint64_t FileSize)
{
    uint64_t ByteOffset;
    uint64_t EndOfMpq = FileSize;
    uint32_t SignatureHeader = 0;
    uint32_t dwArchiveSize32;

    /* This could only be called for MPQs version 1.0 */
    assert(pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1);

    /* Check if we can rely on the archive size in the header */
    if(pHeader->dwBlockTablePos < pHeader->dwArchiveSize)
    {
        /* The block table cannot be compressed, so the sizes must match */
        if((pHeader->dwArchiveSize - pHeader->dwBlockTablePos) == (pHeader->dwBlockTableSize * sizeof(TMPQBlock)))
            return pHeader->dwArchiveSize;

        /* If the archive size in the header is less than real file size */
        dwArchiveSize32 = (uint32_t)(FileSize - MpqOffset);
        if(pHeader->dwArchiveSize == dwArchiveSize32)
            return pHeader->dwArchiveSize;
    }

    /* Check if there is a signature header */
    if((EndOfMpq - MpqOffset) > (MPQ_STRONG_SIGNATURE_SIZE + 4))
    {
        ByteOffset = EndOfMpq - MPQ_STRONG_SIGNATURE_SIZE - 4;
        FileStream_Read(ha->pStream, &ByteOffset, &SignatureHeader, sizeof(uint32_t));
        if(BSWAP_INT32_UNSIGNED(SignatureHeader) == MPQ_STRONG_SIGNATURE_ID)
            EndOfMpq = EndOfMpq - MPQ_STRONG_SIGNATURE_SIZE - 4;
    }

    /* Return the returned archive size */
    return (EndOfMpq - MpqOffset);
}

static uint64_t DetermineArchiveSize_V2(
    TMPQHeader * pHeader,
    uint64_t MpqOffset,
    uint64_t FileSize)
{
    uint64_t EndOfMpq = FileSize;
    uint32_t dwArchiveSize32;

    /* This could only be called for MPQs version 2.0 */
    assert(pHeader->wFormatVersion == MPQ_FORMAT_VERSION_2);

    /* Check if we can rely on the archive size in the header */
    if((FileSize >> 0x20) == 0)
    {
        if(pHeader->dwBlockTablePos < pHeader->dwArchiveSize)
        {
            if((pHeader->dwArchiveSize - pHeader->dwBlockTablePos) <= (pHeader->dwBlockTableSize * sizeof(TMPQBlock)))
                return pHeader->dwArchiveSize;

            /* If the archive size in the header is less than real file size */
            dwArchiveSize32 = (uint32_t)(FileSize - MpqOffset);
            if(pHeader->dwArchiveSize <= dwArchiveSize32)
                return pHeader->dwArchiveSize;
        }
    }

    /* Return the calculated archive size */
    return (EndOfMpq - MpqOffset);
}

uint64_t FileOffsetFromMpqOffset(TMPQArchive * ha, uint64_t MpqOffset)
{
    if(ha->pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1)
    {
        /* For MPQ archive v1, any file offset is only 32-bit */
        return (uint64_t)((uint32_t)ha->MpqPos + (uint32_t)MpqOffset);
    }
    else
    {
        /* For MPQ archive v2+, file offsets are full 64-bit */
        return ha->MpqPos + MpqOffset;
    }
}

uint64_t CalculateRawSectorOffset(
    TMPQFile * hf,
    uint32_t dwSectorOffset)
{
    uint64_t RawFilePos;

    /* Must be used for files within a MPQ */
    assert(hf->ha != NULL);
    assert(hf->ha->pHeader != NULL);
    
    /*
     * Some MPQ protectors place the sector offset table after the actual file data.
     * Sector offsets in the sector offset table are negative. When added
     * to MPQ file offset from the block table entry, the result is a correct
     * position of the file data in the MPQ.
     *
     * For MPQs version 1.0, the offset is purely 32-bit
     */

    RawFilePos = hf->RawFilePos + dwSectorOffset;
    if(hf->ha->pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1)
        RawFilePos = (uint32_t)hf->ha->MpqPos + (uint32_t)hf->pFileEntry->ByteOffset + dwSectorOffset;

    /* We also have to add patch header size, if patch header is present */
    if(hf->pPatchInfo != NULL)
        RawFilePos += hf->pPatchInfo->dwLength;

    /* Return the result offset */
    return RawFilePos;
}

/* This function converts the MPQ header so it always looks like version 4 */
int ConvertMpqHeaderToFormat4(
    TMPQArchive * ha,
    uint64_t MpqOffset,
    uint64_t FileSize,
    uint32_t dwFlags)
{
    TMPQHeader * pHeader = (TMPQHeader *)ha->HeaderData;
    uint64_t BlockTablePos64 = 0;
    uint64_t HashTablePos64 = 0;
    uint64_t BlockTableMask = (uint64_t)-1;
    uint64_t ByteOffset;
    uint16_t wFormatVersion = BSWAP_INT16_UNSIGNED(pHeader->wFormatVersion);
    int nError = ERROR_SUCCESS;

    /* If version 1.0 is forced, then the format version is forced to be 1.0 */
    /* Reason: Storm.dll in Warcraft III ignores format version value */
    if(dwFlags & MPQ_OPEN_FORCE_MPQ_V1)
        wFormatVersion = MPQ_FORMAT_VERSION_1;

    /* Format-specific fixes */
    switch(wFormatVersion)
    {
        case MPQ_FORMAT_VERSION_1:

            /* Check for malformed MPQ header version 1.0 */
            BSWAP_TMPQHEADER(pHeader, MPQ_FORMAT_VERSION_1);
            if(pHeader->wFormatVersion != MPQ_FORMAT_VERSION_1 || pHeader->dwHeaderSize != MPQ_HEADER_SIZE_V1)
            {
                pHeader->wFormatVersion = MPQ_FORMAT_VERSION_1;
                pHeader->dwHeaderSize = MPQ_HEADER_SIZE_V1;
                ha->dwFlags |= MPQ_FLAG_MALFORMED;
            }

            /*
             * Note: The value of "dwArchiveSize" member in the MPQ header
             * is ignored by Storm.dll and can contain garbage value
             * ("w3xmaster" protector).
             */

            Label_ArchiveVersion1:
            if(pHeader->dwHashTablePos <= pHeader->dwHeaderSize)
                ha->dwFlags |= MPQ_FLAG_MALFORMED;
            if(pHeader->dwBlockTablePos <= pHeader->dwHeaderSize)
                ha->dwFlags |= MPQ_FLAG_MALFORMED;

            /* Fill the rest of the header */
            memset((unsigned char *)pHeader + MPQ_HEADER_SIZE_V1, 0, sizeof(TMPQHeader) - MPQ_HEADER_SIZE_V1);
            pHeader->BlockTableSize64 = pHeader->dwBlockTableSize * sizeof(TMPQBlock);
            pHeader->HashTableSize64 = pHeader->dwHashTableSize * sizeof(TMPQHash);
            pHeader->ArchiveSize64 = pHeader->dwArchiveSize;
            
            /* Block table position must be calculated as 32-bit value */
            /* Note: BOBA protector puts block table before the MPQ header, so it is negative */
            BlockTablePos64 = (uint64_t)((uint32_t)MpqOffset + pHeader->dwBlockTablePos);
            BlockTableMask = 0xFFFFFFF0;

            /* Determine the archive size on malformed MPQs */
            if(ha->dwFlags & MPQ_FLAG_MALFORMED)
            {
                /* Calculate the archive size */
                pHeader->ArchiveSize64 = DetermineArchiveSize_V1(ha, pHeader, MpqOffset, FileSize);
                pHeader->dwArchiveSize = (uint32_t)pHeader->ArchiveSize64;
            }
            break;

        case MPQ_FORMAT_VERSION_2:

            /* Check for malformed MPQ header version 1.0 */
            BSWAP_TMPQHEADER(pHeader, MPQ_FORMAT_VERSION_2);
            if(pHeader->wFormatVersion != MPQ_FORMAT_VERSION_2 || pHeader->dwHeaderSize != MPQ_HEADER_SIZE_V2)
            {
                pHeader->wFormatVersion = MPQ_FORMAT_VERSION_1;
                pHeader->dwHeaderSize = MPQ_HEADER_SIZE_V1;
                ha->dwFlags |= MPQ_FLAG_MALFORMED;
                goto Label_ArchiveVersion1;
            }

            /* Fill the rest of the header with zeros */
            memset((unsigned char *)pHeader + MPQ_HEADER_SIZE_V2, 0, sizeof(TMPQHeader) - MPQ_HEADER_SIZE_V2);

            /* Calculate the expected hash table size */
            pHeader->HashTableSize64 = (pHeader->dwHashTableSize * sizeof(TMPQHash));
            HashTablePos64 = MAKE_OFFSET64(pHeader->wHashTablePosHi, pHeader->dwHashTablePos);

            /* Calculate the expected block table size */
            pHeader->BlockTableSize64 = (pHeader->dwBlockTableSize * sizeof(TMPQBlock));
            BlockTablePos64 = MAKE_OFFSET64(pHeader->wBlockTablePosHi, pHeader->dwBlockTablePos);

            /* We require the block table to follow hash table */
            if(BlockTablePos64 >= HashTablePos64)
            {
                /* HashTableSize64 may be less than TblSize * sizeof(TMPQHash). */
                /* That means that the hash table is compressed. */
                pHeader->HashTableSize64 = BlockTablePos64 - HashTablePos64;

                /* Calculate the compressed block table size */
                if(pHeader->HiBlockTablePos64 != 0)
                {
                    /* BlockTableSize64 may be less than TblSize * sizeof(TMPQBlock). */
                    /* That means that the block table is compressed. */
                    pHeader->BlockTableSize64 = pHeader->HiBlockTablePos64 - BlockTablePos64;
                    assert(pHeader->BlockTableSize64 <= (pHeader->dwBlockTableSize * sizeof(TMPQBlock)));

                    /* Determine real archive size */
                    pHeader->ArchiveSize64 = DetermineArchiveSize_V2(pHeader, MpqOffset, FileSize);

                    /* Calculate the size of the hi-block table */
                    pHeader->HiBlockTableSize64 = pHeader->ArchiveSize64 - pHeader->HiBlockTablePos64;
                    assert(pHeader->HiBlockTableSize64 == (pHeader->dwBlockTableSize * sizeof(uint16_t)));
                }
                else
                {
                    /* Determine real archive size */
                    pHeader->ArchiveSize64 = DetermineArchiveSize_V2(pHeader, MpqOffset, FileSize);

                    /* Calculate size of the block table */
                    pHeader->BlockTableSize64 = pHeader->ArchiveSize64 - BlockTablePos64;
                    assert(pHeader->BlockTableSize64 <= (pHeader->dwBlockTableSize * sizeof(TMPQBlock)));
                }
            }
            else
            {
                pHeader->ArchiveSize64 = pHeader->dwArchiveSize;
                ha->dwFlags |= MPQ_FLAG_MALFORMED;
            }

            /* Add the MPQ Offset */
            BlockTablePos64 += MpqOffset;
            break;

        case MPQ_FORMAT_VERSION_3:

            /* In MPQ format 3.0, the entire header is optional */
            /* and the size of the header can actually be identical */
            /* to size of header 2.0 */
            BSWAP_TMPQHEADER(pHeader, MPQ_FORMAT_VERSION_3);
            if(pHeader->dwHeaderSize < MPQ_HEADER_SIZE_V3)
            {
                pHeader->ArchiveSize64 = pHeader->dwArchiveSize;
                pHeader->HetTablePos64 = 0;
                pHeader->BetTablePos64 = 0;
            }

            /*
             * We need to calculate the compressed size of each table. We assume the following order:
             * 1) HET table
             * 2) BET table
             * 3) Classic hash table
             * 4) Classic block table
             * 5) Hi-block table
             */

            /* Fill the rest of the header with zeros */
            memset((unsigned char *)pHeader + MPQ_HEADER_SIZE_V3, 0, sizeof(TMPQHeader) - MPQ_HEADER_SIZE_V3);
            BlockTablePos64 = MAKE_OFFSET64(pHeader->wBlockTablePosHi, pHeader->dwBlockTablePos);
            HashTablePos64 = MAKE_OFFSET64(pHeader->wHashTablePosHi, pHeader->dwHashTablePos);
            ByteOffset = pHeader->ArchiveSize64;

            /* Size of the hi-block table */
            if(pHeader->HiBlockTablePos64)
            {
                pHeader->HiBlockTableSize64 = ByteOffset - pHeader->HiBlockTablePos64;
                ByteOffset = pHeader->HiBlockTablePos64;
            }

            /* Size of the block table */
            if(BlockTablePos64)
            {
                pHeader->BlockTableSize64 = ByteOffset - BlockTablePos64;
                ByteOffset = BlockTablePos64;
            }

            /* Size of the hash table */
            if(HashTablePos64)
            {
                pHeader->HashTableSize64 = ByteOffset - HashTablePos64;
                ByteOffset = HashTablePos64;
            }

            /* Size of the BET table */
            if(pHeader->BetTablePos64)
            {
                pHeader->BetTableSize64 = ByteOffset - pHeader->BetTablePos64;
                ByteOffset = pHeader->BetTablePos64;
            }

            /* Size of the HET table */
            if(pHeader->HetTablePos64)
            {
                pHeader->HetTableSize64 = ByteOffset - pHeader->HetTablePos64;
/*              ByteOffset = pHeader->HetTablePos64; */
            }
            
            /* Add the MPQ Offset */
            BlockTablePos64 += MpqOffset;
            break;

        case MPQ_FORMAT_VERSION_4:

            /* Verify header MD5. Header MD5 is calculated from the MPQ header since the 'MPQ\x1A' */
            /* signature until the position of header MD5 at offset 0xC0 */
            BSWAP_TMPQHEADER(pHeader, MPQ_FORMAT_VERSION_4);
            if(!VerifyDataBlockHash(pHeader, MPQ_HEADER_SIZE_V4 - MD5_DIGEST_SIZE, pHeader->MD5_MpqHeader))
                nError = ERROR_FILE_CORRUPT;

            /* Calculate the block table position */
            BlockTablePos64 = MpqOffset + MAKE_OFFSET64(pHeader->wBlockTablePosHi, pHeader->dwBlockTablePos);
            break;

        default:

            /* Check if it's a War of the Immortal data file (SQP) */
            /* If not, we treat it as malformed MPQ version 1.0 */
            if(ConvertSqpHeaderToFormat4(ha, FileSize, dwFlags) != ERROR_SUCCESS)
            {
                pHeader->wFormatVersion = MPQ_FORMAT_VERSION_1;
                pHeader->dwHeaderSize = MPQ_HEADER_SIZE_V1;
                ha->dwFlags |= MPQ_FLAG_MALFORMED;
                goto Label_ArchiveVersion1;
            }

            /* Calculate the block table position */
            BlockTablePos64 = MpqOffset + MAKE_OFFSET64(pHeader->wBlockTablePosHi, pHeader->dwBlockTablePos);
            break;
    }

    /* Handle case when block table is placed before the MPQ header */
    /* Used by BOBA protector */
    if(BlockTablePos64 < MpqOffset)
        ha->dwFlags |= MPQ_FLAG_MALFORMED;
    return nError;
}

/*-----------------------------------------------------------------------------
 * Support for hash table
 */

/* Returns a hash table entry in the following order: */
/* 1) A hash table entry with the neutral locale */
/* 2) A hash table entry with any other locale */
/* 3) NULL */
static TMPQHash * GetHashEntryAny(TMPQArchive * ha, const char * szFileName)
{
    TMPQHash * pHashNeutral = NULL;
    TMPQHash * pFirstHash = GetFirstHashEntry(ha, szFileName);
    TMPQHash * pHashAny = NULL;
    TMPQHash * pHash = pFirstHash;

    /* Parse the found hashes */
    while(pHash != NULL)
    {
        /* If we found neutral hash, remember it */
        if(pHash->lcLocale == 0)
            pHashNeutral = pHash;
        if(pHashAny == NULL)
            pHashAny = pHash;

        /* Get the next hash entry for that file */
        pHash = GetNextHashEntry(ha, pFirstHash, pHash); 
    }

    /* At the end, return neutral hash (if found), otherwise NULL */
    return (pHashNeutral != NULL) ? pHashNeutral : pHashAny;
}

/* Returns a hash table entry in the following order: */
/* 1) A hash table entry with the preferred locale */
/* 2) A hash table entry with the neutral locale */
/* 3) NULL */
static TMPQHash * GetHashEntryLocale(TMPQArchive * ha, const char * szFileName, uint32_t lcLocale)
{
    TMPQHash * pHashNeutral = NULL;
    TMPQHash * pFirstHash = GetFirstHashEntry(ha, szFileName);
    TMPQHash * pHash = pFirstHash;

    /* Parse the found hashes */
    while(pHash != NULL)
    {
        /* If the locales match, return it */
        if(pHash->lcLocale == lcLocale)
            return pHash;
        
        /* If we found neutral hash, remember it */
        if(pHash->lcLocale == 0)
            pHashNeutral = pHash;

        /* Get the next hash entry for that file */
        pHash = GetNextHashEntry(ha, pFirstHash, pHash); 
    }

    /* At the end, return neutral hash (if found), otherwise NULL */
    return pHashNeutral;
}

/* Returns a hash table entry in the following order: */
/* 1) A hash table entry with the preferred locale */
/* 2) NULL */
static TMPQHash * GetHashEntryExact(TMPQArchive * ha, const char * szFileName, uint32_t lcLocale)
{
    TMPQHash * pFirstHash = GetFirstHashEntry(ha, szFileName);
    TMPQHash * pHash = pFirstHash;

    /* Parse the found hashes */
    while(pHash != NULL)
    {
        /* If the locales match, return it */
        if(pHash->lcLocale == lcLocale)
            return pHash;
        
        /* Get the next hash entry for that file */
        pHash = GetNextHashEntry(ha, pFirstHash, pHash); 
    }

    /* Not found */
    return NULL;
}

static int BuildFileTableFromBlockTable(
    TMPQArchive * ha,
    TFileEntry * pFileTable,
    TMPQBlock * pBlockTable)
{
    TFileEntry * pFileEntry;
    TMPQHeader * pHeader = ha->pHeader;
    TMPQBlock * pBlock;
    TMPQHash * pHashEnd = ha->pHashTable + ha->dwHashTableSize;
    TMPQHash * pHash;

    /* Parse the entire hash table */
    for(pHash = ha->pHashTable; pHash < pHashEnd; pHash++)
    {
        /*
         * Only if the block index is less than block table size.
         * An MPQ protector can place multiple invalid entries
         * into the hash table:
         *
         * a6d79af0 e61a0932 001e0000 0000770b <== Fake valid
         * a6d79af0 e61a0932 0000d761 0000dacb <== Fake valid
         * a6d79af0 e61a0932 00000000 0000002f <== Real file entry (block index is valid)
         * a6d79af0 e61a0932 00005a4f 000093bc <== Fake valid
         */ 
        if(pHash->dwBlockIndex < pHeader->dwBlockTableSize)
        {
            /* Get the pointer to the file entry and the block entry */
            pFileEntry = pFileTable + pHash->dwBlockIndex;
            pBlock = pBlockTable + pHash->dwBlockIndex;

            /* Only if the block table entry contains valid file */
            if((pBlock->dwFlags & MPQ_FILE_EXISTS) && !(pBlock->dwFlags & ~MPQ_FILE_VALID_FLAGS))
            {
                /* ByteOffset is only valid if file size is not zero */
                pFileEntry->ByteOffset = pBlock->dwFilePos;
                if(pFileEntry->ByteOffset == 0 && pBlock->dwCSize == 0)
                    pFileEntry->ByteOffset = ha->pHeader->dwHeaderSize;

                pFileEntry->dwHashIndex = (uint32_t)(pHash - ha->pHashTable);
                pFileEntry->dwFileSize  = pBlock->dwFSize;
                pFileEntry->dwCmpSize   = pBlock->dwCSize;
                pFileEntry->dwFlags     = pBlock->dwFlags;
                pFileEntry->lcLocale    = pHash->lcLocale;
                pFileEntry->wPlatform   = pHash->wPlatform;
                continue;
            }

            /* If the hash table entry doesn't point to the valid file item, */
            /* we invalidate the hash table entry */
            pHash->dwName1      = 0xFFFFFFFF;
            pHash->dwName2      = 0xFFFFFFFF;
            pHash->lcLocale     = 0xFFFF;
            pHash->wPlatform    = 0xFFFF;
            pHash->dwBlockIndex = HASH_ENTRY_DELETED;
        }
    }

    return ERROR_SUCCESS;
}

static int UpdateFileTableFromHashTable(
    TMPQArchive * ha,
    TFileEntry * pFileTable)
{
    TFileEntry * pFileEntry;
    TMPQHash * pHashEnd = ha->pHashTable + ha->dwHashTableSize;
    TMPQHash * pHash;

    for(pHash = ha->pHashTable; pHash < pHashEnd; pHash++)
    {
        if(pHash->dwBlockIndex < ha->dwFileTableSize)
        {
            pFileEntry = pFileTable + pHash->dwBlockIndex;
            if(pFileEntry->dwFlags & MPQ_FILE_EXISTS)
            {
                pFileEntry->dwHashIndex = (uint32_t)(pHash - ha->pHashTable);
                pFileEntry->lcLocale    = pHash->lcLocale;
                pFileEntry->wPlatform   = pHash->wPlatform;
            }
        }
    }

    return ERROR_SUCCESS;
}

static TMPQHash * TranslateHashTable(
    TMPQArchive * ha,
    uint64_t * pcbTableSize)
{
    TMPQHash * pHashTable;
    size_t HashTableSize;

    /* Allocate copy of the hash table */
    pHashTable = STORM_ALLOC(TMPQHash, ha->dwHashTableSize);
    if(pHashTable != NULL)
    {
        /* Copy the hash table */
        HashTableSize = sizeof(TMPQHash) * ha->dwHashTableSize;
        memcpy(pHashTable, ha->pHashTable, HashTableSize);

        /* Give the size to the caller */
        if(pcbTableSize != NULL)
        {
            *pcbTableSize = (uint64_t)HashTableSize;
        }
    }

    return pHashTable;
}

/* Also used in SFileGetFileInfo */
TMPQBlock * TranslateBlockTable(
    TMPQArchive * ha,
    uint64_t * pcbTableSize,
    int * pbNeedHiBlockTable)
{
    TFileEntry * pFileEntry = ha->pFileTable;
    TMPQBlock * pBlockTable;
    TMPQBlock * pBlock;
    size_t NeedHiBlockTable = 0;
    size_t BlockTableSize;

    /* Allocate copy of the hash table */
    pBlockTable = pBlock = STORM_ALLOC(TMPQBlock, ha->dwFileTableSize);
    if(pBlockTable != NULL)
    {
        uint32_t i;
        
        /* Copy the block table */
        BlockTableSize = sizeof(TMPQBlock) * ha->dwFileTableSize;
        for(i = 0; i < ha->dwFileTableSize; i++)
        {
            NeedHiBlockTable |= (pFileEntry->ByteOffset >> 32);
            pBlock->dwFilePos = (uint32_t)pFileEntry->ByteOffset;
            pBlock->dwFSize   = pFileEntry->dwFileSize;
            pBlock->dwCSize   = pFileEntry->dwCmpSize;
            pBlock->dwFlags   = pFileEntry->dwFlags;

            pFileEntry++;
            pBlock++;
        }

        /* Give the size to the caller */
        if(pcbTableSize != NULL)
            *pcbTableSize = (uint64_t)BlockTableSize;

        if(pbNeedHiBlockTable != NULL)
            *pbNeedHiBlockTable = NeedHiBlockTable ? 1 : 0;
    }

    return pBlockTable;
}

static uint16_t * TranslateHiBlockTable(
    TMPQArchive * ha,
    uint64_t * pcbTableSize)
{
    TFileEntry * pFileEntry = ha->pFileTable;
    uint16_t * pHiBlockTable;
    uint16_t * pHiBlock;
    size_t HiBlockTableSize;

    /* Allocate copy of the hash table */
    pHiBlockTable = pHiBlock = STORM_ALLOC(uint16_t, ha->dwFileTableSize);
    if(pHiBlockTable != NULL)
    {
        uint32_t i;
        
        /* Copy the block table */
        HiBlockTableSize = sizeof(uint16_t) * ha->dwFileTableSize;
        for(i = 0; i < ha->dwFileTableSize; i++)
            pHiBlock[i] = (uint16_t)(pFileEntry[i].ByteOffset >> 0x20);

        /* Give the size to the caller */
        if(pcbTableSize != NULL)
            *pcbTableSize = (uint64_t)HiBlockTableSize;
    }

    return pHiBlockTable;
}

/*-----------------------------------------------------------------------------
 * General EXT table functions
 */

TMPQExtHeader * LoadExtTable(
    TMPQArchive * ha,
    uint64_t ByteOffset,
    size_t Size,
    uint32_t dwSignature,
    uint32_t dwKey)
{
    TMPQExtHeader * pCompressed = NULL;      /* Compressed table */
    TMPQExtHeader * pExtTable = NULL;        /* Uncompressed table */

    /* Do nothing if the size is zero */
    if(ByteOffset != 0 && Size != 0)
    {
        /* Allocate size for the compressed table */
        pExtTable = (TMPQExtHeader *)STORM_ALLOC(uint8_t, Size);
        if(pExtTable != NULL)
        {
            /* Load the table from the MPQ */
            ByteOffset += ha->MpqPos;
            if(!FileStream_Read(ha->pStream, &ByteOffset, pExtTable, (uint32_t)Size))
            {
                STORM_FREE(pExtTable);
                return NULL;
            }

            /* Swap the ext table header */
            BSWAP_ARRAY32_UNSIGNED(pExtTable, sizeof(TMPQExtHeader));
            if(pExtTable->dwSignature != dwSignature)
            {
                STORM_FREE(pExtTable);
                return NULL;
            }

            /* Decrypt the block */
            BSWAP_ARRAY32_UNSIGNED(pExtTable + 1, pExtTable->dwDataSize);
            DecryptMpqBlock(pExtTable + 1, (uint32_t)(Size - sizeof(TMPQExtHeader)), dwKey);
            BSWAP_ARRAY32_UNSIGNED(pExtTable + 1, pExtTable->dwDataSize);

            /* If the table is compressed, decompress it */
            if((pExtTable->dwDataSize + sizeof(TMPQExtHeader)) > Size)
            {
                pCompressed = pExtTable;
                pExtTable = (TMPQExtHeader *)STORM_ALLOC(uint8_t, sizeof(TMPQExtHeader) + pCompressed->dwDataSize);
                if(pExtTable != NULL)
                {
                    int cbOutBuffer = (int)pCompressed->dwDataSize;
                    int cbInBuffer = (int)Size;

                    /* Decompress the extended table */
                    pExtTable->dwSignature = pCompressed->dwSignature;
                    pExtTable->dwVersion   = pCompressed->dwVersion;
                    pExtTable->dwDataSize  = pCompressed->dwDataSize;
                    if(!SCompDecompress2(pExtTable + 1, &cbOutBuffer, pCompressed + 1, cbInBuffer))
                    {
                        STORM_FREE(pExtTable);
                        pExtTable = NULL;
                    }
                }

                /* Free the compressed block */
                STORM_FREE(pCompressed);
            }
        }
    }

    /* Return the decompressed table to the caller */
    return pExtTable;
}

static int SaveMpqTable(
    TMPQArchive * ha,
    void * pMpqTable,
    uint64_t ByteOffset,
    size_t Size,
    unsigned char * md5,
    uint32_t dwKey,
    int bCompress)
{
    uint64_t FileOffset;
    void * pCompressed = NULL;
    int nError = ERROR_SUCCESS;

    /* Do we have to compress the table? */
    if(bCompress)
    {
        int cbOutBuffer = (int)Size;
        int cbInBuffer = (int)Size;

        /* Allocate extra space for compressed table */
        pCompressed = STORM_ALLOC(uint8_t, Size);
        if(pCompressed == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        /* Compress the table */
        SCompCompress(pCompressed, &cbOutBuffer, pMpqTable, cbInBuffer, MPQ_COMPRESSION_ZLIB, 0, 0);

        /* If the compression failed, revert it. Otherwise, swap the tables */
        if(cbOutBuffer >= cbInBuffer)
        {
            STORM_FREE(pCompressed);
            pCompressed = NULL;
        }
        else
        {
            pMpqTable = pCompressed;
        }
    }

    /* Encrypt the table */
    if(dwKey != 0)
    {
        BSWAP_ARRAY32_UNSIGNED(pMpqTable, Size);
        EncryptMpqBlock(pMpqTable, (uint32_t)Size, dwKey);
        BSWAP_ARRAY32_UNSIGNED(pMpqTable, Size);
    }

    /* Calculate the MD5 */
    if(md5 != NULL)
    {
        CalculateDataBlockHash(pMpqTable, (uint32_t)Size, md5);
    }

    /* Save the table to the MPQ */
    BSWAP_ARRAY32_UNSIGNED(pMpqTable, Size);
    FileOffset = ha->MpqPos + ByteOffset;
    if(!FileStream_Write(ha->pStream, &FileOffset, pMpqTable, (uint32_t)Size))
        nError = GetLastError();

    /* Free the compressed table, if any */
    if(pCompressed != NULL)
        STORM_FREE(pCompressed);
    return nError;
}

static int SaveExtTable(
    TMPQArchive * ha,
    TMPQExtHeader * pExtTable,
    uint64_t ByteOffset,
    uint32_t dwTableSize,
    unsigned char * md5,
    uint32_t dwKey,
    int bCompress,
    uint32_t * pcbTotalSize)
{
    uint64_t FileOffset;
    TMPQExtHeader * pCompressed = NULL;
    uint32_t cbTotalSize = 0;
    int nError = ERROR_SUCCESS;

    /* Do we have to compress the table? */
    if(bCompress)
    {
        int cbOutBuffer = (int)dwTableSize;
        int cbInBuffer = (int)dwTableSize;

        /* Allocate extra space for compressed table */
        pCompressed = (TMPQExtHeader *)STORM_ALLOC(uint8_t, dwTableSize);
        if(pCompressed == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        /* Compress the table */
        pCompressed->dwSignature = pExtTable->dwSignature;
        pCompressed->dwVersion   = pExtTable->dwVersion;
        pCompressed->dwDataSize  = pExtTable->dwDataSize;
        SCompCompress((pCompressed + 1), &cbOutBuffer, (pExtTable + 1), cbInBuffer, MPQ_COMPRESSION_ZLIB, 0, 0);

        /* If the compression failed, revert it. Otherwise, swap the tables */
        if(cbOutBuffer >= cbInBuffer)
        {
            STORM_FREE(pCompressed);
            pCompressed = NULL;
        }
        else
        {
            pExtTable = pCompressed;
        }
    }

    /* Encrypt the table */
    if(dwKey != 0)
    {
        BSWAP_ARRAY32_UNSIGNED(pExtTable + 1, pExtTable->dwDataSize);
        EncryptMpqBlock(pExtTable + 1, (uint32_t)(dwTableSize - sizeof(TMPQExtHeader)), dwKey);
        BSWAP_ARRAY32_UNSIGNED(pExtTable + 1, pExtTable->dwDataSize);
    }

    /* Calculate the MD5 of the table after */
    if(md5 != NULL)
    {
        CalculateDataBlockHash(pExtTable, dwTableSize, md5);
    }

    /* Save the table to the MPQ */
    FileOffset = ha->MpqPos + ByteOffset;
    if(FileStream_Write(ha->pStream, &FileOffset, pExtTable, dwTableSize))
        cbTotalSize += dwTableSize;
    else
        nError = GetLastError();

    /* We have to write raw data MD5 */
    if(nError == ERROR_SUCCESS && ha->pHeader->dwRawChunkSize != 0)
    {
        nError = WriteMemDataMD5(ha->pStream,
                                 FileOffset,
                                 pExtTable,
                                 dwTableSize,
                                 ha->pHeader->dwRawChunkSize,
                                &cbTotalSize);
    }

    /* Give the total written size, if needed */
    if(pcbTotalSize != NULL)
        *pcbTotalSize = cbTotalSize;

    /* Free the compressed table, if any */
    if(pCompressed != NULL)
        STORM_FREE(pCompressed);
    return nError;
}

/*-----------------------------------------------------------------------------
 * Support for HET table
 */

static void CreateHetHeader(
    TMPQHetTable * pHetTable,
    TMPQHetHeader * pHetHeader)
{
    /* Fill the common header */
    pHetHeader->ExtHdr.dwSignature  = HET_TABLE_SIGNATURE;
    pHetHeader->ExtHdr.dwVersion    = 1;
    pHetHeader->ExtHdr.dwDataSize   = 0;

    /* Fill the HET header */
    pHetHeader->dwEntryCount        = pHetTable->dwEntryCount;
    pHetHeader->dwTotalCount        = pHetTable->dwTotalCount;
    pHetHeader->dwNameHashBitSize   = pHetTable->dwNameHashBitSize;
    pHetHeader->dwIndexSizeTotal    = pHetTable->dwIndexSizeTotal;
    pHetHeader->dwIndexSizeExtra    = pHetTable->dwIndexSizeExtra;
    pHetHeader->dwIndexSize         = pHetTable->dwIndexSize;
    pHetHeader->dwIndexTableSize    = ((pHetHeader->dwIndexSizeTotal * pHetTable->dwTotalCount) + 7) / 8;

    /* Calculate the total size needed for holding HET table */
    pHetHeader->ExtHdr.dwDataSize =
    pHetHeader->dwTableSize = sizeof(TMPQHetHeader) - sizeof(TMPQExtHeader) +
                              pHetHeader->dwTotalCount +
                              pHetHeader->dwIndexTableSize;
}

TMPQHetTable * CreateHetTable(uint32_t dwEntryCount, uint32_t dwTotalCount, uint32_t dwNameHashBitSize, unsigned char * pbSrcData)
{
    TMPQHetTable * pHetTable;

    pHetTable = STORM_ALLOC(TMPQHetTable, 1);
    if(pHetTable != NULL)
    {
        /* Zero the HET table */
        memset(pHetTable, 0, sizeof(TMPQHetTable));

        /* Hash sizes less than 0x40 bits are not tested */
        assert(dwNameHashBitSize == 0x40);

        /* Calculate masks */
        pHetTable->AndMask64 = ((dwNameHashBitSize != 0x40) ? ((uint64_t)1 << dwNameHashBitSize) : 0) - 1;
        pHetTable->OrMask64 = (uint64_t)1 << (dwNameHashBitSize - 1);

        /* If the total count is not entered, use default */
        if(dwTotalCount == 0)
            dwTotalCount = (dwEntryCount * 4) / 3;

        /* Store the HET table parameters */
        pHetTable->dwEntryCount        = dwEntryCount;
        pHetTable->dwTotalCount        = dwTotalCount;
        pHetTable->dwNameHashBitSize   = dwNameHashBitSize;
        pHetTable->dwIndexSizeTotal    = GetNecessaryBitCount(dwEntryCount);
        pHetTable->dwIndexSizeExtra    = 0;
        pHetTable->dwIndexSize         = pHetTable->dwIndexSizeTotal;

        /* Allocate array of hashes */
        pHetTable->pNameHashes = STORM_ALLOC(uint8_t, dwTotalCount);
        if(pHetTable->pNameHashes != NULL)
        {
            /* Make sure the data are initialized */
            memset(pHetTable->pNameHashes, 0, dwTotalCount);

            /* Allocate the bit array for file indexes */
            pHetTable->pBetIndexes = CreateBitArray(dwTotalCount * pHetTable->dwIndexSizeTotal, 0xFF);
            if(pHetTable->pBetIndexes != NULL)
            {
                /* Initialize the HET table from the source data (if given) */
                if(pbSrcData != NULL)
                {
                    /* Copy the name hashes */
                    memcpy(pHetTable->pNameHashes, pbSrcData, dwTotalCount);

                    /* Copy the file indexes */
                    memcpy(pHetTable->pBetIndexes->Elements, pbSrcData + dwTotalCount, pHetTable->pBetIndexes->NumberOfBytes);
                }
                
                /* Return the result HET table */
                return pHetTable;
            }

            /* Free the name hashes */
            STORM_FREE(pHetTable->pNameHashes);
        }

        STORM_FREE(pHetTable);
    }

    /* Failed */
    return NULL;
}

static int InsertHetEntry(TMPQHetTable * pHetTable, uint64_t FileNameHash, uint32_t dwFileIndex)
{
    uint32_t StartIndex;
    uint32_t Index;
    uint8_t NameHash1;

    /* Get the start index and the high 8 bits of the name hash */
    StartIndex = Index = (uint32_t)(FileNameHash % pHetTable->dwTotalCount);
    NameHash1 = (uint8_t)(FileNameHash >> (pHetTable->dwNameHashBitSize - 8));

    /* Find a place where to put it */
    for(;;)
    {
        /* Did we find a free HET entry? */
        if(pHetTable->pNameHashes[Index] == HET_ENTRY_FREE)
        {
            /* Set the entry in the name hash table */
            pHetTable->pNameHashes[Index] = NameHash1;

            /* Set the entry in the file index table */
            SetBits(pHetTable->pBetIndexes, pHetTable->dwIndexSizeTotal * Index,
                                            pHetTable->dwIndexSize,
                                           &dwFileIndex,
                                            4);
            return ERROR_SUCCESS;
        }

        /* Move to the next entry in the HET table */
        /* If we came to the start index again, we are done */
        Index = (Index + 1) % pHetTable->dwTotalCount;
        if(Index == StartIndex)
            break;
    }

    /* No space in the HET table. Should never happen, */
    /* because the HET table is created according to the number of files */
    assert(0);
    return ERROR_DISK_FULL;
}

static TMPQHetTable * TranslateHetTable(TMPQHetHeader * pHetHeader)
{
    TMPQHetTable * pHetTable = NULL;
    unsigned char * pbSrcData = (unsigned char *)(pHetHeader + 1);

    /* Sanity check */
    assert(pHetHeader->ExtHdr.dwSignature == HET_TABLE_SIGNATURE);
    assert(pHetHeader->ExtHdr.dwVersion == 1);

    /* Verify size of the HET table */
    if(pHetHeader->ExtHdr.dwDataSize >= (sizeof(TMPQHetHeader) - sizeof(TMPQExtHeader)))
    {
        /* Verify the size of the table in the header */
        if(pHetHeader->dwTableSize == pHetHeader->ExtHdr.dwDataSize)
        {
            /* The size of the HET table must be sum of header, hash and index table size */
            assert((sizeof(TMPQHetHeader) - sizeof(TMPQExtHeader) + pHetHeader->dwTotalCount + pHetHeader->dwIndexTableSize) == pHetHeader->dwTableSize);

            /* So far, all MPQs with HET Table have had total number of entries equal to 4/3 of file count */
            /* Exception: "2010 - Starcraft II\!maps\Tya's Zerg Defense (unprotected).SC2Map" */
/*          assert(((pHetHeader->dwEntryCount * 4) / 3) == pHetHeader->dwTotalCount); */

            /* The size of one index is predictable as well */
            assert(GetNecessaryBitCount(pHetHeader->dwEntryCount) == pHetHeader->dwIndexSizeTotal);

            /* The size of index table (in entries) is expected */
            /* to be the same like the hash table size (in bytes) */
            assert(((pHetHeader->dwTotalCount * pHetHeader->dwIndexSizeTotal) + 7) / 8 == pHetHeader->dwIndexTableSize);
            
            /* Create translated table */
            pHetTable = CreateHetTable(pHetHeader->dwEntryCount, pHetHeader->dwTotalCount, pHetHeader->dwNameHashBitSize, pbSrcData);
            if(pHetTable != NULL)
            {
                /* Now the sizes in the hash table should be already set */
                assert(pHetTable->dwEntryCount     == pHetHeader->dwEntryCount);
                assert(pHetTable->dwTotalCount     == pHetHeader->dwTotalCount);
                assert(pHetTable->dwIndexSizeTotal == pHetHeader->dwIndexSizeTotal);

                /* Copy the missing variables */
                pHetTable->dwIndexSizeExtra = pHetHeader->dwIndexSizeExtra;
                pHetTable->dwIndexSize      = pHetHeader->dwIndexSize;
            }
        }
    }

    return pHetTable;
}

static TMPQExtHeader * TranslateHetTable2(TMPQHetTable * pHetTable, uint64_t * pcbHetTable)
{
    TMPQHetHeader * pHetHeader = NULL;
    TMPQHetHeader HetHeader;
    unsigned char * pbLinearTable = NULL;
    unsigned char * pbTrgData;

    /* Prepare header of the HET table */
    CreateHetHeader(pHetTable, &HetHeader);

    /* Allocate space for the linear table */
    pbLinearTable = STORM_ALLOC(uint8_t, sizeof(TMPQExtHeader) + HetHeader.dwTableSize);
    if(pbLinearTable != NULL)
    {
        /* Copy the table header */
        pHetHeader = (TMPQHetHeader *)pbLinearTable;
        memcpy(pHetHeader, &HetHeader, sizeof(TMPQHetHeader));
        pbTrgData = (unsigned char *)(pHetHeader + 1);

        /* Copy the array of name hashes */
        memcpy(pbTrgData, pHetTable->pNameHashes, pHetTable->dwTotalCount);
        pbTrgData += pHetTable->dwTotalCount;

        /* Copy the bit array of BET indexes */
        memcpy(pbTrgData, pHetTable->pBetIndexes->Elements, HetHeader.dwIndexTableSize);

        /* Calculate the total size of the table, including the TMPQExtHeader */
        if(pcbHetTable != NULL)
        {
            *pcbHetTable = (uint64_t)(sizeof(TMPQExtHeader) + HetHeader.dwTableSize);
        }
    }

    return &pHetHeader->ExtHdr;
}

uint32_t GetFileIndex_Het(TMPQArchive * ha, const char * szFileName)
{
    TMPQHetTable * pHetTable = ha->pHetTable;
    uint64_t FileNameHash;
    uint32_t StartIndex;
    uint32_t Index;
    uint8_t NameHash1;                 /* Upper 8 bits of the masked file name hash */

    /* If there are no entries in the HET table, do nothing */
    if(pHetTable->dwEntryCount == 0)
        return HASH_ENTRY_FREE;

    /* Do nothing if the MPQ has no HET table */
    assert(ha->pHetTable != NULL);

    /* Calculate 64-bit hash of the file name */
    FileNameHash = (HashStringJenkins(szFileName) & pHetTable->AndMask64) | pHetTable->OrMask64;

    /* Split the file name hash into two parts:
     * NameHash1: The highest 8 bits of the name hash
     * NameHash2: File name hash limited to hash size
     * Note: Our file table contains full name hash, no need to cut the high 8 bits before comparison
     */
    NameHash1 = (uint8_t)(FileNameHash >> (pHetTable->dwNameHashBitSize - 8));

    /* Calculate the starting index to the hash table */
    StartIndex = Index = (uint32_t)(FileNameHash % pHetTable->dwTotalCount);

    /* Go through HET table until we find a terminator */
    while(pHetTable->pNameHashes[Index] != HET_ENTRY_FREE)
    {
        /* Did we find a match ? */
        if(pHetTable->pNameHashes[Index] == NameHash1)
        {
            uint32_t dwFileIndex = 0;

            /* Get the file index */
            GetBits(pHetTable->pBetIndexes, pHetTable->dwIndexSizeTotal * Index,
                                            pHetTable->dwIndexSize,
                                           &dwFileIndex,
                                            4);
            /*
             * TODO: This condition only happens when we are opening a MPQ
             * where some files were deleted by StormLib. Perhaps 
             * we should not allow shrinking of the file table in MPQs v 4.0?
             * assert(dwFileIndex <= ha->dwFileTableSize);
             */

            /* Verify the FileNameHash against the entry in the table of name hashes */
            if(dwFileIndex <= ha->dwFileTableSize && ha->pFileTable[dwFileIndex].FileNameHash == FileNameHash)
                return dwFileIndex;
        }

        /* Move to the next entry in the HET table */
        /* If we came to the start index again, we are done */
        Index = (Index + 1) % pHetTable->dwTotalCount;
        if(Index == StartIndex)
            break;
    }

    /* File not found */
    return HASH_ENTRY_FREE;
}

void FreeHetTable(TMPQHetTable * pHetTable)
{
    if(pHetTable != NULL)
    {
        if(pHetTable->pNameHashes != NULL)
            STORM_FREE(pHetTable->pNameHashes);
        if(pHetTable->pBetIndexes != NULL)
            STORM_FREE(pHetTable->pBetIndexes);

        STORM_FREE(pHetTable);
    }
}

/*-----------------------------------------------------------------------------
 * Support for BET table
 */

static void CreateBetHeader(
    TMPQArchive * ha,
    TMPQBetHeader * pBetHeader)
{
    TFileEntry * pFileTableEnd = ha->pFileTable + ha->dwFileTableSize;
    TFileEntry * pFileEntry;
    uint64_t MaxByteOffset = 0;
    uint32_t FlagArray[MAX_FLAG_INDEX];
    uint32_t dwMaxFlagIndex = 0;
    uint32_t dwMaxFileSize = 0;
    uint32_t dwMaxCmpSize = 0;
    uint32_t dwFlagIndex;

    /* Initialize array of flag combinations */
    InitFileFlagArray(FlagArray);

    /* Fill the common header */
    pBetHeader->ExtHdr.dwSignature  = BET_TABLE_SIGNATURE;
    pBetHeader->ExtHdr.dwVersion    = 1;
    pBetHeader->ExtHdr.dwDataSize   = 0;

    /* Get the maximum values for the BET table */
    for(pFileEntry = ha->pFileTable; pFileEntry < pFileTableEnd; pFileEntry++)
    {
        /*
         * Note: Deleted files must be counted as well
         */

        /* Highest file position in the MPQ */
        if(pFileEntry->ByteOffset > MaxByteOffset)
            MaxByteOffset = pFileEntry->ByteOffset;

        /* Biggest file size */
        if(pFileEntry->dwFileSize > dwMaxFileSize)
            dwMaxFileSize = pFileEntry->dwFileSize;

        /* Biggest compressed size */
        if(pFileEntry->dwCmpSize > dwMaxCmpSize)
            dwMaxCmpSize = pFileEntry->dwCmpSize;

        /* Check if this flag was there before */
        dwFlagIndex = GetFileFlagIndex(FlagArray, pFileEntry->dwFlags);
        if(dwFlagIndex > dwMaxFlagIndex)
            dwMaxFlagIndex = dwFlagIndex;
    }

    /* Now save bit count for every piece of file information */
    pBetHeader->dwBitIndex_FilePos   = 0;
    pBetHeader->dwBitCount_FilePos   = GetNecessaryBitCount(MaxByteOffset);

    pBetHeader->dwBitIndex_FileSize  = pBetHeader->dwBitIndex_FilePos + pBetHeader->dwBitCount_FilePos;
    pBetHeader->dwBitCount_FileSize  = GetNecessaryBitCount(dwMaxFileSize);

    pBetHeader->dwBitIndex_CmpSize   = pBetHeader->dwBitIndex_FileSize + pBetHeader->dwBitCount_FileSize;
    pBetHeader->dwBitCount_CmpSize   = GetNecessaryBitCount(dwMaxCmpSize);

    pBetHeader->dwBitIndex_FlagIndex = pBetHeader->dwBitIndex_CmpSize + pBetHeader->dwBitCount_CmpSize;
    pBetHeader->dwBitCount_FlagIndex = GetNecessaryBitCount(dwMaxFlagIndex + 1);

    pBetHeader->dwBitIndex_Unknown   = pBetHeader->dwBitIndex_FlagIndex + pBetHeader->dwBitCount_FlagIndex;
    pBetHeader->dwBitCount_Unknown   = 0;

    /* Calculate the total size of one entry */
    pBetHeader->dwTableEntrySize     = pBetHeader->dwBitCount_FilePos +
                                       pBetHeader->dwBitCount_FileSize +
                                       pBetHeader->dwBitCount_CmpSize +
                                       pBetHeader->dwBitCount_FlagIndex +
                                       pBetHeader->dwBitCount_Unknown;

    /* Save the file count and flag count */
    pBetHeader->dwEntryCount         = ha->dwFileTableSize;
    pBetHeader->dwFlagCount          = dwMaxFlagIndex + 1;
    pBetHeader->dwUnknown08          = 0x10;

    /* Save the total size of the BET hash */
    pBetHeader->dwBitTotal_NameHash2 = ha->pHetTable->dwNameHashBitSize - 0x08;
    pBetHeader->dwBitExtra_NameHash2 = 0;
    pBetHeader->dwBitCount_NameHash2 = pBetHeader->dwBitTotal_NameHash2;
    pBetHeader->dwNameHashArraySize  = ((pBetHeader->dwBitTotal_NameHash2 * pBetHeader->dwEntryCount) + 7) / 8;

    /* Save the total table size */
    pBetHeader->ExtHdr.dwDataSize    =
    pBetHeader->dwTableSize          = sizeof(TMPQBetHeader) - sizeof(TMPQExtHeader) +
                                       pBetHeader->dwFlagCount * sizeof(uint32_t) +
                                     ((pBetHeader->dwTableEntrySize * pBetHeader->dwEntryCount) + 7) / 8 +
                                      pBetHeader->dwNameHashArraySize;
}

TMPQBetTable * CreateBetTable(uint32_t dwEntryCount)
{
    TMPQBetTable * pBetTable;

    /* Allocate BET table */
    pBetTable = STORM_ALLOC(TMPQBetTable, 1);
    if(pBetTable != NULL)
    {
        memset(pBetTable, 0, sizeof(TMPQBetTable));
        pBetTable->dwEntryCount = dwEntryCount;
    }

    return pBetTable;
}

static TMPQBetTable * TranslateBetTable(
    TMPQArchive * ha,
    TMPQBetHeader * pBetHeader)
{
    TMPQBetTable * pBetTable = NULL;
    unsigned char * pbSrcData = (unsigned char *)(pBetHeader + 1);
    uint32_t LengthInBytes;

    /* Sanity check */
    assert(pBetHeader->ExtHdr.dwSignature == BET_TABLE_SIGNATURE);
    assert(pBetHeader->ExtHdr.dwVersion == 1);
    assert(ha->pHetTable != NULL);
    ha = ha;

    /* Verify size of the HET table */
    if(pBetHeader->ExtHdr.dwDataSize >= (sizeof(TMPQBetHeader) - sizeof(TMPQExtHeader)))
    {
        /* Verify the size of the table in the header */
        if(pBetHeader->dwTableSize == pBetHeader->ExtHdr.dwDataSize)
        {
            /* The number of entries in the BET table must be the same like number of entries in the block table */
            assert(pBetHeader->dwEntryCount == ha->pHeader->dwBlockTableSize);
            assert(pBetHeader->dwEntryCount <= ha->dwMaxFileCount);

            /* The number of entries in the BET table must be the same like number of entries in the HET table */
            /* Note that if it's not, it is not a problem */
            //assert(pBetHeader->dwEntryCount == ha->pHetTable->dwEntryCount); */

            /* Create translated table */
            pBetTable = CreateBetTable(pBetHeader->dwEntryCount);
            if(pBetTable != NULL)
            {
                /* Copy the variables from the header to the BetTable */
                pBetTable->dwTableEntrySize     = pBetHeader->dwTableEntrySize;
                pBetTable->dwBitIndex_FilePos   = pBetHeader->dwBitIndex_FilePos;
                pBetTable->dwBitIndex_FileSize  = pBetHeader->dwBitIndex_FileSize;
                pBetTable->dwBitIndex_CmpSize   = pBetHeader->dwBitIndex_CmpSize;
                pBetTable->dwBitIndex_FlagIndex = pBetHeader->dwBitIndex_FlagIndex;
                pBetTable->dwBitIndex_Unknown   = pBetHeader->dwBitIndex_Unknown;
                pBetTable->dwBitCount_FilePos   = pBetHeader->dwBitCount_FilePos;
                pBetTable->dwBitCount_FileSize  = pBetHeader->dwBitCount_FileSize;
                pBetTable->dwBitCount_CmpSize   = pBetHeader->dwBitCount_CmpSize;
                pBetTable->dwBitCount_FlagIndex = pBetHeader->dwBitCount_FlagIndex;
                pBetTable->dwBitCount_Unknown   = pBetHeader->dwBitCount_Unknown;

                /* Since we don't know what the "unknown" is, we'll assert when it's zero */
                assert(pBetTable->dwBitCount_Unknown == 0);

                /* Allocate array for flags */
                if(pBetHeader->dwFlagCount != 0)
                {
                    /* Allocate array for file flags and load it */
                    pBetTable->pFileFlags = STORM_ALLOC(uint32_t, pBetHeader->dwFlagCount);
                    if(pBetTable->pFileFlags != NULL)
                    {
                        LengthInBytes = pBetHeader->dwFlagCount * sizeof(uint32_t);
                        memcpy(pBetTable->pFileFlags, pbSrcData, LengthInBytes);
                        BSWAP_ARRAY32_UNSIGNED(pBetTable->pFileFlags, LengthInBytes);
                        pbSrcData += LengthInBytes;
                    }

                    /* Save the number of flags */
                    pBetTable->dwFlagCount = pBetHeader->dwFlagCount;
                }

                /* Load the bit-based file table */
                pBetTable->pFileTable = CreateBitArray(pBetTable->dwTableEntrySize * pBetHeader->dwEntryCount, 0);
                LengthInBytes = (pBetTable->pFileTable->NumberOfBits + 7) / 8;
                if(pBetTable->pFileTable != NULL)
                    memcpy(pBetTable->pFileTable->Elements, pbSrcData, LengthInBytes);
                pbSrcData += LengthInBytes;

                /* Fill the sizes of BET hash */
                pBetTable->dwBitTotal_NameHash2 = pBetHeader->dwBitTotal_NameHash2;
                pBetTable->dwBitExtra_NameHash2 = pBetHeader->dwBitExtra_NameHash2;
                pBetTable->dwBitCount_NameHash2 = pBetHeader->dwBitCount_NameHash2;
                
                /* Create and load the array of BET hashes */
                pBetTable->pNameHashes = CreateBitArray(pBetTable->dwBitTotal_NameHash2 * pBetHeader->dwEntryCount, 0);
                LengthInBytes = (pBetTable->pNameHashes->NumberOfBits + 7) / 8;
                if(pBetTable->pNameHashes != NULL)
                    memcpy(pBetTable->pNameHashes->Elements, pbSrcData, LengthInBytes);
/*              pbSrcData += pBetHeader->dwNameHashArraySize; */

                /* Dump both tables */
/*              DumpHetAndBetTable(ha->pHetTable, pBetTable); */
            }
        }
    }

    return pBetTable;
}

TMPQExtHeader * TranslateBetTable2(
    TMPQArchive * ha,
    uint64_t * pcbBetTable)
{
    TMPQBetHeader * pBetHeader = NULL;
    TMPQBetHeader BetHeader;
    TFileEntry * pFileTableEnd = ha->pFileTable + ha->dwFileTableSize;
    TFileEntry * pFileEntry;
    TBitArray * pBitArray = NULL;
    unsigned char * pbLinearTable = NULL;
    unsigned char * pbTrgData;
    uint32_t LengthInBytes;
    uint32_t FlagArray[MAX_FLAG_INDEX];

    /* Calculate the bit sizes of various entries */
    InitFileFlagArray(FlagArray);
    CreateBetHeader(ha, &BetHeader);

    /* Allocate space */
    pbLinearTable = STORM_ALLOC(uint8_t, sizeof(TMPQExtHeader) + BetHeader.dwTableSize);
    if(pbLinearTable != NULL)
    {
        /* Copy the BET header to the linear buffer */
        pBetHeader = (TMPQBetHeader *)pbLinearTable;
        memcpy(pBetHeader, &BetHeader, sizeof(TMPQBetHeader));
        pbTrgData = (unsigned char *)(pBetHeader + 1);

        /* Save the bit-based block table */
        pBitArray = CreateBitArray(BetHeader.dwEntryCount * BetHeader.dwTableEntrySize, 0);
        if(pBitArray != NULL)
        {
            uint32_t dwFlagIndex = 0;
            uint32_t nBitOffset = 0;

            /* Construct the bit-based file table */
            for(pFileEntry = ha->pFileTable; pFileEntry < pFileTableEnd; pFileEntry++)
            {
                /*
                 * Note: Missing files must be included as well
                 */

                /* Save the byte offset */
                SetBits(pBitArray, nBitOffset + BetHeader.dwBitIndex_FilePos,
                                   BetHeader.dwBitCount_FilePos,
                                  &pFileEntry->ByteOffset,
                                   8);
                SetBits(pBitArray, nBitOffset + BetHeader.dwBitIndex_FileSize,
                                   BetHeader.dwBitCount_FileSize,
                                  &pFileEntry->dwFileSize,
                                   4);
                SetBits(pBitArray, nBitOffset + BetHeader.dwBitIndex_CmpSize,
                                   BetHeader.dwBitCount_CmpSize,
                                  &pFileEntry->dwCmpSize,
                                   4);

                /* Save the flag index */
                dwFlagIndex = GetFileFlagIndex(FlagArray, pFileEntry->dwFlags);
                SetBits(pBitArray, nBitOffset + BetHeader.dwBitIndex_FlagIndex,
                                   BetHeader.dwBitCount_FlagIndex,
                                  &dwFlagIndex,
                                   4);

                /* Move the bit offset */
                nBitOffset += BetHeader.dwTableEntrySize;
            }

            /* Write the array of flags */
            LengthInBytes = BetHeader.dwFlagCount * sizeof(uint32_t);
            memcpy(pbTrgData, FlagArray, LengthInBytes);
            BSWAP_ARRAY32_UNSIGNED(pbTrgData, LengthInBytes);
            pbTrgData += LengthInBytes;

            /* Write the bit-based block table */
            LengthInBytes = (pBitArray->NumberOfBits + 7) / 8;
            memcpy(pbTrgData, pBitArray->Elements, LengthInBytes);
            pbTrgData += LengthInBytes;

            /* Free the bit array */
            STORM_FREE(pBitArray);
        }

        /* Create bit array for name hashes */
        pBitArray = CreateBitArray(BetHeader.dwBitTotal_NameHash2 * BetHeader.dwEntryCount, 0);
        if(pBitArray != NULL)
        {
            uint32_t dwFileIndex = 0;

            for(pFileEntry = ha->pFileTable; pFileEntry < pFileTableEnd; pFileEntry++)
            {
                /* Insert the name hash to the bit array */
                SetBits(pBitArray, BetHeader.dwBitTotal_NameHash2 * dwFileIndex,
                                   BetHeader.dwBitCount_NameHash2,
                                  &pFileEntry->FileNameHash,
                                   8);
                
                assert(dwFileIndex < BetHeader.dwEntryCount);
                dwFileIndex++;
            }

            /* Write the array of BET hashes */
            LengthInBytes = (pBitArray->NumberOfBits + 7) / 8;
            memcpy(pbTrgData, pBitArray->Elements, LengthInBytes);
/*          pbTrgData += LengthInBytes; */

            /* Free the bit array */
            STORM_FREE(pBitArray);
        }

        /* Write the size of the BET table in the MPQ */
        if(pcbBetTable != NULL)
        {
            *pcbBetTable = (uint64_t)(sizeof(TMPQExtHeader) + BetHeader.dwTableSize);
        }
    }

    return &pBetHeader->ExtHdr;
}

void FreeBetTable(TMPQBetTable * pBetTable)
{
    if(pBetTable != NULL)
    {
        if(pBetTable->pFileTable != NULL)
            STORM_FREE(pBetTable->pFileTable);
        if(pBetTable->pFileFlags != NULL)
            STORM_FREE(pBetTable->pFileFlags);
        if(pBetTable->pNameHashes != NULL)
            STORM_FREE(pBetTable->pNameHashes);

        STORM_FREE(pBetTable);
    }
}

/*-----------------------------------------------------------------------------
 * Support for file table
 */

TFileEntry * GetFileEntryAny(TMPQArchive * ha, const char * szFileName)
{
    TMPQHash * pHash;
    uint32_t dwFileIndex;

    /* If we have HET table in the MPQ, try to find the file in HET table */
    if(ha->pHetTable != NULL)
    {
        dwFileIndex = GetFileIndex_Het(ha, szFileName);
        if(dwFileIndex != HASH_ENTRY_FREE)
            return ha->pFileTable + dwFileIndex;
    }

    /* Otherwise, perform the file search in the classic hash table */
    if(ha->pHashTable != NULL)
    {
        pHash = GetHashEntryAny(ha, szFileName);
        if(pHash != NULL && pHash->dwBlockIndex < ha->dwFileTableSize)
            return ha->pFileTable + pHash->dwBlockIndex;
    }
    
    /* Not found */
    return NULL;
}

TFileEntry * GetFileEntryLocale(TMPQArchive * ha, const char * szFileName, uint32_t lcLocale)
{
    TMPQHash * pHash;
    uint32_t dwFileIndex;

    /* If we have HET table in the MPQ, try to find the file in HET table */
    if(ha->pHetTable != NULL)
    {
        dwFileIndex = GetFileIndex_Het(ha, szFileName);
        if(dwFileIndex != HASH_ENTRY_FREE)
            return ha->pFileTable + dwFileIndex;
    }

    /* Otherwise, perform the file search in the classic hash table */
    if(ha->pHashTable != NULL)
    {
        pHash = GetHashEntryLocale(ha, szFileName, lcLocale);
        if(pHash != NULL && pHash->dwBlockIndex < ha->dwFileTableSize)
            return ha->pFileTable + pHash->dwBlockIndex;
    }
    
    /* Not found */
    return NULL;
}

TFileEntry * GetFileEntryExact(TMPQArchive * ha, const char * szFileName, uint32_t lcLocale)
{
    TMPQHash * pHash;
    uint32_t dwFileIndex;

    /* If we have HET table in the MPQ, try to find the file in HET table */
    if(ha->pHetTable != NULL)
    {
        dwFileIndex = GetFileIndex_Het(ha, szFileName);
        if(dwFileIndex != HASH_ENTRY_FREE)
            return ha->pFileTable + dwFileIndex;
    }

    /* Otherwise, perform the file search in the classic hash table */
    if(ha->pHashTable != NULL)
    {
        pHash = GetHashEntryExact(ha, szFileName, lcLocale);
        if(pHash != NULL && pHash->dwBlockIndex < ha->dwFileTableSize)
            return ha->pFileTable + pHash->dwBlockIndex;
    }
    
    /* Not found */
    return NULL;
}

TFileEntry * GetFileEntryByIndex(TMPQArchive * ha, uint32_t dwIndex)
{
    /* For MPQs with classic hash table */
    if(dwIndex < ha->dwFileTableSize)
        return ha->pFileTable + dwIndex;
    return NULL;
}

void AllocateFileName(TMPQArchive * ha, TFileEntry * pFileEntry, const char * szFileName)
{
    /* Sanity check */
    assert(pFileEntry != NULL);

    /* If the file name is pseudo file name, free it at this point */
    if(IsPseudoFileName(pFileEntry->szFileName, NULL))
    {
        if(pFileEntry->szFileName != NULL)
            STORM_FREE(pFileEntry->szFileName);
        pFileEntry->szFileName = NULL;
    }

    /* Only allocate new file name if it's not there yet */
    if(pFileEntry->szFileName == NULL)
    {
        pFileEntry->szFileName = STORM_ALLOC(char, strlen(szFileName) + 1);
        if(pFileEntry->szFileName != NULL)
            strcpy(pFileEntry->szFileName, szFileName);
    }

    /* We also need to create the file name hash */
    if(ha->pHetTable != NULL)
    {
        uint64_t AndMask64 = ha->pHetTable->AndMask64;
        uint64_t OrMask64 = ha->pHetTable->OrMask64;

        pFileEntry->FileNameHash = (HashStringJenkins(szFileName) & AndMask64) | OrMask64;
    }
}

TFileEntry * AllocateFileEntry(TMPQArchive * ha, const char * szFileName, uint32_t lcLocale)
{
    TFileEntry * pFileTableEnd;
    TFileEntry * pFileTablePtr;
    TFileEntry * pFileTable = ha->pFileTable;
    TFileEntry * pLastEntry = ha->pFileTable;
    TFileEntry * pFileEntry = NULL;
    TMPQHash * pHash;
    uint32_t dwFreeNeeded = ha->dwReservedFiles;
    uint32_t dwFreeCount = 0;

    /* The entry in the hash table should not be there. */
    /* This case must be handled by the caller */
    assert(ha->pHashTable == NULL || GetHashEntryExact(ha, szFileName, lcLocale) == NULL);
    assert(ha->pHetTable == NULL || GetFileIndex_Het(ha, szFileName) == HASH_ENTRY_FREE);

    /* Determine the end of the file table */
    pFileTableEnd = ha->pFileTable + STORMLIB_MAX(ha->dwFileTableSize, ha->dwMaxFileCount);

    /* Find the first free file entry */
    for(pFileTablePtr = pFileTable; pFileTablePtr < pFileTableEnd; pFileTablePtr++)
    {
        /* Is that entry free? */
        /* Note: Files with "delete marker" are not deleted. */
        /* Don't treat them as available */
        if((pFileTablePtr->dwFlags & MPQ_FILE_EXISTS) == 0)
        {
            if(pFileEntry == NULL)
                pFileEntry = pFileTablePtr;
            dwFreeCount++;
        }
        else
        {
            /* Update the last used entry */
            if(pFileTablePtr > pLastEntry)
                pLastEntry = pFileTablePtr;
        }
    }

    /* Did we find an usable file entry? */
    if(pFileEntry != NULL)
    {
        /* Is there enough free entries? */
        if(!(ha->dwFlags & MPQ_FLAG_SAVING_TABLES))
            dwFreeNeeded++;
        if(dwFreeCount < dwFreeNeeded)
            return NULL;

        /* Make sure that the entry is properly initialized */
        memset(pFileEntry, 0, sizeof(TFileEntry));
        pFileEntry->lcLocale = (uint16_t)lcLocale;
        AllocateFileName(ha, pFileEntry, szFileName);

        /* If the MPQ has hash table, we have to insert the new entry into the hash table */
        /* We expect it to succeed because there must be a free hash entry if there is a free file entry */
        /* Note: Don't bother with the HET table. It will be rebuilt anyway */
        if(ha->pHashTable != NULL)
        {
            pHash = AllocateHashEntry(ha, pFileEntry);
            assert(pHash != NULL);
        }

        /* Update the file table size */
        if(pFileEntry >= pLastEntry)
            pLastEntry = pFileEntry;
        ha->dwFileTableSize = (uint32_t)(pLastEntry - pFileTable) + 1;
    }

    /* Return the file entry */
    return pFileEntry;
}

int RenameFileEntry(
    TMPQArchive * ha,
    TFileEntry * pFileEntry,
    const char * szNewFileName)
{
    TMPQHash * pHash;

    /* Mark the entry as deleted in the hash table */
    if(ha->pHashTable != NULL)
    {
        assert(pFileEntry->dwHashIndex < ha->dwHashTableSize);

        pHash = ha->pHashTable + pFileEntry->dwHashIndex;
        memset(pHash, 0xFF, sizeof(TMPQHash));
        pHash->dwBlockIndex = HASH_ENTRY_DELETED;
    }

    /*
     * Note: Don't bother with the HET table.
     * It will be rebuilt from scratch anyway
     */

    /* Free the old file name */
    if(pFileEntry->szFileName != NULL)
        STORM_FREE(pFileEntry->szFileName);
    pFileEntry->szFileName = NULL;

    /* Allocate new file name */
    AllocateFileName(ha, pFileEntry, szNewFileName);

    /* Now find a hash entry for the new file name */
    if(ha->pHashTable != NULL)
    {
        /* Try to find the hash table entry for the new file name */
        /* Note: Since we deleted one hash entry, this will always succeed */
        pHash = AllocateHashEntry(ha, pFileEntry);
        assert(pHash != NULL);
    }

    /* Invalidate the entries for (listfile) and (attributes) */
    /* After we are done with MPQ changes, we need to re-create them */
    InvalidateInternalFiles(ha);
    return ERROR_SUCCESS;
}

void DeleteFileEntry(
    TMPQArchive * ha,
    TFileEntry * pFileEntry)
{
    TMPQHash * pHash;

    /* If the MPQ has classic hash table, clear the entry there */
    if(ha->pHashTable != NULL)
    {
        /* Only if the file entry is still an existing one */
        if(pFileEntry->dwFlags & MPQ_FILE_EXISTS)
        {
            /* We expect dwHashIndex to be within the hash table */
            pHash = ha->pHashTable + pFileEntry->dwHashIndex;
            assert(pFileEntry->dwHashIndex < ha->dwHashTableSize);

            /* Set the hash table entry as deleted */
            pHash->dwName1      = 0xFFFFFFFF;
            pHash->dwName2      = 0xFFFFFFFF;
            pHash->lcLocale     = 0xFFFF;
            pHash->wPlatform    = 0xFFFF;
            pHash->dwBlockIndex = HASH_ENTRY_DELETED;
        }
    }

    /* Free the file name, and set the file entry as deleted */
    if(pFileEntry->szFileName != NULL)
        STORM_FREE(pFileEntry->szFileName);
    pFileEntry->szFileName = NULL;

    /*
     * Don't modify the HET table, because it gets recreated from scratch at every modification operation
     * Don't decrement the number of entries in the file table
     * Keep Byte Offset, file size and compressed size in the file table
     * Also keep the CRC32 and MD5 in the file attributes
     * Clear the file name hash
     * Clear the MPQ_FILE_EXISTS bit.
     */

    pFileEntry->FileNameHash = 0;
    pFileEntry->dwFlags &= ~MPQ_FILE_EXISTS;
}

void InvalidateInternalFiles(TMPQArchive * ha)
{
    TFileEntry * pFileEntry1 = NULL;
    TFileEntry * pFileEntry2 = NULL;
    TFileEntry * pFileEntry3 = NULL;

    /* Do nothing if we are in the middle of saving internal files */
    if(!(ha->dwFlags & MPQ_FLAG_SAVING_TABLES))
    {
        /*
         * We clear the file entries of (listfile) and (attributes)
         * For each internal file cleared, we increment the number 
         * of reserved entries in the file table.
         */

        /* Invalidate the (listfile), if not done yet */
        if((ha->dwFlags & MPQ_FLAG_LISTFILE_INVALID) == 0)
        {
            /* If the (listfile) is not there but will be, reserve one entry */
            pFileEntry1 = GetFileEntryExact(ha, LISTFILE_NAME, LANG_NEUTRAL);
            if(pFileEntry1 == NULL && ha->dwFileFlags1)
                ha->dwReservedFiles++;
            
            /* Mark the (listfile) as invalid */
            ha->dwFlags |= MPQ_FLAG_LISTFILE_INVALID;
        }

        /* Invalidate the (attributes), if not done yet */
        if((ha->dwFlags & MPQ_FLAG_ATTRIBUTES_INVALID) == 0)
        {
            /* If the (attributes) is not there but will be, reserve one entry */
            pFileEntry2 = GetFileEntryExact(ha, ATTRIBUTES_NAME, LANG_NEUTRAL);
            if(pFileEntry2 == NULL && ha->dwFileFlags2)
                ha->dwReservedFiles++;

            /* Mark (attributes) as invalid */
            ha->dwFlags |= MPQ_FLAG_ATTRIBUTES_INVALID;
        }

        /* Invalidate the (signature), if not done yet */
        if((ha->dwFlags & MPQ_FLAG_SIGNATURE_INVALID) == 0)
        {
            /* If the (signature) is not there but will be, reserve one entry */
            pFileEntry3 = GetFileEntryExact(ha, SIGNATURE_NAME, LANG_NEUTRAL);
            if(pFileEntry3 == NULL && ha->dwFileFlags3)
                ha->dwReservedFiles++;

            /* Mark (signature) as invalid */
            ha->dwFlags |= MPQ_FLAG_SIGNATURE_INVALID;
        }

        /* Remember that the MPQ has been changed */
        ha->dwFlags |= MPQ_FLAG_CHANGED;
    }
}

/*-----------------------------------------------------------------------------
 * Support for file tables - hash table, block table, hi-block table
 */

int CreateHashTable(TMPQArchive * ha, uint32_t dwHashTableSize)
{
    TMPQHash * pHashTable;

    /* Sanity checks */
    assert((dwHashTableSize & (dwHashTableSize - 1)) == 0);
    assert(ha->pHashTable == NULL);

    /* If the required hash table size is zero, don't create anything */
    if(dwHashTableSize == 0)
        dwHashTableSize = HASH_TABLE_SIZE_DEFAULT;

    /* Create the hash table */
    pHashTable = STORM_ALLOC(TMPQHash, dwHashTableSize);
    if(pHashTable == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Fill it */
    memset(pHashTable, 0xFF, dwHashTableSize * sizeof(TMPQHash));
    ha->dwHashTableSize = dwHashTableSize;
    ha->dwMaxFileCount = dwHashTableSize;
    ha->pHashTable = pHashTable;
    return ERROR_SUCCESS;
}

TMPQHash * LoadHashTable(TMPQArchive * ha)
{
    TMPQHeader * pHeader = ha->pHeader;
    uint64_t ByteOffset;
    TMPQHash * pHashTable = NULL;
    uint32_t dwTableSize;
    uint32_t dwCmpSize;
    int bHashTableIsCut = 0;

    /* If the MPQ has no hash table, do nothing */
    if(pHeader->dwHashTablePos == 0 && pHeader->wHashTablePosHi == 0)
        return NULL;

    /* If the hash table size is zero, do nothing */
    if(pHeader->dwHashTableSize == 0)
        return NULL;

    /* Load the hash table for MPQ variations */
    switch(ha->dwSubType)
    {
        case MPQ_SUBTYPE_MPQ:

            /* Calculate the position and size of the hash table */
            ByteOffset = FileOffsetFromMpqOffset(ha, MAKE_OFFSET64(pHeader->wHashTablePosHi, pHeader->dwHashTablePos));
            dwTableSize = pHeader->dwHashTableSize * sizeof(TMPQHash);
            dwCmpSize = (uint32_t)pHeader->HashTableSize64;

            /* Read, decrypt and uncompress the hash table */
            pHashTable = (TMPQHash *)LoadMpqTable(ha, ByteOffset, dwCmpSize, dwTableSize, MPQ_KEY_HASH_TABLE, &bHashTableIsCut);

            /* If the hash table was cut, we need to remember it */
            if(pHashTable != NULL && bHashTableIsCut)
                ha->dwFlags |= (MPQ_FLAG_MALFORMED | MPQ_FLAG_HASH_TABLE_CUT);
            break;

        case MPQ_SUBTYPE_SQP:
            pHashTable = LoadSqpHashTable(ha);
            break;

        case MPQ_SUBTYPE_MPK:
            pHashTable = LoadMpkHashTable(ha);
            break;
    }

    /* Remember the size of the hash table */
    return pHashTable;
}

TMPQBlock * LoadBlockTable(TMPQArchive * ha /* , int bDontFixEntries */)
{
    TMPQHeader * pHeader = ha->pHeader;
    TMPQBlock * pBlockTable = NULL;
    uint64_t ByteOffset;
    uint32_t dwTableSize;
    uint32_t dwCmpSize;
    int bBlockTableIsCut = 0;

    /* Do nothing if the block table position is zero */
    if(pHeader->dwBlockTablePos == 0 && pHeader->wBlockTablePosHi == 0)
        return NULL;

    /* Do nothing if the block table size is zero */
    if(pHeader->dwBlockTableSize == 0)
        return NULL;

    /* Load the block table for MPQ variations */
    switch(ha->dwSubType)
    {
        case MPQ_SUBTYPE_MPQ:

            /* Calculate byte position of the block table */
            ByteOffset = FileOffsetFromMpqOffset(ha, MAKE_OFFSET64(pHeader->wBlockTablePosHi, pHeader->dwBlockTablePos));
            dwTableSize = pHeader->dwBlockTableSize * sizeof(TMPQBlock);
            dwCmpSize = (uint32_t)pHeader->BlockTableSize64;

            /* Read, decrypt and uncompress the block table */
            pBlockTable = (TMPQBlock * )LoadMpqTable(ha, ByteOffset, dwCmpSize, dwTableSize, MPQ_KEY_BLOCK_TABLE, &bBlockTableIsCut);
           
            /* If the block table was cut, we need to remember it */
            if(pBlockTable != NULL && bBlockTableIsCut)
                ha->dwFlags |= (MPQ_FLAG_MALFORMED | MPQ_FLAG_BLOCK_TABLE_CUT);
            break;

        case MPQ_SUBTYPE_SQP:

            pBlockTable = LoadSqpBlockTable(ha);
            break;

        case MPQ_SUBTYPE_MPK:

            pBlockTable = LoadMpkBlockTable(ha);
            break;
    }

    return pBlockTable;
}

TMPQHetTable * LoadHetTable(TMPQArchive * ha)
{
    TMPQExtHeader * pExtTable;
    TMPQHetTable * pHetTable = NULL;
    TMPQHeader * pHeader = ha->pHeader;

    /* If the HET table position is not 0, we expect the table to be present */
    if(pHeader->HetTablePos64 != 0 && pHeader->HetTableSize64 != 0)
    {
        /* Attempt to load the HET table (Hash Extended Table) */
        pExtTable = LoadExtTable(ha, pHeader->HetTablePos64, (size_t)pHeader->HetTableSize64, HET_TABLE_SIGNATURE, MPQ_KEY_HASH_TABLE);
        if(pExtTable != NULL)
        {
            /* If loading HET table fails, we ignore the result. */
            pHetTable = TranslateHetTable((TMPQHetHeader *)pExtTable);
            STORM_FREE(pExtTable);
        }
    }

    return pHetTable;
}

TMPQBetTable * LoadBetTable(TMPQArchive * ha)
{
    TMPQExtHeader * pExtTable;
    TMPQBetTable * pBetTable = NULL;
    TMPQHeader * pHeader = ha->pHeader;

    /* If the BET table position is not 0, we expect the table to be present */
    if(pHeader->BetTablePos64 != 0 && pHeader->BetTableSize64 != 0)
    {
        /* Attempt to load the HET table (Hash Extended Table) */
        pExtTable = LoadExtTable(ha, pHeader->BetTablePos64, (size_t)pHeader->BetTableSize64, BET_TABLE_SIGNATURE, MPQ_KEY_BLOCK_TABLE);
        if(pExtTable != NULL)
        {
            /* If succeeded, we translate the BET table */
            /* to more readable form */
            pBetTable = TranslateBetTable(ha, (TMPQBetHeader *)pExtTable);
            STORM_FREE(pExtTable);
        }
    }

    return pBetTable;
}

int LoadAnyHashTable(TMPQArchive * ha)
{
    TMPQHeader * pHeader = ha->pHeader;

    /* If the MPQ archive is empty, don't bother trying to load anything */
    if(pHeader->dwHashTableSize == 0 && pHeader->HetTableSize64 == 0)
        return CreateHashTable(ha, HASH_TABLE_SIZE_DEFAULT);

    /* Try to load HET table */
    if(pHeader->HetTablePos64 != 0)
        ha->pHetTable = LoadHetTable(ha);

    /* Try to load the hash table */
    if(pHeader->wHashTablePosHi || pHeader->dwHashTablePos)
        ha->pHashTable = LoadHashTable(ha);

    /* At least one of the tables must be present */
    if(ha->pHetTable == NULL && ha->pHashTable == NULL)
        return ERROR_FILE_CORRUPT;

    /* Set the maximum file count to the size of the hash table. */
    /* Note: We don't care about HET table limits, because HET table is rebuilt */
    /* after each file add/rename/delete. */
    ha->dwMaxFileCount = (ha->pHashTable != NULL) ? pHeader->dwHashTableSize : HASH_TABLE_SIZE_MAX;
    ha->dwHashTableSize = pHeader->dwHashTableSize;
    return ERROR_SUCCESS;
}

static int BuildFileTable_Classic(
    TMPQArchive * ha,
    TFileEntry * pFileTable)
{
    TFileEntry * pFileEntry;
    TMPQHeader * pHeader = ha->pHeader;
    TMPQBlock * pBlockTable;
    int nError = ERROR_SUCCESS;

    /* Sanity checks */
    assert(ha->pHashTable != NULL);

    /* If the MPQ has no block table, do nothing */
    if(ha->pHeader->dwBlockTableSize == 0)
        return ERROR_SUCCESS;

    /* Load the block table */
    pBlockTable = (TMPQBlock *)LoadBlockTable(ha);
    if(pBlockTable != NULL)
    {
        /* If we don't have HET table, we build the file entries from the hash&block tables */
        /* If we do, we only update it from the hash table */
        if(ha->pHetTable == NULL)
        {
            nError = BuildFileTableFromBlockTable(ha, pFileTable, pBlockTable);
        }
        else
        {
            nError = UpdateFileTableFromHashTable(ha, pFileTable);
        }

        /* Free the block table */
        STORM_FREE(pBlockTable);
    }
    else
    {
        nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Load the hi-block table */
    if(nError == ERROR_SUCCESS && pHeader->HiBlockTablePos64 != 0)
    {
        uint64_t ByteOffset;
        uint16_t * pHiBlockTable = NULL;
        uint32_t dwTableSize = pHeader->dwBlockTableSize * sizeof(uint16_t);

        /* Allocate space for the hi-block table */
        /* Note: pHeader->dwBlockTableSize can be zero !!! */
        pHiBlockTable = STORM_ALLOC(uint16_t, pHeader->dwBlockTableSize + 1);
        if(pHiBlockTable != NULL)
        {
            /* Load the hi-block table. It is not encrypted, nor compressed */
            ByteOffset = ha->MpqPos + pHeader->HiBlockTablePos64;
            if(!FileStream_Read(ha->pStream, &ByteOffset, pHiBlockTable, dwTableSize))
                nError = GetLastError();

            /* Now merge the hi-block table to the file table */
            if(nError == ERROR_SUCCESS)
            {
                BSWAP_ARRAY16_UNSIGNED(pHiBlockTable, dwTableSize);
                pFileEntry = pFileTable;
                uint32_t i;

                /* Add the high file offset to the base file offset. */
                for(i = 0; i < pHeader->dwBlockTableSize; i++)
                {
                    pFileEntry->ByteOffset = MAKE_OFFSET64(pHiBlockTable[i], pFileEntry->ByteOffset);
                    pFileEntry++;
                }
            }

            /* Free the hi-block table */
            STORM_FREE(pHiBlockTable);
        }
        else
        {
            nError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    /* Set the current size of the file table */
    ha->dwFileTableSize = pHeader->dwBlockTableSize;
    return nError;
}

static int BuildFileTable_HetBet(
    TMPQArchive * ha,
    TFileEntry * pFileTable)
{
    TMPQHetTable * pHetTable = ha->pHetTable;
    TMPQBetTable * pBetTable;
    TFileEntry * pFileEntry = pFileTable;
    TBitArray * pBitArray;
    uint32_t dwBitPosition = 0;
    uint32_t i;
    int nError = ERROR_FILE_CORRUPT;

    /* Load the BET table from the MPQ */
    pBetTable = LoadBetTable(ha);
    if(pBetTable != NULL)
    {
        /* Verify the size of NameHash2 in the BET table. */
        /* It has to be 8 bits less than the information in HET table */
        if((pBetTable->dwBitCount_NameHash2 + 8) != pHetTable->dwNameHashBitSize)
            return ERROR_FILE_CORRUPT;
        
        /* Step one: Fill the name indexes */
        for(i = 0; i < pHetTable->dwTotalCount; i++)
        {
            uint32_t dwFileIndex = 0;

            /* Is the entry in the HET table occupied? */
            if(pHetTable->pNameHashes[i] != 0)
            {
                /* Load the index to the BET table */
                GetBits(pHetTable->pBetIndexes, pHetTable->dwIndexSizeTotal * i,
                                                pHetTable->dwIndexSize,
                                               &dwFileIndex,
                                                4);
                /* Overflow test */
                if(dwFileIndex < pBetTable->dwEntryCount)
                {
                    uint64_t NameHash1 = pHetTable->pNameHashes[i];
                    uint64_t NameHash2 = 0;

                    /* Load the BET hash */
                    GetBits(pBetTable->pNameHashes, pBetTable->dwBitTotal_NameHash2 * dwFileIndex,
                                                    pBetTable->dwBitCount_NameHash2,
                                                   &NameHash2,
                                                    8);

                    /* Combine both part of the name hash and put it to the file table */
                    pFileEntry = pFileTable + dwFileIndex;
                    pFileEntry->FileNameHash = (NameHash1 << pBetTable->dwBitCount_NameHash2) | NameHash2;
                }
            }
        }

        /* Go through the entire BET table and convert it to the file table. */
        pFileEntry = pFileTable;
        pBitArray = pBetTable->pFileTable; 
        for(i = 0; i < pBetTable->dwEntryCount; i++)
        {
            uint32_t dwFlagIndex = 0;

            /* Read the file position */
            GetBits(pBitArray, dwBitPosition + pBetTable->dwBitIndex_FilePos,
                               pBetTable->dwBitCount_FilePos,
                              &pFileEntry->ByteOffset,
                               8);

            /* Read the file size */
            GetBits(pBitArray, dwBitPosition + pBetTable->dwBitIndex_FileSize,
                               pBetTable->dwBitCount_FileSize,
                              &pFileEntry->dwFileSize,
                               4);

            /* Read the compressed size */
            GetBits(pBitArray, dwBitPosition + pBetTable->dwBitIndex_CmpSize,
                               pBetTable->dwBitCount_CmpSize,
                              &pFileEntry->dwCmpSize,
                               4);


            /* Read the flag index */
            if(pBetTable->dwFlagCount != 0)
            {
                GetBits(pBitArray, dwBitPosition + pBetTable->dwBitIndex_FlagIndex,
                                   pBetTable->dwBitCount_FlagIndex,
                                  &dwFlagIndex,
                                   4);
                pFileEntry->dwFlags = pBetTable->pFileFlags[dwFlagIndex];
            }

            /*
             * TODO: Locale (?)
             */

            /* Move the current bit position */
            dwBitPosition += pBetTable->dwTableEntrySize;
            pFileEntry++;
        }

        /* Set the current size of the file table */
        ha->dwFileTableSize = pBetTable->dwEntryCount;
        FreeBetTable(pBetTable);
        nError = ERROR_SUCCESS;
    }
    else
    {
        nError = ERROR_FILE_CORRUPT;
    }

    return nError;
}

int BuildFileTable(TMPQArchive * ha)
{
    TFileEntry * pFileTable;
    uint32_t dwFileTableSize;
    int bFileTableCreated = 0;

    /* Sanity checks */
    assert(ha->dwFileTableSize == 0);
    assert(ha->dwMaxFileCount != 0);

    /* Determine the allocation size for the file table */
    dwFileTableSize = STORMLIB_MAX(ha->pHeader->dwBlockTableSize, ha->dwMaxFileCount);

    /* Allocate the file table with size determined before */
    pFileTable = STORM_ALLOC(TFileEntry, dwFileTableSize);
    if(pFileTable == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Fill the table with zeros */
    memset(pFileTable, 0, dwFileTableSize * sizeof(TFileEntry));

    /* If we have HET table, we load file table from the BET table */
    /* Note: If BET table is corrupt or missing, we set the archive as read only */
    if(ha->pHetTable != NULL)
    {
        if(BuildFileTable_HetBet(ha, pFileTable) != ERROR_SUCCESS)
            ha->dwFlags |= MPQ_FLAG_READ_ONLY;
        else
            bFileTableCreated = 1;
    }

    /* If we have hash table, we load the file table from the block table */
    /* Note: If block table is corrupt or missing, we set the archive as read only */
    if(ha->pHashTable != NULL)
    {
        if(BuildFileTable_Classic(ha, pFileTable) != ERROR_SUCCESS)
            ha->dwFlags |= MPQ_FLAG_READ_ONLY;
        else
            bFileTableCreated = 1;
    }
    
    /* If something failed, we free the file table entry */
    if(!bFileTableCreated)
    {
        STORM_FREE(pFileTable);
        return ERROR_FILE_CORRUPT;
    }

    /* Assign it to the archive structure */
    ha->pFileTable = pFileTable;
    return ERROR_SUCCESS;
}

/* Defragment the file table so it does not contain any gaps */
int DefragmentFileTable(TMPQArchive * ha)
{
    TFileEntry * pFileTable = ha->pFileTable;
    TFileEntry * pSrcEntry = pFileTable;
    TFileEntry * pTrgEntry = pFileTable;
    uint32_t i;

    /* Parse the file entries */
    for(i = 0; i < ha->dwFileTableSize; i++, pSrcEntry++)
    {
        /* Is the source file entry does not contain anything valid, we skip it */
        if(pSrcEntry->dwFlags & MPQ_FILE_EXISTS)
        {
            /* Fix the block table index and move the entry */
            if(pTrgEntry != pSrcEntry)
            {
                if(ha->pHashTable != NULL)
                    ha->pHashTable[pSrcEntry->dwHashIndex].dwBlockIndex = (uint32_t)(pTrgEntry - pFileTable);
                memmove(pTrgEntry, pSrcEntry, sizeof(TFileEntry));
            }

            /* Move the target pointer by one entry */
            pTrgEntry++;
        }
    }

    /* Store the new size of the file table */
    ha->dwFileTableSize = (uint32_t)(pTrgEntry - pFileTable);

    /* Fill the rest with zeros */
    if(ha->dwFileTableSize < ha->dwMaxFileCount)
        memset(pFileTable + ha->dwFileTableSize, 0, (ha->dwMaxFileCount - ha->dwFileTableSize) * sizeof(TFileEntry));
    return ERROR_SUCCESS;
}

/* Performs final validation (and possible defragmentation) */
/* of malformed hash+block tables */
int ShrinkMalformedMpqTables(TMPQArchive * ha)
{
    TFileEntry * pFileTable = ha->pFileTable;
    TMPQHash * pHashTable = ha->pHashTable;
    uint32_t i;

    /* Sanity checks */
    assert(ha->pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1);
    assert(ha->pHashTable != NULL);
    assert(ha->pFileTable != NULL);

    /*
     * Note: Unhandled possible case where multiple hash table entries
     * point to the same block tabl entry.
     */

    /* Now perform the defragmentation of the block table */
    if(ha->dwFlags & MPQ_FLAG_BLOCK_TABLE_CUT)
    {
        assert(ha->dwFileTableSize == ha->pHeader->dwBlockTableSize);
        DefragmentFileTable(ha);
    }

    /* If the hash table has been cut, we need to defragment it */
    /* This will greatly improve performance on searching and loading listfile */
    if(ha->dwFlags & MPQ_FLAG_HASH_TABLE_CUT)
    {
        TMPQHash * pSrcEntry = ha->pHashTable;
        TMPQHash * pTrgEntry = ha->pHashTable;
        uint32_t dwNewEntryCount;
        uint32_t dwNewTableSize;

        assert(ha->dwHashTableSize == ha->pHeader->dwHashTableSize);

        /* Defragment the hash table */
        for(i = 0; i < ha->dwHashTableSize; i++, pSrcEntry++)
        {
            /* If the entry is valid, move it */
            if(pSrcEntry->dwBlockIndex < ha->dwFileTableSize)
            {
                /* Fix the hash table index in the file table and move the entry */
                if(pTrgEntry != pSrcEntry)
                {
                    pFileTable[pSrcEntry->dwBlockIndex].dwHashIndex = (uint32_t)(pTrgEntry - pHashTable);
                    *pTrgEntry = *pSrcEntry;
                }

                /* Increment the number of used entries */
                pTrgEntry++;
            }
        }

        /* Calculate the new hash table size */
        dwNewEntryCount = (uint32_t)(pTrgEntry - pHashTable);
        dwNewTableSize = GetHashTableSizeForFileCount(dwNewEntryCount);

        /* Mark the extra entries in the hash table as deleted */
        /* This causes every hash search to go over the entire hash table, */
        /* but if the hash table is cut, it would do it anyway */
        memset(pHashTable + dwNewEntryCount, 0xFF, (dwNewTableSize - dwNewEntryCount) * sizeof(TMPQHash));
        for(i = dwNewEntryCount; i < dwNewTableSize; i++)
            pHashTable[i].dwBlockIndex = HASH_ENTRY_DELETED;

        /* Change the hash table size in the header */
        ha->dwHashTableSize = dwNewTableSize;
    }

    return ERROR_SUCCESS;
}

/* Rebuilds the HET table from scratch based on the file table */
/* Used after a modifying operation (add, rename, delete) */
int RebuildHetTable(TMPQArchive * ha)
{
    TMPQHetTable * pOldHetTable = ha->pHetTable;
    uint64_t FileNameHash;
    uint32_t i;
    int nError = ERROR_SUCCESS;

    /* Create new HET table based on the total number of entries in the file table */
    /* Note that if we fail to create it, we just stop using HET table */
    ha->pHetTable = CreateHetTable(ha->dwFileTableSize, 0, 0x40, NULL);
    if(ha->pHetTable != NULL)
    {
        /* Go through the file table again and insert all existing files */
        for(i = 0; i < ha->dwFileTableSize; i++)
        {
            if(ha->pFileTable[i].dwFlags & MPQ_FILE_EXISTS)
            {
                /* Get the high */
                FileNameHash = ha->pFileTable[i].FileNameHash;
                nError = InsertHetEntry(ha->pHetTable, FileNameHash, i);
                if(nError != ERROR_SUCCESS)
                    break;
            }
        }
    }

    /* Free the old HET table */
    FreeHetTable(pOldHetTable);
    return nError;
}

/* Rebuilds the file table, removing all deleted file entries. */
/* Used when compacting the archive */
int RebuildFileTable(TMPQArchive * ha, uint32_t dwNewHashTableSize, uint32_t dwNewMaxFileCount)
{
    TFileEntry * pOldFileTableEnd = ha->pFileTable + ha->dwFileTableSize;
    TFileEntry * pOldFileTable = ha->pFileTable;
    TFileEntry * pFileTable;
    TFileEntry * pFileEntry;
    TFileEntry * pOldEntry;
    TMPQHash * pOldHashTable = ha->pHashTable;
    TMPQHash * pHashTable = NULL;
    TMPQHash * pHash;
    int nError = ERROR_SUCCESS;

    /* The new hash table size must be greater or equal to the current hash table size */
    assert(dwNewHashTableSize >= ha->dwHashTableSize);

    /* The new hash table size must be a power of two */
    assert((dwNewHashTableSize & (dwNewHashTableSize - 1)) == 0);

    /* Allocate the new file table */
    pFileTable = pFileEntry = STORM_ALLOC(TFileEntry, dwNewMaxFileCount);
    if(pFileTable == NULL)
        nError = ERROR_NOT_ENOUGH_MEMORY;

    /* Allocate new hash table */
    if(nError == ERROR_SUCCESS && ha->pHashTable != NULL)
    {
        pHashTable = STORM_ALLOC(TMPQHash, dwNewHashTableSize);
        if(pHashTable == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    /* If both succeeded, we need to rebuild the file table */
    if(nError == ERROR_SUCCESS)
    {
        /* Make sure that the hash table is properly filled */
        memset(pFileTable, 0x00, sizeof(TFileEntry) * dwNewMaxFileCount);
        memset(pHashTable, 0xFF, sizeof(TMPQHash) * dwNewHashTableSize);
        
        /* Set the new tables to the MPQ archive */
        ha->pFileTable = pFileTable;
        ha->pHashTable = pHashTable;

        /* Set the new limits to the MPQ archive */
        ha->dwHashTableSize = dwNewHashTableSize;
        ha->dwMaxFileCount = dwNewMaxFileCount;

        /* Now copy all the file entries */
        for(pOldEntry = pOldFileTable; pOldEntry < pOldFileTableEnd; pOldEntry++)
        {
            /* If the file entry exists, we copy it to the new table */
            /* Otherwise, we skip it */
            if(pOldEntry->dwFlags & MPQ_FILE_EXISTS)
            {
                /* Copy the file entry */
                *pFileEntry = *pOldEntry;

                /* Create hash entry for it */
                if(ha->pHashTable != NULL)
                {
                    pHash = AllocateHashEntry(ha, pFileEntry);
                    if(pHash == NULL)
                    {
                        nError = ERROR_DISK_FULL;
                        break;
                    }
                }

                /* Move the file entry by one */
                pFileEntry++;
            }
        }

        /* Update the file table size */
        ha->dwFileTableSize = (uint32_t)(pFileEntry - pFileTable);
        ha->dwFlags |= MPQ_FLAG_CHANGED;
        pFileTable = NULL;
    }

    /* Now free the remaining entries */
    if(pOldFileTable != NULL)
        STORM_FREE(pOldFileTable);
    if(pOldHashTable != NULL)
        STORM_FREE(pOldHashTable);
    if(pFileTable != NULL)
        STORM_FREE(pFileTable);
    return nError;
}

/* Saves MPQ header, hash table, block table and hi-block table. */
int SaveMPQTables(TMPQArchive * ha)
{
    TMPQExtHeader * pHetTable = NULL;
    TMPQExtHeader * pBetTable = NULL;
    TMPQHeader * pHeader = ha->pHeader;
    TMPQBlock * pBlockTable = NULL;
    TMPQHash * pHashTable = NULL;
    uint64_t HetTableSize64 = 0;
    uint64_t BetTableSize64 = 0;
    uint64_t HashTableSize64 = 0;
    uint64_t BlockTableSize64 = 0;
    uint64_t HiBlockTableSize64 = 0;
    uint64_t TablePos = 0;             /* A table position, relative to the begin of the MPQ */
    uint16_t * pHiBlockTable = NULL;
    uint32_t cbTotalSize;
    int bNeedHiBlockTable = 0;
    int nError = ERROR_SUCCESS;

    /* We expect this function to be called only when tables have been changed */
    assert(ha->dwFlags & MPQ_FLAG_CHANGED);

    /* Find the space where the MPQ tables will be saved */
    TablePos = FindFreeMpqSpace(ha);

    /* If the MPQ has HET table, we prepare a ready-to-save version */
    if(nError == ERROR_SUCCESS && ha->pHetTable != NULL)
    {
        pHetTable = TranslateHetTable2(ha->pHetTable, &HetTableSize64);
        if(pHetTable == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    /* If the MPQ has HET table, we also must create BET table to be saved */
    if(nError == ERROR_SUCCESS && ha->pHetTable != NULL)
    {
        pBetTable = TranslateBetTable2(ha, &BetTableSize64);
        if(pBetTable == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Now create hash table */
    if(nError == ERROR_SUCCESS && ha->pHashTable != NULL)
    {
        pHashTable = TranslateHashTable(ha, &HashTableSize64);
        if(pHashTable == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Create block table */
    if(nError == ERROR_SUCCESS && ha->pFileTable != NULL)
    {
        pBlockTable = TranslateBlockTable(ha, &BlockTableSize64, &bNeedHiBlockTable);
        if(pBlockTable == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Create hi-block table, if needed */
    if(nError == ERROR_SUCCESS && bNeedHiBlockTable)
    {
        pHiBlockTable = TranslateHiBlockTable(ha, &HiBlockTableSize64);
        if(pHiBlockTable == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Write the HET table, if any */
    if(nError == ERROR_SUCCESS && pHetTable != NULL)
    {
        pHeader->HetTableSize64 = HetTableSize64;
        pHeader->HetTablePos64  = TablePos;
        nError = SaveExtTable(ha, pHetTable, TablePos, (uint32_t)HetTableSize64, pHeader->MD5_HetTable, MPQ_KEY_HASH_TABLE, 0, &cbTotalSize);
        TablePos += cbTotalSize;
    }

    /* Write the BET table, if any */
    if(nError == ERROR_SUCCESS && pBetTable != NULL)
    {
        pHeader->BetTableSize64 = BetTableSize64;
        pHeader->BetTablePos64  = TablePos;
        nError = SaveExtTable(ha, pBetTable, TablePos, (uint32_t)BetTableSize64, pHeader->MD5_BetTable, MPQ_KEY_BLOCK_TABLE, 0, &cbTotalSize);
        TablePos += cbTotalSize;
    }

    /* Write the hash table, if we have any */
    if(nError == ERROR_SUCCESS && pHashTable != NULL)
    {
        pHeader->HashTableSize64 = HashTableSize64;
        pHeader->wHashTablePosHi = (uint16_t)(TablePos >> 32);
        pHeader->dwHashTableSize = (uint32_t)(HashTableSize64 / sizeof(TMPQHash));
        pHeader->dwHashTablePos = (uint32_t)TablePos;
        nError = SaveMpqTable(ha, pHashTable, TablePos, (size_t)HashTableSize64, pHeader->MD5_HashTable, MPQ_KEY_HASH_TABLE, 0);
        TablePos += HashTableSize64;
    }

    /* Write the block table, if we have any */
    if(nError == ERROR_SUCCESS && pBlockTable != NULL)
    {
        pHeader->BlockTableSize64 = BlockTableSize64;
        pHeader->wBlockTablePosHi = (uint16_t)(TablePos >> 32);
        pHeader->dwBlockTableSize = (uint32_t)(BlockTableSize64 / sizeof(TMPQBlock));
        pHeader->dwBlockTablePos = (uint32_t)TablePos;
        nError = SaveMpqTable(ha, pBlockTable, TablePos, (size_t)BlockTableSize64, pHeader->MD5_BlockTable, MPQ_KEY_BLOCK_TABLE, 0);
        TablePos += BlockTableSize64;
    }

    /* Write the hi-block table, if we have any */
    if(nError == ERROR_SUCCESS && pHiBlockTable != NULL)
    {
        uint64_t ByteOffset = ha->MpqPos + TablePos;

        pHeader->HiBlockTableSize64 = HiBlockTableSize64;
        pHeader->HiBlockTablePos64 = TablePos;
        BSWAP_ARRAY16_UNSIGNED(pHiBlockTable, HiBlockTableSize64);
        
        if(!FileStream_Write(ha->pStream, &ByteOffset, pHiBlockTable, (uint32_t)HiBlockTableSize64))
            nError = GetLastError();
        TablePos += HiBlockTableSize64;
    }

    /* Cut the MPQ */
    if(nError == ERROR_SUCCESS)
    {
        uint64_t FileSize = ha->MpqPos + TablePos;

        if(!FileStream_SetSize(ha->pStream, FileSize))
            nError = GetLastError();
    }

    /* Write the MPQ header */
    if(nError == ERROR_SUCCESS)
    {
        TMPQHeader SaveMpqHeader;

        /* Update the size of the archive */
        pHeader->ArchiveSize64 = TablePos;
        pHeader->dwArchiveSize = (uint32_t)TablePos;
        
        /* Update the MD5 of the archive header */
        CalculateDataBlockHash(pHeader, MPQ_HEADER_SIZE_V4 - MD5_DIGEST_SIZE, pHeader->MD5_MpqHeader);

        /* Write the MPQ header to the file */
        memcpy(&SaveMpqHeader, pHeader, pHeader->dwHeaderSize);
        BSWAP_TMPQHEADER(&SaveMpqHeader, MPQ_FORMAT_VERSION_1);
        BSWAP_TMPQHEADER(&SaveMpqHeader, MPQ_FORMAT_VERSION_2);
        BSWAP_TMPQHEADER(&SaveMpqHeader, MPQ_FORMAT_VERSION_3);
        BSWAP_TMPQHEADER(&SaveMpqHeader, MPQ_FORMAT_VERSION_4);
        if(!FileStream_Write(ha->pStream, &ha->MpqPos, &SaveMpqHeader, pHeader->dwHeaderSize))
            nError = GetLastError();
    }

    /* Clear the changed flag */
    if(nError == ERROR_SUCCESS)
        ha->dwFlags &= ~MPQ_FLAG_CHANGED;

    /* Cleanup and exit */
    if(pHetTable != NULL)
        STORM_FREE(pHetTable);
    if(pBetTable != NULL)
        STORM_FREE(pBetTable);
    if(pHashTable != NULL)
        STORM_FREE(pHashTable);
    if(pBlockTable != NULL)
        STORM_FREE(pBlockTable);
    if(pHiBlockTable != NULL)
        STORM_FREE(pHiBlockTable);
    return nError;
}
