/*****************************************************************************/
/* thunderStorm.h                                   Copyright (c) Ayron 2015 */
/*---------------------------------------------------------------------------*/
/* libThunderStorm v 1.0                                                     */
/*                                                                           */
/* Author : Ladislav Zezula, Ayron                                           */
/* E-mail : ayron@Shadowdrake.fur                                            */
/* WWW    :                                                                  */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 04.03.15  1.0   Ayr  Created                                              */
/*****************************************************************************/

#ifndef _THUNDERSTORM_H
#define _THUNDERSTORM_H


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>


/*-----------------------------------------------------------------------------
 * Defines
 */

#define THUNDERSTORM_VERSION                0x0100  /* Current version of libThunderStorm (1.0) */
#define THUNDERSTORM_VERSION_STRING         "1.0"  /* String version of libThunderStorm version */

#define ID_MPQ                      0x1A51504D  /* MPQ archive header ID ('MPQ\x1A') */
#define ID_MPQ_USERDATA             0x1B51504D  /* MPQ userdata entry ('MPQ\x1B') */
#define ID_MPK                      0x1A4B504D  /* MPK archive header ID ('MPK\x1A') */

#define ERROR_AVI_FILE                   10000  /* Not a MPQ file, but an AVI file. */
#define ERROR_UNKNOWN_FILE_KEY           ENOKEY /* Returned by SFileReadFile when can't find file key */
#define ERROR_CHECKSUM_ERROR             10002  /* Returned by SFileReadFile when sector CRC doesn't match */
#define ERROR_INTERNAL_FILE              10003  /* The given operation is not allowed on internal file */
#define ERROR_BASE_FILE_MISSING          10004  /* The file is present as incremental patch file, but base file is missing */
#define ERROR_MARKED_FOR_DELETE          10005  /* The file was marked as "deleted" in the MPQ */
#define ERROR_FILE_INCOMPLETE            10006  /* The required file part is missing */
#define ERROR_UNKNOWN_FILE_NAMES         10007  /* A name of at least one file is unknown */

/* Values for SFileCreateArchive */
#define HASH_TABLE_SIZE_MIN         0x00000004  /* Verified: If there is 1 file, hash table size is 4 */
#define HASH_TABLE_SIZE_DEFAULT     0x00001000  /* Default hash table size for empty MPQs */
#define HASH_TABLE_SIZE_MAX         0x00080000  /* Maximum acceptable hash table size */

#define HASH_ENTRY_DELETED          0xFFFFFFFE  /* Block index for deleted entry in the hash table */
#define HASH_ENTRY_FREE             0xFFFFFFFF  /* Block index for free entry in the hash table */

#define HET_ENTRY_DELETED                 0x80  /* NameHash1 value for a deleted entry */
#define HET_ENTRY_FREE                    0x00  /* NameHash1 value for free entry */

#define HASH_STATE_SIZE                   0x60  /* Size of LibTomCrypt's hash_state structure */

/* Values for SFileOpenArchive */
#define SFILE_OPEN_HARD_DISK_FILE            2  /* Open the archive on HDD */
#define SFILE_OPEN_CDROM_FILE                3  /* Open the archive only if it is on CDROM */

/* Values for SFileOpenFile */
#define SFILE_OPEN_FROM_MPQ         0x00000000  /* Open the file from the MPQ archive */
#define SFILE_OPEN_CHECK_EXISTS     0xFFFFFFFC  /* Only check whether the file exists */
#define SFILE_OPEN_BASE_FILE        0xFFFFFFFD  /* Reserved for StormLib internal use */
#define SFILE_OPEN_ANY_LOCALE       0xFFFFFFFE  /* Reserved for StormLib internal use */
#define SFILE_OPEN_LOCAL_FILE       0xFFFFFFFF  /* Open a local file */

/* Flags for TMPQArchive::dwFlags */
#define MPQ_FLAG_READ_ONLY          0x00000001  /* If set, the MPQ has been open for read-only access */
#define MPQ_FLAG_CHANGED            0x00000002  /* If set, the MPQ tables have been changed */
#define MPQ_FLAG_MALFORMED          0x00000004  /* Malformed data structure detected (W3M map protectors) */
#define MPQ_FLAG_HASH_TABLE_CUT     0x00000008  /* The hash table goes beyond EOF */
#define MPQ_FLAG_BLOCK_TABLE_CUT    0x00000010  /* The hash table goes beyond EOF */
#define MPQ_FLAG_CHECK_SECTOR_CRC   0x00000020  /* Checking sector CRC when reading files */
#define MPQ_FLAG_SAVING_TABLES      0x00000040  /* If set, we are saving MPQ internal files and MPQ tables */
#define MPQ_FLAG_PATCH              0x00000080  /* If set, this MPQ is a patch archive */
#define MPQ_FLAG_WAR3_MAP           0x00000100  /* If set, this MPQ is a map for Warcraft III */
#define MPQ_FLAG_LISTFILE_NONE      0x00000200  /* Set when no (listfile) was found in InvalidateInternalFiles */
#define MPQ_FLAG_LISTFILE_NEW       0x00000400  /* Set when (listfile) invalidated by InvalidateInternalFiles */
#define MPQ_FLAG_ATTRIBUTES_NONE    0x00000800  /* Set when no (attributes) was found in InvalidateInternalFiles */
#define MPQ_FLAG_ATTRIBUTES_NEW     0x00001000  /* Set when (attributes) invalidated by InvalidateInternalFiles */
#define MPQ_FLAG_SIGNATURE_NONE     0x00002000  /* Set when no (signature) was found in InvalidateInternalFiles */
#define MPQ_FLAG_SIGNATURE_NEW      0x00004000  /* Set when (signature) invalidated by InvalidateInternalFiles */
#define MPQ_FLAG_FILENAME_UNIX      0x80000000  /* If set, filename isn't changed for hash functions */

/* Values for TMPQArchive::dwSubType */
#define MPQ_SUBTYPE_MPQ             0x00000000  /* The file is a MPQ file (Blizzard games) */
#define MPQ_SUBTYPE_SQP             0x00000001  /* The file is a SQP file (War of the Immortals) */
#define MPQ_SUBTYPE_MPK             0x00000002  /* The file is a MPK file (Longwu Online) */

/* Return value for SFileGetFileSize and SFileSetFilePointer */
#define SFILE_INVALID_SIZE          0xFFFFFFFF
#define SFILE_INVALID_POS           0xFFFFFFFF
#define SFILE_INVALID_ATTRIBUTES    0xFFFFFFFF

/* Flags for SFileAddFile */
#define MPQ_FILE_IMPLODE            0x00000100  /* Implode method (By PKWARE Data Compression Library) */
#define MPQ_FILE_COMPRESS           0x00000200  /* Compress methods (By multiple methods) */
#define MPQ_FILE_ENCRYPTED          0x00010000  /* Indicates whether file is encrypted */
#define MPQ_FILE_FIX_KEY            0x00020000  /* File decryption key has to be fixed */
#define MPQ_FILE_ENCRYPT_ANUBIS     0x00040000  /* Use Anubis to encrypt the file */
#define MPQ_FILE_ENCRYPT_SERPENT    0x00080000  /* Use Serpent to encrypt the file */
#define MPQ_FILE_PATCH_FILE         0x00100000  /* The file is a patch file. Raw file data begin with TPatchInfo structure */
#define MPQ_FILE_SINGLE_UNIT        0x01000000  /* File is stored as a single unit, rather than split into sectors (Thx, Quantam) */
#define MPQ_FILE_DELETE_MARKER      0x02000000  /* File is a deletion marker. Used in MPQ patches, indicating that the file no longer exists. */
#define MPQ_FILE_SECTOR_CRC         0x04000000  /* File has checksums for each sector. */
                                                /* Ignored if file is not compressed or imploded. */

