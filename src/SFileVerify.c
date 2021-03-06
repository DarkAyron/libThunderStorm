/*****************************************************************************/
/* SFileVerify.c                          Copyright (c) Ladislav Zezula 2010 */
/*---------------------------------------------------------------------------*/
/* MPQ files and MPQ archives verification.                                  */
/*                                                                           */
/* The MPQ signature verification has been written by Jean-Francois Roy      */
/* <bahamut@macstorm.org> and Justin Olbrantz (Quantam).                     */
/* The MPQ public keys have been created by MPQKit, using OpenSSL library.   */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 04.05.10  1.00  Lad  The first version of SFileVerify.cpp                 */
/* 02.05.15  1.00  Ayr  Ported to plain c                                    */
/*****************************************************************************/

#include "thunderStorm.h"
#include "StormCommon.h"

/*-----------------------------------------------------------------------------
 * Local defines
 */

#define MPQ_DIGEST_UNIT_SIZE      0x10000

/*-----------------------------------------------------------------------------
 * Known Blizzard public keys
 * Created by Jean-Francois Roy using OpenSSL
 */

static const char * szBlizzardWeakPrivateKey =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIBOQIBAAJBAJJidwS/uILMBSO5DLGsBFknIXWWjQJe2kfdfEk3G/j66w4KkhZ1"
    "V61Rt4zLaMVCYpDun7FLwRjkMDSepO1q2DcCAwEAAQJANtiztVDMJh2hE1hjPDKy"
    "UmEJ9U/aN3gomuKOjbQbQ/bWWcM/WfhSVHmPqtqh/bQI2UXFr0rnXngeteZHLr/b"
    "8QIhAMuWriSKGMACw18/rVVfUrThs915odKBH1Alr3vMVVzZAiEAuBHPSQkgwcb6"
    "L4MWaiKuOzq08mSyNqPeN8oSy18q848CIHeMn+3s+eOmu7su1UYQl6yH7OrdBd1q"
    "3UxfFNEJiAbhAiAqxdCyOxHGlbM7aS3DOg3cq5ayoN2cvtV7h1R4t8OmVwIgF+5z"
    "/6vkzBUsZhd8Nwyis+MeQYH0rpFpMKdTlqmPF2Q="
    "-----END RSA PRIVATE KEY-----";

static const char * szBlizzardWeakPublicKey =
    "-----BEGIN PUBLIC KEY-----\n"
    "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAJJidwS/uILMBSO5DLGsBFknIXWWjQJe"
    "2kfdfEk3G/j66w4KkhZ1V61Rt4zLaMVCYpDun7FLwRjkMDSepO1q2DcCAwEAAQ=="
    "-----END PUBLIC KEY-----";

static const char * szBlizzardStrongPublicKey =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsQZ+ziT2h8h+J/iMQpgd"
    "tH1HaJzOBE3agjU4yMPcrixaPOZoA4t8bwfey7qczfWywocYo3pleytFF+IuD4HD"
    "Fl9OXN1SFyupSgMx1EGZlgbFAomnbq9MQJyMqQtMhRAjFgg4TndS7YNb+JMSAEKp"
    "kXNqY28n/EVBHD5TsMuVCL579gIenbr61dI92DDEdy790IzIG0VKWLh/KOTcTJfm"
    "Ds/7HQTkGouVW+WUsfekuqNQo7ND9DBnhLjLjptxeFE2AZqYcA1ao3S9LN3GL1tW"
    "lVXFIX9c7fWqaVTQlZ2oNsI/ARVApOK3grNgqvwH6YoVYVXjNJEo5sQJsPsdV/hk"
    "dwIDAQAB"
    "-----END PUBLIC KEY-----";

static const char * szWarcraft3MapPublicKey =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1BwklUUQ3UvjizOBRoF5"
    "yyOVc7KD+oGOQH5i6eUk1yfs0luCC70kNucNrfqhmviywVtahRse1JtXCPrx2bd3"
    "iN8Dx91fbkxjYIOGTsjYoHKTp0BbaFkJih776fcHgnFSb+7mJcDuJVvJOXxEH6w0"
    "1vo6VtujCqj1arqbyoal+xtAaczF3us5cOEp45sR1zAWTn1+7omN7VWV4QqJPaDS"
    "gBSESc0l1grO0i1VUSumayk7yBKIkb+LBvcG6WnYZHCi7VdLmaxER5m8oZfER66b"
    "heHoiSQIZf9PAY6Guw2DT5BTc54j/AaLQAKf2qcRSgQLVo5kQaddF3rCpsXoB/74"
    "6QIDAQAB"
    "-----END PUBLIC KEY-----";

static const char * szWowPatchPublicKey =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwOsMV0LagAWPEtEQM6b9"
    "6FHFkUyGbbyda2/Dfc9dyl21E9QvX+Yw7qKRMAKPzA2TlQQLZKvXpnKXF/YIK5xa"
    "5uwg9CEHCEAYolLG4xn0FUOE0E/0PuuytI0p0ICe6rk00PifZzTr8na2wI/l/GnQ"
    "bvnIVF1ck6cslATpQJ5JJVMXzoFlUABS19WESw4MXuJAS3AbMhxNWdEhVv7eO51c"
    "yGjRLy9QjogZODZTY0fSEksgBqQxNCoYVJYI/sF5K2flDsGqrIp0OdJ6teJlzg1Y"
    "UjYnb6bKjlidXoHEXI2TgA/mD6O3XFIt08I9s3crOCTgICq7cgX35qrZiIVWZdRv"
    "TwIDAQAB"
    "-----END PUBLIC KEY-----";

static const char * szWowSurveyPublicKey =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAnIt1DR6nRyyKsy2qahHe"
    "MKLtacatn/KxieHcwH87wLBxKy+jZ0gycTmJ7SaTdBAEMDs/V5IPIXEtoqYnid2c"
    "63TmfGDU92oc3Ph1PWUZ2PWxBhT06HYxRdbrgHw9/I29pNPi/607x+lzPORITOgU"
    "BR6MR8au8HsQP4bn4vkJNgnSgojh48/XQOB/cAln7As1neP61NmVimoLR4Bwi3zt"
    "zfgrZaUpyeNCUrOYJmH09YIjbBySTtXOUidoPHjFrMsCWpr6xs8xbETbs7MJFL6a"
    "vcUfTT67qfIZ9RsuKfnXJTIrV0kwDSjjuNXiPTmWAehSsiHIsrUXX5RNcwsSjClr"
    "nQIDAQAB"
    "-----END PUBLIC KEY-----";

