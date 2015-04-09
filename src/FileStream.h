/*****************************************************************************/
/* FileStream.h                           Copyright (c) Ladislav Zezula 2012 */
/*---------------------------------------------------------------------------*/
/* Description: Definitions for FileStream object                            */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 14.04.12  1.00  Lad  The first version of FileStream.h                    */
/*****************************************************************************/

#ifndef __FILESTREAM_H__
#define __FILESTREAM_H__

#include "thunderStorm.h"

/*-----------------------------------------------------------------------------
 * Function prototypes
 */

typedef void (*STREAM_INIT)(
    struct TFileStream * pStream        /* Pointer to an unopened stream */
);

typedef int (*STREAM_CREATE)(
    struct TFileStream * pStream        /* Pointer to an unopened stream */
    );

typedef int (*STREAM_OPEN)(
    struct TFileStream * pStream,       /* Pointer to an unopened stream */
    const char * szFileName,           /* Pointer to file name to be open */
    uint32_t dwStreamFlags                 /* Stream flags */
    );

typedef int (*STREAM_READ)(
    struct TFileStream * pStream,       /* Pointer to an open stream */
    uint64_t * pByteOffset,            /* Pointer to file byte offset. If NULL, it reads from the current position */
    void * pvBuffer,                    /* Pointer to data to be read */
    uint32_t dwBytesToRead                 /* Number of bytes to read from the file */
    );

typedef int (*STREAM_WRITE)(
    struct TFileStream * pStream,       /* Pointer to an open stream */
    uint64_t * pByteOffset,            /* Pointer to file byte offset. If NULL, it writes to the current position */
    const void * pvBuffer,              /* Pointer to data to be written */
    uint32_t dwBytesToWrite                /* Number of bytes to read from the file */
    );

typedef int (*STREAM_RESIZE)(
    struct TFileStream * pStream,       /* Pointer to an open stream */
    uint64_t FileSize                  /* New size for the file, in bytes */
    );

typedef int (*STREAM_GETSIZE)(
    struct TFileStream * pStream,       /* Pointer to an open stream */
    uint64_t * pFileSize               /* Receives the file size, in bytes */
    );

typedef int (*STREAM_GETPOS)(
    struct TFileStream * pStream,       /* Pointer to an open stream */
    uint64_t * pByteOffset             /* Pointer to store current file position */
    );

typedef void (*STREAM_CLOSE)(
    struct TFileStream * pStream        /* Pointer to an open stream */
    );

typedef int (*BLOCK_READ)(
    struct TFileStream * pStream,       /* Pointer to a block-oriented stream */
    uint64_t StartOffset,              /* Byte offset of start of the block array */
    uint64_t EndOffset,                /* End offset (either end of the block or end of the file) */
    unsigned char * BlockBuffer,                 /* Pointer to block-aligned buffer */
    uint32_t BytesNeeded,                  /* Number of bytes that are really needed */
    int bAvailable                     /* true if the block is available */
    );

typedef int (*BLOCK_CHECK)(
    struct TFileStream * pStream,       /* Pointer to a block-oriented stream */
    uint64_t BlockOffset               /* Offset of the file to check */
    );

typedef void (*BLOCK_SAVEMAP)(
    struct TFileStream * pStream        /* Pointer to a block-oriented stream */
    );

/*-----------------------------------------------------------------------------
 * Local structures - partial file structure and bitmap footer
 */

#define ID_FILE_BITMAP_FOOTER   0x33767470  /* Signature of the file bitmap footer ('ptv3') */
#define DEFAULT_BLOCK_SIZE      0x00004000  /* Default size of the stream block */
#define DEFAULT_BUILD_NUMBER         10958  /* Build number for newly created partial MPQs */

typedef struct _PART_FILE_HEADER
{
    uint32_t PartialVersion;                   /* Always set to 2 */
    char  GameBuildNumber[0x20];            /* Minimum build number of the game that can use this MPQ */
    uint32_t Flags;                            /* Flags (details unknown) */
    uint32_t FileSizeLo;                       /* Low 32 bits of the contained file size */
    uint32_t FileSizeHi;                       /* High 32 bits of the contained file size */
    uint32_t BlockSize;                        /* Size of one file block, in bytes */

} PART_FILE_HEADER, *PPART_FILE_HEADER;

/* Structure describing the block-to-file map entry */
typedef struct _PART_FILE_MAP_ENTRY
{
    uint32_t Flags;                            /* 3 = the block is present in the file */
    uint32_t BlockOffsLo;                      /* Low 32 bits of the block position in the file */
    uint32_t BlockOffsHi;                      /* High 32 bits of the block position in the file */
    uint32_t LargeValueLo;                     /* 64-bit value, meaning is unknown */
    uint32_t LargeValueHi;

} PART_FILE_MAP_ENTRY, *PPART_FILE_MAP_ENTRY;

