#####################################################################
###
#
# Makefile for compiling StormLib under linux
#
# Author: Marko Friedemann <marko.friedemann@bmx-chemnitz.de>
# Created at: Mon Jan 29 18:26:01 CEST 2001
# Computer: whiplash.flachland-chemnitz.de
# System: Linux 2.4.0 on i686
#
# Copyright (c) 2001 BMX-Chemnitz.DE All rights reserved.
#
#####################################################################
###

##############################################################
# updated on Monday 3, 2010 by Christopher Chedeau aka vjeux #
# updated on April 24, 2010 by Ivan Komissarov aka Nevermore #
# updated on April 09, 2015 by Ayron                         #
##############################################################

C++ = g++
CC = gcc
AS = as
AR = ar
DFLAGS = -D_7ZIP_ST -DPLATFORM_LITTLE_ENDIAN -DPLATFORM_LINUX
OFLAGS =
#LFLAGS = -m32
LFLAGS = -m64
CFLAGS = -fPIC -std=gnu89 -g -fvisibility=internal
WFLAGS = -Wall -Werror=implicit-int -Werror=implicit-function-declaration -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Werror
CFLAGS += $(OFLAGS) $(DFLAGS) $(WFLAGS)

#ARCH = -m32
ARCH = -m64
#BF = elf32-i386
BF = elf64-x86-64

OBJC = src/FileStream.o \
	src/SBaseCommon.o \
	src/SBaseDumpData.o \
	src/SBaseFileTable.o \
	src/SBaseSubTypes.o \
	src/SCompression.o \
	src/SFileAddFile.o \
	src/SFileAttributes.o \
	src/SFileCompactArchive.o \
	src/SFileCreateArchive.o \
	src/SFileExtractFile.o \
	src/SFileFindFile.o \
	src/SFileGetFileInfo.o \
	src/SFileListFile.o \
	src/SFileOpenArchive.o \
	src/SFileOpenFileEx.o \
	src/SFilePatchArchives.o \
	src/SFileReadFile.o \
	src/SFileVerify.o

