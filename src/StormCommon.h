/*****************************************************************************/
/* SCommon.h                              Copyright (c) Ladislav Zezula 2003 */
/*---------------------------------------------------------------------------*/
/* Common functions for encryption/decryption from Storm.dll. Included by    */
/* SFile*** functions, do not include and do not use this file directly      */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 24.03.03  1.00  Lad  The first version of SFileCommon.h                   */
/* 12.06.04  1.00  Lad  Renamed to SCommon.h                                 */
/* 06.09.10  1.00  Lad  Renamed to StormCommon.h                             */
/* 10.03.15  1.00  Ayr  Ported to plain c                                    */
/*****************************************************************************/

#ifndef _STORMCOMMON_H
#define _STORMCOMMON_H

#include "thunderStorm.h"
#include "config.h"

/* Definitions */
#define EXPORT_SYMBOL __attribute__ ((visibility ("default")))

/*-----------------------------------------------------------------------------
 * Compression support
 */

/* Include functions from Pkware Data Compression Library */
#include "pklib/pklib.h"

/* Include functions from Huffmann compression */
#include "huffman/huff.h"

/* Include functions from IMA ADPCM compression */
#include "adpcm/adpcm.h"

/* Include functions from SPARSE compression */
#include "sparse/sparse.h"

/* Include functions from LZMA compression */
#include "lzma/C/LzmaEnc.h"
#include "lzma/C/LzmaDec.h"

/* Include functions from zlib */
#ifndef __SYS_ZLIB
  #include "zlib/zlib.h"
#else
  #include <zlib.h>
#endif

/* Include functions from bzlib */
#ifndef __SYS_BZLIB
  #include "bzip2/bzlib.h"
#else
  #include <bzlib.h>
#endif

/*-----------------------------------------------------------------------------
 * Cryptography support
 */

/* Headers from LibTomCrypt */
#include "libtomcrypt/src/headers/tomcrypt.h"

/* For HashStringJenkins */
#include "jenkins/lookup.h"

/* Anubis cipher */
#include "anubis/anubis.h"

/* Serpent cipher */
#include "serpent/serpent.h"

/*-----------------------------------------------------------------------------
 * StormLib private defines
 */

#define ID_MPQ_FILE            0x46494c45     /* Used internally for checking TMPQFile ('FILE') */

/* Prevent problems with CRT "min" and "max" functions, */
/* as they are not defined on all platforms */
#define STORMLIB_MIN(a, b) ((a < b) ? a : b)
#define STORMLIB_MAX(a, b) ((a > b) ? a : b)
#define STORMLIB_UNUSED(p) ((void)(p))

/* Macro for building 64-bit file offset from two 32-bit */
#define MAKE_OFFSET64(hi, lo)      (((uint64_t)hi << 32) | (uint64_t)lo)

/* Archive handle structure */
typedef struct _TMPQArchive
{
    TFileStream  * pStream;                     /* Open stream for the MPQ */

    uint64_t      UserDataPos;                 /* Position of user data (relative to the begin of the file) */
    uint64_t      MpqPos;                      /* MPQ header offset (relative to the begin of the file) */

    struct _TMPQArchive * haPatch;              /* Pointer to patch archive, if any */
    struct _TMPQArchive * haBase;               /* Pointer to base ("previous version") archive, if any */
    TMPQNamePrefix * pPatchPrefix;              /* Patch prefix to precede names of patch files */

    TMPQUserData * pUserData;                   /* MPQ user data (NULL if not present in the file) */
    TMPQHeader   * pHeader;                     /* MPQ file header */
    TMPQHash     * pHashTable;                  /* Hash table */
    TMPQHetTable * pHetTable;                   /* HET table */
    TFileEntry   * pFileTable;                  /* File table */
    HASH_STRING    pfnHashString;               /* Hashing function that will convert the file name into hash */
    
    TMPQUserData   UserData;                    /* MPQ user data. Valid only when ID_MPQ_USERDATA has been found */
    uint32_t          HeaderData[MPQ_HEADER_WORDS];  /* Storage for MPQ header */

    uint32_t          dwHETBlockSize;
    uint32_t          dwBETBlockSize;
    uint32_t          dwMaxFileCount;              /* Maximum number of files in the MPQ. Also total size of the file table. */
    uint32_t          dwHashTableSize;             /* Size of the hash table. Different from hash table size in the header if the hash table was shrunk */
    uint32_t          dwFileTableSize;             /* Current size of the file table, e.g. index of the entry past the last occupied one */
    uint32_t          dwReservedFiles;             /* Number of entries reserved for internal MPQ files (listfile, attributes) */
    uint32_t          dwSectorSize;                /* Default size of one file sector */
    uint32_t          dwFileFlags1;                /* Flags for (listfile) */
    uint32_t          dwFileFlags2;                /* Flags for (attributes) */
    uint32_t          dwFileFlags3;                /* Flags for (signature) */
    uint32_t          dwAttrFlags;                 /* Flags for the (attributes) file, see MPQ_ATTRIBUTE_XXX */
    uint32_t          dwFlags;                     /* See MPQ_FLAG_XXXXX */
    uint32_t          dwSubType;                   /* See MPQ_SUBTYPE_XXX */

    SFILE_ADDFILE_CALLBACK pfnAddFileCB;        /* Callback function for adding files */
    void         * pvAddFileUserData;           /* User data thats passed to the callback */

    SFILE_COMPACT_CALLBACK pfnCompactCB;        /* Callback function for compacting the archive */
    uint64_t      CompactBytesProcessed;       /* Amount of bytes that have been processed during a particular compact call */
    uint64_t      CompactTotalBytes;           /* Total amount of bytes to be compacted */
    void         * pvCompactUserData;           /* User data thats passed to the callback */
    anubisSchedule_t    keyScheduleAnubis;      /* Key schedule for anubis encryption */
    serpentSchedule_t   keyScheduleSerpent;     /* Key schedule for serpent encryption */
} TMPQArchive;                                      