typedef struct _FILE_BITMAP_FOOTER
{
    uint32_t Signature;                      /* 'ptv3' (ID_FILE_BITMAP_FOOTER) */
    uint32_t Version;                        /* Unknown, seems to always have value of 3 (version?) */
    uint32_t BuildNumber;                    /* Game build number for that MPQ */
    uint32_t MapOffsetLo;                    /* Low 32-bits of the offset of the bit map */
    uint32_t MapOffsetHi;                    /* High 32-bits of the offset of the bit map */
    uint32_t BlockSize;                      /* Size of one block (usually 0x4000 bytes) */

} FILE_BITMAP_FOOTER, *PFILE_BITMAP_FOOTER;

/*-----------------------------------------------------------------------------
 * Structure for file stream
 */

union TBaseProviderData
{
    struct
    {
        uint64_t FileSize;                 /* Size of the file */
        uint64_t FilePos;                  /* Current file position */
        uint64_t FileTime;                 /* Last write time */
        int hFile;                          /* File handle */
    } File;

    struct
    {
        uint64_t FileSize;                 /* Size of the file */
        uint64_t FilePos;                  /* Current file position */
        uint64_t FileTime;                 /* Last write time */
        unsigned char * pbFile;            /* Pointer to mapped view */
    } Map;

    /*
    struct
    {
        uint64_t FileSize;                  * Size of the file
        uint64_t FilePos;                   * Current file position
        uint64_t FileTime;                  * Last write time
        HANDLE hInternet;                   * Internet handle
        HANDLE hConnect;                    * Connection to the internet server
    } Http;
    */
};

typedef struct TFileStream
{
    /* Stream provider functions */
    STREAM_READ    StreamRead;              /* Pointer to stream read function for this archive. Do not use directly. */
    STREAM_WRITE   StreamWrite;             /* Pointer to stream write function for this archive. Do not use directly. */
    STREAM_RESIZE  StreamResize;            /* Pointer to function changing file size */
    STREAM_GETSIZE StreamGetSize;           /* Pointer to function returning file size */
    STREAM_GETPOS  StreamGetPos;            /* Pointer to function that returns current file position */
    STREAM_CLOSE   StreamClose;             /* Pointer to function closing the stream */

    /* Block-oriented functions */
    BLOCK_READ     BlockRead;               /* Pointer to function reading one or more blocks */
    BLOCK_CHECK    BlockCheck;              /* Pointer to function checking whether the block is present */

    /* Base provider functions */
    STREAM_CREATE  BaseCreate;              /* Pointer to base create function */
    STREAM_OPEN    BaseOpen;                /* Pointer to base open function */
    STREAM_READ    BaseRead;                /* Read from the stream */
    STREAM_WRITE   BaseWrite;               /* Write to the stream */
    STREAM_RESIZE  BaseResize;              /* Pointer to function changing file size */
    STREAM_GETSIZE BaseGetSize;             /* Pointer to function returning file size */
    STREAM_GETPOS  BaseGetPos;              /* Pointer to function that returns current file position */
    STREAM_CLOSE   BaseClose;               /* Pointer to function closing the stream */

    /* Base provider data (file size, file position) */
    union TBaseProviderData Base;

    /* Stream provider data */
    struct TFileStream * pMaster;                  /* Master stream (e.g. MPQ on a web server) */
    char * szFileName;                     /* File name (self-relative pointer) */

    uint64_t StreamSize;                   /* Stream size (can be less than file size) */
    uint64_t StreamPos;                    /* Stream position */
    uint32_t BuildNumber;                      /* Game build number */
    uint32_t dwFlags;                          /* Stream flags */

    /* Followed by stream provider data, with variable length */
} TFileStream_t;

/*-----------------------------------------------------------------------------
 * Structures for block-oriented stream
 */