static const char * szStarcraft2MapPublicKey =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAmk4GT8zb+ICC25a17KZB"
    "q/ygKGJ2VSO6IT5PGHJlm1KfnHBA4B6SH3xMlJ4c6eG2k7QevZv+FOhjsAHubyWq"
    "2VKqWbrIFKv2ILc2RfMn8J9EDVRxvcxh6slRrVL69D0w1tfVGjMiKq2Fym5yGoRT"
    "E7CRgDqbAbXP9LBsCNWHiJLwfxMGzHbk8pIl9oia5pvM7ofZamSHchxlpy6xa4GJ"
    "7xKN01YCNvklTL1D7uol3wkwcHc7vrF8QwuJizuA5bSg4poEGtH62BZOYi+UL/z0"
    "31YK+k9CbQyM0X0pJoJoYz1TK+Y5J7vBnXCZtfcTYQ/ZzN6UcxTa57dJaiOlCh9z"
    "nQIDAQAB"
    "-----END PUBLIC KEY-----";

/*-----------------------------------------------------------------------------
 * Local functions
 */

static void memrev(unsigned char *buf, size_t count)
{
    unsigned char *r;

    for (r = buf + count - 1; buf < r; buf++, r--)
    {
        *buf ^= *r;
        *r   ^= *buf;
        *buf ^= *r;
    }
}

static int decode_base64_key(const char * szKeyBase64, rsa_key * key)
{
    unsigned char decoded_key[0xA00];
    const char * szSecHead = "-----BEGIN RSA PRIVATE KEY-----\n";
    const char * szSecTail = "-----END RSA PRIVATE KEY-----";
    const char * szPubHead = "-----BEGIN PUBLIC KEY-----\n";
    const char * szPubTail = "-----END PUBLIC KEY-----";
    const char * szBase64Begin;
    const char * szBase64End;
    unsigned long decoded_length = sizeof(decoded_key);
    unsigned long length;

    /* Find out the begin of the BASE64 data */
    if(!memcmp(szKeyBase64, szSecHead, strlen(szSecHead)))
    {
        /* Secret key */
        szBase64Begin = strstr(szKeyBase64, szSecHead) + strlen(szSecHead);
        szBase64End = strstr(szKeyBase64, szSecTail);
        if((szBase64End == NULL) || (szBase64Begin == NULL))
            return 0;
    }
    else if(!memcmp(szKeyBase64, szPubHead, strlen(szPubHead)))
    {
        /* Public key */
        szBase64Begin = strstr(szKeyBase64, szPubHead) + strlen(szPubHead);
        szBase64End = strstr(szKeyBase64, szPubTail);
        if((szBase64End == NULL) || (szBase64Begin == NULL))
            return 0;
    }
    else
        return 0;

    /* decode the base64 string */
    length = (unsigned long)(szBase64End - szBase64Begin);
    if(base64_decode((unsigned char *)szBase64Begin, length, decoded_key, &decoded_length) != CRYPT_OK)
        return 0;

    /* Create RSA key */
    if(rsa_import(decoded_key, decoded_length, key) != CRYPT_OK)
        return 0;

    return 1;
}

static void GetPlainAnsiFileName(
    const char * szFileName,
    char * szPlainName)
{
    const char * szPlainNameT = GetPlainFileName(szFileName);

    /* Convert the plain name to ANSI */
    while(*szPlainNameT != 0)
        *szPlainName++ = (char)*szPlainNameT++;
    *szPlainName = 0;
}

/* Calculate begin and end of the MPQ archive */
static void CalculateArchiveRange(
    TMPQArchive * ha,
    PMPQ_SIGNATURE_INFO pSI)
{
    uint64_t TempPos = 0;
    char szMapHeader[0x200];

    /* Get the MPQ begin */
    pSI->BeginMpqData = ha->MpqPos;

    /* Warcraft III maps are signed from the map header to the end */
    if(FileStream_Read(ha->pStream, &TempPos, szMapHeader, sizeof(szMapHeader)))
    {
        /* Is it a map header ? */
        if(szMapHeader[0] == 'H' && szMapHeader[1] == 'M' && szMapHeader[2] == '3' && szMapHeader[3] == 'W')
        {
            /* We will have to hash since the map header */
            pSI->BeginMpqData = 0;
        }
    }

    /* Get the MPQ data end. This is stored in the MPQ header */
    pSI->EndMpqData = ha->MpqPos + ha->pHeader->ArchiveSize64;

    /* Get the size of the entire file */
    FileStream_GetSize(ha->pStream, &pSI->EndOfFile);
}

static int CalculateMpqHashMd5(
    TMPQArchive * ha,
    PMPQ_SIGNATURE_INFO pSI,
    unsigned char * pMd5Digest)
{
    hash_state md5_state;
    uint64_t BeginBuffer;
    uint64_t EndBuffer;
    unsigned char * pbDigestBuffer = NULL;

    /* Allocate buffer for creating the MPQ digest. */
    pbDigestBuffer = STORM_ALLOC(uint8_t, MPQ_DIGEST_UNIT_SIZE);
    if(pbDigestBuffer == NULL)
        return 0;

    /* Initialize the MD5 hash state */
    md5_init(&md5_state);

    /* Set the byte offset of begin of the data */
    BeginBuffer = pSI->BeginMpqData;

    /* Create the digest */
    for(;;)
    {
        uint64_t BytesRemaining;
        unsigned char * pbSigBegin = NULL;
        unsigned char * pbSigEnd = NULL;
        uint32_t dwToRead = MPQ_DIGEST_UNIT_SIZE;

        /* Check the number of bytes remaining */
        BytesRemaining = pSI->EndMpqData - BeginBuffer;
        if(BytesRemaining < MPQ_DIGEST_UNIT_SIZE)
            dwToRead = (uint32_t)BytesRemaining;
        if(dwToRead == 0)
            break;

        /* Read the next chunk */
        if(!FileStream_Read(ha->pStream, &BeginBuffer, pbDigestBuffer, dwToRead))
        {
            STORM_FREE(pbDigestBuffer);
            return 0;
        }

        /* Move the current byte offset */
        EndBuffer = BeginBuffer + dwToRead;

        /* Check if the signature is within the loaded digest */
        if(BeginBuffer <= pSI->BeginExclude && pSI->BeginExclude < EndBuffer)
            pbSigBegin = pbDigestBuffer + (size_t)(pSI->BeginExclude - BeginBuffer);
        if(BeginBuffer <= pSI->EndExclude && pSI->EndExclude < EndBuffer)
            pbSigEnd = pbDigestBuffer + (size_t)(pSI->EndExclude - BeginBuffer);

        /* Zero the part that belongs to the signature */
        if(pbSigBegin != NULL || pbSigEnd != NULL)
        {
            if(pbSigBegin == NULL)
                pbSigBegin = pbDigestBuffer;
            if(pbSigEnd == NULL)
                pbSigEnd = pbDigestBuffer + dwToRead;

            memset(pbSigBegin, 0, (pbSigEnd - pbSigBegin));
        }

        /* Pass the buffer to the hashing function */
        md5_process(&md5_state, pbDigestBuffer, dwToRead);

        /* Move pointers */
        BeginBuffer += dwToRead;
    }

    /* Finalize the MD5 hash */
    md5_done(&md5_state, pMd5Digest);
    STORM_FREE(pbDigestBuffer);
    return 1;
}