#define MPQ_FILE_COMPRESS_MASK      0x0000FF00  /* Mask for a file being compressed */
#define MPQ_FILE_EXISTS             0x80000000  /* Set if file exists, reset when the file was deleted */
#define MPQ_FILE_REPLACEEXISTING    0x80000000  /* Replace when the file exist (SFileAddFile) */

#define MPQ_FILE_EXISTS_MASK        0xF00000FF  /* These must be either zero or MPQ_FILE_EXISTS */

#define MPQ_FILE_VALID_FLAGS     (MPQ_FILE_IMPLODE        |  \
                                  MPQ_FILE_COMPRESS       |  \
                                  MPQ_FILE_ENCRYPTED      |  \
                                  MPQ_FILE_FIX_KEY        |  \
                                  MPQ_FILE_ENCRYPT_ANUBIS |  \
                                  MPQ_FILE_ENCRYPT_SERPENT|  \
                                  MPQ_FILE_PATCH_FILE     |  \
                                  MPQ_FILE_SINGLE_UNIT    |  \
                                  MPQ_FILE_DELETE_MARKER  |  \
                                  MPQ_FILE_SECTOR_CRC     |  \
                                  MPQ_FILE_EXISTS)

/* Compression types for multiple compressions */
#define MPQ_COMPRESSION_HUFFMANN          0x01  /* Huffmann compression (used on WAVE files only) */
#define MPQ_COMPRESSION_ZLIB              0x02  /* ZLIB compression */
#define MPQ_COMPRESSION_PKWARE            0x08  /* PKWARE DCL compression */
#define MPQ_COMPRESSION_BZIP2             0x10  /* BZIP2 compression (added in Warcraft III) */
#define MPQ_COMPRESSION_SPARSE            0x20  /* Sparse compression (added in Starcraft 2) */
#define MPQ_COMPRESSION_ADPCM_MONO        0x40  /* IMA ADPCM compression (mono) */
#define MPQ_COMPRESSION_ADPCM_STEREO      0x80  /* IMA ADPCM compression (stereo) */
#define MPQ_COMPRESSION_LZMA              0x12  /* LZMA compression. Added in Starcraft 2. This value is NOT a combination of flags. */
#define MPQ_COMPRESSION_NEXT_SAME   0xFFFFFFFF  /* Same compression */

/* Constants for SFileAddWave */
#define MPQ_WAVE_QUALITY_HIGH                0  /* Best quality, the worst compression */
#define MPQ_WAVE_QUALITY_MEDIUM              1  /* Medium quality, medium compression */
#define MPQ_WAVE_QUALITY_LOW                 2  /* Low quality, the best compression */

/* Signatures for HET and BET table */
#define HET_TABLE_SIGNATURE         0x1A544548  /* 'HET\x1a' */
#define BET_TABLE_SIGNATURE         0x1A544542  /* 'BET\x1a' */

/* Decryption keys for MPQ tables */
#define MPQ_KEY_HASH_TABLE          0xC3AF3770  /* Obtained by HashString("(hash table)", MPQ_HASH_FILE_KEY) */
#define MPQ_KEY_BLOCK_TABLE         0xEC83B3A3  /* Obtained by HashString("(block table)", MPQ_HASH_FILE_KEY) */

#define LISTFILE_NAME             "(listfile)"  /* Name of internal listfile */
#define SIGNATURE_NAME           "(signature)"  /* Name of internal signature */
#define ATTRIBUTES_NAME         "(attributes)"  /* Name of internal attributes file */
#define PATCH_METADATA_NAME "(patch_metadata)"

#define MPQ_FORMAT_VERSION_1                 0  /* Up to The Burning Crusade */
#define MPQ_FORMAT_VERSION_2                 1  /* The Burning Crusade and newer */
#define MPQ_FORMAT_VERSION_3                 2  /* WoW Cataclysm Beta */
#define MPQ_FORMAT_VERSION_4                 3  /* WoW Cataclysm and newer */

/* Flags for MPQ attributes */
#define MPQ_ATTRIBUTE_CRC32         0x00000001  /* The "(attributes)" contains CRC32 for each file */
#define MPQ_ATTRIBUTE_FILETIME      0x00000002  /* The "(attributes)" contains file time for each file */
#define MPQ_ATTRIBUTE_MD5           0x00000004  /* The "(attributes)" contains MD5 for each file */
#define MPQ_ATTRIBUTE_PATCH_BIT     0x00000008  /* The "(attributes)" contains a patch bit for each file */
#define MPQ_ATTRIBUTE_ALL           0x0000000F  /* Summary mask */

#define MPQ_ATTRIBUTES_V1                  100  /* (attributes) format version 1.00 */

/* Flags for SFileOpenArchive */
#define BASE_PROVIDER_FILE          0x00000000  /* Base data source is a file */
#define BASE_PROVIDER_MAP           0x00000001  /* Base data source is memory-mapped file */
#define BASE_PROVIDER_HTTP          0x00000002  /* Base data source is a file on web server */
#define BASE_PROVIDER_MASK          0x0000000F  /* Mask for base provider value */

#define STREAM_PROVIDER_FLAT        0x00000000  /* Stream is linear with no offset mapping */
#define STREAM_PROVIDER_PARTIAL     0x00000010  /* Stream is partial file (.part) */
#define STREAM_PROVIDER_MPQE        0x00000020  /* Stream is an encrypted MPQ */
#define STREAM_PROVIDER_BLOCK4      0x00000030  /* 0x4000 per block, text MD5 after each block, max 0x2000 blocks per file */
#define STREAM_PROVIDER_MASK        0x000000F0  /* Mask for stream provider value */

#define STREAM_FLAG_READ_ONLY       0x00000100  /* Stream is read only */
#define STREAM_FLAG_WRITE_SHARE     0x00000200  /* Allow write sharing when open for write */
#define STREAM_FLAG_USE_BITMAP      0x00000400  /* If the file has a file bitmap, load it and use it */
#define STREAM_OPTIONS_MASK         0x0000FF00  /* Mask for stream options */

#define STREAM_PROVIDERS_MASK       0x000000FF  /* Mask to get stream providers */
#define STREAM_FLAGS_MASK           0x0000FFFF  /* Mask for all stream flags (providers+options) */

