/*****************************************************************************/
/* SBaseCommon.c                          Copyright (c) Ladislav Zezula 2003 */
/*---------------------------------------------------------------------------*/
/* Common functions for StormLib, used by all SFile*** modules               */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 24.03.03  1.00  Lad  The first version of SFileCommon.cpp                 */
/* 19.11.03  1.01  Dan  Big endian handling                                  */
/* 12.06.04  1.01  Lad  Renamed to SCommon.cpp                               */
/* 06.09.10  1.01  Lad  Renamed to SBaseCommon.cpp                           */
/*****************************************************************************/

#include "thunderStorm.h"
#include "StormCommon.h"

/*-----------------------------------------------------------------------------
 * Local variables
 */

uint32_t    lcFileLocale = LANG_NEUTRAL;            /* File locale */
uint16_t  wPlatform = 0;                          /* File platform */

/*-----------------------------------------------------------------------------
 * Conversion to uppercase/lowercase
 *
 * Converts ASCII characters to lowercase
 * Converts slash (0x2F) to backslash (0x5C)
 */
unsigned char AsciiToLowerTable[256] = 
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x5C, 
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 
    0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* Converts ASCII characters to uppercase */
/* Converts slash (0x2F) to backslash (0x5C) */
unsigned char AsciiToUpperTable[256] = 
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x5C, 
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 
    0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* Converts ASCII characters to uppercase */
/* Does NOT convert slash (0x2F) to backslash (0x5C) */
unsigned char AsciiToUpperTable_Slash[256] = 
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 
    0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/*-----------------------------------------------------------------------------
 * Storm hashing functions
 */

#define STORM_BUFFER_SIZE       0x500

#define HASH_INDEX_MASK(ha) (ha->dwHashTableSize ? (ha->dwHashTableSize - 1) : 0)

static uint32_t StormBuffer[STORM_BUFFER_SIZE];    /* Buffer for the decryption engine */
static int  bMpqCryptographyInitialized = 0;

void InitializeMpqCryptography()
{
    uint32_t dwSeed = 0x00100001;
    uint32_t index1 = 0;
    uint32_t index2 = 0;
    int   i;

    /* Initialize the decryption buffer. */
    /* Do nothing if already done. */
    if(bMpqCryptographyInitialized == 0)
    {
        for(index1 = 0; index1 < 0x100; index1++)
        {
            for(index2 = index1, i = 0; i < 5; i++, index2 += 0x100)
            {
                uint32_t temp1, temp2;

                dwSeed = (dwSeed * 125 + 3) % 0x2AAAAB;
                temp1  = (dwSeed & 0xFFFF) << 0x10;

                dwSeed = (dwSeed * 125 + 3) % 0x2AAAAB;
                temp2  = (dwSeed & 0xFFFF);

                StormBuffer[index2] = (temp1 | temp2);
            }
        }

        /* Also register both MD5 and SHA1 hash algorithms */
        register_hash(&md5_desc);
        register_hash(&sha1_desc);

        /* Use LibTomMath as support math library for LibTomCrypt */
        ltc_mp = ltm_desc;    

        /* Don't do that again */
        bMpqCryptographyInitialized = 1;
    }
}

uint32_t HashString(const char * szFileName, uint32_t dwHashType)
{
    unsigned char * pbKey   = (uint8_t *)szFileName;
    uint32_t  dwSeed1 = 0x7FED7FED;
    uint32_t  dwSeed2 = 0xEEEEEEEE;
    uint32_t  ch;

    while(*pbKey != 0)
    {
        /* Convert the input character to uppercase */
        /* Convert slash (0x2F) to backslash (0x5C) */
        ch = AsciiToUpperTable[*pbKey++];

        dwSeed1 = StormBuffer[dwHashType + ch] ^ (dwSeed1 + dwSeed2);
        dwSeed2 = ch + dwSeed1 + dwSeed2 + (dwSeed2 << 5) + 3;
    }

    return dwSeed1;
}

uint32_t HashStringSlash(const char * szFileName, uint32_t dwHashType)
{
    unsigned char * pbKey   = (uint8_t *)szFileName;
    uint32_t  dwSeed1 = 0x7FED7FED;
    uint32_t  dwSeed2 = 0xEEEEEEEE;
    uint32_t  ch;

    while(*pbKey != 0)
    {
        /* Convert the input character to uppercase */
        /* DON'T convert slash (0x2F) to backslash (0x5C) */
        ch = AsciiToUpperTable_Slash[*pbKey++];

        dwSeed1 = StormBuffer[dwHashType + ch] ^ (dwSeed1 + dwSeed2);
        dwSeed2 = ch + dwSeed1 + dwSeed2 + (dwSeed2 << 5) + 3;
    }

    return dwSeed1;
}

uint32_t HashStringLower(const char * szFileName, uint32_t dwHashType)
{
    unsigned char * pbKey   = (uint8_t *)szFileName;
    uint32_t  dwSeed1 = 0x7FED7FED;
    uint32_t  dwSeed2 = 0xEEEEEEEE;
    uint32_t  ch;

    while(*pbKey != 0)
    {
        /* Convert the input character to lower */
        /* DON'T convert slash (0x2F) to backslash (0x5C) */
        ch = AsciiToLowerTable[*pbKey++];

        dwSeed1 = StormBuffer[dwHashType + ch] ^ (dwSeed1 + dwSeed2);
        dwSeed2 = ch + dwSeed1 + dwSeed2 + (dwSeed2 << 5) + 3;
    }

    return dwSeed1;
}

/*-----------------------------------------------------------------------------
 * Calculates the hash table size for a given amount of files
 */

uint32_t GetHashTableSizeForFileCount(uint32_t dwFileCount)
{
    uint32_t dwPowerOfTwo = HASH_TABLE_SIZE_MIN;

    /* For zero files, there is no hash table needed */
    if(dwFileCount == 0)
        return 0;

    /* Round the hash table size up to the nearest power of two */
    /* Don't allow the hash table size go over allowed maximum */
    while(dwPowerOfTwo < HASH_TABLE_SIZE_MAX && dwPowerOfTwo < dwFileCount)
        dwPowerOfTwo <<= 1;
    return dwPowerOfTwo;
}

/*-----------------------------------------------------------------------------
 * Calculates a Jenkin's Encrypting and decrypting MPQ file data
 */

uint64_t HashStringJenkins(const char * szFileName)
{
    unsigned char * pbFileName = (unsigned char *)szFileName;
    char * szTemp;
    char szLocFileName[0x108];
    size_t nLength = 0;
    unsigned int primary_hash = 1;
    unsigned int secondary_hash = 2;

    /* Normalize the file name - convert to uppercase, and convert "/" to "\\". */
    if(pbFileName != NULL)
    {
        szTemp = szLocFileName;
        while(*pbFileName != 0)
            *szTemp++ = (char)AsciiToLowerTable[*pbFileName++];
        *szTemp = 0;

        nLength = szTemp - szLocFileName;
    }

    /* Thanks Quantam for finding out what the algorithm is.
     * I am really getting old for reversing large chunks of assembly
     * that does hashing :-) */
    hashlittle2(szLocFileName, nLength, &secondary_hash, &primary_hash);

    /* Combine those 2 together */
    return (uint64_t)primary_hash * (uint64_t)0x100000000ULL + (uint64_t)secondary_hash;
}

/*-----------------------------------------------------------------------------
 * Default flags for (attributes) and (listfile)
 */

uint32_t GetDefaultSpecialFileFlags(uint32_t dwFileSize, uint16_t wFormatVersion)
{
    /* Fixed for format 1.0 */
    if(wFormatVersion == MPQ_FORMAT_VERSION_1)
        return MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_FIX_KEY;

    /* Size-dependent for formats 2.0-4.0 */
    return (dwFileSize > 0x4000) ? (MPQ_FILE_COMPRESS | MPQ_FILE_SECTOR_CRC) : (MPQ_FILE_COMPRESS | MPQ_FILE_SINGLE_UNIT);
}


/*-----------------------------------------------------------------------------
 * Encrypting/Decrypting MPQ data block
 */