static int CalculateMpqHashSha1(
    TMPQArchive * ha,
    PMPQ_SIGNATURE_INFO pSI,
    unsigned char * pSha1Digest)
{
    hash_state sha1_state;
    uint64_t BeginBuffer;
    uint64_t EndBuffer;
    unsigned char * pbDigestBuffer = NULL;

    /* Allocate buffer for creating the MPQ digest. */
    pbDigestBuffer = STORM_ALLOC(uint8_t, MPQ_DIGEST_UNIT_SIZE);
    if(pbDigestBuffer == NULL)
        return 0;

    /* Initialize the SHA1 hash state */
    sha1_init(&sha1_state);

    /* Set the byte offset of begin of the data */
    BeginBuffer = pSI->BeginMpqData;

    /* Create the digest */
    for(;;)
    {
        uint64_t BytesRemaining;
        unsigned char * pbSigBegin = NULL;
        unsigned char * pbSigEnd = NULL;
        uint32_t dwToRead = MPQ_DIGEST_UNIT_SIZE;

        /* Check the number of bytes remaining */
        BytesRemaining = pSI->EndMpqData - BeginBuffer;
        if(BytesRemaining < MPQ_DIGEST_UNIT_SIZE)
            dwToRead = (uint32_t)BytesRemaining;
        if(dwToRead == 0)
            break;

        /* Read the next chunk */
        if(!FileStream_Read(ha->pStream, &BeginBuffer, pbDigestBuffer, dwToRead))
        {
            STORM_FREE(pbDigestBuffer);
            return 0;
        }

        /* Move the current byte offset */
        EndBuffer = BeginBuffer + dwToRead;

        /* Check if the signature is within the loaded digest */
        if(BeginBuffer <= pSI->BeginExclude && pSI->BeginExclude < EndBuffer)
            pbSigBegin = pbDigestBuffer + (size_t)(pSI->BeginExclude - BeginBuffer);
        if(BeginBuffer <= pSI->EndExclude && pSI->EndExclude < EndBuffer)
            pbSigEnd = pbDigestBuffer + (size_t)(pSI->EndExclude - BeginBuffer);

        /* Zero the part that belongs to the signature */
        if(pbSigBegin != NULL || pbSigEnd != NULL)
        {
            if(pbSigBegin == NULL)
                pbSigBegin = pbDigestBuffer;
            if(pbSigEnd == NULL)
                pbSigEnd = pbDigestBuffer + dwToRead;

            memset(pbSigBegin, 0, (pbSigEnd - pbSigBegin));
        }

        /* Pass the buffer to the hashing function */
        sha1_process(&sha1_state, pbDigestBuffer, dwToRead);

        /* Move pointers */
        BeginBuffer += dwToRead;
    }

    /* Finalize the SHA1 hash */
    sha1_done(&sha1_state, pSha1Digest);
    STORM_FREE(pbDigestBuffer);
    return 1;
}

static void AddTailToSha1(
    hash_state * psha1_state,
    const char * szTail)
{
    unsigned char * pbTail = (unsigned char *)szTail;
    unsigned char szUpperCase[0x200];
    unsigned long nLength = 0;

    /* Convert the tail to uppercase */
    /* Note that we don't need to terminate the string with zero */
    while(*pbTail != 0)
    {
        szUpperCase[nLength++] = AsciiToUpperTable[*pbTail++];
    }

    /* Append the tail to the SHA1 */
    sha1_process(psha1_state, szUpperCase, nLength);
}

static int CalculateMpqHashSha1WithTail(
    TMPQArchive * ha,
    PMPQ_SIGNATURE_INFO pSI,
    unsigned char * sha1_tail0,
    unsigned char * sha1_tail1,
    unsigned char * sha1_tail2)
{
    uint64_t BeginBuffer;
    hash_state sha1_state_temp;
    hash_state sha1_state;
    unsigned char * pbDigestBuffer = NULL;
    char szPlainName[MAX_PATH];

    /* Allocate buffer for creating the MPQ digest. */
    pbDigestBuffer = STORM_ALLOC(uint8_t, MPQ_DIGEST_UNIT_SIZE);
    if(pbDigestBuffer == NULL)
        return 0;

    /* Initialize SHA1 state structure */
    sha1_init(&sha1_state);

    /* Calculate begin of data to be hashed */
    BeginBuffer = pSI->BeginMpqData;

    /* Create the digest */
    for(;;)
    {
        uint64_t BytesRemaining;
        uint32_t dwToRead = MPQ_DIGEST_UNIT_SIZE;

        /* Check the number of bytes remaining */
        BytesRemaining = pSI->EndMpqData - BeginBuffer;
        if(BytesRemaining < MPQ_DIGEST_UNIT_SIZE)
            dwToRead = (uint32_t)BytesRemaining;
        if(dwToRead == 0)
            break;

        /* Read the next chunk */
        if(!FileStream_Read(ha->pStream, &BeginBuffer, pbDigestBuffer, dwToRead))
        {
            STORM_FREE(pbDigestBuffer);
            return 0;
        }

        /* Pass the buffer to the hashing function */
        sha1_process(&sha1_state, pbDigestBuffer, dwToRead);

        /* Move pointers */
        BeginBuffer += dwToRead;
    }

    /* Add all three known tails and generate three hashes */
    memcpy(&sha1_state_temp, &sha1_state, sizeof(hash_state));
    sha1_done(&sha1_state_temp, sha1_tail0);

    memcpy(&sha1_state_temp, &sha1_state, sizeof(hash_state));
    GetPlainAnsiFileName(FileStream_GetFileName(ha->pStream), szPlainName);
    AddTailToSha1(&sha1_state_temp, szPlainName);
    sha1_done(&sha1_state_temp, sha1_tail1);

    memcpy(&sha1_state_temp, &sha1_state, sizeof(hash_state));
    AddTailToSha1(&sha1_state_temp, "ARCHIVE");
    sha1_done(&sha1_state_temp, sha1_tail2);

    /* Finalize the SHA1 hash */
    STORM_FREE(pbDigestBuffer);
    return 1;
}

