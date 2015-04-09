/*****************************************************************************/
/* FileStream.c                           Copyright (c) Ladislav Zezula 2010 */
/*---------------------------------------------------------------------------*/
/* File stream support for libThunderStorm                                   */
/*                                                                           */
/* Windows support: Written by Ladislav Zezula                               */
/* Mac support:     Written by Sam Wilkins                                   */
/* Linux support:   Written by Sam Wilkins and Ivan Komissarov               */
/* Big-endian:      Written & debugged by Sam Wilkins                        */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 11.06.10  1.00  Lad  Derived from StormPortMac.cpp and StormPortLinux.cpp */
/* 09.03.15  1.00  Ayr  Ported to plain c                                    */
/*****************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include "StormCommon.h"
#include "FileStream.h"

/*-----------------------------------------------------------------------------
 * Local defines
 */

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif

/*-----------------------------------------------------------------------------
 * Local functions - platform-specific functions
 */

static int nLastError = ERROR_SUCCESS;

int GetLastError()
{
    return nLastError;
}

void SetLastError(int nError)
{
    nLastError = nError;
	errno = nError;
}

static uint32_t StringToInt(const char * szString)
{
    uint32_t dwValue = 0;

    while('0' <= szString[0] && szString[0] <= '9')
    {
        dwValue = (dwValue * 10) + (szString[0] - '9');
        szString++;
    }

    return dwValue;
}

/*-----------------------------------------------------------------------------
 * Dummy init function
 */

static void BaseNone_Init(TFileStream_t * stream)
{
    /* Nothing here */
}

/*-----------------------------------------------------------------------------
 * Local functions - base file support
 */