#define MPQ_OPEN_NO_LISTFILE        0x00010000  /* Don't load the internal listfile */
#define MPQ_OPEN_NO_ATTRIBUTES      0x00020000  /* Don't open the attributes */
#define MPQ_OPEN_NO_HEADER_SEARCH   0x00040000  /* Don't search for the MPQ header past the begin of the file */
#define MPQ_OPEN_FORCE_MPQ_V1       0x00080000  /* Always open the archive as MPQ v 1.00, ignore the "wFormatVersion" variable in the header */
#define MPQ_OPEN_CHECK_SECTOR_CRC   0x00100000  /* On files with MPQ_FILE_SECTOR_CRC, the CRC will be checked when reading file */
#define MPQ_OPEN_PATCH              0x00200000  /* This archive is a patch MPQ. Used internally. */
#define MPQ_OPEN_READ_ONLY          STREAM_FLAG_READ_ONLY
#define MPQ_OPEN_UNIX               MPQ_FLAG_FILENAME_UNIX

/* Flags for SFileCreateArchive */
#define MPQ_CREATE_LISTFILE         0x00100000  /* Also add the (listfile) file */
#define MPQ_CREATE_ATTRIBUTES       0x00200000  /* Also add the (attributes) file */
#define MPQ_CREATE_SIGNATURE        0x00400000  /* Also add the (signature) file */
#define MPQ_CREATE_ARCHIVE_V1       0x00000000  /* Creates archive of version 1 (size up to 4GB) */
#define MPQ_CREATE_ARCHIVE_V2       0x01000000  /* Creates archive of version 2 (larger than 4 GB) */
#define MPQ_CREATE_ARCHIVE_V3       0x02000000  /* Creates archive of version 3 */
#define MPQ_CREATE_ARCHIVE_V4       0x03000000  /* Creates archive of version 4 */
#define MPQ_CREATE_ARCHIVE_VMASK    0x0F000000  /* Mask for archive version */
#define MPQ_CREATE_ARCHIVE_UNIX     MPQ_FLAG_FILENAME_UNIX

#define FLAGS_TO_FORMAT_SHIFT               24  /* (MPQ_CREATE_ARCHIVE_V4 >> FLAGS_TO_FORMAT_SHIFT) => MPQ_FORMAT_VERSION_4 */

/* Flags for SFileVerifyFile */
#define SFILE_VERIFY_SECTOR_CRC     0x00000001  /* Verify sector checksum for the file, if available */
#define SFILE_VERIFY_FILE_CRC       0x00000002  /* Verify file CRC, if available */
#define SFILE_VERIFY_FILE_MD5       0x00000004  /* Verify file MD5, if available */
#define SFILE_VERIFY_RAW_MD5        0x00000008  /* Verify raw file MD5, if available */
#define SFILE_VERIFY_ALL            0x0000000F  /* Verify every checksum possible */

/* Return values for SFileVerifyFile */
#define VERIFY_OPEN_ERROR               0x0001  /* Failed to open the file */
#define VERIFY_READ_ERROR               0x0002  /* Failed to read all data from the file */
#define VERIFY_FILE_HAS_SECTOR_CRC      0x0004  /* File has sector CRC */
#define VERIFY_FILE_SECTOR_CRC_ERROR    0x0008  /* Sector CRC check failed */
#define VERIFY_FILE_HAS_CHECKSUM        0x0010  /* File has CRC32 */
#define VERIFY_FILE_CHECKSUM_ERROR      0x0020  /* CRC32 check failed */
#define VERIFY_FILE_HAS_MD5             0x0040  /* File has data MD5 */
#define VERIFY_FILE_MD5_ERROR           0x0080  /* MD5 check failed */
#define VERIFY_FILE_HAS_RAW_MD5         0x0100  /* File has raw data MD5 */
#define VERIFY_FILE_RAW_MD5_ERROR       0x0200  /* Raw MD5 check failed */
#define VERIFY_FILE_ERROR_MASK      (VERIFY_OPEN_ERROR | VERIFY_READ_ERROR | VERIFY_FILE_SECTOR_CRC_ERROR | VERIFY_FILE_CHECKSUM_ERROR | VERIFY_FILE_MD5_ERROR | VERIFY_FILE_RAW_MD5_ERROR)

/* Flags for SFileVerifyRawData (for MPQs version 4.0 or higher) */
#define SFILE_VERIFY_MPQ_HEADER         0x0001  /* Verify raw MPQ header */
#define SFILE_VERIFY_HET_TABLE          0x0002  /* Verify raw data of the HET table */
#define SFILE_VERIFY_BET_TABLE          0x0003  /* Verify raw data of the BET table */
#define SFILE_VERIFY_HASH_TABLE         0x0004  /* Verify raw data of the hash table */
#define SFILE_VERIFY_BLOCK_TABLE        0x0005  /* Verify raw data of the block table */
#define SFILE_VERIFY_HIBLOCK_TABLE      0x0006  /* Verify raw data of the hi-block table */
#define SFILE_VERIFY_FILE               0x0007  /* Verify raw data of a file */

/* Signature types */
#define SIGNATURE_TYPE_NONE             0x0000  /* The archive has no signature in it */
#define SIGNATURE_TYPE_WEAK             0x0001  /* The archive has weak signature */
#define SIGNATURE_TYPE_STRONG           0x0002  /* The archive has strong signature */
#define SIGNATURE_TYPE_SECURE           0x0004  /* The archive has secure signature */

/* Return values for SFileVerifyArchive */
#define ERROR_NO_SIGNATURE                   0  /* There is no signature in the MPQ */
#define ERROR_VERIFY_FAILED                  1  /* There was an error during verifying signature (like no memory) */
#define ERROR_WEAK_SIGNATURE_OK              2  /* There is a weak signature and sign check passed */
#define ERROR_WEAK_SIGNATURE_ERROR           3  /* There is a weak signature but sign check failed */
#define ERROR_STRONG_SIGNATURE_OK            4  /* There is a strong signature and sign check passed */
#define ERROR_STRONG_SIGNATURE_ERROR         5  /* There is a strong signature but sign check failed */
#define ERROR_SECURE_SIGNATURE_OK            6  /* There is a secure signature and sign check passed */
#define ERROR_SECURE_SIGNATURE_ERROR         7  /* There is a secure signature but sign check failed */

/* Platform-specific error codes for UNIX-based platforms */
#define ERROR_SUCCESS                        0
#define ERROR_FILE_NOT_FOUND            ENOENT
#define ERROR_ACCESS_DENIED              EPERM
#define ERROR_INVALID_HANDLE             EBADF
#define ERROR_NOT_ENOUGH_MEMORY         ENOMEM
#define ERROR_NOT_SUPPORTED            EOPNOTSUPP
#define ERROR_INVALID_PARAMETER         EINVAL
#define ERROR_DISK_FULL                 ENOSPC
#define ERROR_ALREADY_EXISTS            EEXIST
#define ERROR_INSUFFICIENT_BUFFER      ENOBUFS
#define ERROR_BAD_FORMAT                  1000
#define ERROR_NO_MORE_FILES               1001
#define ERROR_HANDLE_EOF                  1002
#define ERROR_CAN_NOT_COMPLETE            1003
#define ERROR_FILE_CORRUPT                1004

#ifndef MD5_DIGEST_SIZE
#define MD5_DIGEST_SIZE                   0x10
#endif

#ifndef SHA1_DIGEST_SIZE
#define SHA1_DIGEST_SIZE                  0x14  /* 160 bits */
#endif

#ifndef LANG_NEUTRAL
#define LANG_NEUTRAL                      0x00  /* Neutral locale */
#endif