static int VerifyRawMpqData(
    TMPQArchive * ha,
    uint64_t ByteOffset,
    uint32_t dwDataSize)
{
    uint64_t DataOffset = ha->MpqPos + ByteOffset;
    unsigned char * pbDataChunk;
    unsigned char * pbMD5Array1;                 /* Calculated MD5 array */
    unsigned char * pbMD5Array2;                 /* MD5 array loaded from the MPQ */
    uint32_t dwBytesInChunk;
    uint32_t dwChunkCount;
    uint32_t dwChunkSize = ha->pHeader->dwRawChunkSize;
    uint32_t dwMD5Size;
    int nError = ERROR_SUCCESS;

    /* Don't verify zero-sized blocks */
    if(dwDataSize == 0)
        return ERROR_SUCCESS;

    /* Get the number of data chunks to calculate MD5 */
    assert(dwChunkSize != 0);
    dwChunkCount = ((dwDataSize - 1) / dwChunkSize) + 1;
    dwMD5Size = dwChunkCount * MD5_DIGEST_SIZE;

    /* Allocate space for data chunk and for the MD5 array */
    pbDataChunk = STORM_ALLOC(uint8_t, dwChunkSize);
    if(pbDataChunk == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Allocate space for MD5 array */
    pbMD5Array1 = STORM_ALLOC(uint8_t, dwMD5Size);
    pbMD5Array2 = STORM_ALLOC(uint8_t, dwMD5Size);
    if(pbMD5Array1 == NULL || pbMD5Array2 == NULL)
        nError = ERROR_NOT_ENOUGH_MEMORY;

    /* Calculate MD5 of each data chunk */
    if(nError == ERROR_SUCCESS)
    {
        uint32_t i;
        unsigned char * pbMD5 = pbMD5Array1;

        for(i = 0; i < dwChunkCount; i++)
        {
            /* Get the number of bytes in the chunk */
            dwBytesInChunk = STORMLIB_MIN(dwChunkSize, dwDataSize);

            /* Read the data chunk */
            if(!FileStream_Read(ha->pStream, &DataOffset, pbDataChunk, dwBytesInChunk))
            {
                nError = ERROR_FILE_CORRUPT;
                break;
            }

            /* Calculate MD5 */
            CalculateDataBlockHash(pbDataChunk, dwBytesInChunk, pbMD5);

            /* Move pointers and offsets */
            DataOffset += dwBytesInChunk;
            dwDataSize -= dwBytesInChunk;
            pbMD5 += MD5_DIGEST_SIZE;
        }
    }

    /* Read the MD5 array */
    if(nError == ERROR_SUCCESS)
    {
        /* Read the array of MD5 */
        if(!FileStream_Read(ha->pStream, &DataOffset, pbMD5Array2, dwMD5Size))
            nError = GetLastError();
    }

    /* Compare the array of MD5 */
    if(nError == ERROR_SUCCESS)
    {
        /* Compare the MD5 */
        if(memcmp(pbMD5Array1, pbMD5Array2, dwMD5Size))
            nError = ERROR_FILE_CORRUPT;
    }

    /* Free memory and return result */
    if(pbMD5Array2 != NULL)
        STORM_FREE(pbMD5Array2);
    if(pbMD5Array1 != NULL)
        STORM_FREE(pbMD5Array1);
    if(pbDataChunk != NULL)
        STORM_FREE(pbDataChunk);
    return nError;
}

static uint32_t VerifyWeakSignature(
    TMPQArchive * ha,
    PMPQ_SIGNATURE_INFO pSI)
{
    uint8_t RevSignature[MPQ_WEAK_SIGNATURE_SIZE];
    uint8_t Md5Digest[MD5_DIGEST_SIZE];
    rsa_key * key;
    rsa_key blizzKey;
    int hash_idx = find_hash("md5");
    int result = 0;

    /* The signature might be zeroed out. In that case, we ignore it */
    if(!IsValidSignature(pSI->Signature, pSI->cbSignatureSize))
        return ERROR_WEAK_SIGNATURE_OK;

    /* Calculate hash of the entire archive, skipping the (signature) file */
    if(!CalculateMpqHashMd5(ha, pSI, Md5Digest))
        return ERROR_VERIFY_FAILED;

    key = &(ha->keyRSA);
    
    /* Import the Blizzard key in OpenSSL format if needed */
    if(key->N == NULL)
    {
        if(!decode_base64_key(szBlizzardWeakPublicKey, &blizzKey))
            return ERROR_VERIFY_FAILED;
        else
            key = &blizzKey;
    }

    /* Verify the signature */
    memcpy(RevSignature, &pSI->Signature[8], MPQ_WEAK_SIGNATURE_SIZE);
    memrev(RevSignature, MPQ_WEAK_SIGNATURE_SIZE);
    rsa_verify_hash_ex(RevSignature, MPQ_WEAK_SIGNATURE_SIZE, Md5Digest, sizeof(Md5Digest), LTC_LTC_PKCS_1_V1_5, hash_idx, 0, &result, key);
    if(key == &blizzKey)
        rsa_free(key);

    /* Return the result */
    return result ? ERROR_WEAK_SIGNATURE_OK : ERROR_WEAK_SIGNATURE_ERROR;
}

static uint32_t VerifySecureSignature(
    TMPQArchive * ha,
    PMPQ_SIGNATURE_INFO pSI)
{
    uint8_t RevSignature[MPQ_SECURE_SIGNATURE_SIZE];
    uint8_t Sha1Digest[SHA1_DIGEST_SIZE];
    rsa_key * key = &(ha->keyRSA);
    int hash_idx = find_hash("sha1");
    int result = 0;

    /* The signature might be zeroed out. In that case, we ignore it */
    if(!IsValidSignature(pSI->Signature, pSI->cbSignatureSize))
        return ERROR_SECURE_SIGNATURE_OK;

    /* If we have no key, we can't verify */
    if(key->N == NULL)
        return ENOKEY;

    /* Calculate hash of the entire archive, skipping the (signature) file */
    if(!CalculateMpqHashSha1(ha, pSI, Sha1Digest))
        return ERROR_VERIFY_FAILED;

    /* Verify the signature */
    memcpy(RevSignature, &pSI->Signature[8], pSI->cbSignatureSize);
    memrev(RevSignature, pSI->cbSignatureSize);
    rsa_verify_hash_ex(RevSignature, pSI->cbSignatureSize, Sha1Digest, sizeof(Sha1Digest), LTC_LTC_PKCS_1_V1_5, hash_idx, 0, &result, key);

    /* Return the result */
    return result ? ERROR_SECURE_SIGNATURE_OK : ERROR_SECURE_SIGNATURE_ERROR;
}

static uint32_t VerifyStrongSignatureWithKey(
    unsigned char * reversed_signature,
    unsigned char * padded_digest,
    const char * szPublicKey)
{
    rsa_key key;
    int result = 0;

    /* Import the Blizzard key in OpenSSL format */
    if(!decode_base64_key(szPublicKey, &key))
    {
        assert(0);
        return ERROR_VERIFY_FAILED;
    }

    /* Verify the signature */
    if(rsa_verify_simple(reversed_signature, MPQ_STRONG_SIGNATURE_SIZE, padded_digest, MPQ_STRONG_SIGNATURE_SIZE, &result, &key) != CRYPT_OK)
        return ERROR_VERIFY_FAILED;
    
    /* Free the key and return result */
    rsa_free(&key);
    return result ? ERROR_STRONG_SIGNATURE_OK : ERROR_STRONG_SIGNATURE_ERROR;
}

static uint32_t VerifyStrongSignature(
    TMPQArchive * ha,
    PMPQ_SIGNATURE_INFO pSI)
{
    unsigned char reversed_signature[MPQ_STRONG_SIGNATURE_SIZE];
    unsigned char Sha1Digest_tail0[SHA1_DIGEST_SIZE];
    unsigned char Sha1Digest_tail1[SHA1_DIGEST_SIZE];
    unsigned char Sha1Digest_tail2[SHA1_DIGEST_SIZE];
    unsigned char padded_digest[MPQ_STRONG_SIGNATURE_SIZE];
    uint32_t dwResult;
    size_t digest_offset;

    /* Calculate SHA1 hash of the archive */
    if(!CalculateMpqHashSha1WithTail(ha, pSI, Sha1Digest_tail0, Sha1Digest_tail1, Sha1Digest_tail2))
        return ERROR_VERIFY_FAILED;

    /* Prepare the signature for decryption */
    memcpy(reversed_signature, &pSI->Signature[4], MPQ_STRONG_SIGNATURE_SIZE);
    memrev(reversed_signature, MPQ_STRONG_SIGNATURE_SIZE);

    /* Prepare the padded digest for comparison */
    digest_offset = sizeof(padded_digest) - SHA1_DIGEST_SIZE;
    memset(padded_digest, 0xbb, digest_offset);
    padded_digest[0] = 0x0b;

    /* Try Blizzard Strong public key with no SHA1 tail */
    memcpy(padded_digest + digest_offset, Sha1Digest_tail0, SHA1_DIGEST_SIZE);
    memrev(padded_digest + digest_offset, SHA1_DIGEST_SIZE);
    dwResult = VerifyStrongSignatureWithKey(reversed_signature, padded_digest, szBlizzardStrongPublicKey);
    if(dwResult == ERROR_STRONG_SIGNATURE_OK)
        return dwResult;

    /* Try War 3 map public key with plain file name as SHA1 tail */
    memcpy(padded_digest + digest_offset, Sha1Digest_tail1, SHA1_DIGEST_SIZE);
    memrev(padded_digest + digest_offset, SHA1_DIGEST_SIZE);
    dwResult = VerifyStrongSignatureWithKey(reversed_signature, padded_digest, szWarcraft3MapPublicKey);
    if(dwResult == ERROR_STRONG_SIGNATURE_OK)
        return dwResult;

    /* Try WoW-TBC public key with "ARCHIVE" as SHA1 tail */
    memcpy(padded_digest + digest_offset, Sha1Digest_tail2, SHA1_DIGEST_SIZE);
    memrev(padded_digest + digest_offset, SHA1_DIGEST_SIZE);
    dwResult = VerifyStrongSignatureWithKey(reversed_signature, padded_digest, szWowPatchPublicKey);
    if(dwResult == ERROR_STRONG_SIGNATURE_OK)
        return dwResult;

    /* Try Survey public key with no SHA1 tail */
    memcpy(padded_digest + digest_offset, Sha1Digest_tail0, SHA1_DIGEST_SIZE);
    memrev(padded_digest + digest_offset, SHA1_DIGEST_SIZE);
    dwResult = VerifyStrongSignatureWithKey(reversed_signature, padded_digest, szWowSurveyPublicKey);
    if(dwResult == ERROR_STRONG_SIGNATURE_OK)
        return dwResult;

    /* Try Starcraft II public key with no SHA1 tail */
    memcpy(padded_digest + digest_offset, Sha1Digest_tail0, SHA1_DIGEST_SIZE);
    memrev(padded_digest + digest_offset, SHA1_DIGEST_SIZE);
    dwResult = VerifyStrongSignatureWithKey(reversed_signature, padded_digest, szStarcraft2MapPublicKey);
    if(dwResult == ERROR_STRONG_SIGNATURE_OK)
        return dwResult;

    return ERROR_STRONG_SIGNATURE_ERROR;
}

static uint32_t VerifyFile(
    void * hMpq,
    const char * szFileName,
    uint32_t * pdwCrc32,
    char * pMD5,
    uint32_t dwFlags)
{
    hash_state md5_state;
    unsigned char * pFileMd5;
    unsigned char md5[MD5_DIGEST_SIZE];
    TFileEntry * pFileEntry;
    TMPQFile * hf;
    uint8_t Buffer[0x1000];
    void * hFile = NULL;
    uint32_t dwVerifyResult = 0;
    uint32_t dwTotalBytes = 0;
    uint32_t dwCrc32 = 0;

    /*
     * Note: When the MPQ is patched, it will
     * automatically check the patched version of the file
     */

    /* Make sure the md5 is initialized */
    memset(md5, 0, sizeof(md5));

    /* If we have to verify raw data MD5, do it before file open */
    if(dwFlags & SFILE_VERIFY_RAW_MD5)
    {
        TMPQArchive * ha = (TMPQArchive *)hMpq;

        /* Parse the base MPQ and all patches */
        while(ha != NULL)
        {
            /* Does the archive have support for raw MD5? */
            if(ha->pHeader->dwRawChunkSize != 0)
            {
                /* The file has raw MD5 if the archive supports it */
                dwVerifyResult |= VERIFY_FILE_HAS_RAW_MD5;

                /* Find file entry for the file */
                pFileEntry = GetFileEntryLocale(ha, szFileName, lcFileLocale);
                if(pFileEntry != NULL)
                {
                    /* If the file's raw MD5 doesn't match, don't bother with more checks */
                    if(VerifyRawMpqData(ha, pFileEntry->ByteOffset, pFileEntry->dwCmpSize) != ERROR_SUCCESS)
                        return dwVerifyResult | VERIFY_FILE_RAW_MD5_ERROR;
                }
            }

            /* Move to the next patch */
            ha = ha->haPatch;
        }
    }

    /* Attempt to open the file */
    if(SFileOpenFileEx(hMpq, szFileName, SFILE_OPEN_FROM_MPQ, &hFile))
    {
        /* Get the file size */
        hf = (TMPQFile *)hFile;
        pFileEntry = hf->pFileEntry;
        dwTotalBytes = SFileGetFileSize(hFile, NULL);

        /* Initialize the CRC32 and MD5 contexts */
        md5_init(&md5_state);
        dwCrc32 = crc32(0, Z_NULL, 0);

        /* Also turn on sector checksum verification */
        if(dwFlags & SFILE_VERIFY_SECTOR_CRC)
            hf->bCheckSectorCRCs = 1;

        /* Go through entire file and update both CRC32 and MD5 */
        for(;;)
        {
            size_t dwBytesRead = 0;

            /* Read data from file */
            SFileReadFile(hFile, Buffer, sizeof(Buffer), &dwBytesRead);
            if(dwBytesRead == 0)
            {
                if(GetLastError() == ERROR_CHECKSUM_ERROR)
                    dwVerifyResult |= VERIFY_FILE_SECTOR_CRC_ERROR;
                break;
            }

            /* Update CRC32 value */
            if(dwFlags & SFILE_VERIFY_FILE_CRC)
                dwCrc32 = crc32(dwCrc32, Buffer, dwBytesRead);
            
            /* Update MD5 value */
            if(dwFlags & SFILE_VERIFY_FILE_MD5)
                md5_process(&md5_state, Buffer, dwBytesRead);

            /* Decrement the total size */
            dwTotalBytes -= dwBytesRead;
        }

        /* If the file has sector checksums, indicate it in the flags */
        if(dwFlags & SFILE_VERIFY_SECTOR_CRC)
        {
            if((hf->pFileEntry->dwFlags & MPQ_FILE_SECTOR_CRC) && hf->SectorChksums != NULL && hf->SectorChksums[0] != 0)
                dwVerifyResult |= VERIFY_FILE_HAS_SECTOR_CRC;
        }

        /* Check if the entire file has been read */
        /* No point in checking CRC32 and MD5 if not */
        /* Skip checksum checks if the file has patches */
        if(dwTotalBytes == 0)
        {
            /* Check CRC32 and MD5 only if there is no patches */
            if(hf->hfPatch == NULL)
            {
                /* Check if the CRC32 matches. */
                if(dwFlags & SFILE_VERIFY_FILE_CRC)
                {
                    /* Only check the CRC32 if it is valid */
                    if(pFileEntry->dwCrc32 != 0)
                    {
                        dwVerifyResult |= VERIFY_FILE_HAS_CHECKSUM;
                        if(dwCrc32 != pFileEntry->dwCrc32)
                            dwVerifyResult |= VERIFY_FILE_CHECKSUM_ERROR;
                    }
                }

                /* Check if MD5 matches */
                if(dwFlags & SFILE_VERIFY_FILE_MD5)
                {
                    /* Patch files have their MD5 saved in the patch info */
                    pFileMd5 = (hf->pPatchInfo != NULL) ? hf->pPatchInfo->md5 : pFileEntry->md5;
                    md5_done(&md5_state, md5);

                    /* Only check the MD5 if it is valid */
                    if(IsValidMD5(pFileMd5))
                    {
                        dwVerifyResult |= VERIFY_FILE_HAS_MD5;
                        if(memcmp(md5, pFileMd5, MD5_DIGEST_SIZE))
                            dwVerifyResult |= VERIFY_FILE_MD5_ERROR;
                    }
                }
            }
            else
            {
                /* Patched files are MD5-checked automatically */
                dwVerifyResult |= VERIFY_FILE_HAS_MD5;
            }
        }
        else
        {
            dwVerifyResult |= VERIFY_READ_ERROR;
        }

        SFileCloseFile(hFile);
    }
    else
    {
        /* Remember that the file couldn't be open */
        dwVerifyResult |= VERIFY_OPEN_ERROR;
    }

    /* If the caller required CRC32 and/or MD5, give it to him */
    if(pdwCrc32 != NULL)
        *pdwCrc32 = dwCrc32;
    if(pMD5 != NULL)
        memcpy(pMD5, md5, MD5_DIGEST_SIZE); 

    return dwVerifyResult;
}

/* Used in SFileGetFileInfo */
int QueryMpqSignatureInfo(
    TMPQArchive * ha,
    PMPQ_SIGNATURE_INFO pSI)
{
    TFileEntry * pFileEntry;
    uint64_t ExtraBytes;
    uint32_t dwFileSize;

    /* Make sure it's all zeroed */
    memset(pSI, 0, sizeof(MPQ_SIGNATURE_INFO));

    /* Calculate the range of the MPQ */
    CalculateArchiveRange(ha, pSI);

    /* If there is "(signature)" file in the MPQ, it has a weak or secure signature */
    pFileEntry = GetFileEntryLocale(ha, SIGNATURE_NAME, LANG_NEUTRAL);
    if(pFileEntry != NULL)
    {
        /* Calculate the begin and end of the signature file itself */
        pSI->BeginExclude = ha->MpqPos + pFileEntry->ByteOffset;
        pSI->EndExclude = pSI->BeginExclude + pFileEntry->dwCmpSize;
        dwFileSize = (uint32_t)(pSI->EndExclude - pSI->BeginExclude);

        /* Does the signature have proper size? */
        if(dwFileSize == MPQ_SIGNATURE_FILE_SIZE)
        {
            /* Read the weak signature */
            if(!FileStream_Read(ha->pStream, &pSI->BeginExclude, pSI->Signature, dwFileSize))
                return 0;

            pSI->SignatureTypes |= SIGNATURE_TYPE_WEAK;
            pSI->cbSignatureSize = dwFileSize - 8;
            return 1;
        }
        else if(dwFileSize <= MPQ_SECURE_SIGNATURE_SIZE + 8)
        {
            /* Read the secure signature */
            if(!FileStream_Read(ha->pStream, &pSI->BeginExclude, pSI->Signature, dwFileSize))
                return 0;

            pSI->SignatureTypes |= SIGNATURE_TYPE_SECURE;
            pSI->cbSignatureSize = dwFileSize - 8;
            return 1;
        }
    }

    /* If there is extra bytes beyond the end of the archive, */
    /* it's the strong signature */
    ExtraBytes = pSI->EndOfFile - pSI->EndMpqData;
    if(ExtraBytes >= (MPQ_STRONG_SIGNATURE_SIZE + 4))
    {
        /* Read the strong signature */
        if(!FileStream_Read(ha->pStream, &pSI->EndMpqData, pSI->Signature, (MPQ_STRONG_SIGNATURE_SIZE + 4)))
            return 0;

        /* Check the signature header "NGIS" */
        if(pSI->Signature[0] != 'N' || pSI->Signature[1] != 'G' || pSI->Signature[2] != 'I' || pSI->Signature[3] != 'S')
            return 0;

        pSI->SignatureTypes |= SIGNATURE_TYPE_STRONG;
        return 1;
    }

    /* Succeeded, but no known signature found */
    return 1;
}

/*-----------------------------------------------------------------------------
 * Support for weak signature
 */

int SSignFileCreate(TMPQArchive * ha)
{
    TMPQFile * hf = NULL;
    uint8_t EmptySignature[MPQ_SECURE_SIGNATURE_FILE_SIZE];
    size_t nSignatureSize;
    int nError = ERROR_SUCCESS;

    /* Only save the signature if we should do so */
    if(ha->dwFileFlags3 != 0)
    {
        /* The (signature) file must be non-encrypted and non-compressed */
        assert(ha->dwFlags & MPQ_FLAG_SIGNATURE_NEW);
        assert(ha->dwFileFlags3 == MPQ_FILE_EXISTS);
        assert(ha->dwReservedFiles > 0);

        /* Determine the signature size */
        if(ha->keyRSA.N == NULL)
        {
            nSignatureSize = MPQ_SIGNATURE_FILE_SIZE;
        }
        else
        {
            nSignatureSize = mp_count_bits(ha->keyRSA.N) / 8 + 8;
        }
        /* Create the (signature) file file in the MPQ */
        /* Note that the file must not be compressed or encrypted */
        nError = SFileAddFile_Init(ha, SIGNATURE_NAME,
                                       0,
                                       nSignatureSize,
                                       LANG_NEUTRAL,
                                       ha->dwFileFlags3 | MPQ_FILE_REPLACEEXISTING,
                                      &hf);

        /* Write the empty signature file to the archive */
        if(nError == ERROR_SUCCESS)
        {
            /* Write the empty zeroed file to the MPQ */
            memset(EmptySignature, 0, nSignatureSize);
            nError = SFileAddFile_Write(hf, EmptySignature, (uint32_t)nSignatureSize, 0);
            SFileAddFile_Finish(hf);
            /* Clear the invalid mark */
            ha->dwFlags &= ~(MPQ_FLAG_SIGNATURE_NEW | MPQ_FLAG_SIGNATURE_NONE);
            ha->dwReservedFiles--;
        }
    }

    return nError;
}

int SSignFileFinish(TMPQArchive * ha)
{
    MPQ_SIGNATURE_INFO si;
    unsigned long signature_len;
    uint8_t Signature[MPQ_SECURE_SIGNATURE_SIZE];
    uint8_t Digest[SHA1_DIGEST_SIZE];
    unsigned long DigestLength;
    rsa_key * key;
    rsa_key blizzKey;
    int hash_idx;

    /* Sanity checks */
    assert((ha->dwFlags & MPQ_FLAG_CHANGED) == 0);
    assert(ha->dwFileFlags3 == MPQ_FILE_EXISTS);

    /* Query the signature info */
    memset(&si, 0, sizeof(MPQ_SIGNATURE_INFO));
    if(!QueryMpqSignatureInfo(ha, &si))
        return ERROR_FILE_CORRUPT;
    signature_len = si.cbSignatureSize;

    /* There must be exactly one signature */
    if((si.SignatureTypes != SIGNATURE_TYPE_WEAK) == (si.SignatureTypes != SIGNATURE_TYPE_SECURE))
        return ERROR_FILE_CORRUPT;

    if(si.SignatureTypes == SIGNATURE_TYPE_WEAK)
    {
        hash_idx = find_hash("md5");
        DigestLength = MD5_DIGEST_SIZE;
        
        /* Calculate MD5 of the entire archive */
        if(!CalculateMpqHashMd5(ha, &si, Digest))
            return ERROR_VERIFY_FAILED;
    }
    else
    {
        hash_idx = find_hash("sha1");
        DigestLength = SHA1_DIGEST_SIZE;
        
        /* Calculate SHA1 of the entire archive */
        if(!CalculateMpqHashSha1(ha, &si, Digest))
            return ERROR_VERIFY_FAILED;
    }

    if(ha->keyRSA.N == NULL)
    {
        key = &blizzKey;
        
        /* Decode the private key */
        if(!decode_base64_key(szBlizzardWeakPrivateKey, key))
            return ERROR_VERIFY_FAILED;
    }
    else
    {
        if(ha->keyRSA.type != PK_PRIVATE)
            return ENOKEY;
        
        key = &(ha->keyRSA);
    }

    /* Sign the hash */
    memset(Signature, 0, sizeof(Signature));
    rsa_sign_hash_ex(Digest, DigestLength, Signature + 8, &signature_len, LTC_LTC_PKCS_1_V1_5, 0, 0, hash_idx, 0, key);
	memrev(Signature + 8, signature_len);

    if(key == &blizzKey)
        rsa_free(key);

    /* Write the signature to the MPQ. Don't use SFile* functions, but write the hash directly */
    if(!FileStream_Write(ha->pStream, &si.BeginExclude, Signature, signature_len + 8))
        return GetLastError();

    return ERROR_SUCCESS;
}

/*-----------------------------------------------------------------------------
 * Public (exported) functions
 */

int EXPORT_SYMBOL SFileGetFileChecksums(void * hMpq, const char * szFileName, uint32_t * pdwCrc32, char * pMD5)
{
    uint32_t dwVerifyResult;
    uint32_t dwVerifyFlags = 0;

    if(pdwCrc32 != NULL)
        dwVerifyFlags |= SFILE_VERIFY_FILE_CRC;
    if(pMD5 != NULL)
        dwVerifyFlags |= SFILE_VERIFY_FILE_MD5;

    dwVerifyResult = VerifyFile(hMpq,
                                szFileName,
                                pdwCrc32,
                                pMD5,
                                dwVerifyFlags);

    /* If verification failed, return zero */
    if(dwVerifyResult & VERIFY_FILE_ERROR_MASK)
    {
        SetLastError(ERROR_FILE_CORRUPT);
        return 0;
    }

    return 1;
}


uint32_t EXPORT_SYMBOL SFileVerifyFile(void * hMpq, const char * szFileName, uint32_t dwFlags)
{
    return VerifyFile(hMpq,
                      szFileName,
                      NULL,
                      NULL,
                      dwFlags);
}

/* Verifies raw data of the archive Only works for MPQs version 4 or newer */
int EXPORT_SYMBOL SFileVerifyRawData(void * hMpq, uint32_t dwWhatToVerify, const char * szFileName)
{
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    TFileEntry * pFileEntry;
    TMPQHeader * pHeader;

    /* Verify input parameters */
    if(!IsValidMpqHandle(hMpq))
        return ERROR_INVALID_PARAMETER;
    pHeader = ha->pHeader;

    /* If the archive doesn't have raw data MD5, report it as OK */
    if(pHeader->dwRawChunkSize == 0)
        return ERROR_SUCCESS;

    /* If we have to verify MPQ header, do it */
    switch(dwWhatToVerify)
    {
        case SFILE_VERIFY_MPQ_HEADER:
            
            /* Only if the header is of version 4 or newer */
            if(pHeader->dwHeaderSize >= (MPQ_HEADER_SIZE_V4 - MD5_DIGEST_SIZE))
                return VerifyRawMpqData(ha, 0, MPQ_HEADER_SIZE_V4 - MD5_DIGEST_SIZE);
            return ERROR_SUCCESS;

        case SFILE_VERIFY_HET_TABLE:

            /* Only if we have HET table */
            if(pHeader->HetTablePos64 && pHeader->HetTableSize64)
                return VerifyRawMpqData(ha, pHeader->HetTablePos64, (uint32_t)pHeader->HetTableSize64);
            return ERROR_SUCCESS;

        case SFILE_VERIFY_BET_TABLE:

            /* Only if we have BET table */
            if(pHeader->BetTablePos64 && pHeader->BetTableSize64)
                return VerifyRawMpqData(ha, pHeader->BetTablePos64, (uint32_t)pHeader->BetTableSize64);
            return ERROR_SUCCESS;

        case SFILE_VERIFY_HASH_TABLE:

            /* Hash table is not protected by MD5 */
            return ERROR_SUCCESS;

        case SFILE_VERIFY_BLOCK_TABLE:

            /* Block table is not protected by MD5 */
            return ERROR_SUCCESS;

        case SFILE_VERIFY_HIBLOCK_TABLE:

            /* It is unknown if the hi-block table is protected my MD5 or not. */
            return ERROR_SUCCESS;

        case SFILE_VERIFY_FILE:

            /* Verify parameters */
            if(szFileName == NULL || *szFileName == 0)
                return ERROR_INVALID_PARAMETER;

            /* Get the offset of a file */
            pFileEntry = GetFileEntryLocale(ha, szFileName, lcFileLocale);
            if(pFileEntry == NULL)
                return ERROR_FILE_NOT_FOUND;

            return VerifyRawMpqData(ha, pFileEntry->ByteOffset, pFileEntry->dwCmpSize);
    }

    return ERROR_INVALID_PARAMETER;
}


/* Verifies the archive against the signature */
uint32_t EXPORT_SYMBOL SFileVerifyArchive(void * hMpq)
{
    MPQ_SIGNATURE_INFO si;
    TMPQArchive * ha = (TMPQArchive *)hMpq;

    /* Verify input parameters */
    if(!IsValidMpqHandle(hMpq))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return ERROR_VERIFY_FAILED;
    }

    /* If the archive was modified, we need to flush it */
    if(ha->dwFlags & MPQ_FLAG_CHANGED)
        SFileFlushArchive(hMpq);

    /* Get the MPQ signature and signature type */
    memset(&si, 0, sizeof(MPQ_SIGNATURE_INFO));
    if(!QueryMpqSignatureInfo(ha, &si))
        return ERROR_VERIFY_FAILED;

    SetLastError(ERROR_SUCCESS);

    /* If there is no signature */
    if(si.SignatureTypes == 0)
        return ERROR_NO_SIGNATURE;

    /* We haven't seen a MPQ with both signatures */
    assert(si.SignatureTypes == SIGNATURE_TYPE_WEAK || si.SignatureTypes == SIGNATURE_TYPE_STRONG || si.SignatureTypes == SIGNATURE_TYPE_SECURE);

    /* Verify the secure signature, if present */
    if(si.SignatureTypes & SIGNATURE_TYPE_SECURE)
        return VerifySecureSignature(ha, &si);

    /* Verify the strong signature, if present */
    if(si.SignatureTypes & SIGNATURE_TYPE_STRONG)
        return VerifyStrongSignature(ha, &si);

    /* Verify the weak signature, if present */
    if(si.SignatureTypes & SIGNATURE_TYPE_WEAK)
        return VerifyWeakSignature(ha, &si);

    return ERROR_NO_SIGNATURE;
}

