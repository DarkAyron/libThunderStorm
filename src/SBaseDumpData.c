/*****************************************************************************/
/* SBaseDumpData.c                        Copyright (c) Ladislav Zezula 2011 */
/*---------------------------------------------------------------------------*/
/* Description :                                                             */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 26.01.11  1.00  Lad  The first version of SBaseDumpData.cpp               */
/* 19.03.15  1.00  Ayr  Ported to plain c                                    */
/*****************************************************************************/

/* #define _STORMLIB_DUMP_DATA */

#include "thunderStorm.h"
#include "StormCommon.h"

#ifdef _STORMLIB_DUMP_DATA

void DumpMpqHeader(TMPQHeader * pHeader)
{
    printf("== MPQ Header =================================\n");
    printf("uint32_t dwID                   = %08X\n",    pHeader->dwID);
    printf("uint32_t dwHeaderSize           = %08X\n",    pHeader->dwHeaderSize);
    printf("uint32_t dwArchiveSize          = %08X\n",    pHeader->dwArchiveSize);
    printf("uint16_t wFormatVersion         = %04X\n",    pHeader->wFormatVersion);
    printf("uint16_t wSectorSize            = %04X\n",    pHeader->wSectorSize);
    printf("uint32_t dwHashTablePos         = %08X\n",    pHeader->dwHashTablePos);
    printf("uint32_t dwBlockTablePos        = %08X\n",    pHeader->dwBlockTablePos);
    printf("uint32_t dwHashTableSize        = %08X\n",    pHeader->dwHashTableSize);
    printf("uint32_t dwBlockTableSize       = %08X\n",    pHeader->dwBlockTableSize);
    printf("uint64_t HiBlockTablePos64      = %016llX\n", pHeader->HiBlockTablePos64);
    printf("uint16_t wHashTablePosHi        = %04X\n",    pHeader->wHashTablePosHi);
    printf("uint16_t wBlockTablePosHi       = %04X\n",    pHeader->wBlockTablePosHi);
    printf("uint64_t ArchiveSize64          = %016llX\n", pHeader->ArchiveSize64);
    printf("uint64_t BetTablePos64          = %016llX\n", pHeader->BetTablePos64);
    printf("uint64_t HetTablePos64          = %016llX\n", pHeader->HetTablePos64);
    printf("uint64_t HashTableSize64        = %016llX\n", pHeader->HashTableSize64);
    printf("uint64_t BlockTableSize64       = %016llX\n", pHeader->BlockTableSize64);
    printf("uint64_t HiBlockTableSize64     = %016llX\n", pHeader->HiBlockTableSize64);
    printf("uint64_t HetTableSize64         = %016llX\n", pHeader->HetTableSize64);
    printf("uint64_t BetTableSize64         = %016llX\n", pHeader->BetTableSize64);
    printf("uint32_t dwRawChunkSize         = %08X\n",     pHeader->dwRawChunkSize);
    printf("-----------------------------------------------\n\n");
}

void DumpHashTable(TMPQHash * pHashTable, DWORD dwHashTableSize)
{
    uint32_t i;

    if(pHashTable == NULL || dwHashTableSize == 0)
        return;

    printf("== Hash Table =================================\n");
    for(i = 0; i < dwHashTableSize; i++)
    {
        printf("[%08x] %08X %08X %04X %04X %08X\n", i,
                                                    pHashTable[i].dwName1,
                                                    pHashTable[i].dwName2,
                                                    pHashTable[i].lcLocale,
                                                    pHashTable[i].wPlatform,
                                                    pHashTable[i].dwBlockIndex);
    }
    printf("-----------------------------------------------\n\n");
}