void EncryptMpqBlock(void * pvDataBlock, uint32_t dwLength, uint32_t dwKey1)
{
    uint32_t * DataBlock = (uint32_t *)pvDataBlock;
    uint32_t dwValue32, i;
    uint32_t dwKey2 = 0xEEEEEEEE;

    /* Round to uint32_ts */
    dwLength >>= 2;

    /* Encrypt the data block at array of uint32_ts */
    for(i = 0; i < dwLength; i++)
    {
        /* Modify the second key */
        dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];

        dwValue32 = DataBlock[i];
        DataBlock[i] = DataBlock[i] ^ (dwKey1 + dwKey2);

        dwKey1 = ((~dwKey1 << 0x15) + 0x11111111) | (dwKey1 >> 0x0B);
        dwKey2 = dwValue32 + dwKey2 + (dwKey2 << 5) + 3;
    }
}

void DecryptMpqBlock(void * pvDataBlock, uint32_t dwLength, uint32_t dwKey1)
{
    uint32_t * DataBlock = (uint32_t *)pvDataBlock;
    uint32_t dwValue32, i;
    uint32_t dwKey2 = 0xEEEEEEEE;

    /* Round to uint32_ts */
    dwLength >>= 2;

    /* Decrypt the data block at array of uint32_ts */
    for(i = 0; i < dwLength; i++)
    {
        /* Modify the second key */
        dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];
        
        DataBlock[i] = DataBlock[i] ^ (dwKey1 + dwKey2);
        dwValue32 = DataBlock[i];

        dwKey1 = ((~dwKey1 << 0x15) + 0x11111111) | (dwKey1 >> 0x0B);
        dwKey2 = dwValue32 + dwKey2 + (dwKey2 << 5) + 3;
    }
}

void EncryptMpqBlockAnubis(void * pvDataBlock, uint32_t dwLength, anubisSchedule_t * keySchedule)
{
    uint8_t * DataBlock;
    uint8_t tmpBlock[16];
    uint8_t residualBytes = dwLength % 16;
    uint32_t nBlocks = dwLength / 16;
    int i, j;
    
    /* encrypt sector block by block */
    for(i = 0; i < nBlocks; i++)
    {
        DataBlock = (uint8_t *)pvDataBlock + i * 16;
        
        /* save the block */
        memcpy(tmpBlock, DataBlock, 16);
        
        anubisEncrypt(keySchedule, tmpBlock, DataBlock);
    }
    
    /* if the last block is incomplete, do the CTS */
    if(residualBytes > 0)
    {
        memcpy(tmpBlock, DataBlock + 16, residualBytes);
        memcpy(DataBlock + 16, DataBlock, residualBytes);
        memcpy(tmpBlock + residualBytes, DataBlock + residualBytes, 16 - residualBytes);
        anubisEncrypt(keySchedule, tmpBlock, DataBlock);
    }
}

void DecryptMpqBlockAnubis(void * pvDataBlock, uint32_t dwLength, anubisSchedule_t * keySchedule)
{
    uint8_t * DataBlock;
    int i, j;
    uint8_t tmpBlock[16];
    uint8_t residualBytes = dwLength % 16;
    uint32_t nBlocks = dwLength / 16;
    
    /* decrypt sector block by block */
    for (i = 0; i < nBlocks; i++)
    {
        DataBlock = (uint8_t *)pvDataBlock + i * 16;
        
        /* save the block */
        memcpy(tmpBlock, DataBlock, 16);
        
        anubisDecrypt(keySchedule, tmpBlock, DataBlock);
    }
    
    /* if the last block is incomplete, undo the CTS */
    if(residualBytes > 0)
    {
        memcpy(tmpBlock, DataBlock + 16, residualBytes);
        memcpy(DataBlock + 16, DataBlock, residualBytes);
        memcpy(tmpBlock + residualBytes, DataBlock + residualBytes, 16 - residualBytes);
        anubisDecrypt(keySchedule, tmpBlock, DataBlock);
    }
}

void EncryptMpqBlockSerpent(void * pvDataBlock, uint32_t dwLength, serpentSchedule_t * keySchedule)
{
    uint8_t * DataBlock;
    uint8_t tmpBlock[16];
    uint8_t residualBytes = dwLength % 16;
    uint32_t nBlocks = dwLength / 16;
    int i, j;
    
    /* encrypt sector block by block */
    for(i = 0; i < nBlocks; i++)
    {
        DataBlock = (uint8_t *)pvDataBlock + i * 16;
        
        /* save the block */
        memcpy(tmpBlock, DataBlock, 16);
        
        serpentEncrypt(keySchedule, tmpBlock, DataBlock);
    }
    
    /* if the last block is incomplete, do the CTS */
    if(residualBytes > 0)
    {
        memcpy(tmpBlock, DataBlock + 16, residualBytes);
        memcpy(DataBlock + 16, DataBlock, residualBytes);
        memcpy(tmpBlock + residualBytes, DataBlock + residualBytes, 16 - residualBytes);
        serpentEncrypt(keySchedule, tmpBlock, DataBlock);
    }
}

void DecryptMpqBlockSerpent(void * pvDataBlock, uint32_t dwLength, serpentSchedule_t * keySchedule)
{
    uint8_t * DataBlock;
    int i, j;
    uint8_t tmpBlock[16];
    uint8_t residualBytes = dwLength % 16;
    uint32_t nBlocks = dwLength / 16;
    
    /* decrypt sector block by block */
    for (i = 0; i < nBlocks; i++)
    {
        DataBlock = (uint8_t *)pvDataBlock + i * 16;
        
        /* save the block */
        memcpy(tmpBlock, DataBlock, 16);
        
        serpentDecrypt(keySchedule, tmpBlock, DataBlock);
    }
    
    /* if the last block is incomplete, undo the CTS */
    if(residualBytes > 0)
    {
        memcpy(tmpBlock, DataBlock + 16, residualBytes);
        memcpy(DataBlock + 16, DataBlock, residualBytes);
        memcpy(tmpBlock + residualBytes, DataBlock + residualBytes, 16 - residualBytes);
        serpentDecrypt(keySchedule, tmpBlock, DataBlock);
    }
}

/**
 * Functions tries to get file decryption key. This comes from these facts
 *
 * - We know the decrypted value of the first uint32_t in the encrypted data
 * - We know the decrypted value of the second uint32_t (at least aproximately)
 * - There is only 256 variants of how the second key is modified
 *
 *  The first iteration of dwKey1 and dwKey2 is this:
 *
 *  dwKey2 = 0xEEEEEEEE + StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)]
 *  dwDecrypted0 = DataBlock[0] ^ (dwKey1 + dwKey2);
 *
 *  This means:
 *
 *  (dwKey1 + dwKey2) = DataBlock[0] ^ dwDecrypted0;
 *
 */

uint32_t DetectFileKeyBySectorSize(uint32_t * EncryptedData, uint32_t dwSectorSize, uint32_t dwDecrypted0)
{
    uint32_t dwDecrypted1Max = dwSectorSize + dwDecrypted0;
    uint32_t dwKey1PlusKey2, i;
    uint32_t DataBlock[2];

    /* We must have at least 2 uint32_ts there to be able to decrypt something */
    if(dwSectorSize < 0x08)
        return 0;

    /* Get the value of the combined encryption key */
    dwKey1PlusKey2 = (EncryptedData[0] ^ dwDecrypted0) - 0xEEEEEEEE;

    /* Try all 256 combinations of dwKey1 */
    for(i = 0; i < 0x100; i++)
    {
        uint32_t dwSaveKey1;
        uint32_t dwKey1 = dwKey1PlusKey2 - StormBuffer[MPQ_HASH_KEY2_MIX + i];
        uint32_t dwKey2 = 0xEEEEEEEE;

        /* Modify the second key and decrypt the first uint32_t */
        dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];
        DataBlock[0] = EncryptedData[0] ^ (dwKey1 + dwKey2);

        /* Did we obtain the same value like dwDecrypted0? */
        if(DataBlock[0] == dwDecrypted0)
        {
            /* Save this key value. Increment by one because */
            /* we are decrypting sector offset table */
            dwSaveKey1 = dwKey1 + 1;

            /* Rotate both keys */
            dwKey1 = ((~dwKey1 << 0x15) + 0x11111111) | (dwKey1 >> 0x0B);
            dwKey2 = DataBlock[0] + dwKey2 + (dwKey2 << 5) + 3;

            /* Modify the second key again and decrypt the second uint32_t */
            dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];
            DataBlock[1] = EncryptedData[1] ^ (dwKey1 + dwKey2);

            /* Now compare the results */
            if(DataBlock[1] <= dwDecrypted1Max)
                return dwSaveKey1;
        }
    }

    /* Key not found */
    return 0;
}