/* Signs the archive */
int EXPORT_SYMBOL SFileSignArchive(void * hMpq, uint32_t dwSignatureType)
{
    TMPQArchive * ha;

    /* Verify the archive handle */
    ha = IsValidMpqHandle(hMpq);
    if(ha == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* We only support weak and secure signature */
    if((dwSignatureType != SIGNATURE_TYPE_WEAK) && (dwSignatureType != SIGNATURE_TYPE_SECURE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Check the key length */
    if(dwSignatureType == SIGNATURE_TYPE_WEAK)
    {
        if(ha->keyRSA.N != NULL)
        {
            if(mp_count_bits(ha->keyRSA.N) != 512)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
            }
            if(ha->keyRSA.type == PK_PUBLIC)
            {
                SetLastError(ENOKEY);
                return 0;
            }
        }
    }
    else
    {
        if(ha->keyRSA.N != NULL)
        {
            int nBits = mp_count_bits(ha->keyRSA.N);
            if((nBits > 4096) || (nBits < 1024))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
            }
            if(ha->keyRSA.type == PK_PUBLIC)
            {
                SetLastError(ENOKEY);
                return 0;
            }
        }
        else
        {
                SetLastError(ENOKEY);
                return 0;
        }
    }

    /* The archive must not be malformed and must not be read-only */
    if(ha->dwFlags & (MPQ_FLAG_READ_ONLY | MPQ_FLAG_MALFORMED))
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return 0;
    }

    /* If the signature is not there yet */
    if(ha->dwFileFlags3 == 0)
    {
        /* Turn the signature on. The signature will */
        /* be applied when the archive is closed */
        ha->dwFlags |= MPQ_FLAG_SIGNATURE_NEW | MPQ_FLAG_CHANGED;
        ha->dwFileFlags3 = MPQ_FILE_EXISTS;
        ha->dwReservedFiles++;
    }

    return 1;
}

/* Sets the signing key */
int EXPORT_SYMBOL SFileSetRSAKey(void * hMpq, unsigned char * key, size_t keyLength)
{
    TMPQArchive * ha;

    /* Verify the archive handle */
    ha = IsValidMpqHandle(hMpq);
    if(ha == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if(ha->keyRSA.N != NULL)
        rsa_free(&(ha->keyRSA));

    if(key[0] == '-')
    {
        /* looks like PEM-format */
        if(decode_base64_key((char *)key, &(ha->keyRSA)))
            return 1;
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
    }
    else
    {
        /* must be DER */
        if(rsa_import(key, keyLength, &(ha->keyRSA)) != CRYPT_OK)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
        else
            return 1;
    }
}