OBJC_TC = src/libtomcrypt/src/hashes/sha1.o \
	src/libtomcrypt/src/hashes/hash_memory.o \
	src/libtomcrypt/src/hashes/md5.o \
	src/libtomcrypt/src/misc/crypt_hash_is_valid.o \
	src/libtomcrypt/src/misc/crypt_prng_descriptor.o \
	src/libtomcrypt/src/misc/crypt_register_prng.o \
	src/libtomcrypt/src/misc/crypt_ltc_mp_descriptor.o \
	src/libtomcrypt/src/misc/crypt_find_hash.o \
	src/libtomcrypt/src/misc/zeromem.o \
	src/libtomcrypt/src/misc/base64_decode.o \
	src/libtomcrypt/src/misc/crypt_register_hash.o \
	src/libtomcrypt/src/misc/crypt_find_prng.o \
	src/libtomcrypt/src/misc/crypt_prng_is_valid.o \
	src/libtomcrypt/src/misc/crypt_hash_descriptor.o \
	src/libtomcrypt/src/misc/crypt_libc.o \
	src/libtomcrypt/src/misc/crypt_argchk.o \
	src/libtomcrypt/src/math/multi.o \
	src/libtomcrypt/src/math/ltm_desc.o \
	src/libtomcrypt/src/math/rand_prime.o \
	src/libtomcrypt/src/pk/asn1/der_length_ia5_string.o \
	src/libtomcrypt/src/pk/asn1/der_decode_utctime.o \
	src/libtomcrypt/src/pk/asn1/der_length_boolean.o \
	src/libtomcrypt/src/pk/asn1/der_decode_object_identifier.o \
	src/libtomcrypt/src/pk/asn1/der_decode_sequence_multi.o \
	src/libtomcrypt/src/pk/asn1/der_decode_octet_string.o \
	src/libtomcrypt/src/pk/asn1/der_length_object_identifier.o \
	src/libtomcrypt/src/pk/asn1/der_length_bit_string.o \
	src/libtomcrypt/src/pk/asn1/der_decode_ia5_string.o \
	src/libtomcrypt/src/pk/asn1/der_length_integer.o \
	src/libtomcrypt/src/pk/asn1/der_length_sequence.o \
	src/libtomcrypt/src/pk/asn1/der_decode_choice.o \
	src/libtomcrypt/src/pk/asn1/der_length_octet_string.o \
	src/libtomcrypt/src/pk/asn1/der_decode_sequence_flexi.o \
	src/libtomcrypt/src/pk/asn1/der_decode_printable_string.o \
	src/libtomcrypt/src/pk/asn1/der_decode_bit_string.o \
	src/libtomcrypt/src/pk/asn1/der_decode_short_integer.o \
	src/libtomcrypt/src/pk/asn1/der_length_utctime.o \
	src/libtomcrypt/src/pk/asn1/der_decode_utf8_string.o \
	src/libtomcrypt/src/pk/asn1/der_decode_integer.o \
	src/libtomcrypt/src/pk/asn1/der_decode_boolean.o \
	src/libtomcrypt/src/pk/asn1/der_sequence_free.o \
	src/libtomcrypt/src/pk/asn1/der_decode_sequence_ex.o \
	src/libtomcrypt/src/pk/asn1/der_length_short_integer.o \
	src/libtomcrypt/src/pk/asn1/der_length_printable_string.o \
	src/libtomcrypt/src/pk/asn1/der_length_utf8_string.o \
	src/libtomcrypt/src/pk/asn1/der_encode_bit_string.o \
	src/libtomcrypt/src/pk/asn1/der_encode_boolean.o \
	src/libtomcrypt/src/pk/asn1/der_encode_ia5_string.o \
	src/libtomcrypt/src/pk/asn1/der_encode_integer.o \
	src/libtomcrypt/src/pk/asn1/der_encode_object_identifier.o \
	src/libtomcrypt/src/pk/asn1/der_encode_octet_string.o \
	src/libtomcrypt/src/pk/asn1/der_encode_printable_string.o \
	src/libtomcrypt/src/pk/asn1/der_encode_sequence_ex.o \
	src/libtomcrypt/src/pk/asn1/der_encode_sequence_multi.o \
	src/libtomcrypt/src/pk/asn1/der_encode_set.o \
	src/libtomcrypt/src/pk/asn1/der_encode_setof.o \
	src/libtomcrypt/src/pk/asn1/der_encode_short_integer.o \
	src/libtomcrypt/src/pk/asn1/der_encode_utctime.o \
	src/libtomcrypt/src/pk/asn1/der_encode_utf8_string.o \
	src/libtomcrypt/src/pk/rsa/rsa_make_key.o \
	src/libtomcrypt/src/pk/rsa/rsa_sign_hash.o \
	src/libtomcrypt/src/pk/rsa/rsa_free.o \
	src/libtomcrypt/src/pk/rsa/rsa_verify_simple.o \
	src/libtomcrypt/src/pk/rsa/rsa_import.o \
	src/libtomcrypt/src/pk/rsa/rsa_verify_hash.o \
	src/libtomcrypt/src/pk/rsa/rsa_exptmod.o \
	src/libtomcrypt/src/pk/pkcs1/pkcs_1_v1_5_decode.o \
	src/libtomcrypt/src/pk/pkcs1/pkcs_1_v1_5_encode.o \
	src/libtomcrypt/src/pk/pkcs1/pkcs_1_pss_decode.o \
	src/libtomcrypt/src/pk/pkcs1/pkcs_1_mgf1.o \
	src/libtomcrypt/src/pk/pkcs1/pkcs_1_oaep_decode.o \
	src/libtomcrypt/src/pk/pkcs1/pkcs_1_pss_encode.o \
	src/libtomcrypt/src/pk/ecc/ltc_ecc_projective_dbl_point.o \
	src/libtomcrypt/src/pk/ecc/ltc_ecc_mulmod.o \
	src/libtomcrypt/src/pk/ecc/ltc_ecc_projective_add_point.o \
	src/libtomcrypt/src/pk/ecc/ltc_ecc_map.o \
	src/libtomcrypt/src/pk/ecc/ltc_ecc_points.o \
	src/libtomcrypt/src/pk/ecc/ltc_ecc_mul2add.o