#define MPQ_LANG_C                LANG_NEUTRAL
#define MPQ_LANG_CHINESE                 0x404
#define MPQ_LANG_CZECH                   0x405
#define MPQ_LANG_GERMAN                  0x407
#define MPQ_LANG_ENGLISH                 0x409
#define MPQ_LANG_SPANISH                 0x40a
#define MPQ_LANG_FRENCH                  0x40c
#define MPQ_LANG_ITALIAN                 0x410
#define MPQ_LANG_JAPANESE                0x411
#define MPQ_LANG_KOREAN                  0x412
#define MPQ_LANG_POLISH                  0x415
#define MPQ_LANG_PORTUGUESE              0x416
#define MPQ_LANG_RUSSIAN                 0x419
#define MPQ_LANG_BRITISH                 0x809

/* Pointer to hashing function */
typedef uint32_t (*HASH_STRING)(const char * szFileName, uint32_t dwHashType);

/*-----------------------------------------------------------------------------
 * File information classes for SFileGetFileInfo and SFileFreeFileInfo
 */

typedef enum _SFileInfoClass
{
    /* Info classes for archives */
    SFileMpqFileName,                       /* Name of the archive file (char []) */
    SFileMpqStreamBitmap,                   /* Array of bits, each bit means availability of one block (uint8_t []) */
    SFileMpqUserDataOffset,                 /* Offset of the user data header (uint64_t) */
    SFileMpqUserDataHeader,                 /* Raw (unfixed) user data header (TMPQUserData) */
    SFileMpqUserData,                       /* MPQ USer data, without the header (uint8_t []) */
    SFileMpqHeaderOffset,                   /* Offset of the MPQ header (uint64_t) */
    SFileMpqHeaderSize,                     /* Fixed size of the MPQ header */
    SFileMpqHeader,                         /* Raw (unfixed) archive header (TMPQHeader) */
    SFileMpqHetTableOffset,                 /* Offset of the HET table, relative to MPQ header (uint64_t) */
    SFileMpqHetTableSize,                   /* Compressed size of the HET table (uint64_t) */
    SFileMpqHetHeader,                      /* HET table header (TMPQHetHeader) */
    SFileMpqHetTable,                       /* HET table as pointer. Must be freed using SFileFreeFileInfo */
    SFileMpqBetTableOffset,                 /* Offset of the BET table, relative to MPQ header (uint64_t) */
    SFileMpqBetTableSize,                   /* Compressed size of the BET table (uint64_t) */
    SFileMpqBetHeader,                      /* BET table header, followed by the flags (TMPQBetHeader + uint32_t[]) */
    SFileMpqBetTable,                       /* BET table as pointer. Must be freed using SFileFreeFileInfo */
    SFileMpqHashTableOffset,                /* Hash table offset, relative to MPQ header (uint64_t) */
    SFileMpqHashTableSize64,                /* Compressed size of the hash table (uint64_t) */
    SFileMpqHashTableSize,                  /* Size of the hash table, in entries (uint32_t) */
    SFileMpqHashTable,                      /* Raw (unfixed) hash table (TMPQBlock []) */
    SFileMpqBlockTableOffset,               /* Block table offset, relative to MPQ header (uint64_t) */
    SFileMpqBlockTableSize64,               /* Compressed size of the block table (uint64_t) */
    SFileMpqBlockTableSize,                 /* Size of the block table, in entries (uint32_t) */
    SFileMpqBlockTable,                     /* Raw (unfixed) block table (TMPQBlock []) */
    SFileMpqHiBlockTableOffset,             /* Hi-block table offset, relative to MPQ header (uint64_t) */
    SFileMpqHiBlockTableSize64,             /* Compressed size of the hi-block table (uint64_t) */
    SFileMpqHiBlockTable,                   /* The hi-block table (uint16_t []) */
    SFileMpqSignatures,                     /* Signatures present in the MPQ (uint32_t) */
    SFileMpqStrongSignatureOffset,          /* Byte offset of the strong signature, relative to begin of the file (uint64_t) */
    SFileMpqStrongSignatureSize,            /* Size of the strong signature (uint32_t) */
    SFileMpqStrongSignature,                /* The strong signature (uint8_t []) */
    SFileMpqArchiveSize64,                  /* Archive size from the header (uint64_t) */
    SFileMpqArchiveSize,                    /* Archive size from the header (uint32_t) */
    SFileMpqMaxFileCount,                   /* Max number of files in the archive (uint32_t) */
    SFileMpqFileTableSize,                  /* Number of entries in the file table (uint32_t) */
    SFileMpqSectorSize,                     /* Sector size (uint32_t) */
    SFileMpqNumberOfFiles,                  /* Number of files (uint32_t) */
    SFileMpqRawChunkSize,                   /* Size of the raw data chunk for MD5 */
    SFileMpqStreamFlags,                    /* Stream flags (uint32_t) */
    SFileMpqFlags,                          /* Nonzero if the MPQ is read only (uint32_t) */

    /* Info classes for files */
    SFileInfoPatchChain,                    /* Chain of patches where the file is (char []) */
    SFileInfoFileEntry,                     /* The file entry for the file (TFileEntry) */
    SFileInfoHashEntry,                     /* Hash table entry for the file (TMPQHash) */
    SFileInfoHashIndex,                     /* Index of the hash table entry (uint32_t) */
    SFileInfoNameHash1,                     /* The first name hash in the hash table (uint32_t) */
    SFileInfoNameHash2,                     /* The second name hash in the hash table (uint32_t) */
    SFileInfoNameHash3,                     /* 64-bit file name hash for the HET/BET tables (uint64_t) */
    SFileInfoLocale,                        /* File locale (uint32_t) */
    SFileInfoFileIndex,                     /* Block index (uint32_t) */
    SFileInfoByteOffset,                    /* File position in the archive (uint64_t) */
    SFileInfoFileTime,                      /* File time (uint64_t) */
    SFileInfoFileSize,                      /* Size of the file (uint32_t) */
    SFileInfoCompressedSize,                /* Compressed file size (uint32_t) */
    SFileInfoFlags,                         /* File flags from (uint32_t) */
    SFileInfoEncryptionKey,                 /* File encryption key */
    SFileInfoEncryptionKeyRaw,              /* Unfixed value of the file key */
} SFileInfoClass;

/*-----------------------------------------------------------------------------
 * Callback functions
 */

/* Values for compact callback */
#define CCB_CHECKING_FILES                  1   /* Checking archive (dwParam1 = current, dwParam2 = total) */
#define CCB_CHECKING_HASH_TABLE             2   /* Checking hash table (dwParam1 = current, dwParam2 = total) */
#define CCB_COPYING_NON_MPQ_DATA            3   /* Copying non-MPQ data: No params used */
#define CCB_COMPACTING_FILES                4   /* Compacting archive (dwParam1 = current, dwParam2 = total) */
#define CCB_CLOSING_ARCHIVE                 5   /* Closing archive: No params used */
                                      
typedef void (* SFILE_DOWNLOAD_CALLBACK)(void * pvUserData, uint64_t ByteOffset, uint32_t dwTotalBytes);
typedef void (* SFILE_ADDFILE_CALLBACK)(void * pvUserData, uint32_t dwBytesWritten, uint32_t dwTotalBytes, int bFinalCall);
typedef void (* SFILE_COMPACT_CALLBACK)(void * pvUserData, uint32_t dwWorkType, uint64_t BytesProcessed, uint64_t TotalBytes);