/* File handle structure */
typedef struct _TMPQFile
{
    TFileStream  * pStream;                     /* File stream. Only used on local files */
    TMPQArchive  * ha;                          /* Archive handle */
    TFileEntry   * pFileEntry;                  /* File entry for the file */
    uint32_t          dwFileKey;                   /* Decryption key */
    uint32_t          dwFilePos;                   /* Current file position */
    uint64_t      RawFilePos;                  /* Offset in MPQ archive (relative to file begin) */
    uint64_t      MpqFilePos;                  /* Offset in MPQ archive (relative to MPQ header) */
    uint32_t          dwMagic;                     /* 'FILE' */

    struct _TMPQFile * hfPatch;                 /* Pointer to opened patch file */
    TPatchHeader * pPatchHeader;                /* Patch header. Only used if the file is a patch file */
    uint8_t *         pbFileData;                  /* Data of the file (single unit files, patched files) */
    uint32_t          cbFileData;                  /* Size of file data */
    uint8_t           FileDataMD5[MD5_DIGEST_SIZE];/* MD5 hash of the loaded file data. Used during patch process */

    TPatchInfo   * pPatchInfo;                  /* Patch info block, preceding the sector table */
    uint32_t        * SectorOffsets;               /* Position of each file sector, relative to the begin of the file. Only for compressed files. */
    uint32_t        * SectorChksums;               /* Array of sector checksums (either ADLER32 or MD5) values for each file sector */
    uint32_t          dwCompression0;              /* Compression that will be used on the first file sector */
    uint32_t          dwSectorCount;               /* Number of sectors in the file */
    uint32_t          dwPatchedFileSize;           /* Size of patched file. Used when saving patch file to the MPQ */
    uint32_t          dwDataSize;                  /* Size of data in the file (on patch files, this differs from file size in block table entry) */

    uint8_t *         pbFileSector;                /* Last loaded file sector. For single unit files, entire file content */
    uint32_t          dwSectorOffs;                /* File position of currently loaded file sector */
    uint32_t          dwSectorSize;                /* Size of the file sector. For single unit files, this is equal to the file size */

    unsigned char  hctx[HASH_STATE_SIZE];       /* Hash state for MD5. Used when saving file to MPQ */
    uint32_t          dwCrc32;                     /* CRC32 value, used when saving file to MPQ */

    int            nAddFileError;               /* Result of the "Add File" operations */

    int           bLoadedSectorCRCs;           /* If true, we already tried to load sector CRCs */
    int           bCheckSectorCRCs;            /* If true, then SFileReadFile will check sector CRCs when reading the file */
    int           bIsWriteHandle;              /* If true, this handle has been created by SFileCreateFile */
} TMPQFile;

/*-----------------------------------------------------------------------------
 * MPQ signature information
 */