/* Function tries to detect file encryption key based on expected file content */
/* It is the same function like before, except that we know the value of the second uint32_t */
uint32_t DetectFileKeyByKnownContent(void * pvEncryptedData, uint32_t dwDecrypted0, uint32_t dwDecrypted1)
{
    uint32_t * EncryptedData = (uint32_t *)pvEncryptedData;
    uint32_t dwKey1PlusKey2, i;
    uint32_t DataBlock[2];

    /* Get the value of the combined encryption key */
    dwKey1PlusKey2 = (EncryptedData[0] ^ dwDecrypted0) - 0xEEEEEEEE;

    /* Try all 256 combinations of dwKey1 */
    for(i = 0; i < 0x100; i++)
    {
        uint32_t dwSaveKey1;
        uint32_t dwKey1 = dwKey1PlusKey2 - StormBuffer[MPQ_HASH_KEY2_MIX + i];
        uint32_t dwKey2 = 0xEEEEEEEE;

        /* Modify the second key and decrypt the first uint32_t */
        dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];
        DataBlock[0] = EncryptedData[0] ^ (dwKey1 + dwKey2);

        /* Did we obtain the same value like dwDecrypted0? */
        if(DataBlock[0] == dwDecrypted0)
        {
            /* Save this key value */
            dwSaveKey1 = dwKey1;

            /* Rotate both keys */
            dwKey1 = ((~dwKey1 << 0x15) + 0x11111111) | (dwKey1 >> 0x0B);
            dwKey2 = DataBlock[0] + dwKey2 + (dwKey2 << 5) + 3;

            /* Modify the second key again and decrypt the second uint32_t */
            dwKey2 += StormBuffer[MPQ_HASH_KEY2_MIX + (dwKey1 & 0xFF)];
            DataBlock[1] = EncryptedData[1] ^ (dwKey1 + dwKey2);

            /* Now compare the results */
            if(DataBlock[1] == dwDecrypted1)
                return dwSaveKey1;
        }
    }

    /* Key not found */
    return 0;
}

uint32_t DetectFileKeyByContent(void * pvEncryptedData, uint32_t dwSectorSize, uint32_t dwFileSize)
{
    uint32_t dwFileKey;

    /* Try to break the file encryption key as if it was a WAVE file */
    if(dwSectorSize >= 0x0C)
    {
        dwFileKey = DetectFileKeyByKnownContent(pvEncryptedData, 0x46464952, dwFileSize - 8);
        if(dwFileKey != 0)
            return dwFileKey;
    }

    /* Try to break the encryption key as if it was an EXE file */
    if(dwSectorSize > 0x40)
    {
        dwFileKey = DetectFileKeyByKnownContent(pvEncryptedData, 0x00905A4D, 0x00000003);
        if(dwFileKey != 0)
            return dwFileKey;
    }

    /* Try to break the encryption key as if it was a XML file */
    if(dwSectorSize > 0x04)
    {
        dwFileKey = DetectFileKeyByKnownContent(pvEncryptedData, 0x6D783F3C, 0x6576206C);
        if(dwFileKey != 0)
            return dwFileKey;
    }

    /* Not detected, sorry */
    return 0;
}

uint32_t DecryptFileKey(
    const char * szFileName,
    uint64_t MpqPos,
    uint32_t dwFileSize,
    uint32_t dwFlags)
{
    uint32_t dwFileKey;
    uint32_t dwMpqPos = (uint32_t)MpqPos;

    /* File key is calculated from plain name */
    szFileName = GetPlainFileName(szFileName);
    dwFileKey = HashString(szFileName, MPQ_HASH_FILE_KEY);

    /* Fix the key, if needed */
    if(dwFlags & MPQ_FILE_FIX_KEY)
        dwFileKey = (dwFileKey + dwMpqPos) ^ dwFileSize;

    /* Return the key */
    return dwFileKey;
}

/*-----------------------------------------------------------------------------
 * Handle validation functions
 */

TMPQArchive * IsValidMpqHandle(void * hMpq)
{
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    
    return (ha != NULL && ha->pHeader != NULL && ha->pHeader->dwID == ID_MPQ) ? ha : NULL;
}

TMPQFile * IsValidFileHandle(void * hFile)
{
    TMPQFile * hf = (TMPQFile *)hFile;

    /* Must not be NULL */
    if(hf != NULL && hf->dwMagic == ID_MPQ_FILE)
    {
        /* Local file handle? */
        if(hf->pStream != NULL)
            return hf;

        /* Also verify the MPQ handle within the file handle */
        if(IsValidMpqHandle(hf->ha))
            return hf;
    }

    return NULL;
}

/*-----------------------------------------------------------------------------
 * Hash table and block table manipulation
 */
/* Attempts to search a free hash entry, or an entry whose names and locale matches */
TMPQHash * FindFreeHashEntry(TMPQArchive * ha, uint32_t dwStartIndex, uint32_t dwName1, uint32_t dwName2, uint32_t lcLocale)
{
    TMPQHash * pDeletedEntry = NULL;            /* If a deleted entry was found in the continuous hash range */
    TMPQHash * pFreeEntry = NULL;               /* If a free entry was found in the continuous hash range */
    uint32_t dwHashIndexMask = HASH_INDEX_MASK(ha);
    uint32_t dwIndex;

    /* Set the initial index */
    dwStartIndex = dwIndex = (dwStartIndex & dwHashIndexMask);

    /* Search the hash table and return the found entries in the following priority: */
    /* 1) <MATCHING_ENTRY> */
    /* 2) <DELETED-ENTRY> */
    /* 3) <FREE-ENTRY> */
    /* 4) NULL */
    for(;;)
    {
        TMPQHash * pHash = ha->pHashTable + dwIndex;

        /* If we found a matching entry, return that one */
        if(pHash->dwName1 == dwName1 && pHash->dwName2 == dwName2 && pHash->lcLocale == lcLocale)
            return pHash;

        /* If we found a deleted entry, remember it but keep searching */
        if(pHash->dwBlockIndex == HASH_ENTRY_DELETED && pDeletedEntry == NULL)
            pDeletedEntry = pHash;

        /* If we found a free entry, we need to stop searching */
        if(pHash->dwBlockIndex == HASH_ENTRY_FREE)
        {
            pFreeEntry = pHash;
            break;
        }

        /* Move to the next hash entry. */
        /* If we reached the starting entry, it's failure. */
        dwIndex = (dwIndex + 1) & dwHashIndexMask;
        if(dwIndex == dwStartIndex)
            break;
    }

    /* If we found a deleted entry, return that one preferentially */
    return (pDeletedEntry != NULL) ? pDeletedEntry : pFreeEntry;
}

/* Retrieves the first hash entry for the given file. */
/* Every locale version of a file has its own hash entry */
TMPQHash * GetFirstHashEntry(TMPQArchive * ha, const char * szFileName)
{
    uint32_t dwHashIndexMask = HASH_INDEX_MASK(ha);
    uint32_t dwStartIndex = ha->pfnHashString(szFileName, MPQ_HASH_TABLE_INDEX);
    uint32_t dwName1 = ha->pfnHashString(szFileName, MPQ_HASH_NAME_A);
    uint32_t dwName2 = ha->pfnHashString(szFileName, MPQ_HASH_NAME_B);
    uint32_t dwIndex;

    /* Set the initial index */
    dwStartIndex = dwIndex = (dwStartIndex & dwHashIndexMask);

    /* Search the hash table */
    for(;;)
    {
        TMPQHash * pHash = ha->pHashTable + dwIndex;

        /* If the entry matches, we found it. */
        if(pHash->dwName1 == dwName1 && pHash->dwName2 == dwName2 && pHash->dwBlockIndex < ha->dwFileTableSize)
            return pHash;

        /* If that hash entry is a free entry, it means we haven't found the file */
        if(pHash->dwBlockIndex == HASH_ENTRY_FREE)
            return NULL;

        /* Move to the next hash entry. Stop searching */
        /* if we got reached the original hash entry */
        dwIndex = (dwIndex + 1) & dwHashIndexMask;
        if(dwIndex == dwStartIndex)
            return NULL;
    }
}

