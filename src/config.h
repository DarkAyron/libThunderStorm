/*****************************************************************************/
/* config.h                           Copyright (c) Ayron 2015               */
/*---------------------------------------------------------------------------*/
/* Portability module for the StormLib library. Contains a wrapper symbols   */
/* to make the compilation under Linux work                                  */
/*                                                                           */
/* Author: Ayron <ayron@Shadowdrake.fur>                                     */
/* Created at: 04.03.2015                                                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 04.03.15  1.00  Ayr  Created                                              */
/*****************************************************************************/

#ifndef _CONFIG_H
#define _CONFIG_H

#define PLATFORM_LITTLE_ENDIAN
#define PLATFORM_LINUX

#if __LP64__
  #define PLATFORM_64BIT
#else
  #define PLATFORM_32BIT
#endif

#ifdef PLATFORM_32BIT
  #define _LZMA_UINT32_IS_ULONG
#endif

#ifndef MAX_PATH
  #define MAX_PATH 1024
#endif

/*
  #define FILE_BEGIN    SEEK_SET
  #define FILE_CURRENT  SEEK_CUR
  #define FILE_END      SEEK_END

  #define _T(x)     x
  #define _tcslen   strlen
  #define _tcscpy   strcpy
  #define _tcscat   strcat
  #define _tcschr   strchr
  #define _tcsrchr  strrchr
  #define _tcsstr   strstr
  #define _tprintf  printf
  #define _stprintf sprintf
  #define _tremove  remove

  #define _stricmp  strcasecmp
  #define _strnicmp strncasecmp
  #define _tcsicmp  strcasecmp
  #define _tcsnicmp strncasecmp

#endif // !PLATFORM_WINDOWS
*/
                                                


/*-----------------------------------------------------------------------------
 * Swapping functions
 */

#ifdef PLATFORM_LITTLE_ENDIAN
    #define    BSWAP_INT16_UNSIGNED(a)          (a)
    #define    BSWAP_INT16_SIGNED(a)            (a)
    #define    BSWAP_INT32_UNSIGNED(a)          (a)
    #define    BSWAP_INT32_SIGNED(a)            (a)
    #define    BSWAP_INT64_SIGNED(a)            (a)
    #define    BSWAP_INT64_UNSIGNED(a)          (a)
    #define    BSWAP_ARRAY16_UNSIGNED(a,b)      {}
    #define    BSWAP_ARRAY32_UNSIGNED(a,b)      {}
    #define    BSWAP_ARRAY64_UNSIGNED(a,b)      {}
    #define    BSWAP_PART_HEADER(a)             {}
    #define    BSWAP_TMPQHEADER(a,b)            {}
    #define    BSWAP_TMPKHEADER(a)              {}
#else
    int16_t  SwapInt16(uint16_t);
    uint16_t SwapUInt16(uint16_t);
    int32_t  SwapInt32(uint32_t);
    uint32_t SwapUInt32(uint32_t);
    int64_t  SwapInt64(uint64_t);
    uint64_t SwapUInt64(uint64_t);
    void ConvertUInt16Buffer(void * ptr, size_t length);
    void ConvertUInt32Buffer(void * ptr, size_t length);
    void ConvertUInt64Buffer(void * ptr, size_t length);
    void ConvertTMPQUserData(void *userData);
    void ConvertTMPQHeader(void *header, uint16_t wPart);
    void ConvertTMPKHeader(void *header);

    #define    BSWAP_INT16_SIGNED(a)            SwapInt16((a))
    #define    BSWAP_INT16_UNSIGNED(a)          SwapUInt16((a))
    #define    BSWAP_INT32_SIGNED(a)            SwapInt32((a))
    #define    BSWAP_INT32_UNSIGNED(a)          SwapUInt32((a))
    #define    BSWAP_INT64_SIGNED(a)            SwapInt64((a))
    #define    BSWAP_INT64_UNSIGNED(a)          SwapUInt64((a))
    #define    BSWAP_ARRAY16_UNSIGNED(a,b)      ConvertUInt16Buffer((a),(b))
    #define    BSWAP_ARRAY32_UNSIGNED(a,b)      ConvertUInt32Buffer((a),(b))
    #define    BSWAP_ARRAY64_UNSIGNED(a,b)      ConvertUInt64Buffer((a),(b))
    #define    BSWAP_TMPQHEADER(a,b)            ConvertTMPQHeader((a),(b))
    #define    BSWAP_TMPKHEADER(a)              ConvertTMPKHeader((a))
#endif

#endif /* _CONFIG_H */