/* Size of each signature type */
#define MPQ_WEAK_SIGNATURE_SIZE                 64
#define MPQ_STRONG_SIGNATURE_SIZE              256 
#define MPQ_SECURE_SIGNATURE_SIZE              512
#define MPQ_STRONG_SIGNATURE_ID         0x5349474E      /* ID of the strong signature ("NGIS") */
#define MPQ_SIGNATURE_FILE_SIZE (MPQ_WEAK_SIGNATURE_SIZE + 8)

/* MPQ signature info */
typedef struct _MPQ_SIGNATURE_INFO
{
    uint64_t BeginMpqData;                     /* File offset where the hashing starts */
    uint64_t BeginExclude;                     /* Begin of the excluded area (used for (signature) file) */
    uint64_t EndExclude;                       /* End of the excluded area (used for (signature) file) */
    uint64_t EndMpqData;                       /* File offset where the hashing ends */
    uint64_t EndOfFile;                        /* Size of the entire file */
    uint8_t  Signature[MPQ_SECURE_SIGNATURE_SIZE + 0x10];
    uint32_t cbSignatureSize;                      /* Length of the signature */
    uint32_t SignatureTypes;                       /* See SIGNATURE_TYPE_XXX */

} MPQ_SIGNATURE_INFO, *PMPQ_SIGNATURE_INFO;

/*-----------------------------------------------------------------------------
 * Memory management
 *
 * We use our own macros for allocating/freeing memory. If you want
 * to redefine them, please keep the following rules:
 *
 *  - The memory allocation must return NULL if not enough memory
 *    (i.e not to throw exception)
 *  - The allocating function does not need to fill the allocated buffer with zeros
 *  - Memory freeing function doesn't have to test the pointer to NULL
 */

#define STORM_ALLOC(type, nitems) (type *)malloc((nitems) * sizeof(type))
#define STORM_FREE(ptr)           free(ptr)

/*-----------------------------------------------------------------------------
 * StormLib internal global variables
 */

extern uint32_t lcFileLocale;                       /* Preferred file locale */

/*-----------------------------------------------------------------------------
 * Conversion to uppercase/lowercase (and "/" to "\")
 */

extern unsigned char AsciiToLowerTable[256];
extern unsigned char AsciiToUpperTable[256];

/*-----------------------------------------------------------------------------
 * Encryption and decryption functions
 */

#define MPQ_HASH_TABLE_INDEX    0x000
#define MPQ_HASH_NAME_A         0x100
#define MPQ_HASH_NAME_B         0x200
#define MPQ_HASH_FILE_KEY       0x300
#define MPQ_HASH_KEY2_MIX       0x400

uint32_t HashString(const char * szFileName, uint32_t dwHashType);
uint32_t HashStringSlash(const char * szFileName, uint32_t dwHashType);
uint32_t HashStringLower(const char * szFileName, uint32_t dwHashType);

void  InitializeMpqCryptography();

uint32_t GetHashTableSizeForFileCount(uint32_t dwFileCount);

int IsPseudoFileName(const char * szFileName, uint32_t * pdwFileIndex);
uint64_t HashStringJenkins(const char * szFileName);

uint32_t GetDefaultSpecialFileFlags(uint32_t dwFileSize, uint16_t wFormatVersion);

void  EncryptMpqBlock(void * pvDataBlock, uint32_t dwLength, uint32_t dwKey);
void  DecryptMpqBlock(void * pvDataBlock, uint32_t dwLength, uint32_t dwKey);
void  EncryptMpqBlockAnubis(void * pvDataBlock, uint32_t dwLength, anubisSchedule_t * keySchedule);
void  DecryptMpqBlockAnubis(void * pvDataBlock, uint32_t dwLength, anubisSchedule_t * keySchedule);
void  EncryptMpqBlockSerpent(void * pvDataBlock, uint32_t dwLength, serpentSchedule_t * keySchedule);
void  DecryptMpqBlockSerpent(void * pvDataBlock, uint32_t dwLength, serpentSchedule_t * keySchedule);

uint32_t DetectFileKeyBySectorSize(uint32_t * EncryptedData, uint32_t dwSectorSize, uint32_t dwSectorOffsLen);
uint32_t DetectFileKeyByContent(void * pvEncryptedData, uint32_t dwSectorSize, uint32_t dwFileSize);
uint32_t DecryptFileKey(const char * szFileName, uint64_t MpqPos, uint32_t dwFileSize, uint32_t dwFlags);