TMPQHash * GetNextHashEntry(TMPQArchive * ha, TMPQHash * pFirstHash, TMPQHash * pHash)
{
    uint32_t dwHashIndexMask = HASH_INDEX_MASK(ha);
    uint32_t dwStartIndex = (uint32_t)(pFirstHash - ha->pHashTable);
    uint32_t dwName1 = pHash->dwName1;
    uint32_t dwName2 = pHash->dwName2;
    uint32_t dwIndex = (uint32_t)(pHash - ha->pHashTable);

    /* Now go for any next entry that follows the pHash, */
    /* until either free hash entry was found, or the start entry was reached */
    for(;;)
    {
        /* Move to the next hash entry. Stop searching */
        /* if we got reached the original hash entry */
        dwIndex = (dwIndex + 1) & dwHashIndexMask;
        if(dwIndex == dwStartIndex)
            return NULL;
        pHash = ha->pHashTable + dwIndex;

        /* If the entry matches, we found it. */
        if(pHash->dwName1 == dwName1 && pHash->dwName2 == dwName2 && pHash->dwBlockIndex < ha->dwFileTableSize)
            return pHash;

        /* If that hash entry is a free entry, it means we haven't found the file */
        if(pHash->dwBlockIndex == HASH_ENTRY_FREE)
            return NULL;
    }
}

/* Allocates an entry in the hash table */
TMPQHash * AllocateHashEntry(
    TMPQArchive * ha,
    TFileEntry * pFileEntry)
{
    TMPQHash * pHash;
    uint32_t dwStartIndex = ha->pfnHashString(pFileEntry->szFileName, MPQ_HASH_TABLE_INDEX);
    uint32_t dwName1 = ha->pfnHashString(pFileEntry->szFileName, MPQ_HASH_NAME_A);
    uint32_t dwName2 = ha->pfnHashString(pFileEntry->szFileName, MPQ_HASH_NAME_B);

    /* Attempt to find a free hash entry */
    pHash = FindFreeHashEntry(ha, dwStartIndex, dwName1, dwName2, pFileEntry->lcLocale);
    if(pHash != NULL)
    {
        /* Fill the free hash entry */
        pHash->dwName1      = dwName1;
        pHash->dwName2      = dwName2;
        pHash->lcLocale     = pFileEntry->lcLocale;
        pHash->wPlatform    = pFileEntry->wPlatform;
        pHash->dwBlockIndex = (uint32_t)(pFileEntry - ha->pFileTable);

        /* Fill the hash index in the file entry */
        pFileEntry->dwHashIndex = (uint32_t)(pHash - ha->pHashTable);
    }

    return pHash;
}

/* Finds a free space in the MPQ where to store next data
 * The free space begins beyond the file that is stored at the fuhrtest
 * position in the MPQ. (listfile), (attributes) and (signature) are ignored,
 * unless the MPQ is being flushed.
 */
uint64_t FindFreeMpqSpace(TMPQArchive * ha)
{
    TMPQHeader * pHeader = ha->pHeader;
    TFileEntry * pFileTableEnd = ha->pFileTable + ha->dwFileTableSize;
    TFileEntry * pFileEntry;
    uint64_t FreeSpacePos = ha->pHeader->dwHeaderSize;
    uint32_t dwChunkCount;

    /* Parse the entire block table */
    for(pFileEntry = ha->pFileTable; pFileEntry < pFileTableEnd; pFileEntry++)
    {
        /* Only take existing files with nonzero size */
        if((pFileEntry->dwFlags & MPQ_FILE_EXISTS) && (pFileEntry->dwCmpSize != 0))
        {
            /* If we are not saving MPQ tables, ignore internal MPQ files */
            if((ha->dwFlags & MPQ_FLAG_SAVING_TABLES) == 0 && IsInternalMpqFileName(pFileEntry->szFileName))
                continue;

            /* If the end of the file is bigger than current MPQ table pos, update it */
            if((pFileEntry->ByteOffset + pFileEntry->dwCmpSize) > FreeSpacePos)
            {
                /* Get the end of the file data */
                FreeSpacePos = pFileEntry->ByteOffset + pFileEntry->dwCmpSize;

                /* Add the MD5 chunks, if present */
                if(pHeader->dwRawChunkSize != 0 && pFileEntry->dwCmpSize != 0)
                {
                    dwChunkCount = ((pFileEntry->dwCmpSize - 1) / pHeader->dwRawChunkSize) + 1;
                    FreeSpacePos += dwChunkCount * MD5_DIGEST_SIZE;
                }
            }
        }
    }

    /* Give the free space position to the caller */
    return FreeSpacePos;
}

/*-----------------------------------------------------------------------------
 * Common functions - MPQ File
 */

TMPQFile * CreateFileHandle(TMPQArchive * ha, TFileEntry * pFileEntry)
{
    TMPQFile * hf;

    /* Allocate space for TMPQFile */
    hf = STORM_ALLOC(TMPQFile, 1);
    if(hf != NULL)
    {
        /* Fill the file structure */
        memset(hf, 0, sizeof(TMPQFile));
        hf->dwMagic = ID_MPQ_FILE;
        hf->pStream = NULL;
        hf->ha = ha;

        /* If the called entered a file entry, we also copy informations from the file entry */
        if(ha != NULL && pFileEntry != NULL)
        {
            /* Set the raw position and MPQ position */
            hf->RawFilePos = FileOffsetFromMpqOffset(ha, pFileEntry->ByteOffset);
            hf->MpqFilePos = pFileEntry->ByteOffset;

            /* Set the data size */
            hf->dwDataSize = pFileEntry->dwFileSize;
            hf->pFileEntry = pFileEntry;
        }
    }

    return hf;
}

/* Loads a table from MPQ. */
/* Can be used for hash table, block table, sector offset table or sector checksum table */
void * LoadMpqTable(
    TMPQArchive * ha,
    uint64_t ByteOffset,
    uint32_t dwCompressedSize,
    uint32_t dwTableSize,
    uint32_t dwKey,
    int * pbTableIsCut)
{
    uint64_t FileSize = 0;
    unsigned char * pbCompressed = NULL;
    unsigned char * pbMpqTable;
    unsigned char * pbToRead;
    uint32_t dwBytesToRead = dwCompressedSize;
    int nError = ERROR_SUCCESS;

    /* Allocate the MPQ table */
    pbMpqTable = pbToRead = STORM_ALLOC(uint8_t, dwTableSize);
    if(pbMpqTable != NULL)
    {
        /* Check if the MPQ table is encrypted */
        if(dwCompressedSize < dwTableSize)
        {
            /* Allocate temporary buffer for holding compressed data */
            pbCompressed = pbToRead = STORM_ALLOC(uint8_t, dwCompressedSize);
            if(pbCompressed == NULL)
            {
                STORM_FREE(pbMpqTable);
                return NULL;
            }
        }

        /* Get the file offset from which we will read the table
         * Note: According to Storm.dll from Warcraft III (version 2002),
         * if the hash table position is 0xFFFFFFFF, no SetFilePointer call is done
         * and the table is loaded from the current file offset
		 */
        if(ByteOffset == SFILE_INVALID_POS)
            FileStream_GetPos(ha->pStream, &ByteOffset);

        /* On archives v 1.0, hash table and block table can go beyond EOF.
         * Storm.dll reads as much as possible, then fills the missing part with zeros.
         * Abused by Spazzler map protector which sets hash table size to 0x00100000
		 */
        if(ha->pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1)
        {
            /* Cut the table size */
            FileStream_GetSize(ha->pStream, &FileSize);
            if((ByteOffset + dwBytesToRead) > FileSize)
            {
                /* Fill the extra data with zeros */
                dwBytesToRead = (uint32_t)(FileSize - ByteOffset);
                memset(pbMpqTable + dwBytesToRead, 0, (dwTableSize - dwBytesToRead));
                
                /* Give the caller information that the table was cut */
                if(pbTableIsCut != NULL)
                    pbTableIsCut[0] = 1;
            }
        }

        /* If everything succeeded, read the raw table form the MPQ */
        if(FileStream_Read(ha->pStream, &ByteOffset, pbToRead, dwBytesToRead))
        {
            /* First of all, decrypt the table */
            if(dwKey != 0)
            {
                BSWAP_ARRAY32_UNSIGNED(pbToRead, dwCompressedSize);
                DecryptMpqBlock(pbToRead, dwCompressedSize, dwKey);
                BSWAP_ARRAY32_UNSIGNED(pbToRead, dwCompressedSize);
            }

            /* If the table is compressed, decompress it */
            if(dwCompressedSize < dwTableSize)
            {
                int cbOutBuffer = (int)dwTableSize;
                int cbInBuffer = (int)dwCompressedSize;

                if(!SCompDecompress2(pbMpqTable, &cbOutBuffer, pbCompressed, cbInBuffer))
                    nError = GetLastError();
            }

            /* Make sure that the table is properly byte-swapped */
            BSWAP_ARRAY32_UNSIGNED(pbMpqTable, dwTableSize);
        }
        else
        {
            nError = GetLastError();
        }

        /* If read failed, free the table and return */
        if(nError != ERROR_SUCCESS)
        {
            STORM_FREE(pbMpqTable);
            pbMpqTable = NULL;
        }

        /* Free the compression buffer, if any */
        if(pbCompressed != NULL)
            STORM_FREE(pbCompressed);
    }

    /* Return the MPQ table */
    return pbMpqTable;
}