void DumpHetAndBetTable(TMPQHetTable * pHetTable, TMPQBetTable * pBetTable)
{
    uint32_t i;

    if(pHetTable == NULL || pBetTable == NULL)
        return;

    printf("== HET Header =================================\n");
    printf("uint64_t      AndMask64         = %016llX\n",  pHetTable->AndMask64);       
    printf("uint64_t      OrMask64          = %016llX\n",  pHetTable->OrMask64);        
    printf("uint32_t      dwEntryCount      = %08X\n",     pHetTable->dwEntryCount);
    printf("uint32_t      dwTotalCount      = %08X\n",     pHetTable->dwTotalCount);
    printf("uint32_t      dwNameHashBitSize = %08X\n",     pHetTable->dwNameHashBitSize);
    printf("uint32_t      dwIndexSizeTotal  = %08X\n",     pHetTable->dwIndexSizeTotal);
    printf("uint32_t      dwIndexSizeExtra  = %08X\n",     pHetTable->dwIndexSizeExtra);
    printf("uint32_t      dwIndexSize       = %08X\n",     pHetTable->dwIndexSize);     
    printf("-----------------------------------------------\n\n");

    printf("== BET Header =================================\n");
    printf("uint32_t dwTableEntrySize       = %08X\n",     pBetTable->dwTableEntrySize);
    printf("uint32_t dwBitIndex_FilePos     = %08X\n",     pBetTable->dwBitIndex_FilePos);
    printf("uint32_t dwBitIndex_FileSize    = %08X\n",     pBetTable->dwBitIndex_FileSize);
    printf("uint32_t dwBitIndex_CmpSize     = %08X\n",     pBetTable->dwBitIndex_CmpSize);
    printf("uint32_t dwBitIndex_FlagIndex   = %08X\n",     pBetTable->dwBitIndex_FlagIndex);
    printf("uint32_t dwBitIndex_Unknown     = %08X\n",     pBetTable->dwBitIndex_Unknown);
    printf("uint32_t dwBitCount_FilePos     = %08X\n",     pBetTable->dwBitCount_FilePos);
    printf("uint32_t dwBitCount_FileSize    = %08X\n",     pBetTable->dwBitCount_FileSize);
    printf("uint32_t dwBitCount_CmpSize     = %08X\n",     pBetTable->dwBitCount_CmpSize);
    printf("uint32_t dwBitCount_FlagIndex   = %08X\n",     pBetTable->dwBitCount_FlagIndex);   
    printf("uint32_t dwBitCount_Unknown     = %08X\n",     pBetTable->dwBitCount_Unknown);
    printf("uint32_t dwBitTotal_NameHash2   = %08X\n",     pBetTable->dwBitTotal_NameHash2);
    printf("uint32_t dwBitExtra_NameHash2   = %08X\n",     pBetTable->dwBitExtra_NameHash2);
    printf("uint32_t dwBitCount_NameHash2   = %08X\n",     pBetTable->dwBitCount_NameHash2);
    printf("uint32_t dwEntryCount           = %08X\n",     pBetTable->dwEntryCount);
    printf("uint32_t dwFlagCount            = %08X\n",     pBetTable->dwFlagCount);
    printf("-----------------------------------------------\n\n");

    printf("== HET & Bet Table ======================================================================\n\n");
    printf("HetIdx HetHash BetIdx BetHash          uint8_tOffset       FileSize CmpSize  FlgIdx Flags   \n");
    printf("------ ------- ------ ---------------- ---------------- -------- -------- ------ --------\n");
    for(i = 0; i < pHetTable->dwTotalCount; i++)
    {
        uint64_t uint8_tOffset = 0;
        uint64_t BetHash = 0;
        uint32_t dwFileSize = 0;
        uint32_t dwCmpSize = 0;
        uint32_t dwFlagIndex = 0;
        uint32_t dwFlags = 0;
        uint32_t dwBetIndex = 0;

        GetBits(pHetTable->pBetIndexes, i * pHetTable->dwIndexSizeTotal,
                                        pHetTable->dwIndexSize,
                                       &dwBetIndex,
                                        4);
        
        if(dwBetIndex < pHetTable->dwTotalCount)
        {
            uint32_t dwEntryIndex = pBetTable->dwTableEntrySize * dwBetIndex;

            GetBits(pBetTable->pNameHashes, dwBetIndex * pBetTable->dwBitTotal_NameHash2,
                                            pBetTable->dwBitCount_NameHash2,
                                           &BetHash,
                                            8);

            GetBits(pBetTable->pFileTable, dwEntryIndex + pBetTable->dwBitIndex_FilePos,
                                           pBetTable->dwBitCount_FilePos,
                                          &uint8_tOffset,
                                           8);

            GetBits(pBetTable->pFileTable, dwEntryIndex + pBetTable->dwBitIndex_FileSize,
                                           pBetTable->dwBitCount_FileSize,
                                          &dwFileSize,
                                           4);

            GetBits(pBetTable->pFileTable, dwEntryIndex + pBetTable->dwBitIndex_CmpSize,
                                           pBetTable->dwBitCount_CmpSize,
                                          &dwCmpSize,
                                           4);

            GetBits(pBetTable->pFileTable, dwEntryIndex + pBetTable->dwBitIndex_FlagIndex,
                                           pBetTable->dwBitCount_FlagIndex,
                                          &dwFlagIndex,
                                           4);

            dwFlags = pBetTable->pFileFlags[dwFlagIndex];
        }

        printf(" %04X    %02lX     %04X  %016llX %016llX %08X %08X  %04X  %08X\n", i,
                                                         pHetTable->pNameHashes[i],
                                                         dwBetIndex,
                                                         BetHash,
                                                         uint8_tOffset,
                                                         dwFileSize,
                                                         dwCmpSize,
                                                         dwFlagIndex,
                                                         dwFlags);
    }
    printf("-----------------------------------------------------------------------------------------\n");
}

#endif  /* _STORMLIB_DUMP_DATA */