int IsValidMD5(unsigned char * pbMd5);
int IsValidSignature(unsigned char * pbSignature);
int VerifyDataBlockHash(void * pvDataBlock, uint32_t cbDataBlock, unsigned char * expected_md5);
void CalculateDataBlockHash(void * pvDataBlock, uint32_t cbDataBlock, unsigned char * md5_hash);

/*-----------------------------------------------------------------------------
 * Handle validation functions
 */

TMPQArchive * IsValidMpqHandle(void * hMpq);
TMPQFile * IsValidFileHandle(void * hFile);

/*-----------------------------------------------------------------------------
 * Support for MPQ file tables
 */

uint64_t FileOffsetFromMpqOffset(TMPQArchive * ha, uint64_t MpqOffset);
uint64_t CalculateRawSectorOffset(TMPQFile * hf, uint32_t dwSectorOffset);

int ConvertMpqHeaderToFormat4(TMPQArchive * ha, uint64_t MpqOffset, uint64_t FileSize, uint32_t dwFlags);

TMPQHash * FindFreeHashEntry(TMPQArchive * ha, uint32_t dwStartIndex, uint32_t dwName1, uint32_t dwName2, uint32_t lcLocale);
TMPQHash * GetFirstHashEntry(TMPQArchive * ha, const char * szFileName);
TMPQHash * GetNextHashEntry(TMPQArchive * ha, TMPQHash * pFirstHash, TMPQHash * pPrevHash);
TMPQHash * AllocateHashEntry(TMPQArchive * ha, TFileEntry * pFileEntry);

TMPQExtHeader * LoadExtTable(TMPQArchive * ha, uint64_t ByteOffset, size_t Size, uint32_t dwSignature, uint32_t dwKey);
TMPQHetTable * LoadHetTable(TMPQArchive * ha);
TMPQBetTable * LoadBetTable(TMPQArchive * ha);

TMPQHash * LoadHashTable(TMPQArchive * ha);
TMPQBlock * LoadBlockTable(TMPQArchive * ha);
TMPQBlock * TranslateBlockTable(TMPQArchive * ha, uint64_t * pcbTableSize, int * pbNeedHiBlockTable);

uint64_t FindFreeMpqSpace(TMPQArchive * ha);

/* Functions that load the HET and BET tables */
int  CreateHashTable(TMPQArchive * ha, uint32_t dwHashTableSize);
int  LoadAnyHashTable(TMPQArchive * ha);
int  BuildFileTable(TMPQArchive * ha);
int  DefragmentFileTable(TMPQArchive * ha);
int  ShrinkMalformedMpqTables(TMPQArchive * ha);
int  RebuildHetTable(TMPQArchive * ha);
int  RebuildFileTable(TMPQArchive * ha, uint32_t dwNewHashTableSize, uint32_t dwNewMaxFileCount);
int  SaveMPQTables(TMPQArchive * ha);

TMPQHetTable * CreateHetTable(uint32_t dwEntryCount, uint32_t dwTotalCount, uint32_t dwHashBitSize, unsigned char * pbSrcData);
void FreeHetTable(TMPQHetTable * pHetTable);

TMPQBetTable * CreateBetTable(uint32_t dwMaxFileCount);
void FreeBetTable(TMPQBetTable * pBetTable);

/* Functions for finding files in the file table */
TFileEntry * GetFileEntryAny(TMPQArchive * ha, const char * szFileName);
TFileEntry * GetFileEntryLocale(TMPQArchive * ha, const char * szFileName, uint32_t lcLocale);
TFileEntry * GetFileEntryExact(TMPQArchive * ha, const char * szFileName, uint32_t lcLocale);
TFileEntry * GetFileEntryByIndex(TMPQArchive * ha, uint32_t dwIndex);

/* Allocates file name in the file entry */
void AllocateFileName(TMPQArchive * ha, TFileEntry * pFileEntry, const char * szFileName);

/* Allocates new file entry in the MPQ tables. Reuses existing, if possible */
TFileEntry * AllocateFileEntry(TMPQArchive * ha, const char * szFileName, uint32_t lcLocale);
int  RenameFileEntry(TMPQArchive * ha, TFileEntry * pFileEntry, const char * szNewFileName);
void DeleteFileEntry(TMPQArchive * ha, TFileEntry * pFileEntry);

/* Invalidates entries for (listfile) and (attributes) */
void InvalidateInternalFiles(TMPQArchive * ha);

/* Retrieves information about the strong signature */
int QueryMpqSignatureInfo(TMPQArchive * ha, PMPQ_SIGNATURE_INFO pSignatureInfo);