typedef struct TFileStream TFileStream;

/*-----------------------------------------------------------------------------
 * Structure for bit arrays used for HET and BET tables
 */

typedef struct _TBitArray
{
    uint32_t NumberOfBytes;                        /* Total number of bytes in "Elements" */
    uint32_t NumberOfBits;                         /* Total number of bits that are available */
    uint8_t Elements[1];                           /* Array of elements (variable length) */
} TBitArray;

void GetBits(TBitArray * array, unsigned int nBitPosition, unsigned int nBitLength, void * pvBuffer, int nResultSize);
void SetBits(TBitArray * array, unsigned int nBitPosition, unsigned int nBitLength, void * pvBuffer, int nResultSize);

/*-----------------------------------------------------------------------------
 * Structures related to MPQ format
 *
 * Note: All structures in this header file are supposed to remain private
 * to StormLib. The structures may (and will) change over time, as the MPQ
 * file format evolves. Programmers directly using these structures need to
 * be aware of this. And the last, but not least, NEVER do any modifications
 * to those structures directly, always use SFile* functions.
 */

#define MPQ_HEADER_SIZE_V1    0x20
#define MPQ_HEADER_SIZE_V2    0x2C
#define MPQ_HEADER_SIZE_V3    0x44
#define MPQ_HEADER_SIZE_V4    0xD0
#define MPQ_HEADER_WORDS      (MPQ_HEADER_SIZE_V4 / 0x04)

typedef struct _TMPQUserData
{
    /* The ID_MPQ_USERDATA ('MPQ\x1B') signature */
    uint32_t dwID;

    /* Maximum size of the user data */
    uint32_t cbUserDataSize;

    /* Offset of the MPQ header, relative to the begin of this header */
    uint32_t dwHeaderOffs;

    /* Appears to be size of user data header (Starcraft II maps) */
    uint32_t cbUserDataHeader;
} TMPQUserData;

/* MPQ file header
 *
 * We have to make sure that the header is packed OK.
 * Reason: A 64-bit integer at the beginning of 3.0 part,
 * which is offset 0x2C
 */
#pragma pack(push, 1)
typedef struct _TMPQHeader
{
    /* The ID_MPQ ('MPQ\x1A') signature */
    uint32_t dwID;

    /* Size of the archive header */
    uint32_t dwHeaderSize;

    /* 32-bit size of MPQ archive
     * This field is deprecated in the Burning Crusade MoPaQ format, and the size of the archive
     * is calculated as the size from the beginning of the archive to the end of the hash table,
     * block table, or hi-block table (whichever is largest).
     */
    uint32_t dwArchiveSize;

    /* 0 = Format 1 (up to The Burning Crusade)
     * 1 = Format 2 (The Burning Crusade and newer)
     * 2 = Format 3 (WoW - Cataclysm beta or newer)
     * 3 = Format 4 (WoW - Cataclysm beta or newer)
     */
    uint16_t wFormatVersion;

    /* Power of two exponent specifying the number of 512-byte disk sectors in each file sector */
    /* in the archive. The size of each file sector in the archive is 512 * 2 ^ wSectorSize. */
    uint16_t wSectorSize;

    /* Offset to the beginning of the hash table, relative to the beginning of the archive. */
    uint32_t dwHashTablePos;
    
    /* Offset to the beginning of the block table, relative to the beginning of the archive. */
    uint32_t dwBlockTablePos;
    
    /* Number of entries in the hash table. Must be a power of two, and must be less than 2^16 for */
    /* the original MoPaQ format, or less than 2^20 for the Burning Crusade format. */
    uint32_t dwHashTableSize;
    
    /* Number of entries in the block table */
    uint32_t dwBlockTableSize;

    /*-- MPQ HEADER v 2 -------------------------------------------*/

    /* Offset to the beginning of array of 16-bit high parts of file offsets. */
    uint64_t HiBlockTablePos64;

    /* High 16 bits of the hash table offset for large archives. */
    uint16_t wHashTablePosHi;

    /* High 16 bits of the block table offset for large archives. */
    uint16_t wBlockTablePosHi;

    /*-- MPQ HEADER v 3 ------------------------------------------- */

    /* 64-bit version of the archive size */
    uint64_t ArchiveSize64;

    /* 64-bit position of the BET table */
    uint64_t BetTablePos64;

    /* 64-bit position of the HET table */
    uint64_t HetTablePos64;

    /*-- MPQ HEADER v 4 -------------------------------------------*/

    /* Compressed size of the hash table */
    uint64_t HashTableSize64;

    /* Compressed size of the block table */
    uint64_t BlockTableSize64;

    /* Compressed size of the hi-block table */
    uint64_t HiBlockTableSize64;

    /* Compressed size of the HET block */
    uint64_t HetTableSize64;

    /* Compressed size of the BET block */
    uint64_t BetTableSize64;

    /* Size of raw data chunk to calculate MD5. */
    /* MD5 of each data chunk follows the raw file data. */
    uint32_t dwRawChunkSize;                                 

    /* MD5 of MPQ tables */
    unsigned char MD5_BlockTable[MD5_DIGEST_SIZE];      /* MD5 of the block table before decryption */
    unsigned char MD5_HashTable[MD5_DIGEST_SIZE];       /* MD5 of the hash table before decryption */
    unsigned char MD5_HiBlockTable[MD5_DIGEST_SIZE];    /* MD5 of the hi-block table */
    unsigned char MD5_BetTable[MD5_DIGEST_SIZE];        /* MD5 of the BET table before decryption */
    unsigned char MD5_HetTable[MD5_DIGEST_SIZE];        /* MD5 of the HET table before decryption */
    unsigned char MD5_MpqHeader[MD5_DIGEST_SIZE];       /* MD5 of the MPQ header from signature to (including) MD5_HetTable */
} TMPQHeader;

/* Hash table entry. All files in the archive are searched by their hashes. */
typedef struct _TMPQHash
{
    /* The hash of the file path, using method A. */
    uint32_t dwName1;
    
    /* The hash of the file path, using method B. */
    uint32_t dwName2;

#ifdef PLATFORM_LITTLE_ENDIAN

    /* The language of the file. This is a Windows LANGID data type, and uses the same values. */
    /* 0 indicates the default language (American English), or that the file is language-neutral. */
    uint16_t lcLocale;

    /* The platform the file is used for. 0 indicates the default platform. */
    /* No other values have been observed. */
    /* Note: wPlatform is actually just uint8_t, but since it has never been used, we don't care. */
    uint16_t wPlatform;

#else

    uint16_t wPlatform;
    uint16_t lcLocale;

#endif

    /* If the hash table entry is valid, this is the index into the block table of the file.
     * Otherwise, one of the following two values:
     *  - FFFFFFFFh: Hash table entry is empty, and has always been empty.
     *               Terminates searches for a given file.
     *  - FFFFFFFEh: Hash table entry is empty, but was valid at some point (a deleted file).
     *               Does not terminate searches for a given file.
     */
    uint32_t dwBlockIndex;
} TMPQHash;
#pragma pack(pop)