unsigned char * AllocateMd5Buffer(
    uint32_t dwRawDataSize,
    uint32_t dwChunkSize,
    uint32_t * pcbMd5Size)
{
    unsigned char * md5_array;
    uint32_t cbMd5Size;

    /* Sanity check */
    assert(dwRawDataSize != 0);
    assert(dwChunkSize != 0);

    /* Calculate how many MD5's we will calculate */
    cbMd5Size = (((dwRawDataSize - 1) / dwChunkSize) + 1) * MD5_DIGEST_SIZE;

    /* Allocate space for array or MD5s */
    md5_array = STORM_ALLOC(uint8_t, cbMd5Size);

    /* Give the size of the MD5 array */
    if(pcbMd5Size != NULL)
        *pcbMd5Size = cbMd5Size;
    return md5_array;
}

/* Allocates sector buffer and sector offset table */
int AllocateSectorBuffer(TMPQFile * hf)
{
    TMPQArchive * ha = hf->ha;

    /* Caller of AllocateSectorBuffer must ensure these */
    assert(hf->pbFileSector == NULL);
    assert(hf->pFileEntry != NULL);
    assert(hf->ha != NULL);

    /* Don't allocate anything if the file has zero size */
    if(hf->pFileEntry->dwFileSize == 0 || hf->dwDataSize == 0)
        return ERROR_SUCCESS;

    /* Determine the file sector size and allocate buffer for it */
    hf->dwSectorSize = (hf->pFileEntry->dwFlags & MPQ_FILE_SINGLE_UNIT) ? hf->dwDataSize : ha->dwSectorSize;
    hf->pbFileSector = STORM_ALLOC(uint8_t, hf->dwSectorSize);
    hf->dwSectorOffs = SFILE_INVALID_POS;

    /* Return result */
    return (hf->pbFileSector != NULL) ? (int)ERROR_SUCCESS : (int)ERROR_NOT_ENOUGH_MEMORY;
}

