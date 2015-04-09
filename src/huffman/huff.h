/*****************************************************************************/
/* huffman.h                              Copyright (c) Ladislav Zezula 2003 */
/*---------------------------------------------------------------------------*/
/* Description :                                                             */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* xx.xx.xx  1.00  Lad  The first version of huffman.h                       */
/* 03.05.03  2.00  Lad  Added compression                                    */
/* 08.12.03  2.01  Dan  High-memory handling (> 0x80000000)                  */
/*****************************************************************************/
 
#ifndef _HUFFMAN_H
#define _HUFFMAN_H

#include <stdlib.h>

/*-----------------------------------------------------------------------------
 * Defines
 */
 
#define HUFF_ITEM_COUNT    0x203        /* Number of items in the item pool */
#define LINK_ITEM_COUNT    0x80         /* Maximum number of quick-link items */

/*-----------------------------------------------------------------------------
 * Structures and classes
 */

typedef struct
{
    unsigned char * pbBufferEnd;      /* End position in the the input buffer */
    unsigned char * pbBuffer;         /* Current position in the the input buffer */
    unsigned int BitBuffer;             /* Input bit buffer */
    unsigned int BitCount;              /* Number of bits remaining in 'dwBitBuff' */
} huff_Buffer;

/* Input stream for Huffmann decompression */

void huff_TInputStreamInitialise(huff_Buffer * huffBuffer, void * pvInBuffer, size_t cbInBuffer);

/* Output stream for Huffmann compression */
 
void huff_TOutputStreamInitialise(huff_Buffer * huffBuffer, void * pvOutBuffer, size_t cbOutLength);



enum TInsertPoint
{
    InsertAfter = 1,
    InsertBefore = 2
};

/* Huffmann tree item */
typedef struct THTreeItem
{
/*    THTreeItem()    { pPrev = pNext = NULL;} */
/*  ~THTreeItem()   { RemoveItem(); } */

/*    void         RemoveItem(); */
/*  void         RemoveEntry(); */
 
    struct THTreeItem  * pNext;                /* Pointer to lower-weight tree item */
    struct THTreeItem  * pPrev;                /* Pointer to higher-weight item */
    unsigned int  DecompressedValue;    /* 08 - Decompressed byte value (also index in the array) */
    unsigned int  Weight;               /* 0C - Weight */
    struct THTreeItem  * pParent;              /* 10 - Pointer to parent item (NULL if none) */
    struct THTreeItem  * pChildLo;             /* 14 - Pointer to the child with lower-weight child ("left child") */
} THTreeItem_t;

void huffTItem_RemoveItem(THTreeItem_t * treeItem);

/* Structure used for quick navigating in the huffmann tree. */
/* Allows skipping up to 7 bits in the compressed stream, thus */
/* decompressing a bit faster. Sometimes it can even get the decompressed */
/* byte directly. */
typedef struct 
{      
    unsigned int ValidValue;            /* If greater than THuffmannTree::MinValidValue, the entry is valid */
    unsigned int ValidBits;             /* Number of bits that are valid for this item link */
    union
    {
        struct THTreeItem  * pItem;            /* Pointer to the item within the Huffmann tree */
        unsigned int DecompressedValue; /* Value for direct decompression */
    };
} TQuickLink;
                                           

/* Structure for Huffman tree (Size 0x3674 bytes). Because I'm not expert */
/* for the decompression, I do not know actually if the class is really a Hufmann */
/* tree. If someone knows the decompression details, please let me know */
typedef struct
{
    THTreeItem_t   ItemBuffer[HUFF_ITEM_COUNT];   /* Buffer for tree items. No memory allocation is needed */
    unsigned int ItemsUsed;                     /* Number of tree items used from ItemBuffer */
 
    /* Head of the linear item list */
    THTreeItem_t * pFirst;                        /* Pointer to the highest weight item */
    THTreeItem_t * pLast;                         /* Pointer to the lowest weight item */

    THTreeItem_t * ItemsByByte[0x102];            /* Array of item pointers, one for each possible byte value */
    TQuickLink   QuickLinks[LINK_ITEM_COUNT];   /* Array of quick-link items */
    
    unsigned int MinValidValue;                 /* A minimum value of TQDecompress::ValidValue to be considered valid */
    unsigned int bIsCmp0;                       /* 1 if compression type 0 */
} THuffmannTree;

void huffTree_init(THuffmannTree * huffTree, int bCompression);
unsigned int huff_Compress(THuffmannTree * huffTree, huff_Buffer * os, void * pvInBuffer, int cbInBuffer, int nCmpType);
unsigned int huff_Decompress(THuffmannTree * huffTree, void * pvOutBuffer, unsigned int cbOutLength, huff_Buffer * is);
 
#endif /* _HUFFMAN_H */