OBJC_TM = src/libtommath/bn_mp_exptmod_fast.o \
	src/libtommath/bn_mp_jacobi.o \
	src/libtommath/bn_mp_mod.o \
	src/libtommath/bn_mp_signed_bin_size.o \
	src/libtommath/bn_mp_invmod.o \
	src/libtommath/bn_mp_is_square.o \
	src/libtommath/bn_mp_neg.o \
	src/libtommath/bn_mp_reduce_2k.o \
	src/libtommath/bn_mp_xor.o \
	src/libtommath/bn_mp_karatsuba_mul.o \
	src/libtommath/bn_mp_dr_setup.o \
	src/libtommath/bn_mp_mul.o \
	src/libtommath/bn_mp_init_multi.o \
	src/libtommath/bn_mp_clear.o \
	src/libtommath/bn_s_mp_sqr.o \
	src/libtommath/bn_mp_rshd.o \
	src/libtommath/bn_s_mp_sub.o \
	src/libtommath/bn_mp_sub.o \
	src/libtommath/bn_mp_toradix.o \
	src/libtommath/bn_mp_reduce.o \
	src/libtommath/bn_mp_prime_is_prime.o \
	src/libtommath/bn_mp_prime_next_prime.o \
	src/libtommath/bn_mp_exptmod.o \
	src/libtommath/bn_mp_mod_2d.o \
	src/libtommath/bn_reverse.o \
	src/libtommath/bn_mp_init.o \
	src/libtommath/bn_fast_s_mp_sqr.o \
	src/libtommath/bn_mp_sqr.o \
	src/libtommath/bn_mp_cnt_lsb.o \
	src/libtommath/bn_mp_clear_multi.o \
	src/libtommath/bn_mp_exch.o \
	src/libtommath/bn_fast_s_mp_mul_digs.o \
	src/libtommath/bn_mp_grow.o \
	src/libtommath/bn_mp_read_radix.o \
	src/libtommath/bn_mp_mul_2.o \
	src/libtommath/bn_mp_shrink.o \
	src/libtommath/bn_mp_div_2.o \
	src/libtommath/bn_fast_mp_invmod.o \
	src/libtommath/bn_mp_prime_miller_rabin.o \
	src/libtommath/bn_mp_to_unsigned_bin.o \
	src/libtommath/bn_mp_prime_rabin_miller_trials.o \
	src/libtommath/bn_mp_2expt.o \
	src/libtommath/bn_mp_cmp_mag.o \
	src/libtommath/bn_mp_to_signed_bin.o \
	src/libtommath/bn_mp_get_int.o \
	src/libtommath/bn_mp_montgomery_reduce.o \
	src/libtommath/bn_mp_dr_reduce.o \
	src/libtommath/bn_mp_fwrite.o \
	src/libtommath/bn_mp_and.o \
	src/libtommath/bn_mp_exteuclid.o \
	src/libtommath/bn_fast_mp_montgomery_reduce.o \
	src/libtommath/bn_s_mp_mul_high_digs.o \
	src/libtommath/bn_mp_reduce_setup.o \
	src/libtommath/bn_mp_lcm.o \
	src/libtommath/bn_mp_abs.o \
	src/libtommath/bn_mp_cmp.o \
	src/libtommath/bn_mp_submod.o \
	src/libtommath/bn_mp_div_d.o \
	src/libtommath/bn_s_mp_mul_digs.o \
	src/libtommath/bn_mp_mul_d.o \
	src/libtommath/bn_mp_to_unsigned_bin_n.o \
	src/libtommath/bn_mp_prime_random_ex.o \
	src/libtommath/bn_mp_rand.o \
	src/libtommath/bn_mp_div_2d.o \
	src/libtommath/bn_mp_addmod.o \
	src/libtommath/bn_mp_init_copy.o \
	src/libtommath/bn_mp_read_unsigned_bin.o \
	src/libtommath/bn_mp_toradix_n.o \
	src/libtommath/bn_fast_s_mp_mul_high_digs.o \
	src/libtommath/bn_mp_toom_sqr.o \
	src/libtommath/bn_mp_to_signed_bin_n.o \
	src/libtommath/bn_mp_reduce_2k_setup_l.o \
	src/libtommath/bn_mp_div.o \
	src/libtommath/bn_prime_tab.o \
	src/libtommath/bn_mp_karatsuba_sqr.o \
	src/libtommath/bn_mp_gcd.o \
	src/libtommath/bn_mp_prime_is_divisible.o \
	src/libtommath/bn_mp_set_int.o \
	src/libtommath/bn_mp_prime_fermat.o \
	src/libtommath/bn_mp_cmp_d.o \
	src/libtommath/bn_mp_add.o \
	src/libtommath/bn_mp_sub_d.o \
	src/libtommath/bn_s_mp_exptmod.o \
	src/libtommath/bn_mp_init_size.o \
	src/libtommath/bncore.o \
	src/libtommath/bn_mp_radix_smap.o \
	src/libtommath/bn_mp_reduce_2k_l.o \
	src/libtommath/bn_mp_montgomery_calc_normalization.o \
	src/libtommath/bn_mp_mod_d.o \
	src/libtommath/bn_mp_set.o \
	src/libtommath/bn_mp_or.o \
	src/libtommath/bn_mp_sqrt.o \
	src/libtommath/bn_mp_invmod_slow.o \
	src/libtommath/bn_mp_count_bits.o \
	src/libtommath/bn_mp_read_signed_bin.o \
	src/libtommath/bn_mp_div_3.o \
	src/libtommath/bn_mp_unsigned_bin_size.o \
	src/libtommath/bn_mp_mulmod.o \
	src/libtommath/bn_mp_clamp.o \
	src/libtommath/bn_mp_reduce_2k_setup.o \
	src/libtommath/bn_mp_toom_mul.o \
	src/libtommath/bn_mp_montgomery_setup.o \
	src/libtommath/bn_mp_expt_d.o \
	src/libtommath/bn_mp_copy.o \
	src/libtommath/bn_mp_dr_is_modulus.o \
	src/libtommath/bn_mp_sqrmod.o \
	src/libtommath/bn_mp_reduce_is_2k_l.o \
	src/libtommath/bn_mp_mul_2d.o \
	src/libtommath/bn_mp_fread.o \
	src/libtommath/bn_mp_init_set.o \
	src/libtommath/bn_mp_add_d.o \
	src/libtommath/bn_mp_zero.o \
	src/libtommath/bn_s_mp_add.o \
	src/libtommath/bn_mp_radix_size.o \
	src/libtommath/bn_mp_init_set_int.o \
	src/libtommath/bn_mp_n_root.o \
	src/libtommath/bn_mp_lshd.o \
	src/libtommath/bn_mp_reduce_is_2k.o