/*-----------------------------------------------------------------------------
 * Support for alternate file formats (SBaseSubTypes.cpp)
 */

int ConvertSqpHeaderToFormat4(TMPQArchive * ha, uint64_t FileSize, uint32_t dwFlags);
TMPQHash * LoadSqpHashTable(TMPQArchive * ha);
TMPQBlock * LoadSqpBlockTable(TMPQArchive * ha);

int ConvertMpkHeaderToFormat4(TMPQArchive * ha, uint64_t FileSize, uint32_t dwFlags);
void DecryptMpkTable(void * pvMpkTable, size_t cbSize);
TMPQHash * LoadMpkHashTable(TMPQArchive * ha);
TMPQBlock * LoadMpkBlockTable(TMPQArchive * ha);
int SCompDecompressMpk(void * pvOutBuffer, int * pcbOutBuffer, void * pvInBuffer, int cbInBuffer);

/*-----------------------------------------------------------------------------
 * Common functions - MPQ File
 */

TMPQFile * CreateFileHandle(TMPQArchive * ha, TFileEntry * pFileEntry);
void * LoadMpqTable(TMPQArchive * ha, uint64_t ByteOffset, uint32_t dwCompressedSize, uint32_t dwTableSize, uint32_t dwKey, int * pbTableIsCut);
int  AllocateSectorBuffer(TMPQFile * hf);
int  AllocatePatchInfo(TMPQFile * hf, int bLoadFromFile);
int  AllocateSectorOffsets(TMPQFile * hf, int bLoadFromFile);
int  AllocateSectorChecksums(TMPQFile * hf, int bLoadFromFile);
int  WritePatchInfo(TMPQFile * hf);
int  WriteSectorOffsets(TMPQFile * hf);
int  WriteSectorChecksums(TMPQFile * hf);
int  WriteMemDataMD5(TFileStream * pStream, uint64_t RawDataOffs, void * pvRawData, uint32_t dwRawDataSize, uint32_t dwChunkSize, uint32_t * pcbTotalSize);
int  WriteMpqDataMD5(TFileStream * pStream, uint64_t RawDataOffs, uint32_t dwRawDataSize, uint32_t dwChunkSize);
void FreeFileHandle(TMPQFile ** hf);

int IsIncrementalPatchFile(const void * pvData, uint32_t cbData, uint32_t * pdwPatchedFileSize);
int  PatchFileData(TMPQFile * hf);

void FreeArchiveHandle(TMPQArchive ** ha);

/*-----------------------------------------------------------------------------
 * Utility functions
 */

int CheckWildCard(const char * szString, const char * szWildCard);
int IsInternalMpqFileName(const char * szFileName);

const char * GetPlainFileName(const char * szFileName);
const char * GetPlainFileName(const char * szFileName);

void CopyFileName(char * szTarget, const char * szSource, size_t cchLength);
void CopyFileName(char * szTarget, const char * szSource, size_t cchLength);

/*-----------------------------------------------------------------------------
 * Support for adding files to the MPQ
 */

int SFileAddFile_Init(
    TMPQArchive * ha,
    const char * szArchivedName,
    uint64_t ft,
    uint32_t dwFileSize,
    uint32_t lcLocale,
    uint32_t dwFlags,
    TMPQFile ** phf
    );

int SFileAddFile_Write(
    TMPQFile * hf,
    const void * pvData,
    uint32_t dwSize,
    uint32_t dwCompression
    );

int SFileAddFile_Finish(
    TMPQFile * hf
    );

/*-----------------------------------------------------------------------------
 * Attributes support
 */

int  SAttrLoadAttributes(TMPQArchive * ha);
int  SAttrFileSaveToMpq(TMPQArchive * ha);

/*-----------------------------------------------------------------------------
 * Listfile functions
 */

int  SListFileSaveToMpq(TMPQArchive * ha);

/*-----------------------------------------------------------------------------
 * Weak signature support
 */

int SSignFileCreate(TMPQArchive * ha);
int SSignFileFinish(TMPQArchive * ha);

/*-----------------------------------------------------------------------------
 * Dump data support
 */

#ifdef _STORMLIB_DUMP_DATA
void DumpMpqHeader(TMPQHeader * pHeader);
void DumpHetAndBetTable(TMPQHetTable * pHetTable, TMPQBetTable * pBetTable);

#else
#define DumpMpqHeader(h)           /* */
#define DumpHetAndBetTable(h, b)   /* */
#endif

#endif /* _STORMCOMMON_H */