static int BaseFile_Create(TFileStream_t * pStream)
{
    int handle;
        
    handle = open(pStream->szFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(handle == -1)
    {
        nLastError = errno;
        return 0;
    }

    pStream->Base.File.hFile = handle;

    /* Reset the file size and position */
    pStream->Base.File.FileSize = 0;
    pStream->Base.File.FilePos = 0;
    return 1;
}

static int BaseFile_Open(TFileStream_t * pStream, const char * szFileName, uint32_t dwStreamFlags)
{
    struct stat fileinfo;
    int oflag = (dwStreamFlags & STREAM_FLAG_READ_ONLY) ? O_RDONLY : O_RDWR;
    int handle;

    /* Open the file */
    handle = open(szFileName, oflag);
    if(handle == -1)
    {
        nLastError = errno;
        return 0;
    }

    /* Get the file size */
    if(fstat(handle, &fileinfo) == -1)
    {
        nLastError = errno;
        return 0;
    }

    /* time_t is number of seconds since 1.1.1970, UTC. */
    /* 1 second = 10000000 (decimal) in FILETIME */
    /* Set the start to 1.1.1970 00:00:00 */
    pStream->Base.File.FileTime = 0x019DB1DED53E8000ULL + (10000000 * fileinfo.st_mtime);
    pStream->Base.File.FileSize = (uint64_t)fileinfo.st_size;
    pStream->Base.File.hFile = handle;


    /* Reset the file position */
    pStream->Base.File.FilePos = 0;
    return 1;
}

static int BaseFile_Read(
    TFileStream_t * pStream,                  /* Pointer to an open stream */
    uint64_t * pByteOffset,                /* Pointer to file byte offset. If NULL, it reads from the current position */
    void * pvBuffer,                        /* Pointer to data to be read */
    uint32_t dwBytesToRead)                    /* Number of bytes to read from the file */
{
    uint64_t ByteOffset = (pByteOffset != NULL) ? *pByteOffset : pStream->Base.File.FilePos;
    uint32_t dwBytesRead = 0;                  /* Must be set by platform-specific code */

    ssize_t bytes_read;

    /* If the byte offset is different from the current file position, */
    /* we have to update the file position   xxx */
    if(ByteOffset != pStream->Base.File.FilePos)
    {
        lseek(pStream->Base.File.hFile, ByteOffset, SEEK_SET);
        pStream->Base.File.FilePos = ByteOffset;
    }

    /* Perform the read operation */
    if(dwBytesToRead != 0)
    {
        bytes_read = read(pStream->Base.File.hFile, pvBuffer, (size_t)dwBytesToRead);
        if(bytes_read == -1)
        {
            nLastError = errno;
            return 0;
        }

        dwBytesRead = (uint32_t)bytes_read;
    }

    /* Increment the current file position by number of bytes read */
    /* If the number of bytes read doesn't match to required amount, return 0 */
    pStream->Base.File.FilePos = ByteOffset + dwBytesRead;
    if(dwBytesRead != dwBytesToRead)
        SetLastError(ERROR_HANDLE_EOF);
    return (dwBytesRead == dwBytesToRead);
}

/**
 * \a pStream Pointer to an open stream
 * \a pByteOffset Pointer to file byte offset. If NULL, writes to current position
 * \a pvBuffer Pointer to data to be written
 * \a dwBytesToWrite Number of bytes to write to the file
 */

static int BaseFile_Write(TFileStream_t * pStream, uint64_t * pByteOffset, const void * pvBuffer, uint32_t dwBytesToWrite)
{
    uint64_t ByteOffset = (pByteOffset != NULL) ? *pByteOffset : pStream->Base.File.FilePos;
    uint32_t dwBytesWritten = 0;               /* Must be set by platform-specific code */

    ssize_t bytes_written;

    /* If the byte offset is different from the current file position, */
    /* we have to update the file position */
    if(ByteOffset != pStream->Base.File.FilePos)
    {
        lseek(pStream->Base.File.hFile, ByteOffset, SEEK_SET);
        pStream->Base.File.FilePos = ByteOffset;
    }

    /* Perform the read operation */
    bytes_written = write(pStream->Base.File.hFile, pvBuffer, (size_t)dwBytesToWrite);
    if(bytes_written == -1)
    {
        nLastError = errno;
        return 0;
    }

    dwBytesWritten = (uint32_t)bytes_written;

    /* Increment the current file position by number of bytes read */
    pStream->Base.File.FilePos = ByteOffset + dwBytesWritten;

    /* Also modify the file size, if needed */
    if(pStream->Base.File.FilePos > pStream->Base.File.FileSize)
        pStream->Base.File.FileSize = pStream->Base.File.FilePos;

    if(dwBytesWritten != dwBytesToWrite)
        SetLastError(ERROR_DISK_FULL);
    return (dwBytesWritten == dwBytesToWrite);
}

/**
 * \a pStream Pointer to an open stream
 * \a NewFileSize New size of the file
 */
static int BaseFile_Resize(TFileStream_t * pStream, uint64_t NewFileSize)
{
    if(ftruncate(pStream->Base.File.hFile, NewFileSize) == -1)
    {
        nLastError = errno;
        return 0;
    }

    pStream->Base.File.FileSize = NewFileSize;
    return 1;
}

/* Gives the current file size */
static int BaseFile_GetSize(TFileStream_t * pStream, uint64_t * pFileSize)
{
    /* Note: Used by all thre base providers. */
    /* Requires the TBaseData union to have the same layout for all three base providers */
    *pFileSize = pStream->Base.File.FileSize;
    return 1;
}

/* Gives the current file position */
static int BaseFile_GetPos(TFileStream_t * pStream, uint64_t * pByteOffset)
{
    /* Note: Used by all thre base providers. */
    /* Requires the TBaseData union to have the same layout for all three base providers */
    *pByteOffset = pStream->Base.File.FilePos;
    return 1;
}

/* Renames the file pointed by pStream so that it contains data from pNewStream */
static int BaseFile_Replace(TFileStream_t * pStream, TFileStream_t * pNewStream)
{
    /* "rename" on Linux also works if the target file exists */
    if(rename(pNewStream->szFileName, pStream->szFileName) == -1)
    {
        nLastError = errno;
        return 0;
    }
    
    return 1;
}

static void BaseFile_Close(TFileStream_t * pStream)
{
    if(pStream->Base.File.hFile != -1)
    {
        close(pStream->Base.File.hFile);
    }

    /* Also invalidate the handle */
    pStream->Base.File.hFile = -1;
}

/* Initializes base functions for the disk file */
static void BaseFile_Init(TFileStream_t * pStream)
{
    pStream->BaseCreate  = BaseFile_Create;
    pStream->BaseOpen    = BaseFile_Open;
    pStream->BaseRead    = BaseFile_Read;
    pStream->BaseWrite   = BaseFile_Write;
    pStream->BaseResize  = BaseFile_Resize;
    pStream->BaseGetSize = BaseFile_GetSize;
    pStream->BaseGetPos  = BaseFile_GetPos;
    pStream->BaseClose   = BaseFile_Close;
}

/*-----------------------------------------------------------------------------
 * Local functions - base memory-mapped file support
 */

static int BaseMap_Open(TFileStream_t * pStream, const char * szFileName, uint32_t dwStreamFlags)
{
    struct stat fileinfo;
    int handle;
    int bResult = 0;

    /* Open the file */
    handle = open(szFileName, O_RDONLY);
    if(handle != -1)
    {
        /* Get the file size */
        if(fstat(handle, &fileinfo) != -1)
        {
            pStream->Base.Map.pbFile = (unsigned char *)mmap(NULL, (size_t)fileinfo.st_size, PROT_READ, MAP_PRIVATE, handle, 0);
            if(pStream->Base.Map.pbFile != NULL)
            {
                /* time_t is number of seconds since 1.1.1970, UTC. */
                /* 1 second = 10000000 (decimal) in FILETIME */
                /* Set the start to 1.1.1970 00:00:00 */
                pStream->Base.Map.FileTime = 0x019DB1DED53E8000ULL + (10000000 * fileinfo.st_mtime);
                pStream->Base.Map.FileSize = (uint64_t)fileinfo.st_size;
                pStream->Base.Map.FilePos = 0;
                bResult = 1;
            }
        }
        close(handle);
    }

    /* Did the mapping fail? */
    if(bResult == 0)
    {
        nLastError = errno;
        return 0;
    }

    return 1;
}

static int BaseMap_Read(
    TFileStream_t * pStream,                  /* Pointer to an open stream */
    uint64_t * pByteOffset,                /* Pointer to file byte offset. If NULL, it reads from the current position */
    void * pvBuffer,                        /* Pointer to data to be read */
    uint32_t dwBytesToRead)                    /* Number of bytes to read from the file */
{
    uint64_t ByteOffset = (pByteOffset != NULL) ? *pByteOffset : pStream->Base.Map.FilePos;

    /* Do we have to read anything at all? */
    if(dwBytesToRead != 0)
    {
        /* Don't allow reading past file size */
        if((ByteOffset + dwBytesToRead) > pStream->Base.Map.FileSize)
            return 0;

        /* Copy the required data */
        memcpy(pvBuffer, pStream->Base.Map.pbFile + (size_t)ByteOffset, dwBytesToRead);
    }

    /* Move the current file position */
    pStream->Base.Map.FilePos += dwBytesToRead;
    return 1;
}

static void BaseMap_Close(TFileStream_t * pStream)
{
    if(pStream->Base.Map.pbFile != NULL)
        munmap(pStream->Base.Map.pbFile, (size_t )pStream->Base.Map.FileSize);

    pStream->Base.Map.pbFile = NULL;
}

/* Initializes base functions for the mapped file */
static void BaseMap_Init(TFileStream_t * pStream)
{
    /* Supply the file stream functions */
    pStream->BaseOpen    = BaseMap_Open;
    pStream->BaseRead    = BaseMap_Read;
    pStream->BaseGetSize = BaseFile_GetSize;    /* Reuse BaseFile function */
    pStream->BaseGetPos  = BaseFile_GetPos;     /* Reuse BaseFile function */
    pStream->BaseClose   = BaseMap_Close;

    /* Mapped files are read-only */
    pStream->dwFlags |= STREAM_FLAG_READ_ONLY;
}

/*-----------------------------------------------------------------------------
 * Local functions - base HTTP file support
 */

static const char * BaseHttp_ExtractServerName(const char * szFileName, char * szServerName)
{
    /* Check for HTTP */
    if(!strncasecmp(szFileName, "http://", 7))
        szFileName += 7;

    /* Cut off the server name */
    if(szServerName != NULL)
    {
        while(szFileName[0] != 0 && szFileName[0] != '/')
            *szServerName++ = *szFileName++;
        *szServerName = 0;
    }
    else
    {
        while(szFileName[0] != 0 && szFileName[0] != '/')
            szFileName++;
    }

    /* Return the remainder */
    return szFileName;
}

static int BaseHttp_Open(TFileStream_t * pStream, const char * szFileName, uint32_t dwStreamFlags)
{
    /* Not supported */
    SetLastError(ERROR_NOT_SUPPORTED);
    pStream = pStream;
    return 0;
}

static int BaseHttp_Read(
    TFileStream_t * pStream,                  /* Pointer to an open stream */
    uint64_t * pByteOffset,                /* Pointer to file byte offset. If NULL, it reads from the current position */
    void * pvBuffer,                        /* Pointer to data to be read */
    uint32_t dwBytesToRead)                    /* Number of bytes to read from the file */
{
    /* Not supported */
    pStream = pStream;
    pByteOffset = pByteOffset;
    pvBuffer = pvBuffer;
    dwBytesToRead = dwBytesToRead;
    SetLastError(ERROR_NOT_SUPPORTED);
    return 0;
}

static void BaseHttp_Close(TFileStream_t * pStream)
{
    pStream = pStream;
}

/* Initializes base functions for the mapped file */
static void BaseHttp_Init(TFileStream_t * pStream)
{
    /* Supply the stream functions */
    pStream->BaseOpen    = BaseHttp_Open;
    pStream->BaseRead    = BaseHttp_Read;
    pStream->BaseGetSize = BaseFile_GetSize;    /* Reuse BaseFile function */
    pStream->BaseGetPos  = BaseFile_GetPos;     /* Reuse BaseFile function */
    pStream->BaseClose   = BaseHttp_Close;

    /* HTTP files are read-only */
    pStream->dwFlags |= STREAM_FLAG_READ_ONLY;
}

/*-----------------------------------------------------------------------------
 * Local functions - base block-based support
 */

/* Generic function that loads blocks from the file */
/* The function groups the block with the same availability, */
/* so the called BlockRead can finish the request in a single system call */
static int BlockStream_Read(
    TBlockStream_t * pStream,                 /* Pointer to an open stream */
    uint64_t * pByteOffset,                /* Pointer to file byte offset. If NULL, it reads from the current position */
    void * pvBuffer,                        /* Pointer to data to be read */
    uint32_t dwBytesToRead)                    /* Number of bytes to read from the file */
{
    uint64_t BlockOffset0;
    uint64_t BlockOffset;
    uint64_t ByteOffset;
    uint64_t EndOffset;
    unsigned char * TransferBuffer;
    unsigned char * BlockBuffer;
    uint32_t BlockBufferOffset;                /* Offset of the desired data in the block buffer */
    uint32_t BytesNeeded;                      /* Number of bytes that really need to be read */
    uint32_t BlockSize = pStream->BlockSize;
    uint32_t BlockCount;
    int bPrevBlockAvailable;
    int bCallbackCalled = 0;
    int bBlockAvailable;
    int bResult = 1;

    /* The base block read function must be present */
    assert(pStream->BlockRead != NULL);

    /* NOP reading of zero bytes */
    if(dwBytesToRead == 0)
        return 1;

    /* Get the current position in the stream */
    ByteOffset = (pByteOffset != NULL) ? pByteOffset[0] : pStream->StreamPos;
    EndOffset = ByteOffset + dwBytesToRead;
    if(EndOffset > pStream->StreamSize)
    {
        SetLastError(ERROR_HANDLE_EOF);
        return 0;
    }

    /* Calculate the block parameters */
    BlockOffset0 = BlockOffset = ByteOffset & ~((uint64_t)BlockSize - 1);
    BlockCount  = (uint32_t)(((EndOffset - BlockOffset) + (BlockSize - 1)) / BlockSize);
    BytesNeeded = (uint32_t)(EndOffset - BlockOffset);

    /* Remember where we have our data */
    assert((BlockSize & (BlockSize - 1)) == 0);
    BlockBufferOffset = (uint32_t)(ByteOffset & (BlockSize - 1));

    /* Allocate buffer for reading blocks */
    TransferBuffer = BlockBuffer = STORM_ALLOC(uint8_t, (BlockCount * BlockSize));
    if(TransferBuffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    /* If all blocks are available, just read all blocks at once */
    if(pStream->IsComplete == 0)
    {
        /* Now parse the blocks and send the block read request */
        /* to all blocks with the same availability */
        assert(pStream->BlockCheck != NULL);
        bPrevBlockAvailable = pStream->BlockCheck((TFileStream_t *)pStream, BlockOffset);

        /* Loop as long as we have something to read */
        while(BlockOffset < EndOffset)
        {
            /* Determine availability of the next block */
            bBlockAvailable = pStream->BlockCheck((TFileStream_t *)pStream, BlockOffset);

            /* If the availability has changed, read all blocks up to this one */
            if(bBlockAvailable != bPrevBlockAvailable)
            {
                /* Call the file stream callback, if the block is not available */
                if(pStream->pMaster && pStream->pfnCallback && bPrevBlockAvailable == 0)
                {
                    pStream->pfnCallback(pStream->UserData, BlockOffset0, (uint32_t)(BlockOffset - BlockOffset0));
                    bCallbackCalled = 1;
                }

                /* Load the continuous blocks with the same availability */
                assert(BlockOffset > BlockOffset0);
                bResult = pStream->BlockRead((TFileStream_t *)pStream, BlockOffset0, BlockOffset, BlockBuffer, BytesNeeded, bPrevBlockAvailable);
                if(!bResult)
                    break;

                /* Move the block offset */
                BlockBuffer += (uint32_t)(BlockOffset - BlockOffset0);
                BytesNeeded -= (uint32_t)(BlockOffset - BlockOffset0);
                bPrevBlockAvailable = bBlockAvailable;
                BlockOffset0 = BlockOffset;
            }

            /* Move to the block offset in the stream */
            BlockOffset += BlockSize;
        }

        /* If there is a block(s) remaining to be read, do it */
        if(BlockOffset > BlockOffset0)
        {
            /* Call the file stream callback, if the block is not available */
            if(pStream->pMaster && pStream->pfnCallback && bPrevBlockAvailable == 0)
            {
                pStream->pfnCallback(pStream->UserData, BlockOffset0, (uint32_t)(BlockOffset - BlockOffset0));
                bCallbackCalled = 1;
            }

            /* Read the complete blocks from the file */
            if(BlockOffset > pStream->StreamSize)
                BlockOffset = pStream->StreamSize;
            bResult = pStream->BlockRead((TFileStream_t *)pStream, BlockOffset0, BlockOffset, BlockBuffer, BytesNeeded, bPrevBlockAvailable);
        }
    }
    else
    {
        /* Read the complete blocks from the file */
        if(EndOffset > pStream->StreamSize)
            EndOffset = pStream->StreamSize;
        bResult = pStream->BlockRead((TFileStream_t *)pStream, BlockOffset, EndOffset, BlockBuffer, BytesNeeded, 1);
    }

    /* Now copy the data to the user buffer */
    if(bResult)
    {
        memcpy(pvBuffer, TransferBuffer + BlockBufferOffset, dwBytesToRead);
        pStream->StreamPos = ByteOffset + dwBytesToRead;
    }
    else
    {
        /* If the block read failed, set the last error */
        SetLastError(ERROR_FILE_INCOMPLETE);
    }

    /* Call the callback to indicate we are done */
    if(bCallbackCalled)
        pStream->pfnCallback(pStream->UserData, 0, 0);

    /* Free the block buffer and return */
    STORM_FREE(TransferBuffer);
    return bResult;
}

static int BlockStream_GetSize(TFileStream_t * pStream, uint64_t * pFileSize)
{
    *pFileSize = pStream->StreamSize;
    return 1;
}

static int BlockStream_GetPos(TFileStream_t * pStream, uint64_t * pByteOffset)
{
    *pByteOffset = pStream->StreamPos;
    return 1;
}

static void BlockStream_Close(TBlockStream_t * pStream)
{
    /* Free the data map, if any */
    if(pStream->FileBitmap != NULL)
        STORM_FREE(pStream->FileBitmap);
    pStream->FileBitmap = NULL;

    /* Call the base class for closing the stream */
    pStream->BaseClose((TFileStream_t *)pStream);
}

/*-----------------------------------------------------------------------------
 * File stream allocation function
 */

static STREAM_INIT StreamBaseInit[4] =
{
    BaseFile_Init,
    BaseMap_Init, 
    BaseHttp_Init,
    BaseNone_Init
};

/* This function allocates an empty structure for the file stream */
/* The stream structure is created as flat block, variable length */
/* The file name is placed after the end of the stream structure data */
static TFileStream_t * AllocateFileStream(
    const char * szFileName,
    size_t StreamSize,
    uint32_t dwStreamFlags)
{
    TFileStream_t * pMaster = NULL;
    TFileStream_t * pStream;
    const char * szNextFile = szFileName;
    size_t FileNameSize;

    /* Sanity check */
    assert(StreamSize != 0);

    /* The caller can specify chain of files in the following form: */
    /* C:\archive.MPQ* http://www.server.com/MPQs/archive-server.MPQ */
    /* In that case, we use the part after "*" as master file name */
    while(szNextFile[0] != 0 && szNextFile[0] != '*')
        szNextFile++;
    FileNameSize = (size_t)((szNextFile - szFileName) * sizeof(char));

    /* If we have a next file, we need to open it as master stream */
    /* Note that we don't care if the master stream exists or not, */
    /* If it doesn't, later attempts to read missing file block will fail */
    if(szNextFile[0] == '*')
    {
        /* Don't allow another master file in the string */
        if(strchr(szNextFile + 1, '*') != NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
        }
        
        /* Open the master file */
        pMaster = FileStream_OpenFile(szNextFile + 1, STREAM_FLAG_READ_ONLY);
    }

    /* Allocate the stream structure for the given stream type */
    pStream = (TFileStream_t *)STORM_ALLOC(uint8_t, StreamSize + FileNameSize + sizeof(char));
    if(pStream != NULL)
    {
        /* Zero the entire structure */
        memset(pStream, 0, StreamSize);
        pStream->pMaster = pMaster;
        pStream->dwFlags = dwStreamFlags;

        /* Initialize the file name */
        pStream->szFileName = (char *)((uint8_t *)pStream + StreamSize);
        memcpy(pStream->szFileName, szFileName, FileNameSize);
        pStream->szFileName[FileNameSize / sizeof(char)] = 0;

        /* Initialize the stream functions */
        StreamBaseInit[dwStreamFlags & 0x03](pStream);
    }

    return pStream;
}

/*-----------------------------------------------------------------------------
 * Local functions - flat stream support
 */

static uint32_t FlatStream_CheckFile(TBlockStream_t * pStream)
{
    unsigned char * FileBitmap = (unsigned char *)pStream->FileBitmap;
    uint32_t WholeByteCount = (pStream->BlockCount / 8);
    uint32_t ExtraBitsCount = (pStream->BlockCount & 7);
    uint8_t ExpectedValue;
	uint32_t i;

    /* Verify the whole bytes - their value must be 0xFF */
    for(i = 0; i < WholeByteCount; i++)
    {
        if(FileBitmap[i] != 0xFF)
            return 0;
    }

    /* If there are extra bits, calculate the mask */
    if(ExtraBitsCount != 0)
    {
        ExpectedValue = (uint8_t)((1 << ExtraBitsCount) - 1);
        if(FileBitmap[WholeByteCount] != ExpectedValue)
            return 0;
    }

    /* Yes, the file is complete */
    return 1;
}

static int FlatStream_LoadBitmap(TBlockStream_t * pStream)
{
    FILE_BITMAP_FOOTER Footer;
    uint64_t ByteOffset; 
    unsigned char * FileBitmap;
    uint32_t BlockCount;
    uint32_t BitmapSize;

    /* Do not load the bitmap if we should not have to */
    if(!(pStream->dwFlags & STREAM_FLAG_USE_BITMAP))
        return 0;

    /* Only if the size is greater than size of bitmap footer */
    if(pStream->Base.File.FileSize > sizeof(FILE_BITMAP_FOOTER))
    {
        /* Load the bitmap footer */
        ByteOffset = pStream->Base.File.FileSize - sizeof(FILE_BITMAP_FOOTER);
        if(pStream->BaseRead((TFileStream_t *)pStream, &ByteOffset, &Footer, sizeof(FILE_BITMAP_FOOTER)))
        {
            /* Make sure that the array is properly BSWAP-ed */
            BSWAP_ARRAY32_UNSIGNED((uint32_t *)(&Footer), sizeof(FILE_BITMAP_FOOTER));

            /* Verify if there is actually a footer */
            if(Footer.Signature == ID_FILE_BITMAP_FOOTER && Footer.Version == 0x03)
            {
                /* Get the offset of the bitmap, number of blocks and size of the bitmap */
                ByteOffset = MAKE_OFFSET64(Footer.MapOffsetHi, Footer.MapOffsetLo);
                BlockCount = (uint32_t)(((ByteOffset - 1) / Footer.BlockSize) + 1);
                BitmapSize = ((BlockCount + 7) / 8);

                /* Check if the sizes match */
                if(ByteOffset + BitmapSize + sizeof(FILE_BITMAP_FOOTER) == pStream->Base.File.FileSize)
                {
                    /* Allocate space for the bitmap */
                    FileBitmap = STORM_ALLOC(uint8_t, BitmapSize);
                    if(FileBitmap != NULL)
                    {
                        /* Load the bitmap bits */
                        if(!pStream->BaseRead((TFileStream_t *)pStream, &ByteOffset, FileBitmap, BitmapSize))
                        {
                            STORM_FREE(FileBitmap);
                            return 0;
                        }

                        /* Update the stream size */
                        pStream->BuildNumber = Footer.BuildNumber;
                        pStream->StreamSize = ByteOffset;

                        /* Fill the bitmap information */
                        pStream->FileBitmap = FileBitmap;
                        pStream->BitmapSize = BitmapSize;
                        pStream->BlockSize  = Footer.BlockSize;
                        pStream->BlockCount = BlockCount;
                        pStream->IsComplete = FlatStream_CheckFile(pStream);
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

static void FlatStream_UpdateBitmap(
    TBlockStream_t * pStream,                /* Pointer to an open stream */
    uint64_t StartOffset,
    uint64_t EndOffset)
{
    unsigned char * FileBitmap = (unsigned char *)pStream->FileBitmap;
    uint32_t BlockIndex;
    uint32_t BlockSize = pStream->BlockSize;
    uint32_t ByteIndex;
    uint8_t BitMask;

    /* Sanity checks */
    assert((StartOffset & (BlockSize - 1)) == 0);
    assert(FileBitmap != NULL);

    /* Calculate the index of the block */
    BlockIndex = (uint32_t)(StartOffset / BlockSize);
    ByteIndex = (BlockIndex / 0x08);
    BitMask = (uint8_t)(1 << (BlockIndex & 0x07));

    /* Set all bits for the specified range */
    while(StartOffset < EndOffset)
    {
        /* Set the bit */
        FileBitmap[ByteIndex] |= BitMask;

        /* Move all */
        StartOffset += BlockSize;
        ByteIndex += (BitMask >> 0x07);
        BitMask = (BitMask >> 0x07) | (BitMask << 0x01);
    }

    /* Increment the bitmap update count */
    pStream->IsModified = 1;
}

static int FlatStream_BlockCheck(
    TBlockStream_t * pStream,                /* Pointer to an open stream */
    uint64_t BlockOffset)
{
    unsigned char * FileBitmap = (unsigned char *)pStream->FileBitmap;
    uint32_t BlockIndex;
    uint8_t BitMask;

    /* Sanity checks */
    assert((BlockOffset & (pStream->BlockSize - 1)) == 0);
    assert(FileBitmap != NULL);
    
    /* Calculate the index of the block */
    BlockIndex = (uint32_t)(BlockOffset / pStream->BlockSize);
    BitMask = (uint8_t)(1 << (BlockIndex & 0x07));

    /* Check if the bit is present */
    return (FileBitmap[BlockIndex / 0x08] & BitMask) ? 1 : 0;
}

static int FlatStream_BlockRead(
    TBlockStream_t * pStream,                /* Pointer to an open stream */
    uint64_t StartOffset,
    uint64_t EndOffset,
    unsigned char * BlockBuffer,
    uint32_t BytesNeeded,
    int bAvailable)
{
    uint32_t BytesToRead = (uint32_t)(EndOffset - StartOffset);

    /* The starting offset must be aligned to size of the block */
    assert(pStream->FileBitmap != NULL);
    assert((StartOffset & (pStream->BlockSize - 1)) == 0);
    assert(StartOffset < EndOffset);

    /* If the blocks are not available, we need to load them from the master */
    /* and then save to the mirror */
    if(bAvailable == 0)
    {
        /* If we have no master, we cannot satisfy read request */
        if(pStream->pMaster == NULL)
            return 0;

        /* Load the blocks from the master stream */
        /* Note that we always have to read complete blocks */
        /* so they get properly stored to the mirror stream */
        if(!FileStream_Read(pStream->pMaster, &StartOffset, BlockBuffer, BytesToRead))
            return 0;

        /* Store the loaded blocks to the mirror file. */
        /* Note that this operation is not required to succeed */
        if(pStream->BaseWrite((TFileStream_t *)pStream, &StartOffset, BlockBuffer, BytesToRead))
            FlatStream_UpdateBitmap(pStream, StartOffset, EndOffset);

        return 1;
    }
    else
    {
        if(BytesToRead > BytesNeeded)
            BytesToRead = BytesNeeded;
        return pStream->BaseRead((TFileStream_t *)pStream, &StartOffset, BlockBuffer, BytesToRead);
    }
}

static void FlatStream_Close(TBlockStream_t * pStream)
{
    FILE_BITMAP_FOOTER Footer;

    if(pStream->FileBitmap && pStream->IsModified)
    {
        /* Write the file bitmap */
        pStream->BaseWrite((TFileStream_t *)pStream, &pStream->StreamSize, pStream->FileBitmap, pStream->BitmapSize);
        
        /* Prepare and write the file footer */
        Footer.Signature   = ID_FILE_BITMAP_FOOTER;
        Footer.Version     = 3;
        Footer.BuildNumber = pStream->BuildNumber;
        Footer.MapOffsetLo = (uint32_t)(pStream->StreamSize & 0xFFFFFFFF);
        Footer.MapOffsetHi = (uint32_t)(pStream->StreamSize >> 0x20);
        Footer.BlockSize   = pStream->BlockSize;
        BSWAP_ARRAY32_UNSIGNED(&Footer, sizeof(FILE_BITMAP_FOOTER));
        pStream->BaseWrite((TFileStream_t *)pStream, NULL, &Footer, sizeof(FILE_BITMAP_FOOTER));
    }

    /* Close the base class */
    BlockStream_Close(pStream);
}

static int FlatStream_CreateMirror(TBlockStream_t * pStream)
{
    uint64_t MasterSize = 0;
    uint64_t MirrorSize = 0;
    unsigned char * FileBitmap = NULL;
    uint32_t dwBitmapSize;
    uint32_t dwBlockCount;
    int bNeedCreateMirrorStream = 1;
    int bNeedResizeMirrorStream = 1;

    /* Do we have master function and base creation function? */
    if(pStream->pMaster == NULL || pStream->BaseCreate == NULL)
        return 0;

    /* Retrieve the master file size, block count and bitmap size */
    FileStream_GetSize(pStream->pMaster, &MasterSize);
    dwBlockCount = (uint32_t)((MasterSize + DEFAULT_BLOCK_SIZE - 1) / DEFAULT_BLOCK_SIZE);
    dwBitmapSize = (uint32_t)((dwBlockCount + 7) / 8);

    /* Setup stream size and position */
    pStream->BuildNumber = DEFAULT_BUILD_NUMBER;        /* BUGBUG: Really??? */
    pStream->StreamSize = MasterSize;
    pStream->StreamPos = 0;

    /* Open the base stream for write access */
    if(pStream->BaseOpen((TFileStream_t *)pStream, pStream->szFileName, 0))
    {
        /* If the file open succeeded, check if the file size matches required size */
        pStream->BaseGetSize((TFileStream_t *)pStream, &MirrorSize);
        if(MirrorSize == MasterSize + dwBitmapSize + sizeof(FILE_BITMAP_FOOTER))
        {
            /* Attempt to load an existing file bitmap */
            if(FlatStream_LoadBitmap(pStream))
                return 1;

            /* We need to create new file bitmap */
            bNeedResizeMirrorStream = 0;
        }

        /* We need to create mirror stream */
        bNeedCreateMirrorStream = 0;
    }

    /* Create a new stream, if needed */
    if(bNeedCreateMirrorStream)
    {
        if(!pStream->BaseCreate((TFileStream_t *)pStream))
            return 0;
    }

    /* If we need to, then resize the mirror stream */
    if(bNeedResizeMirrorStream)
    {
        if(!pStream->BaseResize((TFileStream_t *)pStream, MasterSize + dwBitmapSize + sizeof(FILE_BITMAP_FOOTER)))
            return 0;
    }

    /* Allocate the bitmap array */
    FileBitmap = STORM_ALLOC(uint8_t, dwBitmapSize);
    if(FileBitmap == NULL)
        return 0;

    /* Initialize the bitmap */
    memset(FileBitmap, 0, dwBitmapSize);
    pStream->FileBitmap = FileBitmap;
    pStream->BitmapSize = dwBitmapSize;
    pStream->BlockSize  = DEFAULT_BLOCK_SIZE;
    pStream->BlockCount = dwBlockCount;
    pStream->IsComplete = 0;
    pStream->IsModified = 1;

    /* Note: Don't write the stream bitmap right away. */
    /* Doing so would cause sparse file resize on NTFS, */
    /* which would take long time on larger files. */
    return 1;
}

static TFileStream_t * FlatStream_Open(const char * szFileName, uint32_t dwStreamFlags)
{
    TBlockStream_t * pStream;    
    uint64_t ByteOffset = 0;

    /* Create new empty stream */
    pStream = (TBlockStream_t *)AllocateFileStream(szFileName, sizeof(TBlockStream_t), dwStreamFlags);
    if(pStream == NULL)
        return NULL;

    /* Do we have a master stream? */
    if(pStream->pMaster != NULL)
    {
        if(!FlatStream_CreateMirror(pStream))
        {
            FileStream_Close((TFileStream_t *)pStream);
            SetLastError(ERROR_FILE_NOT_FOUND);
            return NULL;
        }
    }
    else
    {
        /* Attempt to open the base stream */
        if(!pStream->BaseOpen((TFileStream_t *)pStream, pStream->szFileName, dwStreamFlags))
        {
            FileStream_Close((TFileStream_t *)pStream);
            return NULL;
        }

        /* Load the bitmap, if required to */
        if(dwStreamFlags & STREAM_FLAG_USE_BITMAP)
            FlatStream_LoadBitmap(pStream);
    }

    /* If we have a stream bitmap, set the reading functions */
    /* which check presence of each file block */
    if(pStream->FileBitmap != NULL)
    {
        /* Set the stream position to zero. Stream size is already set */
        assert(pStream->StreamSize != 0);
        pStream->StreamPos = 0;
        pStream->dwFlags |= STREAM_FLAG_READ_ONLY;

        /* Supply the stream functions */
        pStream->StreamRead    = (STREAM_READ)BlockStream_Read;
        pStream->StreamGetSize = BlockStream_GetSize;
        pStream->StreamGetPos  = BlockStream_GetPos;
        pStream->StreamClose   = (STREAM_CLOSE)FlatStream_Close;

        /* Supply the block functions */
        pStream->BlockCheck    = (BLOCK_CHECK)FlatStream_BlockCheck;
        pStream->BlockRead     = (BLOCK_READ)FlatStream_BlockRead;
    }
    else
    {
        /* Reset the base position to zero */
        pStream->BaseRead((TFileStream_t *)pStream, &ByteOffset, NULL, 0);

        /* Setup stream size and position */
        pStream->StreamSize = pStream->Base.File.FileSize;
        pStream->StreamPos = 0;

        /* Set the base functions */
        pStream->StreamRead    = pStream->BaseRead;
        pStream->StreamWrite   = pStream->BaseWrite;
        pStream->StreamResize  = pStream->BaseResize;
        pStream->StreamGetSize = pStream->BaseGetSize;
        pStream->StreamGetPos  = pStream->BaseGetPos;
        pStream->StreamClose   = pStream->BaseClose;
    }

    return (TFileStream_t *)pStream;
}

/*-----------------------------------------------------------------------------
 * Local functions - partial stream support
 */

static int IsPartHeader(PPART_FILE_HEADER pPartHdr)
{
    /* Version number must be 2 */
    if(pPartHdr->PartialVersion == 2)
    {
        /* GameBuildNumber must be an ASCII number */
        if(isdigit(pPartHdr->GameBuildNumber[0]) && isdigit(pPartHdr->GameBuildNumber[1]) && isdigit(pPartHdr->GameBuildNumber[2]))
        {
            /* Block size must be power of 2 */
            if((pPartHdr->BlockSize & (pPartHdr->BlockSize - 1)) == 0)
                return 1;
        }
    }

    return 0;
}

static uint32_t PartStream_CheckFile(TBlockStream_t * pStream)
{
    PPART_FILE_MAP_ENTRY FileBitmap = (PPART_FILE_MAP_ENTRY)pStream->FileBitmap;
    uint32_t dwBlockCount;
	uint32_t i;

    /* Get the number of blocks */
    dwBlockCount = (uint32_t)((pStream->StreamSize + pStream->BlockSize - 1) / pStream->BlockSize);

    /* Check all blocks */
    for(i = 0; i < dwBlockCount; i++, FileBitmap++)
    {
        /* Few sanity checks */
        assert(FileBitmap->LargeValueHi == 0);
        assert(FileBitmap->LargeValueLo == 0);
        assert(FileBitmap->Flags == 0 || FileBitmap->Flags == 3);

        /* Check if this block is present */
        if(FileBitmap->Flags != 3)
            return 0;
    }

    /* Yes, the file is complete */
    return 1;
}

static int PartStream_LoadBitmap(TBlockStream_t * pStream)
{
    PPART_FILE_MAP_ENTRY FileBitmap;
    PART_FILE_HEADER PartHdr;
    uint64_t ByteOffset = 0;
    uint64_t StreamSize = 0;
    uint32_t BlockCount;
    uint32_t BitmapSize;

    /* Only if the size is greater than size of the bitmap header */
    if(pStream->Base.File.FileSize > sizeof(PART_FILE_HEADER))
    {
        /* Attempt to read PART file header */
        if(pStream->BaseRead((TFileStream_t *)pStream, &ByteOffset, &PartHdr, sizeof(PART_FILE_HEADER)))
        {
            /* We need to swap PART file header on big-endian platforms */
            BSWAP_ARRAY32_UNSIGNED(&PartHdr, sizeof(PART_FILE_HEADER));

            /* Verify the PART file header */
            if(IsPartHeader(&PartHdr))
            {
                /* Get the number of blocks and size of one block */
                StreamSize = MAKE_OFFSET64(PartHdr.FileSizeHi, PartHdr.FileSizeLo);
                ByteOffset = sizeof(PART_FILE_HEADER);
                BlockCount = (uint32_t)((StreamSize + PartHdr.BlockSize - 1) / PartHdr.BlockSize);
                BitmapSize = BlockCount * sizeof(PART_FILE_MAP_ENTRY);

                /* Check if sizes match */
                if((ByteOffset + BitmapSize) < pStream->Base.File.FileSize)
                {
                    /* Allocate space for the array of PART_FILE_MAP_ENTRY */
                    FileBitmap = STORM_ALLOC(PART_FILE_MAP_ENTRY, BlockCount);
                    if(FileBitmap != NULL)
                    {
                        /* Load the block map */
                        if(!pStream->BaseRead((TFileStream_t *)pStream, &ByteOffset, FileBitmap, BitmapSize))
                        {
                            STORM_FREE(FileBitmap);
                            return 0;
                        }

                        /* Make sure that the byte order is correct */
                        BSWAP_ARRAY32_UNSIGNED(FileBitmap, BitmapSize);

                        /* Update the stream size */
                        pStream->BuildNumber = StringToInt(PartHdr.GameBuildNumber);
                        pStream->StreamSize = StreamSize;

                        /* Fill the bitmap information */
                        pStream->FileBitmap = FileBitmap;
                        pStream->BitmapSize = BitmapSize;
                        pStream->BlockSize  = PartHdr.BlockSize;
                        pStream->BlockCount = BlockCount;
                        pStream->IsComplete = PartStream_CheckFile(pStream);
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

static void PartStream_UpdateBitmap(
    TBlockStream_t * pStream,                /* Pointer to an open stream */
    uint64_t StartOffset,
    uint64_t EndOffset,
    uint64_t RealOffset)
{
    PPART_FILE_MAP_ENTRY FileBitmap;
    uint32_t BlockSize = pStream->BlockSize;

    /* Sanity checks */
    assert((StartOffset & (BlockSize - 1)) == 0);
    assert(pStream->FileBitmap != NULL);

    /* Calculate the first entry in the block map */
    FileBitmap = (PPART_FILE_MAP_ENTRY)pStream->FileBitmap + (StartOffset / BlockSize);

    /* Set all bits for the specified range */
    while(StartOffset < EndOffset)
    {
        /* Set the bit */
        FileBitmap->BlockOffsHi = (uint32_t)(RealOffset >> 0x20);
        FileBitmap->BlockOffsLo = (uint32_t)(RealOffset & 0xFFFFFFFF);
        FileBitmap->Flags = 3;

        /* Move all */
        StartOffset += BlockSize;
        RealOffset += BlockSize;
        FileBitmap++;
    }

    /* Increment the bitmap update count */
    pStream->IsModified = 1;
}

static int PartStream_BlockCheck(
    TBlockStream_t * pStream,                /* Pointer to an open stream */
    uint64_t BlockOffset)
{
    PPART_FILE_MAP_ENTRY FileBitmap;

    /* Sanity checks */
    assert((BlockOffset & (pStream->BlockSize - 1)) == 0);
    assert(pStream->FileBitmap != NULL);
    
    /* Calculate the block map entry */
    FileBitmap = (PPART_FILE_MAP_ENTRY)pStream->FileBitmap + (BlockOffset / pStream->BlockSize);

    /* Check if the flags are present */
    return (FileBitmap->Flags & 0x03) ? 1 : 0;
}

static int PartStream_BlockRead(
    TBlockStream_t * pStream,
    uint64_t StartOffset,
    uint64_t EndOffset,
    unsigned char * BlockBuffer,
    uint32_t BytesNeeded,
    int bAvailable)
{
    PPART_FILE_MAP_ENTRY FileBitmap;
    uint64_t ByteOffset;
    uint32_t BytesToRead;
    uint32_t BlockIndex = (uint32_t)(StartOffset / pStream->BlockSize);

    /* The starting offset must be aligned to size of the block */
    assert(pStream->FileBitmap != NULL);
    assert((StartOffset & (pStream->BlockSize - 1)) == 0);
    assert(StartOffset < EndOffset);

    /* If the blocks are not available, we need to load them from the master */
    /* and then save to the mirror */
    if(!bAvailable)
    {
        /* If we have no master, we cannot satisfy read request */
        if(pStream->pMaster == NULL)
            return 0;

        /* Load the blocks from the master stream */
        /* Note that we always have to read complete blocks */
        /* so they get properly stored to the mirror stream */
        BytesToRead = (uint32_t)(EndOffset - StartOffset);
        if(!FileStream_Read(pStream->pMaster, &StartOffset, BlockBuffer, BytesToRead))
            return 0;

        /* The loaded blocks are going to be stored to the end of the file */
        /* Note that this operation is not required to succeed */
        if(pStream->BaseGetSize((TFileStream_t *)pStream, &ByteOffset))
        {
            /* Store the loaded blocks to the mirror file. */
            if(pStream->BaseWrite((TFileStream_t *)pStream, &ByteOffset, BlockBuffer, BytesToRead))
            {
                PartStream_UpdateBitmap(pStream, StartOffset, EndOffset, ByteOffset);
            }
        }
    }
    else
    {
        /* Get the file map entry */
        FileBitmap = (PPART_FILE_MAP_ENTRY)pStream->FileBitmap + BlockIndex;

        /* Read all blocks */
        while(StartOffset < EndOffset)
        {
            /* Get the number of bytes to be read */
            BytesToRead = (uint32_t)(EndOffset - StartOffset);
            if(BytesToRead > pStream->BlockSize)
                BytesToRead = pStream->BlockSize;
            if(BytesToRead > BytesNeeded)
                BytesToRead = BytesNeeded;

            /* Read the block */
            ByteOffset = MAKE_OFFSET64(FileBitmap->BlockOffsHi, FileBitmap->BlockOffsLo);
            if(!pStream->BaseRead((TFileStream_t *)pStream, &ByteOffset, BlockBuffer, BytesToRead))
                return 0;

            /* Move the pointers */
            StartOffset += pStream->BlockSize;
            BlockBuffer += pStream->BlockSize;
            BytesNeeded -= pStream->BlockSize;
            FileBitmap++;
        }
    }

    return 1;
}

static void PartStream_Close(TBlockStream_t * pStream)
{
    PART_FILE_HEADER PartHeader;
    uint64_t ByteOffset = 0;

    if(pStream->FileBitmap && pStream->IsModified)
    {
        /* Prepare the part file header */
        memset(&PartHeader, 0, sizeof(PART_FILE_HEADER));
        PartHeader.PartialVersion = 2;
        PartHeader.FileSizeHi     = (uint32_t)(pStream->StreamSize >> 0x20);
        PartHeader.FileSizeLo     = (uint32_t)(pStream->StreamSize & 0xFFFFFFFF);
        PartHeader.BlockSize      = pStream->BlockSize;
        
        /* Make sure that the header is properly BSWAPed */
        BSWAP_ARRAY32_UNSIGNED(&PartHeader, sizeof(PART_FILE_HEADER));
        sprintf(PartHeader.GameBuildNumber, "%u", (unsigned int)pStream->BuildNumber);

        /* Write the part header */
        pStream->BaseWrite((TFileStream_t *)pStream, &ByteOffset, &PartHeader, sizeof(PART_FILE_HEADER));

        /* Write the block bitmap */
        BSWAP_ARRAY32_UNSIGNED(pStream->FileBitmap, pStream->BitmapSize);
        pStream->BaseWrite((TFileStream_t *)pStream, NULL, pStream->FileBitmap, pStream->BitmapSize);
    }

    /* Close the base class */
    BlockStream_Close(pStream);
}

static int PartStream_CreateMirror(TBlockStream_t * pStream)
{
    uint64_t RemainingSize;
    uint64_t MasterSize = 0;
    uint64_t MirrorSize = 0;
    unsigned char * FileBitmap = NULL;
    uint32_t dwBitmapSize;
    uint32_t dwBlockCount;
    int bNeedCreateMirrorStream = 1;
    int bNeedResizeMirrorStream = 1;

    /* Do we have master function and base creation function? */
    if(pStream->pMaster == NULL || pStream->BaseCreate == NULL)
        return 0;

    /* Retrieve the master file size, block count and bitmap size */
    FileStream_GetSize(pStream->pMaster, &MasterSize);
    dwBlockCount = (uint32_t)((MasterSize + DEFAULT_BLOCK_SIZE - 1) / DEFAULT_BLOCK_SIZE);
    dwBitmapSize = (uint32_t)(dwBlockCount * sizeof(PART_FILE_MAP_ENTRY));

    /* Setup stream size and position */
    pStream->BuildNumber = DEFAULT_BUILD_NUMBER;        /* BUGBUG: Really??? */
    pStream->StreamSize = MasterSize;
    pStream->StreamPos = 0;

    /* Open the base stream for write access */
    if(pStream->BaseOpen((TFileStream_t *)pStream, pStream->szFileName, 0))
    {
        /* If the file open succeeded, check if the file size matches required size */
        pStream->BaseGetSize((TFileStream_t *)pStream, &MirrorSize);
        if(MirrorSize >= sizeof(PART_FILE_HEADER) + dwBitmapSize)
        {
            /* Check if the remaining size is aligned to block */
            RemainingSize = MirrorSize - sizeof(PART_FILE_HEADER) - dwBitmapSize;
            if((RemainingSize & (DEFAULT_BLOCK_SIZE - 1)) == 0 || RemainingSize == MasterSize)
            {
                /* Attempt to load an existing file bitmap */
                if(PartStream_LoadBitmap(pStream))
                    return 1;
            }
        }

        /* We need to create mirror stream */
        bNeedCreateMirrorStream = 0;
    }

    /* Create a new stream, if needed */
    if(bNeedCreateMirrorStream)
    {
        if(!pStream->BaseCreate((TFileStream_t *)pStream))
            return 0;
    }

    /* If we need to, then resize the mirror stream */
    if(bNeedResizeMirrorStream)
    {
        if(!pStream->BaseResize((TFileStream_t *)pStream, sizeof(PART_FILE_HEADER) + dwBitmapSize))
            return 0;
    }

    /* Allocate the bitmap array */
    FileBitmap = STORM_ALLOC(uint8_t, dwBitmapSize);
    if(FileBitmap == NULL)
        return 0;

    /* Initialize the bitmap */
    memset(FileBitmap, 0, dwBitmapSize);
    pStream->FileBitmap = FileBitmap;
    pStream->BitmapSize = dwBitmapSize;
    pStream->BlockSize  = DEFAULT_BLOCK_SIZE;
    pStream->BlockCount = dwBlockCount;
    pStream->IsComplete = 0;
    pStream->IsModified = 1;

    /* Note: Don't write the stream bitmap right away. */
    /* Doing so would cause sparse file resize on NTFS, */
    /* which would take long time on larger files. */
    return 1;
}


static TFileStream_t * PartStream_Open(const char * szFileName, uint32_t dwStreamFlags)
{
    TBlockStream_t * pStream;

    /* Create new empty stream */
    pStream = (TBlockStream_t *)AllocateFileStream(szFileName, sizeof(TBlockStream_t), dwStreamFlags);
    if(pStream == NULL)
        return NULL;

    /* Do we have a master stream? */
    if(pStream->pMaster != NULL)
    {
        if(!PartStream_CreateMirror(pStream))
        {
            FileStream_Close((TFileStream_t *)pStream);
            SetLastError(ERROR_FILE_NOT_FOUND);
            return NULL;
        }
    }
    else
    {
        /* Attempt to open the base stream */
        if(!pStream->BaseOpen((TFileStream_t *)pStream, pStream->szFileName, dwStreamFlags))
        {
            FileStream_Close((TFileStream_t *)pStream);
            return NULL;
        }

        /* Load the part stream block map */
        if(!PartStream_LoadBitmap(pStream))
        {
            FileStream_Close((TFileStream_t *)pStream);
            SetLastError(ERROR_BAD_FORMAT);
            return NULL;
        }
    }

    /* Set the stream position to zero. Stream size is already set */
    assert(pStream->StreamSize != 0);
    pStream->StreamPos = 0;
    pStream->dwFlags |= STREAM_FLAG_READ_ONLY;

    /* Set new function pointers */
    pStream->StreamRead    = (STREAM_READ)BlockStream_Read;
    pStream->StreamGetPos  = BlockStream_GetPos;
    pStream->StreamGetSize = BlockStream_GetSize;
    pStream->StreamClose   = (STREAM_CLOSE)PartStream_Close;

    /* Supply the block functions */
    pStream->BlockCheck    = (BLOCK_CHECK)PartStream_BlockCheck;
    pStream->BlockRead     = (BLOCK_READ)PartStream_BlockRead;
    return (TFileStream_t *)pStream;
}

/*-----------------------------------------------------------------------------
 * Local functions - MPQE stream support
 */

static const char * szKeyTemplate = "expand 32-byte k000000000000000000000000000000000000000000000000";

static const char * AuthCodeArray[] =
{
    /* Starcraft II (Heart of the Swarm)
     * Authentication code URL: http://dist.blizzard.com/mediakey/hots-authenticationcode-bgdl.txt
     *                                                                                          -0C-    -1C--08-    -18--04-    -14--00-    -10- */
    "S48B6CDTN5XEQAKQDJNDLJBJ73FDFM3U",         /* SC2 Heart of the Swarm-all : "expand 32-byte kQAKQ0000FM3UN5XE000073FD6CDT0000LJBJS48B0000DJND"

     * Diablo III: Agent.exe (1.0.0.954)
     * Address of decryption routine: 00502b00                             
     * Pointer to decryptor object: ECX
     * Pointer to key: ECX+0x5C
     * Authentication code URL: http://dist.blizzard.com/mediakey/d3-authenticationcode-enGB.txt
     *                                                                                           -0C-    -1C--08-    -18--04-    -14--00-    -10- */
    "UCMXF6EJY352EFH4XFRXCFH2XC9MQRZK",         /* Diablo III Installer (deDE): "expand 32-byte kEFH40000QRZKY3520000XC9MF6EJ0000CFH2UCMX0000XFRX" */
    "MMKVHY48RP7WXP4GHYBQ7SL9J9UNPHBP",         /* Diablo III Installer (enGB): "expand 32-byte kXP4G0000PHBPRP7W0000J9UNHY4800007SL9MMKV0000HYBQ" */
    "8MXLWHQ7VGGLTZ9MQZQSFDCLJYET3CPP",         /* Diablo III Installer (enSG): "expand 32-byte kTZ9M00003CPPVGGL0000JYETWHQ70000FDCL8MXL0000QZQS" */
    "EJ2R5TM6XFE2GUNG5QDGHKQ9UAKPWZSZ",         /* Diablo III Installer (enUS): "expand 32-byte kGUNG0000WZSZXFE20000UAKP5TM60000HKQ9EJ2R00005QDG" */
    "PBGFBE42Z6LNK65UGJQ3WZVMCLP4HQQT",         /* Diablo III Installer (esES): "expand 32-byte kK65U0000HQQTZ6LN0000CLP4BE420000WZVMPBGF0000GJQ3" */
    "X7SEJJS9TSGCW5P28EBSC47AJPEY8VU2",         /* Diablo III Installer (esMX): "expand 32-byte kW5P200008VU2TSGC0000JPEYJJS90000C47AX7SE00008EBS" */
    "5KVBQA8VYE6XRY3DLGC5ZDE4XS4P7YA2",         /* Diablo III Installer (frFR): "expand 32-byte kRY3D00007YA2YE6X0000XS4PQA8V0000ZDE45KVB0000LGC5" */
    "478JD2K56EVNVVY4XX8TDWYT5B8KB254",         /* Diablo III Installer (itIT): "expand 32-byte kVVY40000B2546EVN00005B8KD2K50000DWYT478J0000XX8T" */
    "8TS4VNFQRZTN6YWHE9CHVDH9NVWD474A",         /* Diablo III Installer (koKR): "expand 32-byte k6YWH0000474ARZTN0000NVWDVNFQ0000VDH98TS40000E9CH" */
    "LJ52Z32DF4LZ4ZJJXVKK3AZQA6GABLJB",         /* Diablo III Installer (plPL): "expand 32-byte k4ZJJ0000BLJBF4LZ0000A6GAZ32D00003AZQLJ520000XVKK" */
    "K6BDHY2ECUE2545YKNLBJPVYWHE7XYAG",         /* Diablo III Installer (ptBR): "expand 32-byte k545Y0000XYAGCUE20000WHE7HY2E0000JPVYK6BD0000KNLB" */
    "NDVW8GWLAYCRPGRNY8RT7ZZUQU63VLPR",         /* Diablo III Installer (ruRU): "expand 32-byte kXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" */
    "6VWCQTN8V3ZZMRUCZXV8A8CGUX2TAA8H",         /* Diablo III Installer (zhTW): "expand 32-byte kMRUC0000AA8HV3ZZ0000UX2TQTN80000A8CG6VWC0000ZXV8" */
/*  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",          * Diablo III Installer (zhCN): "expand 32-byte kXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" */

    /* Starcraft II (Wings of Liberty): Installer.exe (4.1.1.4219)
     * Address of decryption routine: 0053A3D0
     * Pointer to decryptor object: ECX
     * Pointer to key: ECX+0x5C
     * Authentication code URL: http://dist.blizzard.com/mediakey/sc2-authenticationcode-enUS.txt
     *                                                                                          -0C-    -1C--08-    -18--04-    -14--00-    -10- */
    "Y45MD3CAK4KXSSXHYD9VY64Z8EKJ4XFX",         /* SC2 Wings of Liberty (deDE): "expand 32-byte kSSXH00004XFXK4KX00008EKJD3CA0000Y64ZY45M0000YD9V" */
    "G8MN8UDG6NA2ANGY6A3DNY82HRGF29ZH",         /* SC2 Wings of Liberty (enGB): "expand 32-byte kANGY000029ZH6NA20000HRGF8UDG0000NY82G8MN00006A3D" */
    "W9RRHLB2FDU9WW5B3ECEBLRSFWZSF7HW",         /* SC2 Wings of Liberty (enSG): "expand 32-byte kWW5B0000F7HWFDU90000FWZSHLB20000BLRSW9RR00003ECE" */
    "3DH5RE5NVM5GTFD85LXGWT6FK859ETR5",         /* SC2 Wings of Liberty (enUS): "expand 32-byte kTFD80000ETR5VM5G0000K859RE5N0000WT6F3DH500005LXG" */
    "8WLKUAXE94PFQU4Y249PAZ24N4R4XKTQ",         /* SC2 Wings of Liberty (esES): "expand 32-byte kQU4Y0000XKTQ94PF0000N4R4UAXE0000AZ248WLK0000249P" */
    "A34DXX3VHGGXSQBRFE5UFFDXMF9G4G54",         /* SC2 Wings of Liberty (esMX): "expand 32-byte kSQBR00004G54HGGX0000MF9GXX3V0000FFDXA34D0000FE5U" */
    "ZG7J9K938HJEFWPQUA768MA2PFER6EAJ",         /* SC2 Wings of Liberty (frFR): "expand 32-byte kFWPQ00006EAJ8HJE0000PFER9K9300008MA2ZG7J0000UA76" */
    "NE7CUNNNTVAPXV7E3G2BSVBWGVMW8BL2",         /* SC2 Wings of Liberty (itIT): "expand 32-byte kXV7E00008BL2TVAP0000GVMWUNNN0000SVBWNE7C00003G2B" */
    "3V9E2FTMBM9QQWK7U6MAMWAZWQDB838F",         /* SC2 Wings of Liberty (koKR): "expand 32-byte kQWK70000838FBM9Q0000WQDB2FTM0000MWAZ3V9E0000U6MA" */
    "2NSFB8MELULJ83U6YHA3UP6K4MQD48L6",         /* SC2 Wings of Liberty (plPL): "expand 32-byte k83U6000048L6LULJ00004MQDB8ME0000UP6K2NSF0000YHA3" */
    "QA2TZ9EWZ4CUU8BMB5WXCTY65F9CSW4E",         /* SC2 Wings of Liberty (ptBR): "expand 32-byte kU8BM0000SW4EZ4CU00005F9CZ9EW0000CTY6QA2T0000B5WX" */
    "VHB378W64BAT9SH7D68VV9NLQDK9YEGT",         /* SC2 Wings of Liberty (ruRU): "expand 32-byte k9SH70000YEGT4BAT0000QDK978W60000V9NLVHB30000D68V" */
    "U3NFQJV4M6GC7KBN9XQJ3BRDN3PLD9NE",         /* SC2 Wings of Liberty (zhTW): "expand 32-byte k7KBN0000D9NEM6GC0000N3PLQJV400003BRDU3NF00009XQJ" */

    NULL
};

static uint32_t Rol32(uint32_t dwValue, uint32_t dwRolCount)
{
    uint32_t dwShiftRight = 32 - dwRolCount;

    return (dwValue << dwRolCount) | (dwValue >> dwShiftRight);
}

static void CreateKeyFromAuthCode(
    unsigned char * pbKeyBuffer,
    const char * szAuthCode)
{
    uint32_t * KeyPosition = (uint32_t *)(pbKeyBuffer + 0x10);
    uint32_t * AuthCode32 = (uint32_t *)szAuthCode;

    memcpy(pbKeyBuffer, szKeyTemplate, MPQE_CHUNK_SIZE);
    KeyPosition[0x00] = AuthCode32[0x03];
    KeyPosition[0x02] = AuthCode32[0x07];
    KeyPosition[0x03] = AuthCode32[0x02];
    KeyPosition[0x05] = AuthCode32[0x06];
    KeyPosition[0x06] = AuthCode32[0x01];
    KeyPosition[0x08] = AuthCode32[0x05];
    KeyPosition[0x09] = AuthCode32[0x00];
    KeyPosition[0x0B] = AuthCode32[0x04];
    BSWAP_ARRAY32_UNSIGNED(pbKeyBuffer, MPQE_CHUNK_SIZE);
}

static void DecryptFileChunk(
    uint32_t * MpqData,
    unsigned char * pbKey,
    uint64_t ByteOffset,
    uint32_t dwLength)
{
    uint64_t ChunkOffset;
    uint32_t KeyShuffled[0x10];
    uint32_t KeyMirror[0x10];
    uint32_t RoundCount = 0x14;

    /* Prepare the key */
    ChunkOffset = ByteOffset / MPQE_CHUNK_SIZE;
    memcpy(KeyMirror, pbKey, MPQE_CHUNK_SIZE);
    BSWAP_ARRAY32_UNSIGNED(KeyMirror, MPQE_CHUNK_SIZE);
    KeyMirror[0x05] = (uint32_t)(ChunkOffset >> 32);
    KeyMirror[0x08] = (uint32_t)(ChunkOffset);

    while(dwLength >= MPQE_CHUNK_SIZE)
    {
		uint32_t i;
		
        /* Shuffle the key - part 1 */
        KeyShuffled[0x0E] = KeyMirror[0x00];
        KeyShuffled[0x0C] = KeyMirror[0x01];
        KeyShuffled[0x05] = KeyMirror[0x02];
        KeyShuffled[0x0F] = KeyMirror[0x03];
        KeyShuffled[0x0A] = KeyMirror[0x04];
        KeyShuffled[0x07] = KeyMirror[0x05];
        KeyShuffled[0x0B] = KeyMirror[0x06];
        KeyShuffled[0x09] = KeyMirror[0x07];
        KeyShuffled[0x03] = KeyMirror[0x08];
        KeyShuffled[0x06] = KeyMirror[0x09];
        KeyShuffled[0x08] = KeyMirror[0x0A];
        KeyShuffled[0x0D] = KeyMirror[0x0B];
        KeyShuffled[0x02] = KeyMirror[0x0C];
        KeyShuffled[0x04] = KeyMirror[0x0D];
        KeyShuffled[0x01] = KeyMirror[0x0E];
        KeyShuffled[0x00] = KeyMirror[0x0F];
        
        /* Shuffle the key - part 2 */
        for(i = 0; i < RoundCount; i += 2)
        {
            KeyShuffled[0x0A] = KeyShuffled[0x0A] ^ Rol32((KeyShuffled[0x0E] + KeyShuffled[0x02]), 0x07);
            KeyShuffled[0x03] = KeyShuffled[0x03] ^ Rol32((KeyShuffled[0x0A] + KeyShuffled[0x0E]), 0x09);
            KeyShuffled[0x02] = KeyShuffled[0x02] ^ Rol32((KeyShuffled[0x03] + KeyShuffled[0x0A]), 0x0D);
            KeyShuffled[0x0E] = KeyShuffled[0x0E] ^ Rol32((KeyShuffled[0x02] + KeyShuffled[0x03]), 0x12);

            KeyShuffled[0x07] = KeyShuffled[0x07] ^ Rol32((KeyShuffled[0x0C] + KeyShuffled[0x04]), 0x07);
            KeyShuffled[0x06] = KeyShuffled[0x06] ^ Rol32((KeyShuffled[0x07] + KeyShuffled[0x0C]), 0x09);
            KeyShuffled[0x04] = KeyShuffled[0x04] ^ Rol32((KeyShuffled[0x06] + KeyShuffled[0x07]), 0x0D);
            KeyShuffled[0x0C] = KeyShuffled[0x0C] ^ Rol32((KeyShuffled[0x04] + KeyShuffled[0x06]), 0x12);

            KeyShuffled[0x0B] = KeyShuffled[0x0B] ^ Rol32((KeyShuffled[0x05] + KeyShuffled[0x01]), 0x07);
            KeyShuffled[0x08] = KeyShuffled[0x08] ^ Rol32((KeyShuffled[0x0B] + KeyShuffled[0x05]), 0x09);
            KeyShuffled[0x01] = KeyShuffled[0x01] ^ Rol32((KeyShuffled[0x08] + KeyShuffled[0x0B]), 0x0D);
            KeyShuffled[0x05] = KeyShuffled[0x05] ^ Rol32((KeyShuffled[0x01] + KeyShuffled[0x08]), 0x12);

            KeyShuffled[0x09] = KeyShuffled[0x09] ^ Rol32((KeyShuffled[0x0F] + KeyShuffled[0x00]), 0x07);
            KeyShuffled[0x0D] = KeyShuffled[0x0D] ^ Rol32((KeyShuffled[0x09] + KeyShuffled[0x0F]), 0x09);
            KeyShuffled[0x00] = KeyShuffled[0x00] ^ Rol32((KeyShuffled[0x0D] + KeyShuffled[0x09]), 0x0D);
            KeyShuffled[0x0F] = KeyShuffled[0x0F] ^ Rol32((KeyShuffled[0x00] + KeyShuffled[0x0D]), 0x12);

            KeyShuffled[0x04] = KeyShuffled[0x04] ^ Rol32((KeyShuffled[0x0E] + KeyShuffled[0x09]), 0x07);
            KeyShuffled[0x08] = KeyShuffled[0x08] ^ Rol32((KeyShuffled[0x04] + KeyShuffled[0x0E]), 0x09);
            KeyShuffled[0x09] = KeyShuffled[0x09] ^ Rol32((KeyShuffled[0x08] + KeyShuffled[0x04]), 0x0D);
            KeyShuffled[0x0E] = KeyShuffled[0x0E] ^ Rol32((KeyShuffled[0x09] + KeyShuffled[0x08]), 0x12);

            KeyShuffled[0x01] = KeyShuffled[0x01] ^ Rol32((KeyShuffled[0x0C] + KeyShuffled[0x0A]), 0x07);
            KeyShuffled[0x0D] = KeyShuffled[0x0D] ^ Rol32((KeyShuffled[0x01] + KeyShuffled[0x0C]), 0x09);
            KeyShuffled[0x0A] = KeyShuffled[0x0A] ^ Rol32((KeyShuffled[0x0D] + KeyShuffled[0x01]), 0x0D);
            KeyShuffled[0x0C] = KeyShuffled[0x0C] ^ Rol32((KeyShuffled[0x0A] + KeyShuffled[0x0D]), 0x12);

            KeyShuffled[0x00] = KeyShuffled[0x00] ^ Rol32((KeyShuffled[0x05] + KeyShuffled[0x07]), 0x07);
            KeyShuffled[0x03] = KeyShuffled[0x03] ^ Rol32((KeyShuffled[0x00] + KeyShuffled[0x05]), 0x09);
            KeyShuffled[0x07] = KeyShuffled[0x07] ^ Rol32((KeyShuffled[0x03] + KeyShuffled[0x00]), 0x0D);
            KeyShuffled[0x05] = KeyShuffled[0x05] ^ Rol32((KeyShuffled[0x07] + KeyShuffled[0x03]), 0x12);

            KeyShuffled[0x02] = KeyShuffled[0x02] ^ Rol32((KeyShuffled[0x0F] + KeyShuffled[0x0B]), 0x07);
            KeyShuffled[0x06] = KeyShuffled[0x06] ^ Rol32((KeyShuffled[0x02] + KeyShuffled[0x0F]), 0x09);
            KeyShuffled[0x0B] = KeyShuffled[0x0B] ^ Rol32((KeyShuffled[0x06] + KeyShuffled[0x02]), 0x0D);
            KeyShuffled[0x0F] = KeyShuffled[0x0F] ^ Rol32((KeyShuffled[0x0B] + KeyShuffled[0x06]), 0x12);
        }

        /* Decrypt one data chunk */
        BSWAP_ARRAY32_UNSIGNED(MpqData, MPQE_CHUNK_SIZE);
        MpqData[0x00] = MpqData[0x00] ^ (KeyShuffled[0x0E] + KeyMirror[0x00]);
        MpqData[0x01] = MpqData[0x01] ^ (KeyShuffled[0x04] + KeyMirror[0x0D]);
        MpqData[0x02] = MpqData[0x02] ^ (KeyShuffled[0x08] + KeyMirror[0x0A]);
        MpqData[0x03] = MpqData[0x03] ^ (KeyShuffled[0x09] + KeyMirror[0x07]);
        MpqData[0x04] = MpqData[0x04] ^ (KeyShuffled[0x0A] + KeyMirror[0x04]);
        MpqData[0x05] = MpqData[0x05] ^ (KeyShuffled[0x0C] + KeyMirror[0x01]);
        MpqData[0x06] = MpqData[0x06] ^ (KeyShuffled[0x01] + KeyMirror[0x0E]);
        MpqData[0x07] = MpqData[0x07] ^ (KeyShuffled[0x0D] + KeyMirror[0x0B]);
        MpqData[0x08] = MpqData[0x08] ^ (KeyShuffled[0x03] + KeyMirror[0x08]);
        MpqData[0x09] = MpqData[0x09] ^ (KeyShuffled[0x07] + KeyMirror[0x05]);
        MpqData[0x0A] = MpqData[0x0A] ^ (KeyShuffled[0x05] + KeyMirror[0x02]);
        MpqData[0x0B] = MpqData[0x0B] ^ (KeyShuffled[0x00] + KeyMirror[0x0F]);
        MpqData[0x0C] = MpqData[0x0C] ^ (KeyShuffled[0x02] + KeyMirror[0x0C]);
        MpqData[0x0D] = MpqData[0x0D] ^ (KeyShuffled[0x06] + KeyMirror[0x09]);
        MpqData[0x0E] = MpqData[0x0E] ^ (KeyShuffled[0x0B] + KeyMirror[0x06]);
        MpqData[0x0F] = MpqData[0x0F] ^ (KeyShuffled[0x0F] + KeyMirror[0x03]);
        BSWAP_ARRAY32_UNSIGNED(MpqData, MPQE_CHUNK_SIZE);

        /* Update byte offset in the key */
        KeyMirror[0x08]++;
        if(KeyMirror[0x08] == 0)
            KeyMirror[0x05]++;

        /* Move pointers and decrease number of bytes to decrypt */
        MpqData  += (MPQE_CHUNK_SIZE / sizeof(uint32_t));
        dwLength -= MPQE_CHUNK_SIZE;
    }
}

static int MpqeStream_DetectFileKey(TEncryptedStream_t * pStream)
{
    uint64_t ByteOffset = 0;
    uint8_t EncryptedHeader[MPQE_CHUNK_SIZE];
    uint8_t FileHeader[MPQE_CHUNK_SIZE];

    /* Read the first file chunk */
    if(pStream->BaseRead((TFileStream_t *)pStream, &ByteOffset, EncryptedHeader, sizeof(EncryptedHeader)))
    {
		int i;
		
        /* We just try all known keys one by one */
        for(i = 0; AuthCodeArray[i] != NULL; i++)
        {
            /* Prepare they decryption key from game serial number */
            CreateKeyFromAuthCode(pStream->Key, AuthCodeArray[i]);

            /* Try to decrypt with the given key */
            memcpy(FileHeader, EncryptedHeader, MPQE_CHUNK_SIZE);
            DecryptFileChunk((uint32_t *)FileHeader, pStream->Key, ByteOffset, MPQE_CHUNK_SIZE);

            /* We check the decrypted data */
            /* All known encrypted MPQs have header at the begin of the file, */
            /* so we check for MPQ signature there. */
            if(FileHeader[0] == 'M' && FileHeader[1] == 'P' && FileHeader[2] == 'Q')
            {
                /* Update the stream size */
                pStream->StreamSize = pStream->Base.File.FileSize;

                /* Fill the block information */
                pStream->BlockSize  = MPQE_CHUNK_SIZE;
                pStream->BlockCount = (uint32_t)(pStream->Base.File.FileSize + MPQE_CHUNK_SIZE - 1) / MPQE_CHUNK_SIZE;
                pStream->IsComplete = 1;
                return 1;
            }
        }
    }

    /* Key not found, sorry */
    return 0;
}

static int MpqeStream_BlockRead(
    TEncryptedStream_t * pStream,
    uint64_t StartOffset,
    uint64_t EndOffset,
    unsigned char * BlockBuffer,
    uint32_t BytesNeeded,
    int bAvailable)
{
    uint32_t dwBytesToRead;

    assert((StartOffset & (pStream->BlockSize - 1)) == 0);
    assert(StartOffset < EndOffset);
    assert(bAvailable);
    BytesNeeded = BytesNeeded;
    bAvailable = bAvailable;

    /* Read the file from the stream as-is */
    /* Limit the reading to number of blocks really needed */
    dwBytesToRead = (uint32_t)(EndOffset - StartOffset);
    if(!pStream->BaseRead((TFileStream_t *)pStream, &StartOffset, BlockBuffer, dwBytesToRead))
        return 0;

    /* Decrypt the data */
    dwBytesToRead = (dwBytesToRead + MPQE_CHUNK_SIZE - 1) & ~(MPQE_CHUNK_SIZE - 1);
    DecryptFileChunk((uint32_t *)BlockBuffer, pStream->Key, StartOffset, dwBytesToRead);
    return 1;
}

static TFileStream_t * MpqeStream_Open(const char * szFileName, uint32_t dwStreamFlags)
{
    TEncryptedStream_t * pStream;

    /* Create new empty stream */
    pStream = (TEncryptedStream_t *)AllocateFileStream(szFileName, sizeof(TEncryptedStream_t), dwStreamFlags);
    if(pStream == NULL)
        return NULL;

    /* Attempt to open the base stream */
    assert(pStream->BaseOpen != NULL);
    if(!pStream->BaseOpen((TFileStream_t *)pStream, pStream->szFileName, dwStreamFlags))
        return NULL;

    /* Determine the encryption key for the MPQ */
    if(MpqeStream_DetectFileKey(pStream))
    {
        /* Set the stream position and size */
        assert(pStream->StreamSize != 0);
        pStream->StreamPos = 0;
        pStream->dwFlags |= STREAM_FLAG_READ_ONLY;

        /* Set new function pointers */
        pStream->StreamRead    = (STREAM_READ)BlockStream_Read;
        pStream->StreamGetPos  = BlockStream_GetPos;
        pStream->StreamGetSize = BlockStream_GetSize;
        pStream->StreamClose   = pStream->BaseClose;

        /* Supply the block functions */
        pStream->BlockRead     = (BLOCK_READ)MpqeStream_BlockRead;
        return (TFileStream_t *)pStream;
    }

    /* Cleanup the stream and return */
    FileStream_Close((TFileStream_t *)pStream);
    SetLastError(ERROR_UNKNOWN_FILE_KEY);
    return NULL;
}

/*-----------------------------------------------------------------------------
 * Local functions - Block4 stream support
 */

#define BLOCK4_BLOCK_SIZE   0x4000          /* Size of one block */
#define BLOCK4_HASH_SIZE    0x20            /* Size of MD5 hash that is after each block */
#define BLOCK4_MAX_BLOCKS   0x00002000      /* Maximum amount of blocks per file */
#define BLOCK4_MAX_FSIZE    0x08040000      /* Max size of one file */

static int Block4Stream_BlockRead(
    TBlockStream_t * pStream,                /* Pointer to an open stream */
    uint64_t StartOffset,
    uint64_t EndOffset,
    unsigned char * BlockBuffer,
    uint32_t BytesNeeded,
    int bAvailable)
{
    union TBaseProviderData * BaseArray = (union TBaseProviderData *)pStream->FileBitmap;
    uint64_t ByteOffset;
    uint32_t BytesToRead;
    uint32_t StreamIndex;
    uint32_t BlockIndex;
    int bResult;

    /* The starting offset must be aligned to size of the block */
    assert(pStream->FileBitmap != NULL);
    assert((StartOffset & (pStream->BlockSize - 1)) == 0);
    assert(StartOffset < EndOffset);
    assert(bAvailable);

    /* Keep compiler happy 
    bAvailable = bAvailable;
    EndOffset = EndOffset; */

    while(BytesNeeded != 0)
    {
        /* Calculate the block index and the file index */
        StreamIndex = (uint32_t)((StartOffset / pStream->BlockSize) / BLOCK4_MAX_BLOCKS);
        BlockIndex  = (uint32_t)((StartOffset / pStream->BlockSize) % BLOCK4_MAX_BLOCKS);
        if(StreamIndex > pStream->BitmapSize)
            return 0;

        /* Calculate the block offset */
        ByteOffset = ((uint64_t)BlockIndex * (BLOCK4_BLOCK_SIZE + BLOCK4_HASH_SIZE));
        BytesToRead = STORMLIB_MIN(BytesNeeded, BLOCK4_BLOCK_SIZE);

        /* Read from the base stream */
        pStream->Base = BaseArray[StreamIndex];
        bResult = pStream->BaseRead((TFileStream_t *)pStream, &ByteOffset, BlockBuffer, BytesToRead);
        BaseArray[StreamIndex] = pStream->Base;

        /* Did the result succeed? */
        if(!bResult)
            return 0;

        /* Move pointers */
        StartOffset += BytesToRead;
        BlockBuffer += BytesToRead;
        BytesNeeded -= BytesToRead;
    }

    return 1;
}


static void Block4Stream_Close(TBlockStream_t * pStream)
{
    union TBaseProviderData * BaseArray = (union TBaseProviderData *)pStream->FileBitmap;

    /* If we have a non-zero count of base streams, */
    /* we have to close them all */
    if(BaseArray != NULL)
    {
		uint32_t i;
		
        /* Close all base streams */
        for(i = 0; i < pStream->BitmapSize; i++)
        {
            memcpy(&pStream->Base, BaseArray + i, sizeof(union TBaseProviderData));
            pStream->BaseClose((TFileStream_t *)pStream);
        }
    }

    /* Free the data map, if any */
    if(pStream->FileBitmap != NULL)
        STORM_FREE(pStream->FileBitmap);
    pStream->FileBitmap = NULL;

    /* Do not call the BaseClose function, */
    /* we closed all handles already */
    return;
}

static TFileStream_t * Block4Stream_Open(const char * szFileName, uint32_t dwStreamFlags)
{
    union TBaseProviderData * NewBaseArray = NULL;
    uint64_t RemainderBlock;
    uint64_t BlockCount;
    uint64_t FileSize;
    TBlockStream_t * pStream;
    char * szNameBuff;
    size_t nNameLength;
    uint32_t dwBaseFiles = 0;
    uint32_t dwBaseFlags;

    /* Create new empty stream */
    pStream = (TBlockStream_t *)AllocateFileStream(szFileName, sizeof(TBlockStream_t), dwStreamFlags);
    if(pStream == NULL)
        return NULL;

    /* Sanity check */
    assert(pStream->BaseOpen != NULL);

    /* Get the length of the file name without numeric suffix */
    nNameLength = strlen(pStream->szFileName);
    if(pStream->szFileName[nNameLength - 2] == '.' && pStream->szFileName[nNameLength - 1] == '0')
        nNameLength -= 2;
    pStream->szFileName[nNameLength] = 0;

    /* Supply the stream functions */
    pStream->StreamRead    = (STREAM_READ)BlockStream_Read;
    pStream->StreamGetSize = BlockStream_GetSize;
    pStream->StreamGetPos  = BlockStream_GetPos;
    pStream->StreamClose   = (STREAM_CLOSE)Block4Stream_Close;
    pStream->BlockRead     = (BLOCK_READ)Block4Stream_BlockRead;

    /* Allocate work space for numeric names */
    szNameBuff = STORM_ALLOC(char, nNameLength + 4);
    if(szNameBuff != NULL)
    {
		int nSuffix;
		
        /* Set the base flags */
        dwBaseFlags = (dwStreamFlags & STREAM_PROVIDERS_MASK) | STREAM_FLAG_READ_ONLY;

        /* Go all suffixes from 0 to 30 */
        for(nSuffix = 0; nSuffix < 30; nSuffix++)
        {
            /* Open the n-th file */
            sprintf(szNameBuff, "%s.%u", pStream->szFileName, nSuffix);
            if(!pStream->BaseOpen((TFileStream_t *)pStream, szNameBuff, dwBaseFlags))
                break;

            /* If the open succeeded, we re-allocate the base provider array */
            NewBaseArray = STORM_ALLOC(union TBaseProviderData, dwBaseFiles + 1);
            if(NewBaseArray == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return NULL;
            }

            /* Copy the old base data array to the new base data array */
            if(pStream->FileBitmap != NULL)
            {
                memcpy(NewBaseArray, pStream->FileBitmap, sizeof(union TBaseProviderData) * dwBaseFiles);
                STORM_FREE(pStream->FileBitmap);
            }

            /* Also copy the opened base array */
            memcpy(NewBaseArray + dwBaseFiles, &pStream->Base, sizeof(union TBaseProviderData));
            pStream->FileBitmap = NewBaseArray;
            dwBaseFiles++;

            /* Get the size of the base stream */
            pStream->BaseGetSize((TFileStream_t *)pStream, &FileSize);
            assert(FileSize <= BLOCK4_MAX_FSIZE);
            RemainderBlock = FileSize % (BLOCK4_BLOCK_SIZE + BLOCK4_HASH_SIZE);
            BlockCount = FileSize / (BLOCK4_BLOCK_SIZE + BLOCK4_HASH_SIZE);
            
            /* Increment the stream size and number of blocks */
            pStream->StreamSize += (BlockCount * BLOCK4_BLOCK_SIZE);
            pStream->BlockCount += (uint32_t)BlockCount;

            /* Is this the last file? */
            if(FileSize < BLOCK4_MAX_FSIZE)
            {
                if(RemainderBlock)
                {
                    pStream->StreamSize += (RemainderBlock - BLOCK4_HASH_SIZE);
                    pStream->BlockCount++;
                }
                break;
            }
        }

        /* Fill the remainining block stream variables */
        pStream->BitmapSize = dwBaseFiles;
        pStream->BlockSize  = BLOCK4_BLOCK_SIZE;
        pStream->IsComplete = 1;
        pStream->IsModified = 0;

        /* Fill the remaining stream variables */
        pStream->StreamPos = 0;
        pStream->dwFlags |= STREAM_FLAG_READ_ONLY;

        STORM_FREE(szNameBuff);
    }

    /* If we opened something, return success */
    if(dwBaseFiles == 0)
    {
        FileStream_Close((TFileStream_t *)pStream);
        SetLastError(ERROR_FILE_NOT_FOUND);
        pStream = NULL;
    }

    return (TFileStream_t *)pStream;
}

/*-----------------------------------------------------------------------------
 * Public functions
 */

/**
 * This function creates a new file for read-write access
 *
 * - If the current platform supports file sharing,
 *   the file must be created for read sharing (i.e. another application
 *   can open the file for read, but not for write)
 * - If the file does not exist, the function must create new one
 * - If the file exists, the function must rewrite it and set to zero size
 * - The parameters of the function must be validate by the caller
 * - The function must initialize all stream function pointers in TFileStream_t
 * - If the function fails from any reason, it must close all handles
 *   and free all memory that has been allocated in the process of stream creation,
 *   including the TFileStream_t structure itself
 *
 * \a szFileName Name of the file to create
 */

TFileStream_t * FileStream_CreateFile(
    const char * szFileName,
    uint32_t dwStreamFlags)
{
    TFileStream_t * pStream;

    /* We only support creation of flat, local file */
    if((dwStreamFlags & (STREAM_PROVIDERS_MASK)) != (STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE))
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        return NULL;
    }

    /* Allocate file stream structure for flat stream */
    pStream = AllocateFileStream(szFileName, sizeof(TBlockStream_t), dwStreamFlags);
    if(pStream != NULL)
    {
        /* Attempt to create the disk file */
        if(BaseFile_Create(pStream))
        {
            /* Fill the stream provider functions */
            pStream->StreamRead    = pStream->BaseRead;
            pStream->StreamWrite   = pStream->BaseWrite;
            pStream->StreamResize  = pStream->BaseResize;
            pStream->StreamGetSize = pStream->BaseGetSize;
            pStream->StreamGetPos  = pStream->BaseGetPos;
            pStream->StreamClose   = pStream->BaseClose;
            return pStream;
        }

        /* File create failed, delete the stream */
        STORM_FREE(pStream);
        pStream = NULL;
    }

    /* Return the stream */
    return pStream;
}

/**
 * This function opens an existing file for read or read-write access
 * - If the current platform supports file sharing,
 *   the file must be open for read sharing (i.e. another application
 *   can open the file for read, but not for write)
 * - If the file does not exist, the function must return NULL
 * - If the file exists but cannot be open, then function must return NULL
 * - The parameters of the function must be validate by the caller
 * - The function must initialize all stream function pointers in TFileStream_t
 * - If the function fails from any reason, it must close all handles
 *   and free all memory that has been allocated in the process of stream creation,
 *   including the TFileStream_t structure itself
 *
 * \a szFileName Name of the file to open
 * \a dwStreamFlags specifies the provider and base storage type
 */

TFileStream_t * FileStream_OpenFile(
    const char * szFileName,
    uint32_t dwStreamFlags)
{
    uint32_t dwProvider = dwStreamFlags & STREAM_PROVIDERS_MASK;
    size_t nPrefixLength = FileStream_Prefix(szFileName, &dwProvider);

    /* Re-assemble the stream flags */
    dwStreamFlags = (dwStreamFlags & STREAM_OPTIONS_MASK) | dwProvider;
    szFileName += nPrefixLength;

    /* Perform provider-specific open */
    switch(dwStreamFlags & STREAM_PROVIDER_MASK)
    {
        case STREAM_PROVIDER_FLAT:
            return FlatStream_Open(szFileName, dwStreamFlags);

        case STREAM_PROVIDER_PARTIAL:
            return PartStream_Open(szFileName, dwStreamFlags);

        case STREAM_PROVIDER_MPQE:
            return MpqeStream_Open(szFileName, dwStreamFlags);

        case STREAM_PROVIDER_BLOCK4:
            return Block4Stream_Open(szFileName, dwStreamFlags);

        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
    }
}

/**
 * Returns the file name of the stream
 *
 * \a pStream Pointer to an open stream
 */
const char * FileStream_GetFileName(TFileStream_t * pStream)
{
    assert(pStream != NULL);
    return pStream->szFileName;
}

/**
 * Returns the length of the provider prefix. Returns zero if no prefix
 *
 * \a szFileName Pointer to a stream name (file, mapped file, URL)
 * \a pdwStreamProvider Pointer to a uint32_t variable that receives stream provider (STREAM_PROVIDER_XXX)
 */

size_t FileStream_Prefix(const char * szFileName, uint32_t * pdwProvider)
{
    size_t nPrefixLength1 = 0;
    size_t nPrefixLength2 = 0;
    uint32_t dwProvider = 0;

    if(szFileName != NULL)
    {
        /*
         * Determine the stream provider
         */

        if(!strncasecmp(szFileName, "flat-", 5))
        {
            dwProvider |= STREAM_PROVIDER_FLAT;
            nPrefixLength1 = 5;
        }

        else if(!strncasecmp(szFileName, "part-", 5))
        {
            dwProvider |= STREAM_PROVIDER_PARTIAL;
            nPrefixLength1 = 5;
        }

        else if(!strncasecmp(szFileName, "mpqe-", 5))
        {
            dwProvider |= STREAM_PROVIDER_MPQE;
            nPrefixLength1 = 5;
        }

        else if(!strncasecmp(szFileName, "blk4-", 5))
        {
            dwProvider |= STREAM_PROVIDER_BLOCK4;
            nPrefixLength1 = 5;
        }

        /*
         * Determine the base provider
         */

        if(!strncasecmp(szFileName+nPrefixLength1, "file:", 5))
        {
            dwProvider |= BASE_PROVIDER_FILE;
            nPrefixLength2 = 5;
        }

        else if(!strncasecmp(szFileName+nPrefixLength1, "map:", 4))
        {
            dwProvider |= BASE_PROVIDER_MAP;
            nPrefixLength2 = 4;
        }
        
        else if(!strncasecmp(szFileName+nPrefixLength1, "http:", 5))
        {
            dwProvider |= BASE_PROVIDER_HTTP;
            nPrefixLength2 = 5;
        }

        /* Only accept stream provider if we recognized the base provider */
        if(nPrefixLength2 != 0)
        {
            /* It is also allowed to put "//" after the base provider, e.g. "file://", "http://" */
            if(szFileName[nPrefixLength1+nPrefixLength2] == '/' && szFileName[nPrefixLength1+nPrefixLength2+1] == '/')
                nPrefixLength2 += 2;

            if(pdwProvider != NULL)
                *pdwProvider = dwProvider;
            return nPrefixLength1 + nPrefixLength2;
        }
    }

    return 0;
}

/**
 * Sets a download callback. Whenever the stream needs to download one or more blocks
 * from the server, the callback is called
 *
 * \a pStream Pointer to an open stream
 * \a pfnCallback Pointer to callback function
 * \a pvUserData Arbitrary user pointer passed to the download callback
 */

int FileStream_SetCallback(TFileStream_t * pStream, SFILE_DOWNLOAD_CALLBACK pfnCallback, void * pvUserData)
{
    TBlockStream_t * pBlockStream = (TBlockStream_t *)pStream;

    if(pStream->BlockRead == NULL)
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        return 0;
    }

    pBlockStream->pfnCallback = pfnCallback;
    pBlockStream->UserData = pvUserData;
    return 1;
}

/**
 * This function gives the block map. The 'pvBitmap' pointer must point to a buffer
 * of at least sizeof(STREAM_BLOCK_MAP) size. It can also have size of the complete
 * block map (i.e. sizeof(STREAM_BLOCK_MAP) + BitmapSize). In that case, the function
 * also copies the bit-based block map.
 *
 * \a pStream Pointer to an open stream
 * \a pvBitmap Pointer to buffer where the block map will be stored
 * \a cbBitmap Length of the buffer, of the block map
 * \a cbLengthNeeded Length of the bitmap, in bytes
 */

int FileStream_GetBitmap(TFileStream_t * pStream, void * pvBitmap, uint32_t cbBitmap, uint32_t * pcbLengthNeeded)
{
    TStreamBitmap * pBitmap = (TStreamBitmap *)pvBitmap;
    TBlockStream_t * pBlockStream = (TBlockStream_t *)pStream;
    uint64_t BlockOffset;
    unsigned char * Bitmap = (unsigned char *)(pBitmap + 1);
    uint32_t BitmapSize;
    uint32_t BlockCount;
    uint32_t BlockSize;
    int bResult = 0;

    /* Retrieve the size of one block */
    if(pStream->BlockCheck != NULL)
    {
        BlockCount = pBlockStream->BlockCount;
        BlockSize = pBlockStream->BlockSize;
    }
    else
    {
        BlockCount = (uint32_t)((pStream->StreamSize + DEFAULT_BLOCK_SIZE - 1) / DEFAULT_BLOCK_SIZE);
        BlockSize = DEFAULT_BLOCK_SIZE;
    }

    /* Fill-in the variables */
    BitmapSize = (BlockCount + 7) / 8;

    /* Give the number of blocks */
    if(pcbLengthNeeded != NULL)
        pcbLengthNeeded[0] = sizeof(TStreamBitmap) + BitmapSize;

    /* If the length of the buffer is not enough */
    if(pvBitmap != NULL && cbBitmap != 0)
    {
        /* Give the STREAM_BLOCK_MAP structure */
        if(cbBitmap >= sizeof(TStreamBitmap))
        {
            pBitmap->StreamSize = pStream->StreamSize;
            pBitmap->BitmapSize = BitmapSize;
            pBitmap->BlockCount = BlockCount;
            pBitmap->BlockSize  = BlockSize;
            pBitmap->IsComplete = (pStream->BlockCheck != NULL) ? pBlockStream->IsComplete : 1;
            bResult = 1;
        }

        /* Give the block bitmap, if enough space */
        if(cbBitmap >= sizeof(TStreamBitmap) + BitmapSize)
        {
            /* Version with bitmap present */
            if(pStream->BlockCheck != NULL)
            {
                uint32_t ByteIndex = 0;
                uint8_t BitMask = 0x01;

                /* Initialize the map with zeros */
                memset(Bitmap, 0, BitmapSize);

                /* Fill the map */
                for(BlockOffset = 0; BlockOffset < pStream->StreamSize; BlockOffset += BlockSize)
                {
                    /* Set the bit if the block is present */
                    if(pBlockStream->BlockCheck(pStream, BlockOffset))
                        Bitmap[ByteIndex] |= BitMask;

                    /* Move bit position */
                    ByteIndex += (BitMask >> 0x07);
                    BitMask = (BitMask >> 0x07) | (BitMask << 0x01);
                }
            }
            else
            {
                memset(Bitmap, 0xFF, BitmapSize);
            }
        }
    }

    /* Set last error value and return */
    if(!bResult)
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return bResult;
}

/**
 * Reads data from the stream
 *
 * - Returns 1 if the read operation succeeded and all bytes have been read
 * - Returns 0 if either read failed or not all bytes have been read
 * - If the pByteOffset is NULL, the function must read the data from the current file position
 * - The function can be called with dwBytesToRead = 0. In that case, pvBuffer is ignored
 *   and the function just adjusts file pointer.
 *
 * \a pStream Pointer to an open stream
 * \a pByteOffset Pointer to file byte offset. If NULL, it reads from the current position
 * \a pvBuffer Pointer to data to be read
 * \a dwBytesToRead Number of bytes to read from the file
 *
 * \returns
 * - If the function reads the required amount of bytes, it returns 1.
 * - If the function reads less than required bytes, it returns 0 and GetLastError() returns ERROR_HANDLE_EOF
 * - If the function fails, it reads 0 and GetLastError() returns an error code different from ERROR_HANDLE_EOF
 */
int FileStream_Read(TFileStream_t * pStream, uint64_t * pByteOffset, void * pvBuffer, uint32_t dwBytesToRead)
{
    assert(pStream->StreamRead != NULL);
    return pStream->StreamRead(pStream, pByteOffset, pvBuffer, dwBytesToRead);
}

/**
 * This function writes data to the stream
 *
 * - Returns 1 if the write operation succeeded and all bytes have been written
 * - Returns 0 if either write failed or not all bytes have been written
 * - If the pByteOffset is NULL, the function must write the data to the current file position
 *
 * \a pStream Pointer to an open stream
 * \a pByteOffset Pointer to file byte offset. If NULL, it reads from the current position
 * \a pvBuffer Pointer to data to be written
 * \a dwBytesToWrite Number of bytes to write to the file
 */
int FileStream_Write(TFileStream_t * pStream, uint64_t * pByteOffset, const void * pvBuffer, uint32_t dwBytesToWrite)
{
    if(pStream->dwFlags & STREAM_FLAG_READ_ONLY)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return 0;
    }

    assert(pStream->StreamWrite != NULL);
    return pStream->StreamWrite(pStream, pByteOffset, pvBuffer, dwBytesToWrite);
}

/**
 * Returns the size of a file
 *
 * \a pStream Pointer to an open stream
 * \a FileSize Pointer where to store the file size
 */
int FileStream_GetSize(TFileStream_t * pStream, uint64_t * pFileSize)
{
    assert(pStream->StreamGetSize != NULL);
    return pStream->StreamGetSize(pStream, pFileSize);
}

/**
 * Sets the size of a file
 *
 * \a pStream Pointer to an open stream
 * \a NewFileSize File size to set
 */
int FileStream_SetSize(TFileStream_t * pStream, uint64_t NewFileSize)
{                                 
    if(pStream->dwFlags & STREAM_FLAG_READ_ONLY)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return 0;
    }

    assert(pStream->StreamResize != NULL);
    return pStream->StreamResize(pStream, NewFileSize);
}

/**
 * This function returns the current file position
 * \a pStream
 * \a pByteOffset
 */
int FileStream_GetPos(TFileStream_t * pStream, uint64_t * pByteOffset)
{
    assert(pStream->StreamGetPos != NULL);
    return pStream->StreamGetPos(pStream, pByteOffset);
}

/**
 * Returns the last write time of a file
 *
 * \a pStream Pointer to an open stream
 * \a pFileType Pointer where to store the file last write time
 */
int FileStream_GetTime(TFileStream_t * pStream, uint64_t * pFileTime)
{
    /* Just use the saved filetime value */
    *pFileTime = pStream->Base.File.FileTime;
    return 1;
}

/**
 * Returns the stream flags
 *
 * \a pStream Pointer to an open stream
 * \a pdwStreamFlags Pointer where to store the stream flags
 */
int FileStream_GetFlags(TFileStream_t * pStream, uint32_t * pdwStreamFlags)
{
    *pdwStreamFlags = pStream->dwFlags;
    return 1;
}

/**
 * Switches a stream with another. Used for final phase of archive compacting.
 * Performs these steps:
 *
 * 1) Closes the handle to the existing MPQ
 * 2) Renames the temporary MPQ to the original MPQ, overwrites existing one
 * 3) Opens the MPQ stores the handle and stream position to the new stream structure
 *
 * \a pStream Pointer to an open stream
 * \a pNewStream Temporary ("working") stream (created during archive compacting)
 */
int FileStream_Replace(TFileStream_t * pStream, TFileStream_t * pNewStream)
{
    /* Only supported on flat files */
    if((pStream->dwFlags & STREAM_PROVIDERS_MASK) != (STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE))
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        return 0;
    }

    /* Not supported on read-only streams */
    if(pStream->dwFlags & STREAM_FLAG_READ_ONLY)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return 0;
    }

    /* Close both stream's base providers */
    pNewStream->BaseClose(pNewStream);
    pStream->BaseClose(pStream);

    /* Now we have to delete the (now closed) old file and rename the new file */
    if(!BaseFile_Replace(pStream, pNewStream))
        return 0;

    /* Now open the base file again */
    if(!BaseFile_Open(pStream, pStream->szFileName, pStream->dwFlags))
        return 0;

    /* Cleanup the new stream */
    FileStream_Close(pNewStream);
    return 1;
}

/**
 * This function closes an archive file and frees any data buffers
 * that have been allocated for stream management. The function must also
 * support partially allocated structure, i.e. one or more buffers
 * can be NULL, if there was an allocation failure during the process
 *
 * \a pStream Pointer to an open stream
 */
void FileStream_Close(TFileStream_t * pStream)
{
    /* Check if the stream structure is allocated at all */
    if(pStream != NULL)
    {
        /* Free the master stream, if any */
        if(pStream->pMaster != NULL)
            FileStream_Close(pStream->pMaster);
        pStream->pMaster = NULL;

        /* Close the stream provider. */
        if(pStream->StreamClose != NULL)
            pStream->StreamClose(pStream);
        
        /* Also close base stream, if any */
        else if(pStream->BaseClose != NULL)
            pStream->BaseClose(pStream);

        /* Free the stream itself */
        STORM_FREE(pStream);
    }
}

/*-----------------------------------------------------------------------------
 * Utility functions (ANSI)
 */

const char * GetPlainFileName(const char * szFileName)
{
    const char * szPlainName = szFileName;

    while(*szFileName != 0)
    {
        if(*szFileName == '\\' || *szFileName == '/')
            szPlainName = szFileName + 1;
        szFileName++;
    }

    return szPlainName;
}

void CopyFileName(char * szTarget, const char * szSource, size_t cchLength)
{
    memcpy(szTarget, szSource, cchLength);
    szTarget[cchLength] = 0;
}

/*-----------------------------------------------------------------------------
 * Utility functions (UNICODE) only exist in the ANSI version of the library
 * In ANSI builds, char = char, so we don't need these functions implemented
 */

#ifdef _UNICODE
const char * GetPlainFileName(const char * szFileName)
{
    const char * szPlainName = szFileName;

    while(*szFileName != 0)
    {
        if(*szFileName == '\\' || *szFileName == '/')
            szPlainName = szFileName + 1;
        szFileName++;
    }

    return szPlainName;
}

void CopyFileName(char * szTarget, const char * szSource, size_t cchLength)
{
    mbstowcs(szTarget, szSource, cchLength);
    szTarget[cchLength] = 0;
}

void CopyFileName(char * szTarget, const char * szSource, size_t cchLength)
{
    wcstombs(szTarget, szSource, cchLength);
    szTarget[cchLength] = 0;
}
#endif

/*-----------------------------------------------------------------------------
 * main - for testing purposes
 */

#ifdef __STORMLIB_TEST__
int FileStream_Test(const char * szFileName, uint32_t dwStreamFlags)
{
    TFileStream_t * pStream;
    TMPQHeader MpqHeader;
    uint64_t FilePos;
    TMPQBlock * pBlock;
    TMPQHash * pHash;

    InitializeMpqCryptography();

    pStream = FileStream_OpenFile(szFileName, dwStreamFlags);
    if(pStream == NULL)
        return GetLastError();

    /* Read the MPQ header */
    FileStream_Read(pStream, NULL, &MpqHeader, MPQ_HEADER_SIZE_V2);
    if(MpqHeader.dwID != ID_MPQ)
        return ERROR_FILE_CORRUPT;

    /* Read the hash table */
    pHash = STORM_ALLOC(TMPQHash, MpqHeader.dwHashTableSize);
    if(pHash != NULL)
    {
        FilePos = MpqHeader.dwHashTablePos;
        FileStream_Read(pStream, &FilePos, pHash, MpqHeader.dwHashTableSize * sizeof(TMPQHash));
        DecryptMpqBlock(pHash, MpqHeader.dwHashTableSize * sizeof(TMPQHash), MPQ_KEY_HASH_TABLE);
        STORM_FREE(pHash);
    }

    /* Read the block table */
    pBlock = STORM_ALLOC(TMPQBlock, MpqHeader.dwBlockTableSize);
    if(pBlock != NULL)
    {
        FilePos = MpqHeader.dwBlockTablePos;
        FileStream_Read(pStream, &FilePos, pBlock, MpqHeader.dwBlockTableSize * sizeof(TMPQBlock));
        DecryptMpqBlock(pBlock, MpqHeader.dwBlockTableSize * sizeof(TMPQBlock), MPQ_KEY_BLOCK_TABLE);
        STORM_FREE(pBlock);
    }

    FileStream_Close(pStream);
    return ERROR_SUCCESS;
}
#endif