/* File description block contains informations about the file */
typedef struct _TMPQBlock
{
    /* Offset of the beginning of the file, relative to the beginning of the archive. */
    uint32_t dwFilePos;
    
    /* Compressed file size */
    uint32_t dwCSize;
    
    /* Only valid if the block is a file; otherwise meaningless, and should be 0. */
    /* If the file is compressed, this is the size of the uncompressed file data. */
    uint32_t dwFSize;                      
    
    /* Flags for the file. See MPQ_FILE_XXXX constants */
    uint32_t dwFlags;                      
} TMPQBlock;

/* Patch file information, preceding the sector offset table */
typedef struct _TPatchInfo
{
    uint32_t dwLength;                             /* Length of patch info header, in bytes */
    uint32_t dwFlags;                              /* Flags. 0x80000000 = MD5 (?) */
    uint32_t dwDataSize;                           /* Uncompressed size of the patch file */
    uint8_t  md5[0x10];                            /* MD5 of the entire patch file after decompression */

    /* Followed by the sector table (variable length) */
} TPatchInfo;

/* This is the combined file entry for maintaining file list in the MPQ. */
/* This structure is combined from block table, hi-block table, */
/* (attributes) file and from (listfile). */
typedef struct _TFileEntry
{
    uint64_t FileNameHash;                     /* Jenkins hash of the file name. Only used when the MPQ has BET table. */
    uint64_t ByteOffset;                       /* Position of the file content in the MPQ, relative to the MPQ header */
    uint64_t FileTime;                         /* FileTime from the (attributes) file. 0 if not present. */
    uint32_t dwFileSize;                       /* Decompressed size of the file */
    uint32_t dwCmpSize;                        /* Compressed size of the file (i.e., size of the file data in the MPQ) */
    uint32_t dwFlags;                          /* File flags (from block table) */
    uint32_t dwCrc32;                          /* CRC32 from (attributes) file. 0 if not present. */
    unsigned char md5[MD5_DIGEST_SIZE];         /* File MD5 from the (attributes) file. 0 if not present. */
    char * szFileName;                          /* File name. NULL if not known. */
} TFileEntry;

/* Common header for HET and BET tables */
typedef struct _TMPQExtHeader
{
    uint32_t dwSignature;                          /* 'HET\x1A' or 'BET\x1A' */
    uint32_t dwVersion;                            /* Version. Seems to be always 1 */
    uint32_t dwDataSize;                           /* Size of the contained table */

    /* Followed by the table header */
    /* Followed by the table data */

} TMPQExtHeader;

/* Structure for HET table header */
typedef struct _TMPQHetHeader
{
    TMPQExtHeader ExtHdr;

    uint32_t dwTableSize;                      /* Size of the entire HET table, including HET_TABLE_HEADER (in bytes) */
    uint32_t dwEntryCount;                     /* Number of occupied entries in the HET table */
    uint32_t dwTotalCount;                     /* Total number of entries in the HET table */
    uint32_t dwNameHashBitSize;                /* Size of the name hash entry (in bits) */
    uint32_t dwIndexSizeTotal;                 /* Total size of file index (in bits) */
    uint32_t dwIndexSizeExtra;                 /* Extra bits in the file index */
    uint32_t dwIndexSize;                      /* Effective size of the file index (in bits) */
    uint32_t dwIndexTableSize;                 /* Size of the block index subtable (in bytes) */

} TMPQHetHeader;

/* Structure for BET table header */
typedef struct _TMPQBetHeader
{
    TMPQExtHeader ExtHdr;

    uint32_t dwTableSize;                      /* Size of the entire BET table, including the header (in bytes) */
    uint32_t dwEntryCount;                     /* Number of entries in the BET table. Must match HET_TABLE_HEADER::dwEntryCount */
    uint32_t dwUnknown08;
    uint32_t dwTableEntrySize;                 /* Size of one table entry (in bits) */
    uint32_t dwBitIndex_FilePos;               /* Bit index of the file position (within the entry record) */
    uint32_t dwBitIndex_FileSize;              /* Bit index of the file size (within the entry record) */
    uint32_t dwBitIndex_CmpSize;               /* Bit index of the compressed size (within the entry record) */
    uint32_t dwBitIndex_FlagIndex;             /* Bit index of the flag index (within the entry record) */
    uint32_t dwBitIndex_Unknown;               /* Bit index of the ??? (within the entry record) */
    uint32_t dwBitCount_FilePos;               /* Bit size of file position (in the entry record) */
    uint32_t dwBitCount_FileSize;              /* Bit size of file size (in the entry record) */
    uint32_t dwBitCount_CmpSize;               /* Bit size of compressed file size (in the entry record) */
    uint32_t dwBitCount_FlagIndex;             /* Bit size of flags index (in the entry record) */
    uint32_t dwBitCount_Unknown;               /* Bit size of ??? (in the entry record) */
    uint32_t dwBitTotal_NameHash2;             /* Total bit size of the NameHash2 */
    uint32_t dwBitExtra_NameHash2;             /* Extra bits in the NameHash2 */
    uint32_t dwBitCount_NameHash2;             /* Effective size of NameHash2 (in bits) */
    uint32_t dwNameHashArraySize;              /* Size of NameHash2 table, in bytes */
    uint32_t dwFlagCount;                      /* Number of flags in the following array */

} TMPQBetHeader;

/* Structure for parsed HET table */
typedef struct _TMPQHetTable
{
    TBitArray * pBetIndexes;                    /* Bit array of FileIndex values */
    uint8_t *     pNameHashes;                     /* Array of NameHash1 values (NameHash1 = upper 8 bits of FileName hashe) */
    uint64_t  AndMask64;                       /* AND mask used for calculating file name hash */
    uint64_t  OrMask64;                        /* OR mask used for setting the highest bit of the file name hash */

    uint32_t      dwEntryCount;                    /* Number of occupied entries in the HET table */
    uint32_t      dwTotalCount;                    /* Number of entries in both NameHash and FileIndex table */
    uint32_t      dwNameHashBitSize;               /* Size of the name hash entry (in bits) */
    uint32_t      dwIndexSizeTotal;                /* Total size of one entry in pBetIndexes (in bits) */
    uint32_t      dwIndexSizeExtra;                /* Extra bits in the entry in pBetIndexes */
    uint32_t      dwIndexSize;                     /* Effective size of one entry in pBetIndexes (in bits) */
} TMPQHetTable;

/* Structure for parsed BET table */
typedef struct _TMPQBetTable
{
    TBitArray * pNameHashes;                    /* Array of NameHash2 entries (lower 24 bits of FileName hash) */
    TBitArray * pFileTable;                     /* Bit-based file table */
    uint32_t * pFileFlags;                         /* Array of file flags */

    uint32_t dwTableEntrySize;                     /* Size of one table entry, in bits */
    uint32_t dwBitIndex_FilePos;                   /* Bit index of the file position in the table entry */
    uint32_t dwBitIndex_FileSize;                  /* Bit index of the file size in the table entry */
    uint32_t dwBitIndex_CmpSize;                   /* Bit index of the compressed size in the table entry */
    uint32_t dwBitIndex_FlagIndex;                 /* Bit index of the flag index in the table entry */
    uint32_t dwBitIndex_Unknown;                   /* Bit index of ??? in the table entry */
    uint32_t dwBitCount_FilePos;                   /* Size of file offset (in bits) within table entry */
    uint32_t dwBitCount_FileSize;                  /* Size of file size (in bits) within table entry */
    uint32_t dwBitCount_CmpSize;                   /* Size of compressed file size (in bits) within table entry */
    uint32_t dwBitCount_FlagIndex;                 /* Size of flag index (in bits) within table entry */
    uint32_t dwBitCount_Unknown;                   /* Size of ??? (in bits) within table entry */
    uint32_t dwBitTotal_NameHash2;                 /* Total size of the NameHash2 */
    uint32_t dwBitExtra_NameHash2;                 /* Extra bits in the NameHash2 */
    uint32_t dwBitCount_NameHash2;                 /* Effective size of the NameHash2 */
    uint32_t dwEntryCount;                         /* Number of entries */
    uint32_t dwFlagCount;                          /* Number of file flags in pFileFlags */
} TMPQBetTable;

