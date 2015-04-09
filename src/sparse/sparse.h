/*****************************************************************************/
/* sparse.h                               Copyright (c) Ladislav Zezula 2010 */
/*---------------------------------------------------------------------------*/
/* implementation of Sparse compression, used in Starcraft II                */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 05.03.10  1.00  Lad  The first version of sparse.h                        */
/*****************************************************************************/

#ifndef _SPARSE_H
#define _SPARSE_H

void CompressSparse(void * pvOutBuffer, int * pcbOutBuffer, void * pvInBuffer, int cbInBuffer);
int  DecompressSparse(void * pvOutBuffer, int * pcbOutBuffer, void * pvInBuffer, int cbInBuffer);

#endif /* _SPARSE_H */