OBJC_PK = src/pklib/implode.o \
	src/pklib/crc32.o \
	src/pklib/explode.o \

OBJC_ZLIB = src/zlib/crc32_zlib.o \
	src/zlib/trees.o \
	src/zlib/compress_zlib.o \
	src/zlib/adler32.o \
	src/zlib/inftrees.o \
	src/zlib/inffast.o \
	src/zlib/deflate.o \
	src/zlib/inflate.o \
	src/zlib/zutil.o

OBJC_LZMA = src/lzma/C/LzFind.o \
	src/lzma/C/LzmaEnc.o \
	src/lzma/C/LzmaDec.o \

OBJC_BZ2 = src/bzip2/blocksort.o \
	src/bzip2/bzlib.o \
	src/bzip2/compress.o \
	src/bzip2/crctable.o \
	src/bzip2/decompress.o \
	src/bzip2/huffman.o \
	src/bzip2/randtable.o

OBJC_LIB = src/jenkins/lookup3.o \
	src/adpcm/adpcm.o \
	src/anubis/anubis.o \
	src/huffman/huff.o \
	src/jenkins/lookup3.o \
	src/sparse/sparse.o

LIBS = src/libtomcrypt/libtomcrypt.a \
	src/libtommath/libtommath.a \
	src/pklib/pklib.a \
	src/zlib/zlib.a \
	src/lzma/lzma.a \
	src/bzip2/bzip2.a \
	src/adpcm/adpcm.o \
	src/anubis/anubis.o \
	src/serpent/serpent.o \
	src/huffman/huff.o \
	src/jenkins/lookup3.o \
	src/sparse/sparse.o