typedef struct TBlockStream
{
    /* Stream provider functions */
    STREAM_READ    StreamRead;              /* Pointer to stream read function for this archive. Do not use directly. */
    STREAM_WRITE   StreamWrite;             /* Pointer to stream write function for this archive. Do not use directly. */
    STREAM_RESIZE  StreamResize;            /* Pointer to function changing file size */
    STREAM_GETSIZE StreamGetSize;           /* Pointer to function returning file size */
    STREAM_GETPOS  StreamGetPos;            /* Pointer to function that returns current file position */
    STREAM_CLOSE   StreamClose;             /* Pointer to function closing the stream */

    /* Block-oriented functions */
    BLOCK_READ     BlockRead;               /* Pointer to function reading one or more blocks */
    BLOCK_CHECK    BlockCheck;              /* Pointer to function checking whether the block is present */

    /* Base provider functions */
    STREAM_CREATE  BaseCreate;              /* Pointer to base create function */
    STREAM_OPEN    BaseOpen;                /* Pointer to base open function */
    STREAM_READ    BaseRead;                /* Read from the stream */
    STREAM_WRITE   BaseWrite;               /* Write to the stream */
    STREAM_RESIZE  BaseResize;              /* Pointer to function changing file size */
    STREAM_GETSIZE BaseGetSize;             /* Pointer to function returning file size */
    STREAM_GETPOS  BaseGetPos;              /* Pointer to function that returns current file position */
    STREAM_CLOSE   BaseClose;               /* Pointer to function closing the stream */

    /* Base provider data (file size, file position) */
    union TBaseProviderData Base;

    /* Stream provider data */
    struct TFileStream * pMaster;                  /* Master stream (e.g. MPQ on a web server) */
    char * szFileName;                     /* File name (self-relative pointer) */

    uint64_t StreamSize;                   /* Stream size (can be less than file size) */
    uint64_t StreamPos;                    /* Stream position */
    uint32_t BuildNumber;                      /* Game build number */
    uint32_t dwFlags;                          /* Stream flags */

    /* Followed by stream provider data, with variable length */
    
    SFILE_DOWNLOAD_CALLBACK pfnCallback;    /* Callback for downloading */
    void * FileBitmap;                      /* Array of bits for file blocks */
    void * UserData;                        /* User data to be passed to the download callback */
    uint32_t BitmapSize;                       /* Size of the file bitmap (in bytes) */
    uint32_t BlockSize;                        /* Size of one block, in bytes */
    uint32_t BlockCount;                       /* Number of data blocks in the file */
    uint32_t IsComplete;                       /* If nonzero, no blocks are missing */
    uint32_t IsModified;                       /* nonzero if the bitmap has been modified */
} TBlockStream_t;        

/*-----------------------------------------------------------------------------
 * Structure for encrypted stream
 */

#define MPQE_CHUNK_SIZE 0x40                /* Size of one chunk to be decrypted */

typedef struct TEncryptedStream
{
    /* Stream provider functions */
    STREAM_READ    StreamRead;              /* Pointer to stream read function for this archive. Do not use directly. */
    STREAM_WRITE   StreamWrite;             /* Pointer to stream write function for this archive. Do not use directly. */
    STREAM_RESIZE  StreamResize;            /* Pointer to function changing file size */
    STREAM_GETSIZE StreamGetSize;           /* Pointer to function returning file size */
    STREAM_GETPOS  StreamGetPos;            /* Pointer to function that returns current file position */
    STREAM_CLOSE   StreamClose;             /* Pointer to function closing the stream */

    /* Block-oriented functions */
    BLOCK_READ     BlockRead;               /* Pointer to function reading one or more blocks */
    BLOCK_CHECK    BlockCheck;              /* Pointer to function checking whether the block is present */

    /* Base provider functions */
    STREAM_CREATE  BaseCreate;              /* Pointer to base create function */
    STREAM_OPEN    BaseOpen;                /* Pointer to base open function */
    STREAM_READ    BaseRead;                /* Read from the stream */
    STREAM_WRITE   BaseWrite;               /* Write to the stream */
    STREAM_RESIZE  BaseResize;              /* Pointer to function changing file size */
    STREAM_GETSIZE BaseGetSize;             /* Pointer to function returning file size */
    STREAM_GETPOS  BaseGetPos;              /* Pointer to function that returns current file position */
    STREAM_CLOSE   BaseClose;               /* Pointer to function closing the stream */

    /* Base provider data (file size, file position) */
    union TBaseProviderData Base;

    /* Stream provider data */
    struct TFileStream * pMaster;                  /* Master stream (e.g. MPQ on a web server) */
    char * szFileName;                     /* File name (self-relative pointer) */

    uint64_t StreamSize;                   /* Stream size (can be less than file size) */
    uint64_t StreamPos;                    /* Stream position */
    uint32_t BuildNumber;                      /* Game build number */
    uint32_t dwFlags;                          /* Stream flags */

    /* Followed by stream provider data, with variable length */
    
    SFILE_DOWNLOAD_CALLBACK pfnCallback;    /* Callback for downloading */
    void * FileBitmap;                      /* Array of bits for file blocks */
    void * UserData;                        /* User data to be passed to the download callback */
    uint32_t BitmapSize;                       /* Size of the file bitmap (in bytes) */
    uint32_t BlockSize;                        /* Size of one block, in bytes */
    uint32_t BlockCount;                       /* Number of data blocks in the file */
    uint32_t IsComplete;                       /* If nonzero, no blocks are missing */
    uint32_t IsModified;                       /* nonzero if the bitmap has been modified */
    uint8_t Key[MPQE_CHUNK_SIZE];              /* File key */
} TEncryptedStream_t;

#endif /* __FILESTREAM_H__ */