/* Allocates sector offset table */
int AllocatePatchInfo(TMPQFile * hf, int bLoadFromFile)
{
    TMPQArchive * ha = hf->ha;
    uint32_t dwLength = sizeof(TPatchInfo);

    /* The following conditions must be true */
    assert(hf->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE);
    assert(hf->pPatchInfo == NULL);

__AllocateAndLoadPatchInfo:

    /* Allocate space for patch header. Start with default size, */
    /* and if its size if bigger, then we reload them */
    hf->pPatchInfo = STORM_ALLOC(TPatchInfo, 1);
    if(hf->pPatchInfo == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Do we have to load the patch header from the file ? */
    if(bLoadFromFile)
    {
        /* Load the patch header */
        if(!FileStream_Read(ha->pStream, &hf->RawFilePos, hf->pPatchInfo, dwLength))
        {
            /* Free the patch info */
            STORM_FREE(hf->pPatchInfo);
            hf->pPatchInfo = NULL;
            return GetLastError();
        }

        /* Perform necessary swapping */
        hf->pPatchInfo->dwLength = BSWAP_INT32_UNSIGNED(hf->pPatchInfo->dwLength);
        hf->pPatchInfo->dwFlags = BSWAP_INT32_UNSIGNED(hf->pPatchInfo->dwFlags);
        hf->pPatchInfo->dwDataSize = BSWAP_INT32_UNSIGNED(hf->pPatchInfo->dwDataSize);

        /* Verify the size of the patch header */
        /* If it's not default size, we have to reload them */
        if(hf->pPatchInfo->dwLength > dwLength)
        {
            /* Free the patch info */
            dwLength = hf->pPatchInfo->dwLength;
            STORM_FREE(hf->pPatchInfo);
            hf->pPatchInfo = NULL;

            /* If the length is out of all possible ranges, fail the operation */
            if(dwLength > 0x400)
                return ERROR_FILE_CORRUPT;
            goto __AllocateAndLoadPatchInfo;
        }

        /* Patch file data size according to the patch header */
        hf->dwDataSize = hf->pPatchInfo->dwDataSize;
    }
    else
    {
        memset(hf->pPatchInfo, 0, dwLength);
    }

    /* Save the final length to the patch header */
    hf->pPatchInfo->dwLength = dwLength;
    hf->pPatchInfo->dwFlags  = 0x80000000;
    return ERROR_SUCCESS;
}

/* Allocates sector offset table */
int AllocateSectorOffsets(TMPQFile * hf, int bLoadFromFile)
{
    TMPQArchive * ha = hf->ha;
    TFileEntry * pFileEntry = hf->pFileEntry;
    uint32_t dwSectorOffsLen;
    int bSectorOffsetTableCorrupt = 0;

    /* Caller of AllocateSectorOffsets must ensure these */
    assert(hf->SectorOffsets == NULL);
    assert(hf->pFileEntry != NULL);
    assert(hf->dwDataSize != 0);
    assert(hf->ha != NULL);

    /* If the file is stored as single unit, just set number of sectors to 1 */
    if(pFileEntry->dwFlags & MPQ_FILE_SINGLE_UNIT)
    {
        hf->dwSectorCount = 1;
        return ERROR_SUCCESS;
    }

    /* Calculate the number of data sectors */
    /* Note that this doesn't work if the file size is zero */
    hf->dwSectorCount = ((hf->dwDataSize - 1) / hf->dwSectorSize) + 1;

    /* Calculate the number of file sectors */
    dwSectorOffsLen = (hf->dwSectorCount + 1) * sizeof(uint32_t);
    
    /* If MPQ_FILE_SECTOR_CRC flag is set, there will either be extra uint32_t */
    /* or an array of MD5's. Either way, we read at least 4 bytes more */
    /* in order to save additional read from the file. */
    if(pFileEntry->dwFlags & MPQ_FILE_SECTOR_CRC)
        dwSectorOffsLen += sizeof(uint32_t);

    /* Only allocate and load the table if the file is compressed */
    if(pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK)
    {
        __LoadSectorOffsets:

        /* Allocate the sector offset table */
        hf->SectorOffsets = STORM_ALLOC(uint32_t, (dwSectorOffsLen / sizeof(uint32_t)));
        if(hf->SectorOffsets == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        /* Only read from the file if we are supposed to do so */
        if(bLoadFromFile)
        {
            uint32_t i;
            uint64_t RawFilePos = hf->RawFilePos;

            /* Append the length of the patch info, if any */
            if(hf->pPatchInfo != NULL)
            {
                if((RawFilePos + hf->pPatchInfo->dwLength) < RawFilePos)
                    return ERROR_FILE_CORRUPT;
                RawFilePos += hf->pPatchInfo->dwLength;
            }

            /* Load the sector offsets from the file */
            if(!FileStream_Read(ha->pStream, &RawFilePos, hf->SectorOffsets, dwSectorOffsLen))
            {
                /* Free the sector offsets */
                STORM_FREE(hf->SectorOffsets);
                hf->SectorOffsets = NULL;
                return GetLastError();
            }

            /* Swap the sector positions */
            BSWAP_ARRAY32_UNSIGNED(hf->SectorOffsets, dwSectorOffsLen);

            /* Decrypt loaded sector positions if necessary */
            if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED)
            {
                /* If we don't know the file key, try to find it. */
                if(hf->dwFileKey == 0)
                {
                    hf->dwFileKey = DetectFileKeyBySectorSize(hf->SectorOffsets, ha->dwSectorSize, dwSectorOffsLen);
                    if(hf->dwFileKey == 0)
                    {
                        STORM_FREE(hf->SectorOffsets);
                        hf->SectorOffsets = NULL;
                        return ERROR_UNKNOWN_FILE_KEY;
                    }
                }

                /* Decrypt sector positions */
                DecryptMpqBlock(hf->SectorOffsets, dwSectorOffsLen, hf->dwFileKey - 1);
            }

            /*
             * Validate the sector offset table
             *
             * Note: Some MPQ protectors put the actual file data before the sector offset table.
             * In this case, the sector offsets are negative (> 0x80000000).
             */

            for(i = 0; i < hf->dwSectorCount; i++)
            {
                uint32_t dwSectorOffset1 = hf->SectorOffsets[i+1];
                uint32_t dwSectorOffset0 = hf->SectorOffsets[i];

                /* Every following sector offset must be bigger than the previous one */
                if(dwSectorOffset1 <= dwSectorOffset0)
                {
                    bSectorOffsetTableCorrupt = 1;
                    break;
                }

                /* The sector size must not be bigger than compressed file size */
                /* Edit: Yes, but apparently, in original Storm.dll, the compressed */
                /* size is not checked anywhere. However, we need to do this check */
                /* in order to sector offset table malformed by MPQ protectors */
                if((dwSectorOffset1 - dwSectorOffset0) > ha->dwSectorSize)
                {
                    bSectorOffsetTableCorrupt = 1;
                    break;
                }
            }

            /* If data corruption detected, free the sector offset table */
            if(bSectorOffsetTableCorrupt)
            {
                STORM_FREE(hf->SectorOffsets);
                hf->SectorOffsets = NULL;
                return ERROR_FILE_CORRUPT;
            }

            /*
             * There may be various extra uint32_ts loaded after the sector offset table.
             * They are mostly empty on WoW release MPQs, but on MPQs from PTR,
             * they contain random non-zero data. Their meaning is unknown.
             *
             * These extra values are, however, include in the dwCmpSize in the file
             * table. We cannot ignore them, because compacting archive would fail
             */ 

            if(hf->SectorOffsets[0] > dwSectorOffsLen)
            {
                /* MPQ protectors put some ridiculous values there. We must limit the extra bytes */
                if(hf->SectorOffsets[0] > (dwSectorOffsLen + 0x400))
                    return ERROR_FILE_CORRUPT;
                
                /* Free the old sector offset table */
                dwSectorOffsLen = hf->SectorOffsets[0];
                STORM_FREE(hf->SectorOffsets);
                goto __LoadSectorOffsets;
            }
        }
        else
        {
            memset(hf->SectorOffsets, 0, dwSectorOffsLen);
            hf->SectorOffsets[0] = dwSectorOffsLen;
        }
    }

    return ERROR_SUCCESS;
}

int AllocateSectorChecksums(TMPQFile * hf, int bLoadFromFile)
{
    TMPQArchive * ha = hf->ha;
    TFileEntry * pFileEntry = hf->pFileEntry;
    uint64_t RawFilePos;
    uint32_t dwCompressedSize = 0;
    uint32_t dwExpectedSize;
    uint32_t dwCrcOffset;                      /* Offset of the CRC table, relative to file offset in the MPQ */
    uint32_t dwCrcSize;

    /* Caller of AllocateSectorChecksums must ensure these */
    assert(hf->SectorChksums == NULL);
    assert(hf->SectorOffsets != NULL);
    assert(hf->pFileEntry != NULL);
    assert(hf->ha != NULL);

    /* Single unit files don't have sector checksums */
    if(pFileEntry->dwFlags & MPQ_FILE_SINGLE_UNIT)
        return ERROR_SUCCESS;

    /* Caller must ensure that we are only called when we have sector checksums */
    assert(pFileEntry->dwFlags & MPQ_FILE_SECTOR_CRC);

    /* 
     * Older MPQs store an array of CRC32's after
     * the raw file data in the MPQ.
     *
     * In newer MPQs, the (since Cataclysm BETA) the (attributes) file
     * contains additional 32-bit values beyond the sector table.
     * Their number depends on size of the (attributes), but their
     * meaning is unknown. They are usually zeroed in retail game files,
     * but contain some sort of checksum in BETA MPQs
     */

    /* Does the size of the file table match with the CRC32-based checksums? */
    dwExpectedSize = (hf->dwSectorCount + 2) * sizeof(uint32_t);
    if(hf->SectorOffsets[0] != 0 && hf->SectorOffsets[0] == dwExpectedSize)
    {
        /* If we are not loading from the MPQ file, we just allocate the sector table */
        /* In that case, do not check any sizes */
        if(!bLoadFromFile)
        {
            hf->SectorChksums = STORM_ALLOC(uint32_t, hf->dwSectorCount);
            if(hf->SectorChksums == NULL)
                return ERROR_NOT_ENOUGH_MEMORY;

            /* Fill the checksum table with zeros */
            memset(hf->SectorChksums, 0, hf->dwSectorCount * sizeof(uint32_t));
            return ERROR_SUCCESS;
        }
        else
        {
            /* Is there valid size of the sector checksums? */
            if(hf->SectorOffsets[hf->dwSectorCount + 1] >= hf->SectorOffsets[hf->dwSectorCount])
                dwCompressedSize = hf->SectorOffsets[hf->dwSectorCount + 1] - hf->SectorOffsets[hf->dwSectorCount];

            /* Ignore cases when the length is too small or too big. */
            if(dwCompressedSize < sizeof(uint32_t) || dwCompressedSize > hf->dwSectorSize)
                return ERROR_SUCCESS;

            /* Calculate offset of the CRC table */
            dwCrcSize = hf->dwSectorCount * sizeof(uint32_t);
            dwCrcOffset = hf->SectorOffsets[hf->dwSectorCount];
            RawFilePos = CalculateRawSectorOffset(hf, dwCrcOffset); 

            /* Now read the table from the MPQ */
            hf->SectorChksums = (uint32_t *)LoadMpqTable(ha, RawFilePos, dwCompressedSize, dwCrcSize, 0, NULL);
            if(hf->SectorChksums == NULL)
                return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    /* If the size doesn't match, we ignore sector checksums */
/*  assert(0); */
    return ERROR_SUCCESS;
}

int WritePatchInfo(TMPQFile * hf)
{
    TMPQArchive * ha = hf->ha;
    TPatchInfo * pPatchInfo = hf->pPatchInfo;

    /* The caller must make sure that this function is only called */
    /* when the following is true. */
    assert(hf->pFileEntry->dwFlags & MPQ_FILE_PATCH_FILE);
    assert(pPatchInfo != NULL);

    BSWAP_ARRAY32_UNSIGNED(pPatchInfo, 3 * sizeof(uint32_t));
    if(!FileStream_Write(ha->pStream, &hf->RawFilePos, pPatchInfo, sizeof(TPatchInfo)))
        return GetLastError();

    return ERROR_SUCCESS;
}

int WriteSectorOffsets(TMPQFile * hf)
{
    TMPQArchive * ha = hf->ha;
    TFileEntry * pFileEntry = hf->pFileEntry;
    uint64_t RawFilePos = hf->RawFilePos;
    uint32_t dwSectorOffsLen;

    /* The caller must make sure that this function is only called */
    /* when the following is true. */
    assert(hf->pFileEntry->dwFlags & MPQ_FILE_COMPRESS_MASK);
    assert(hf->SectorOffsets != NULL);
    dwSectorOffsLen = hf->SectorOffsets[0];

    /* If file is encrypted, sector positions are also encrypted */
    if(pFileEntry->dwFlags & MPQ_FILE_ENCRYPTED)
        EncryptMpqBlock(hf->SectorOffsets, dwSectorOffsLen, hf->dwFileKey - 1);
    BSWAP_ARRAY32_UNSIGNED(hf->SectorOffsets, dwSectorOffsLen);

    /* Adjust sector offset table position, if we also have patch info */
    if(hf->pPatchInfo != NULL)
        RawFilePos += hf->pPatchInfo->dwLength;

    /* Write sector offsets to the archive */
    if(!FileStream_Write(ha->pStream, &RawFilePos, hf->SectorOffsets, dwSectorOffsLen))
        return GetLastError();
    
    /* Not necessary, as the sector checksums */
    /* are going to be freed when this is done. */
/*  BSWAP_ARRAY32_UNSIGNED(hf->SectorOffsets, dwSectorOffsLen); */
    return ERROR_SUCCESS;
}


int WriteSectorChecksums(TMPQFile * hf)
{
    TMPQArchive * ha = hf->ha;
    uint64_t RawFilePos;
    TFileEntry * pFileEntry = hf->pFileEntry;
    unsigned char * pbCompressed;
    uint32_t dwCompressedSize = 0;
    uint32_t dwCrcSize;
    int nOutSize;
    int nError = ERROR_SUCCESS;

    /* The caller must make sure that this function is only called */
    /* when the following is true. */
    assert(hf->pFileEntry->dwFlags & MPQ_FILE_SECTOR_CRC);
    assert(hf->SectorOffsets != NULL);
    assert(hf->SectorChksums != NULL);

    /* If the MPQ has MD5 of each raw data chunk, */
    /* we leave sector offsets empty */
    if(ha->pHeader->dwRawChunkSize != 0)
    {
        hf->SectorOffsets[hf->dwSectorCount + 1] = hf->SectorOffsets[hf->dwSectorCount];
        return ERROR_SUCCESS;
    }

    /* Calculate size of the checksum array */
    dwCrcSize = hf->dwSectorCount * sizeof(uint32_t);

    /* Allocate buffer for compressed sector CRCs. */
    pbCompressed = STORM_ALLOC(uint8_t, dwCrcSize);
    if(pbCompressed == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Perform the compression */
    BSWAP_ARRAY32_UNSIGNED(hf->SectorChksums, dwCrcSize);

    nOutSize = (int)dwCrcSize;
    SCompCompress(pbCompressed, &nOutSize, hf->SectorChksums, (int)dwCrcSize, MPQ_COMPRESSION_ZLIB, 0, 0);
    dwCompressedSize = (uint32_t)nOutSize;

    /* Write the sector CRCs to the archive */
    RawFilePos = hf->RawFilePos + hf->SectorOffsets[hf->dwSectorCount];
    if(hf->pPatchInfo != NULL)
        RawFilePos += hf->pPatchInfo->dwLength;
    if(!FileStream_Write(ha->pStream, &RawFilePos, pbCompressed, dwCompressedSize))
        nError = GetLastError();

    /* Not necessary, as the sector checksums */
    /* are going to be freed when this is done. */
/*  BSWAP_ARRAY32_UNSIGNED(hf->SectorChksums, dwCrcSize); */

    /* Store the sector CRCs */
    hf->SectorOffsets[hf->dwSectorCount + 1] = hf->SectorOffsets[hf->dwSectorCount] + dwCompressedSize;
    pFileEntry->dwCmpSize += dwCompressedSize;
    STORM_FREE(pbCompressed);
    return nError;
}

int WriteMemDataMD5(
    TFileStream * pStream,
    uint64_t RawDataOffs,
    void * pvRawData,
    uint32_t dwRawDataSize,
    uint32_t dwChunkSize,
    uint32_t * pcbTotalSize)
{
    unsigned char * md5_array;
    unsigned char * md5;
    unsigned char * pbRawData = (unsigned char *)pvRawData;
    uint32_t dwBytesRemaining = dwRawDataSize;
    uint32_t dwMd5ArraySize = 0;
    int nError = ERROR_SUCCESS;

    /* Allocate buffer for array of MD5 */
    md5_array = md5 = AllocateMd5Buffer(dwRawDataSize, dwChunkSize, &dwMd5ArraySize);
    if(md5_array == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* For every file chunk, calculate MD5 */
    while(dwBytesRemaining != 0)
    {
        /* Get the remaining number of bytes to read */
        dwChunkSize = STORMLIB_MIN(dwBytesRemaining, dwChunkSize);

        /* Calculate MD5 */
        CalculateDataBlockHash(pbRawData, dwChunkSize, md5);
        md5 += MD5_DIGEST_SIZE;

        /* Move offset and size */
        dwBytesRemaining -= dwChunkSize;
        pbRawData += dwChunkSize;
    }

    /* Write the array od MD5's to the file */
    RawDataOffs += dwRawDataSize;
    if(!FileStream_Write(pStream, &RawDataOffs, md5_array, dwMd5ArraySize))
        nError = GetLastError();

    /* Give the caller the size of the MD5 array */
    if(pcbTotalSize != NULL)
        *pcbTotalSize = dwRawDataSize + dwMd5ArraySize;

    /* Free buffers and exit */
    STORM_FREE(md5_array);
    return nError;
}


/* Writes the MD5 for each chunk of the raw file data */
int WriteMpqDataMD5(
    TFileStream * pStream,
    uint64_t RawDataOffs,
    uint32_t dwRawDataSize,
    uint32_t dwChunkSize)
{
    unsigned char * md5_array;
    unsigned char * md5;
    unsigned char * pbFileChunk;
    uint32_t dwMd5ArraySize = 0;
    uint32_t dwToRead = dwRawDataSize;
    int nError = ERROR_SUCCESS;

    /* Allocate buffer for array of MD5 */
    md5_array = md5 = AllocateMd5Buffer(dwRawDataSize, dwChunkSize, &dwMd5ArraySize);
    if(md5_array == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Allocate space for file chunk */
    pbFileChunk = STORM_ALLOC(uint8_t, dwChunkSize);
    if(pbFileChunk == NULL)
    {
        STORM_FREE(md5_array);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* For every file chunk, calculate MD5 */
    while(dwRawDataSize != 0)
    {
        /* Get the remaining number of bytes to read */
        dwToRead = STORMLIB_MIN(dwRawDataSize, dwChunkSize);

        /* Read the chunk */
        if(!FileStream_Read(pStream, &RawDataOffs, pbFileChunk, dwToRead))
        {
            nError = GetLastError();
            break;
        }

        /* Calculate MD5 */
        CalculateDataBlockHash(pbFileChunk, dwToRead, md5);
        md5 += MD5_DIGEST_SIZE;

        /* Move offset and size */
        RawDataOffs += dwToRead;
        dwRawDataSize -= dwToRead;
    }

    /* Write the array od MD5's to the file */
    if(nError == ERROR_SUCCESS)
    {
        if(!FileStream_Write(pStream, NULL, md5_array, dwMd5ArraySize))
            nError = GetLastError();
    }

    /* Free buffers and exit */
    STORM_FREE(pbFileChunk);
    STORM_FREE(md5_array);
    return nError;
}

/* Frees the structure for MPQ file */
void FreeFileHandle(TMPQFile ** hf)
{
    if(*hf != NULL)
    {
        /* If we have patch file attached to this one, free it first */
        if((*hf)->hfPatch != NULL)
            FreeFileHandle(&((*hf)->hfPatch));

        /* Then free all buffers allocated in the file structure */
        if((*hf)->pbFileData != NULL)
            STORM_FREE((*hf)->pbFileData);
        if((*hf)->pPatchHeader != NULL)
            STORM_FREE((*hf)->pPatchHeader);
        if((*hf)->pPatchInfo != NULL)
            STORM_FREE((*hf)->pPatchInfo);
        if((*hf)->SectorOffsets != NULL)
            STORM_FREE((*hf)->SectorOffsets);
        if((*hf)->SectorChksums != NULL)
            STORM_FREE((*hf)->SectorChksums);
        if((*hf)->pbFileSector != NULL)
            STORM_FREE((*hf)->pbFileSector);
        if((*hf)->pStream != NULL)
            FileStream_Close((*hf)->pStream);
        STORM_FREE(*hf);
        *hf = NULL;
    }
}

/* Frees the MPQ archive */
void FreeArchiveHandle(TMPQArchive ** ha)
{
    if(*ha != NULL)
    {
        /* First of all, free the patch archive, if any */
        if((*ha)->haPatch != NULL)
            FreeArchiveHandle(&((*ha)->haPatch));

        /* Free the patch prefix, if any */
        if((*ha)->pPatchPrefix != NULL)
            STORM_FREE((*ha)->pPatchPrefix);

        /* Close the file stream */
        FileStream_Close((*ha)->pStream);
        (*ha)->pStream = NULL;

        /* Free the file names from the file table */
        if((*ha)->pFileTable != NULL)
        {
            uint32_t i;
            
            for(i = 0; i < (*ha)->dwFileTableSize; i++)
            {
                if((*ha)->pFileTable[i].szFileName != NULL)
                    STORM_FREE((*ha)->pFileTable[i].szFileName);
                (*ha)->pFileTable[i].szFileName = NULL;
            }

            /* Then free all buffers allocated in the archive structure */
            STORM_FREE((*ha)->pFileTable);
        }

        if((*ha)->pHashTable != NULL)
            STORM_FREE((*ha)->pHashTable);
        if((*ha)->pHetTable != NULL)
            FreeHetTable((*ha)->pHetTable);
        STORM_FREE(*ha);
        *ha = NULL;
    }
}

int IsInternalMpqFileName(const char * szFileName)
{
    if(szFileName != NULL && szFileName[0] == '(')
    {
        if(!strcasecmp(szFileName, LISTFILE_NAME) ||
           !strcasecmp(szFileName, ATTRIBUTES_NAME) ||
           !strcasecmp(szFileName, SIGNATURE_NAME))
        {
            return 1;
        }
    }

    return 0;
}

/* Verifies if the file name is a pseudo-name */
int IsPseudoFileName(const char * szFileName, uint32_t * pdwFileIndex)
{
    uint32_t dwFileIndex = 0;

    if(szFileName != NULL)
    {
        /* Must be "File########.ext" */
        if(!strncasecmp(szFileName, "File", 4))
        {
            int i;
            
            /* Check 8 digits */
            for(i = 4; i < 4+8; i++)
            {
                if(szFileName[i] < '0' || szFileName[i] > '9')
                    return 0;
                dwFileIndex = (dwFileIndex * 10) + (szFileName[i] - '0');
            }

            /* An extension must follow */
            if(szFileName[12] == '.')
            {
                if(pdwFileIndex != NULL)
                    *pdwFileIndex = dwFileIndex;
                return 1;
            }
        }
    }

    /* Not a pseudo-name */
    return 0;
}

/*-----------------------------------------------------------------------------
 * Functions calculating and verifying the MD5 signature
 */

int IsValidMD5(unsigned char * pbMd5)
{
    uint32_t * Md5 = (uint32_t *)pbMd5;

    return (Md5[0] | Md5[1] | Md5[2] | Md5[3]) ? 1 : 0;
}

int IsValidSignature(unsigned char * pbSignature)
{
    uint32_t * Signature = (uint32_t *)pbSignature;
    uint32_t SigValid = 0;
    int i;

    for(i = 0; i < MPQ_WEAK_SIGNATURE_SIZE / sizeof(uint32_t); i++)
        SigValid |= Signature[i];

    return (SigValid != 0) ? 1 : 0;
}


int VerifyDataBlockHash(void * pvDataBlock, uint32_t cbDataBlock, unsigned char * expected_md5)
{
    hash_state md5_state;
    uint8_t md5_digest[MD5_DIGEST_SIZE];

    /* Don't verify the block if the MD5 is not valid. */
    if(!IsValidMD5(expected_md5))
        return 1;

    /* Calculate the MD5 of the data block */
    md5_init(&md5_state);
    md5_process(&md5_state, (unsigned char *)pvDataBlock, cbDataBlock);
    md5_done(&md5_state, md5_digest);

    /* Does the MD5's match? */
    return (memcmp(md5_digest, expected_md5, MD5_DIGEST_SIZE) == 0);
}

void CalculateDataBlockHash(void * pvDataBlock, uint32_t cbDataBlock, unsigned char * md5_hash)
{
    hash_state md5_state;

    md5_init(&md5_state);
    md5_process(&md5_state, (unsigned char *)pvDataBlock, cbDataBlock);
    md5_done(&md5_state, md5_hash);
}


/*-----------------------------------------------------------------------------
 * Swapping functions
 */

#ifndef PLATFORM_LITTLE_ENDIAN

/*
 * Note that those functions are implemented for Mac operating system,
 * as this is the only supported platform that uses big endian.
 */

/* Swaps a signed 16-bit integer */
int16_t SwapInt16(uint16_t data)
{
	return (int16_t)CFSwapInt16(data);
}

/* Swaps an unsigned 16-bit integer */
uint16_t SwapUInt16(uint16_t data)
{
	return CFSwapInt16(data);
}

/* Swaps signed 32-bit integer */
int32_t SwapInt32(uint32_t data)
{
	return (int32_t)CFSwapInt32(data);
}

/* Swaps an unsigned 32-bit integer */
uint32_t SwapUInt32(uint32_t data)
{
	return CFSwapInt32(data);
}

/* Swaps signed 64-bit integer */
int64_t SwapInt64(int64_t data)
{
       return (int64_t)CFSwapInt64(data);
}

/* Swaps an unsigned 64-bit integer */
uint64_t SwapUInt64(uint64_t data)
{
       return CFSwapInt64(data);
}

/* Swaps array of unsigned 16-bit integers */
void ConvertUInt16Buffer(void * ptr, size_t length)
{
    uint16_t * buffer = (uint16_t *)ptr;
    uint32_t nElements = (uint32_t)(length / sizeof(uint16_t));

    while(nElements-- > 0)
	{
		*buffer = SwapUInt16(*buffer);
		buffer++;
	}
}

/* Swaps array of unsigned 32-bit integers */
void ConvertUInt32Buffer(void * ptr, size_t length)
{
    uint32_t * buffer = (uint32_t *)ptr;
    uint32_t nElements = (uint32_t)(length / sizeof(uint32_t));

	while(nElements-- > 0)
	{
		*buffer = SwapUInt32(*buffer);
		buffer++;
	}
}

/* Swaps array of unsigned 64-bit integers */
void ConvertUInt64Buffer(void * ptr, size_t length)
{
    uint64_t * buffer = (uint64_t *)ptr;
    uint32_t nElements = (uint32_t)(length / sizeof(uint64_t));

	while(nElements-- > 0)
	{
		*buffer = SwapUInt64(*buffer);
		buffer++;
	}
}

/* Swaps the TMPQHeader structure */
void ConvertTMPQHeader(void *header, uint16_t version)
{
	TMPQHeader * theHeader = (TMPQHeader *)header;

    /* Swap header part version 1 */
    if(version == MPQ_FORMAT_VERSION_1)
    {
	    theHeader->dwID = SwapUInt32(theHeader->dwID);
	    theHeader->dwHeaderSize = SwapUInt32(theHeader->dwHeaderSize);
	    theHeader->dwArchiveSize = SwapUInt32(theHeader->dwArchiveSize);
	    theHeader->wFormatVersion = SwapUInt16(theHeader->wFormatVersion);
	    theHeader->wSectorSize = SwapUInt16(theHeader->wSectorSize);
	    theHeader->dwHashTablePos = SwapUInt32(theHeader->dwHashTablePos);
	    theHeader->dwBlockTablePos = SwapUInt32(theHeader->dwBlockTablePos);
	    theHeader->dwHashTableSize = SwapUInt32(theHeader->dwHashTableSize);
	    theHeader->dwBlockTableSize = SwapUInt32(theHeader->dwBlockTableSize);
    }

	if(version == MPQ_FORMAT_VERSION_2)
	{
		theHeader->HiBlockTablePos64 = SwapUInt64(theHeader->HiBlockTablePos64);
        theHeader->wHashTablePosHi = SwapUInt16(theHeader->wHashTablePosHi);
		theHeader->wBlockTablePosHi = SwapUInt16(theHeader->wBlockTablePosHi);
    }

    if(version == MPQ_FORMAT_VERSION_3)
    {
        theHeader->ArchiveSize64 = SwapUInt64(theHeader->ArchiveSize64);
        theHeader->BetTablePos64 = SwapUInt64(theHeader->BetTablePos64);
        theHeader->HetTablePos64 = SwapUInt64(theHeader->HetTablePos64);
    }

    if(version == MPQ_FORMAT_VERSION_4)
	{
        theHeader->HashTableSize64    = SwapUInt64(theHeader->HashTableSize64);
        theHeader->BlockTableSize64   = SwapUInt64(theHeader->BlockTableSize64);
        theHeader->HiBlockTableSize64 = SwapUInt64(theHeader->HiBlockTableSize64);
        theHeader->HetTableSize64     = SwapUInt64(theHeader->HetTableSize64);
        theHeader->BetTableSize64     = SwapUInt64(theHeader->BetTableSize64);
    }
}

#endif  /* PLATFORM_LITTLE_ENDIAN */