SO = libThunderStorm.so
SLIB = libThunderStorm.a

so: $(SO)

static: $(SLIB)

libs: $(LIBS)

$(SO): $(OBJC_TC) $(OBJC_TM) $(OBJC_PK) $(OBJC_ZLIB) $(OBJC_LZMA) $(OBJC_BZ2) $(LIBS) $(OBJC)
	@echo [LD] $@
	@$(CC) $(ARCH) -shared -o $(SO) $(OBJC) $(LIBS) $(LFLAGS)

$(SLIB): $(OBJC_TC) $(OBJC_TM) $(OBJC_PK) $(OBJC_ZLIB) $(OBJC_LZMA) $(OBJC_BZ2) $(LIBS) $(OBJC)
	@echo [AR] $@
	@$(AR) rcs $(SLIB) $(OBJC) $(OBJC_TC) $(OBJC_TM) $(OBJC_PK) $(OBJC_ZLIB) $(OBJC_LZMA) $(OBJC_BZ2) $(LIBS)

src/libtomcrypt/libtomcrypt.a: $(OBJC_TC)
	@echo [AR] $@
	@$(AR) cr src/libtomcrypt/libtomcrypt.a $(OBJC_TC)

src/libtommath/libtommath.a: $(OBJC_TM)
	@echo [AR] $@
	@$(AR) cr src/libtommath/libtommath.a $(OBJC_TM)

src/pklib/pklib.a: $(OBJC_PK)
	@echo [AR] $@
	@$(AR) cr src/pklib/pklib.a $(OBJC_PK)

src/zlib/zlib.a: $(OBJC_ZLIB)
	@echo [AR] $@
	@$(AR) cr src/zlib/zlib.a $(OBJC_ZLIB)
	
src/lzma/lzma.a: $(OBJC_LZMA)
	@echo [AR] $@
	@$(AR) cr src/lzma/lzma.a $(OBJC_LZMA)

src/bzip2/bzip2.a: $(OBJC_BZ2)
	@echo [AR] $@
	@$(AR) cr src/bzip2/bzip2.a $(OBJC_BZ2)

clean:
	rm -f $(OVJS) $(OBJC) $(OBJC_TC) $(OBJC_TM) $(OBJC_PK) $(OBJC_ZLIB) $(OBJC_LZMA) $(OBJC_BZ2) $(LIBS) $(SO) $(SLIB) $(OVL) libThunderstorm

$(OBJS): %.o: %.s
	$(AS) -o $@ $(ASFLAGS) $<

%.o: %.c
	@echo [CC] $@
	@$(CC) -o $@ $(CFLAGS) $(ARCH) -c $<

install: $(SO)
	install $(SO) /usr/local/lib
	cp src/thunderStorm.h /usr/local/include/thunderStorm.h
	ldconfig