/* Structure for patch prefix */
typedef struct _TMPQNamePrefix
{
    size_t nLength;                             /* Length of this patch prefix. Can be 0 */
    char szPatchPrefix[1];                      /* Patch name prefix (variable length). If not empty, it always starts with backslash. */
} TMPQNamePrefix;

/* Structure for name cache */
typedef struct _TMPQNameCache
{
    uint32_t FirstNameOffset;                   /* Offset of the first name in the name list (in bytes) */
    uint32_t FreeSpaceOffset;                   /* Offset of the first free byte in the name cache (in bytes) */
    uint32_t TotalCacheSize;                    /* Size, in bytes, of the cache. Includes wildcard */
    uint32_t SearchOffset;                      /* Used by SListFileFindFirstFile */

    /* Followed by search mask (ASCIIZ, '\0' if none) */
    /* Followed by name cache (ANSI multistring) */

} TMPQNameCache;

/* Structure for SFileFindFirstFile and SFileFindNextFile */
typedef struct _SFILE_FIND_DATA
{
    char      cFileName[1024];                 /* Full name of the found file */
    char      * szPlainName;                       /* Plain name of the found file */
    uint32_t  dwBlockIndex;                        /* Block table index for the file */
    uint32_t  dwFileSize;                          /* File size in bytes */
    uint32_t  dwFileFlags;                         /* MPQ file flags */
    uint32_t  dwCompSize;                          /* Compressed file size */
    uint32_t  dwFileTimeLo;                        /* Low 32-bits of the file time (0 if not present) */
    uint32_t  dwFileTimeHi;                        /* High 32-bits of the file time (0 if not present) */
    uint32_t  lcLocale;                            /* Locale version */

} SFILE_FIND_DATA, *PSFILE_FIND_DATA;

typedef struct _SFILE_CREATE_MPQ
{
    uint32_t cbSize;                               /* Size of this structure, in bytes */
    uint32_t dwMpqVersion;                         /* Version of the MPQ to be created */
    void *pvUserData;                           /* Reserved, must be NULL */
    uint32_t cbUserData;                           /* Reserved, must be 0 */
    uint32_t dwStreamFlags;                        /* Stream flags for creating the MPQ */
    uint32_t dwFileFlags1;                         /* File flags for (listfile). 0 = default */
    uint32_t dwFileFlags2;                         /* File flags for (attributes). 0 = default */
    uint32_t dwFileFlags3;                         /* File flags for (signature). 0 = default */
    uint32_t dwAttrFlags;                          /* Flags for the (attributes) file. If 0, no attributes will be created */
    uint32_t dwSectorSize;                         /* Sector size for compressed files */
    uint32_t dwRawChunkSize;                       /* Size of raw data chunk */
    uint32_t dwMaxFileCount;                       /* File limit for the MPQ */

} SFILE_CREATE_MPQ, *PSFILE_CREATE_MPQ;

/*-----------------------------------------------------------------------------
 * Stream support - functions
 */

/* Structure used by FileStream_GetBitmap */
typedef struct _TStreamBitmap
{
    uint64_t StreamSize;                       /* Size of the stream, in bytes */
    uint32_t BitmapSize;                           /* Size of the block map, in bytes */
    uint32_t BlockCount;                           /* Number of blocks in the stream */
    uint32_t BlockSize;                            /* Size of one block */
    uint32_t IsComplete;                           /* Nonzero if the file is complete */

    /* Followed by the uint8_t array, each bit means availability of one block */

} TStreamBitmap;

/* UNICODE versions of the file access functions */
TFileStream * FileStream_CreateFile(const char * szFileName, uint32_t dwStreamFlags);
TFileStream * FileStream_OpenFile(const char * szFileName, uint32_t dwStreamFlags);
const char * FileStream_GetFileName(TFileStream * pStream);
size_t FileStream_Prefix(const char * szFileName, uint32_t * pdwProvider);

int FileStream_SetCallback(TFileStream * pStream, SFILE_DOWNLOAD_CALLBACK pfnCallback, void * pvUserData);

int FileStream_GetBitmap(TFileStream * pStream, void * pvBitmap, uint32_t cbBitmap, uint32_t * pcbLengthNeeded);
int FileStream_Read(TFileStream * pStream, uint64_t * pByteOffset, void * pvBuffer, uint32_t dwBytesToRead);
int FileStream_Write(TFileStream * pStream, uint64_t * pByteOffset, const void * pvBuffer, uint32_t dwBytesToWrite);
int FileStream_SetSize(TFileStream * pStream, uint64_t NewFileSize);
int FileStream_GetSize(TFileStream * pStream, uint64_t * pFileSize);
int FileStream_GetPos(TFileStream * pStream, uint64_t * pByteOffset);
int FileStream_GetTime(TFileStream * pStream, uint64_t * pFT);
int FileStream_GetFlags(TFileStream * pStream, uint32_t * pdwStreamFlags);
int FileStream_Replace(TFileStream * pStream, TFileStream * pNewStream);
void FileStream_Close(TFileStream * pStream);

/*-----------------------------------------------------------------------------
 * Functions prototypes for Storm.dll
 */

/* Typedefs for functions exported by Storm.dll */
typedef uint32_t  (* SFILESETLOCALE)(uint32_t);
typedef int  (* SFILEOPENARCHIVE)(const char *, uint32_t, uint32_t, void * *);
typedef int  (* SFILECLOSEARCHIVE)(void *);
typedef int  (* SFILEOPENFILEEX)(void *, const char *, uint32_t, void * *);
typedef int  (* SFILECLOSEFILE)(void *);
typedef uint32_t (* SFILEGETFILESIZE)(void *, uint32_t *);
typedef uint32_t (* SFILESETFILEPOINTER)(void *, int, int *, uint32_t);
typedef int  (* SFILEREADFILE)(void *, void *, uint32_t, uint32_t *, void *);

/*-----------------------------------------------------------------------------
 * Functions for manipulation with StormLib global flags
 */

uint32_t   SFileGetLocale();
uint32_t   SFileSetLocale(uint32_t lcNewLocale);

/*-----------------------------------------------------------------------------
 * Functions for archive manipulation
 */

int   SFileOpenArchive(const char * szMpqName, uint32_t dwPriority, uint32_t dwFlags, void * * phMpq);
int   SFileCreateArchive(const char * szMpqName, uint32_t dwCreateFlags, uint32_t dwMaxFileCount, void * * phMpq);
int   SFileCreateArchive2(const char * szMpqName, PSFILE_CREATE_MPQ pCreateInfo, void * * phMpq);
int   SFileSetAnubisKey(void * hMpq, const unsigned char * key, int keySize);
int   SFileSetSerpentKey(void * hMpq, const unsigned char * key, int keySize);

int   SFileSetDownloadCallback(void * hMpq, SFILE_DOWNLOAD_CALLBACK DownloadCB, void * pvUserData);
int   SFileFlushArchive(void * hMpq);
int   SFileCloseArchive(void * hMpq);

/* Adds another listfile into MPQ. The currently added listfile(s) remain,
 * so you can use this API to combining more listfiles.
 * Note that this function is internally called by SFileFindFirstFile
 */
int    SFileAddListFile(void * hMpq, const char * szListFile);

/* Archive compacting */
int   SFileSetCompactCallback(void * hMpq, SFILE_COMPACT_CALLBACK CompactCB, void * pvUserData);
int   SFileCompactArchive(void * hMpq, const char * szListFile, int bReserved);

/* Changing the maximum file count */
uint32_t  SFileGetMaxFileCount(void * hMpq);
int   SFileSetMaxFileCount(void * hMpq, uint32_t dwMaxFileCount);

/* Changing (attributes) file */
uint32_t  SFileGetAttributes(void * hMpq);
int   SFileSetAttributes(void * hMpq, uint32_t dwFlags);
int   SFileUpdateFileAttributes(void * hMpq, const char * szFileName);

/*-----------------------------------------------------------------------------
 * Functions for manipulation with patch archives
 */

int   SFileOpenPatchArchive(void * hMpq, const char * szPatchMpqName, const char * szPatchPathPrefix, uint32_t dwFlags);
int   SFileIsPatchedArchive(void * hMpq);

/*-----------------------------------------------------------------------------
 * Functions for file manipulation
 */

/* Reading from MPQ file */
int   SFileHasFile(void * hMpq, const char * szFileName);
int   SFileOpenFileEx(void * hMpq, const char * szFileName, uint32_t dwSearchScope, void * * phFile);
size_t  SFileGetFileSize(void * hFile, uint32_t * pdwFileSizeHigh);
size_t  SFileSetFilePointer(void * hFile, off_t lFilePos, int * plFilePosHigh, uint32_t dwMoveMethod);
int   SFileReadFile(void * hFile, void * lpBuffer, size_t dwToRead, size_t * pdwRead);
int   SFileCloseFile(void * hFile);

/* Retrieving info about a file in the archive */
int   SFileGetFileInfo(void * hMpqOrFile, SFileInfoClass InfoClass, void * pvFileInfo, uint32_t cbFileInfo, uint32_t * pcbLengthNeeded);
int   SFileGetFileName(void * hFile, char * szFileName);
int   SFileFreeFileInfo(void * pvFileInfo, SFileInfoClass InfoClass);

/* High-level extract function */
int   SFileExtractFile(void * hMpq, const char * szToExtract, const char * szExtracted, uint32_t dwSearchScope);

/*-----------------------------------------------------------------------------
 * Functions for file and archive verification
 */

/* Generates file CRC32 */
int   SFileGetFileChecksums(void * hMpq, const char * szFileName, uint32_t * pdwCrc32, char * pMD5);

/* Verifies file against its checksums stored in (attributes) attributes (depending on dwFlags). */
/* For dwFlags, use one or more of MPQ_ATTRIBUTE_MD5 */
uint32_t  SFileVerifyFile(void * hMpq, const char * szFileName, uint32_t dwFlags);

/* Verifies raw data of the archive. Only works for MPQs version 4 or newer */
int    SFileVerifyRawData(void * hMpq, uint32_t dwWhatToVerify, const char * szFileName);

/* Verifies the signature, if present */
int   SFileSetRSAKey(void * hMpq, unsigned char * key, size_t keyLength);
int   SFileSignArchive(void * hMpq, uint32_t dwSignatureType);
uint32_t  SFileVerifyArchive(void * hMpq);

/*-----------------------------------------------------------------------------
 * Functions for file searching
 */

void * SFileFindFirstFile(void * hMpq, const char * szMask, SFILE_FIND_DATA * lpFindFileData, const char * szListFile);
int   SFileFindNextFile(void * hFind, SFILE_FIND_DATA * lpFindFileData);
int   SFileFindClose(void * hFind);

void * SListFileFindFirstFile(void * hMpq, const char * szListFile, const char * szMask, SFILE_FIND_DATA * lpFindFileData);
int   SListFileFindNextFile(void * hFind, SFILE_FIND_DATA * lpFindFileData);
int   SListFileFindClose(void * hFind);

/* Locale support */
int    SFileEnumLocales(void * hMpq, const char * szFileName, uint32_t * plcLocales, uint32_t * pdwMaxLocales, uint32_t dwSearchScope);

/*-----------------------------------------------------------------------------
 * Support for adding files to the MPQ
 */

int   SFileCreateFile(void * hMpq, const char * szArchivedName, uint64_t FileTime, size_t dwFileSize, uint32_t lcLocale, uint32_t dwFlags, void ** phFile);
int   SFileWriteFile(void * hFile, const void * pvData, size_t dwSize, uint32_t dwCompression);
int   SFileFinishFile(void * hFile);

int   SFileAddFileEx(void * hMpq, const char * szFileName, const char * szArchivedName, uint32_t dwFlags, uint32_t dwCompression, uint32_t dwCompressionNext);
int   SFileAddFile(void * hMpq, const char * szFileName, const char * szArchivedName, uint32_t dwFlags); 
int   SFileAddWave(void * hMpq, const char * szFileName, const char * szArchivedName, uint32_t dwFlags, uint32_t dwQuality); 
int   SFileRemoveFile(void * hMpq, const char * szFileName, uint32_t dwSearchScope);
int   SFileRenameFile(void * hMpq, const char * szOldFileName, const char * szNewFileName);
int   SFileSetFileLocale(void * hFile, uint32_t lcNewLocale);
int   SFileSetDataCompression(uint32_t DataCompression);

int   SFileSetAddFileCallback(void * hMpq, SFILE_ADDFILE_CALLBACK AddFileCB, void * pvUserData);


/*-----------------------------------------------------------------------------
 * Compression and decompression
 */

int    SCompImplode    (void * pvOutBuffer, int * pcbOutBuffer, void * pvInBuffer, int cbInBuffer);
int    SCompExplode    (void * pvOutBuffer, int * pcbOutBuffer, void * pvInBuffer, int cbInBuffer);
int    SCompCompress   (void * pvOutBuffer, int * pcbOutBuffer, void * pvInBuffer, int cbInBuffer, unsigned uCompressionMask, int nCmpType, int nCmpLevel);
int    SCompDecompress (void * pvOutBuffer, int * pcbOutBuffer, void * pvInBuffer, int cbInBuffer);
int    SCompDecompress2(void * pvOutBuffer, int * pcbOutBuffer, void * pvInBuffer, int cbInBuffer);

/*-----------------------------------------------------------------------------
 * Non-Windows support for SetLastError/GetLastError
 */

void  SetLastError(int err);
int   GetLastError();

#endif  /* _THUNDERSTORM_H */
