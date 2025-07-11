#ifndef FALCON_INNER_H__
#define FALCON_INNER_H__

/*
 * Internal functions for Falcon. This is not the API intended to be
 * used by applications; instead, this internal API provides all the
 * primitives on which wrappers build to provide external APIs.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2017-2019  Falcon Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@nccgroup.com>
 */

/*
 * IMPORTANT API RULES
 * -------------------
 *
 * This API has some non-trivial usage rules:
 *
 *
 *  - All public functions (i.e. the non-static ones) must be referenced
 *    with the Zf() macro (e.g. Zf(verify_raw) for the verify_raw()
 *    function). That macro adds a prefix to the name, which is
 *    configurable with the FALCON_PREFIX macro. This allows compiling
 *    the code into a specific "namespace" and potentially including
 *    several versions of this code into a single application (e.g. to
 *    have an AVX2 and a non-AVX2 variants and select the one to use at
 *    runtime based on availability of AVX2 opcodes).
 *
 *  - Functions that need temporary buffers expects them as a final
 *    tmp[] array of type uint8_t*, with a size which is documented for
 *    each function. However, most have some alignment requirements,
 *    because they will use the array to store 16-bit, 32-bit or 64-bit
 *    values (e.g. uint64_t or double). The caller must ensure proper
 *    alignment. What happens on unaligned access depends on the
 *    underlying architecture, ranging from a slight time penalty
 *    to immediate termination of the process.
 *
 *  - Some functions rely on specific rounding rules and precision for
 *    floating-point numbers. On some systems (in particular 32-bit x86
 *    with the 387 FPU), this requires setting an hardware control
 *    word. The caller MUST use set_fpu_cw() to ensure proper precision:
 *
 *      oldcw = set_fpu_cw(2);
 *      Zf(sign_dyn)(...);
 *      set_fpu_cw(oldcw);
 *
 *    On systems where the native floating-point precision is already
 *    proper, or integer-based emulation is used, the set_fpu_cw()
 *    function does nothing, so it can be called systematically.
 */

// yyyPQCLEAN+0 yyyNIST+0 yyySUPERCOP+0
#include "config.h"
// yyyPQCLEAN- yyyNIST- yyySUPERCOP-
// yyySUPERCOP+1
// yyyCONF*
// yyySUPERCOP-

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>

#if defined FALCON_AVX2 && FALCON_AVX2 // yyyAVX2+1
/*
 * This implementation uses AVX2 and optionally FMA intrinsics.
 */
#include <immintrin.h>
#ifndef FALCON_LE
#define FALCON_LE   1
#endif
#ifndef FALCON_UNALIGNED
#define FALCON_UNALIGNED   1
#endif
#if defined __GNUC__
#if defined FALCON_FMA && FALCON_FMA
#define TARGET_AVX2   __attribute__((target("avx2,fma")))
#else
#define TARGET_AVX2   __attribute__((target("avx2")))
#endif
#elif defined _MSC_VER && _MSC_VER
#pragma warning( disable : 4752 )
#endif
#if defined FALCON_FMA && FALCON_FMA
#define FMADD(a, b, c)   _mm256_fmadd_pd(a, b, c)
#define FMSUB(a, b, c)   _mm256_fmsub_pd(a, b, c)
#else
#define FMADD(a, b, c)   _mm256_add_pd(_mm256_mul_pd(a, b), c)
#define FMSUB(a, b, c)   _mm256_sub_pd(_mm256_mul_pd(a, b), c)
#endif
#endif // yyyAVX2-

// yyyNIST+0 yyyPQCLEAN+0
/*
 * On MSVC, disable warning about applying unary minus on an unsigned
 * type: this is perfectly defined standard behaviour and we do it
 * quite often.
 */
#if defined _MSC_VER && _MSC_VER
#pragma warning( disable : 4146 )
#endif

// yyySUPERCOP+0
/*
 * Enable ARM assembly on any ARMv7m platform (if it was not done before).
 */
#ifndef FALCON_ASM_CORTEXM4
#if (defined __ARM_ARCH_7EM__ && __ARM_ARCH_7EM__) \
	&& (defined __ARM_FEATURE_DSP && __ARM_FEATURE_DSP)
#define FALCON_ASM_CORTEXM4   1
#else
#define FALCON_ASM_CORTEXM4   0
#endif
#endif
// yyySUPERCOP-

#if defined __i386__ || defined _M_IX86 \
	|| defined __x86_64__ || defined _M_X64 || \
	(defined _ARCH_PWR8 && \
		(defined __LITTLE_ENDIAN || defined __LITTLE_ENDIAN__))

#ifndef FALCON_LE
#define FALCON_LE     1
#endif
#ifndef FALCON_UNALIGNED
#define FALCON_UNALIGNED   1
#endif

#elif defined FALCON_ASM_CORTEXM4 && FALCON_ASM_CORTEXM4

#ifndef FALCON_LE
#define FALCON_LE     1
#endif
#ifndef FALCON_UNALIGNED
#define FALCON_UNALIGNED   0
#endif

#elif (defined __LITTLE_ENDIAN__ && __LITTLE_ENDIAN__) \
	|| (defined __BYTE_ORDER__ && defined __ORDER_LITTLE_ENDIAN__ \
		&& __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

#ifndef FALCON_LE
#define FALCON_LE     1
#endif
#ifndef FALCON_UNALIGNED
#define FALCON_UNALIGNED   0
#endif

#else

#ifndef FALCON_LE
#define FALCON_LE     0
#endif
#ifndef FALCON_UNALIGNED
#define FALCON_UNALIGNED   0
#endif

#endif

/*
 * We ensure that both FALCON_FPEMU and FALCON_FPNATIVE are defined,
 * with compatible values (exactly one of them must be non-zero).
 * If none is defined, then default FP implementation is 'native'
 * except on ARM Cortex M4.
 */
#if !defined FALCON_FPEMU && !defined FALCON_FPNATIVE

#if (defined __ARM_FP && ((__ARM_FP & 0x08) == 0x08)) \
	|| (!defined __ARM_FP && defined __ARM_VFPV2__)
#define FALCON_FPEMU      0
#define FALCON_FPNATIVE   1
#elif defined FALCON_ASM_CORTEXM4 && FALCON_ASM_CORTEXM4
#define FALCON_FPEMU      1
#define FALCON_FPNATIVE   0
#else
#define FALCON_FPEMU      0
#define FALCON_FPNATIVE   1
#endif

#elif defined FALCON_FPEMU && !defined FALCON_FPNATIVE

#if FALCON_FPEMU
#define FALCON_FPNATIVE   0
#else
#define FALCON_FPNATIVE   1
#endif

#elif defined FALCON_FPNATIVE && !defined FALCON_FPEMU

#if FALCON_FPNATIVE
#define FALCON_FPEMU   0
#else
#define FALCON_FPEMU   1
#endif

#endif

#if (FALCON_FPEMU && FALCON_FPNATIVE) || (!FALCON_FPEMU && !FALCON_FPNATIVE)
#error Exactly one of FALCON_FPEMU and FALCON_FPNATIVE must be selected
#endif

// yyySUPERCOP+0
/*
 * For seed generation from the operating system:
 *  - On Linux and glibc-2.25+, FreeBSD 12+ and OpenBSD, use getentropy().
 *  - On Unix-like systems, use /dev/urandom (including as a fallback
 *    for failed getentropy() calls).
 *  - On Windows, use CryptGenRandom().
 */

#ifndef FALCON_RAND_GETENTROPY
#if (defined __linux__ && defined __GLIBC__ \
	&& (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))) \
	|| (defined __FreeBSD__ && __FreeBSD__ >= 12) \
	|| defined __OpenBSD__
#define FALCON_RAND_GETENTROPY   1
#else
#define FALCON_RAND_GETENTROPY   0
#endif
#endif

#ifndef FALCON_RAND_URANDOM
#if defined _AIX \
	|| defined __ANDROID__ \
	|| defined __FreeBSD__ \
	|| defined __NetBSD__ \
	|| defined __OpenBSD__ \
	|| defined __DragonFly__ \
	|| defined __linux__ \
	|| (defined __sun && (defined __SVR4 || defined __svr4__)) \
	|| (defined __APPLE__ && defined __MACH__)
#define FALCON_RAND_URANDOM   1
#else
#define FALCON_RAND_URANDOM   0
#endif
#endif

#ifndef FALCON_RAND_WIN32
#if defined _WIN32 || defined _WIN64
#define FALCON_RAND_WIN32   1
#else
#define FALCON_RAND_WIN32   0
#endif
#endif
// yyySUPERCOP-

/*
 * For still undefined compile-time macros, define them to 0 to avoid
 * warnings with -Wundef.
 */
#ifndef FALCON_AVX2
#define FALCON_AVX2   0
#endif
#ifndef FALCON_FMA
#define FALCON_FMA   0
#endif
#ifndef FALCON_KG_CHACHA20
#define FALCON_KG_CHACHA20   0
#endif
// yyyNIST- yyyPQCLEAN-

// yyyPQCLEAN+0 yyySUPERCOP+0
/*
 * "Naming" macro used to apply a consistent prefix over all global
 * symbols.
 */
#ifndef FALCON_PREFIX
#define FALCON_PREFIX   falcon_inner
#endif
#define Zf(name)             Zf_(FALCON_PREFIX, name)
#define Zf_(prefix, name)    Zf__(prefix, name)
#define Zf__(prefix, name)   prefix ## _ ## name  
// yyyPQCLEAN- yyySUPERCOP-

// yyyAVX2+1
/*
 * We use the TARGET_AVX2 macro to tag some functions which, in some
 * configurations, may use AVX2 and FMA intrinsics; this depends on
 * the compiler. In all other cases, we just define it to emptiness
 * (i.e. it will have no effect).
 */
#ifndef TARGET_AVX2
#define TARGET_AVX2
#endif
// yyyAVX2-

/*
 * Some computations with floating-point elements, in particular
 * rounding to the nearest integer, rely on operations using _exactly_
 * the precision of IEEE-754 binary64 type (i.e. 52 bits). On 32-bit
 * x86, the 387 FPU may be used (depending on the target OS) and, in
 * that case, may use more precision bits (i.e. 64 bits, for an 80-bit
 * total type length); to prevent miscomputations, we define an explicit
 * function that modifies the precision in the FPU control word.
 *
 * set_fpu_cw() sets the precision to the provided value, and returns
 * the previously set precision; callers are supposed to restore the
 * previous precision on exit. The correct (52-bit) precision is
 * configured with the value "2". On unsupported compilers, or on
 * targets other than 32-bit x86, or when the native 'double' type is
 * not used, the set_fpu_cw() function does nothing at all.
 */
#if FALCON_FPNATIVE  // yyyFPNATIVE+1
#if defined __GNUC__ && defined __i386__
static inline unsigned
set_fpu_cw(unsigned x)
{
	unsigned short t;
	unsigned old;

	__asm__ __volatile__ ("fstcw %0" : "=m" (t) : : );
	old = (t & 0x0300u) >> 8;
	t = (unsigned short)((t & ~0x0300u) | (x << 8));
	__asm__ __volatile__ ("fldcw %0" : : "m" (t) : );
	return old;
}
#elif defined _M_IX86
static inline unsigned
set_fpu_cw(unsigned x)
{
	unsigned short t;
	unsigned old;

	__asm { fstcw t }
	old = (t & 0x0300u) >> 8;
	t = (unsigned short)((t & ~0x0300u) | (x << 8));
	__asm { fldcw t }
	return old;
}
#else
static inline unsigned
set_fpu_cw(unsigned x)
{
	return x;
}
#endif
#else  // yyyFPNATIVE+0
static inline unsigned
set_fpu_cw(unsigned x)
{
	return x;
}
#endif  // yyyFPNATIVE-

#if FALCON_FPNATIVE && !FALCON_AVX2  // yyyFPNATIVE+1 yyyAVX2+0
/*
 * If using the native 'double' type but not AVX2 code, on an x86
 * machine with SSE2 activated for maths, then we will use the
 * SSE2 intrinsics.
 */
#if defined __GNUC__ && defined __SSE2_MATH__
#include <immintrin.h>
#endif
#endif  // yyyFPNATIVE- yyyAVX2-

#if FALCON_FPNATIVE  // yyyFPNATIVE+1
/*
 * For optimal reproducibility of values, we need to disable contraction
 * of floating-point expressions; otherwise, on some architectures (e.g.
 * PowerPC), the compiler may generate fused-multiply-add opcodes that
 * may round differently than two successive separate opcodes. C99 defines
 * a standard pragma for that, but GCC-6.2.2 appears to ignore it,
 * hence the GCC-specific pragma (that Clang does not support).
 */
#if defined __clang__
#pragma STDC FP_CONTRACT OFF
#elif defined __GNUC__
#pragma GCC optimize ("fp-contract=off")
#endif
#endif  // yyyFPNATIVE-

// yyyPQCLEAN+0
/*
 * MSVC 2015 does not know the C99 keyword 'restrict'.
 */
#if defined _MSC_VER && _MSC_VER
#ifndef restrict
#define restrict   __restrict
#endif
#endif
// yyyPQCLEAN-

/* ==================================================================== */
/*
 * SHAKE256 implementation (shake.c).
 *
 * API is defined to be easily replaced with the fips202.h API defined
 * as part of PQClean.
 */

// yyyPQCLEAN+0
typedef struct {
	union {
		uint64_t A[25];
		uint8_t dbuf[200];
	} st;
	uint64_t dptr;
} inner_shake256_context;

#define inner_shake256_init      Zf(i_shake256_init)
#define inner_shake256_inject    Zf(i_shake256_inject)
#define inner_shake256_flip      Zf(i_shake256_flip)
#define inner_shake256_extract   Zf(i_shake256_extract)

void Zf(i_shake256_init)(
	inner_shake256_context *sc);
void Zf(i_shake256_inject)(
	inner_shake256_context *sc, const uint8_t *in, size_t len);
void Zf(i_shake256_flip)(
	inner_shake256_context *sc);
void Zf(i_shake256_extract)(
	inner_shake256_context *sc, uint8_t *out, size_t len);

/*
// yyyPQCLEAN+1

#include "fips202.h"

#define inner_shake256_context                shake256incctx
#define inner_shake256_init(sc)               shake256_inc_init(sc)
#define inner_shake256_inject(sc, in, len)    shake256_inc_absorb(sc, in, len)
#define inner_shake256_flip(sc)               shake256_inc_finalize(sc)
#define inner_shake256_extract(sc, out, len)  shake256_inc_squeeze(out, len, sc)

// yyyPQCLEAN+0
 */
// yyyPQCLEAN-

/* ==================================================================== */
/*
 * Encoding/decoding functions (codec.c).
 *
 * Encoding functions take as parameters an output buffer (out) with
 * a given maximum length (max_out_len); returned value is the actual
 * number of bytes which have been written. If the output buffer is
 * not large enough, then 0 is returned (some bytes may have been
 * written to the buffer). If 'out' is NULL, then 'max_out_len' is
 * ignored; instead, the function computes and returns the actual
 * required output length (in bytes).
 *
 * Decoding functions take as parameters an input buffer (in) with
 * its maximum length (max_in_len); returned value is the actual number
 * of bytes that have been read from the buffer. If the provided length
 * is too short, then 0 is returned.
 *
 * Values to encode or decode are vectors of integers, with N = 2^logn
 * elements.
 *
 * Three encoding formats are defined:
 *
 *   - modq: sequence of values modulo 12289, each encoded over exactly
 *     14 bits. The encoder and decoder verify that integers are within
 *     the valid range (0..12288). Values are arrays of uint16.
 *
 *   - trim: sequence of signed integers, a specified number of bits
 *     each. The number of bits is provided as parameter and includes
 *     the sign bit. Each integer x must be such that |x| < 2^(bits-1)
 *     (which means that the -2^(bits-1) value is forbidden); encode and
 *     decode functions check that property. Values are arrays of
 *     int16_t or int8_t, corresponding to names 'trim_i16' and
 *     'trim_i8', respectively.
 *
 *   - comp: variable-length encoding for signed integers; each integer
 *     uses a minimum of 9 bits, possibly more. This is normally used
 *     only for signatures.
 *
 */

size_t Zf(modq_encode)(void *out, size_t max_out_len,
	const uint16_t *x, unsigned logn);
size_t Zf(trim_i16_encode)(void *out, size_t max_out_len,
	const int16_t *x, unsigned logn, unsigned bits);
size_t Zf(trim_i8_encode)(void *out, size_t max_out_len,
	const int8_t *x, unsigned logn, unsigned bits);
size_t Zf(comp_encode)(void *out, size_t max_out_len,
	const int16_t *x, unsigned logn);

size_t Zf(modq_decode)(uint16_t *x, unsigned logn,
	const void *in, size_t max_in_len);
size_t Zf(trim_i16_decode)(int16_t *x, unsigned logn, unsigned bits,
	const void *in, size_t max_in_len);
size_t Zf(trim_i8_decode)(int8_t *x, unsigned logn, unsigned bits,
	const void *in, size_t max_in_len);
size_t Zf(comp_decode)(int16_t *x, unsigned logn,
	const void *in, size_t max_in_len);

/*
 * Number of bits for key elements, indexed by logn (1 to 10). This
 * is at most 8 bits for all degrees, but some degrees may have shorter
 * elements.
 */
extern const uint8_t Zf(max_fg_bits)[];
extern const uint8_t Zf(max_FG_bits)[];

/*
 * Maximum size, in bits, of elements in a signature, indexed by logn
 * (1 to 10). The size includes the sign bit.
 */
extern const uint8_t Zf(max_sig_bits)[];

/* ==================================================================== */
/*
 * Support functions used for both signature generation and signature
 * verification (common.c).
 */

/*
 * From a SHAKE256 context (must be already flipped), produce a new
 * point. This is the non-constant-time version, which may leak enough
 * information to serve as a stop condition on a brute force attack on
 * the hashed message (provided that the nonce value is known).
 */
void Zf(hash_to_point_vartime)(inner_shake256_context *sc,
	uint16_t *x, unsigned logn);

void Zf(hash_to_point_vartime_check1)(inner_shake256_context *sc,
	uint16_t *x, unsigned logn);


void Zf(hash_to_point_vartime_check)(inner_shake256_context *sc,
	uint32_t *x, unsigned logn);

/*
 * From a SHAKE256 context (must be already flipped), produce a new
 * point. The temporary buffer (tmp) must have room for 2*2^logn bytes.
 * This function is constant-time but is typically more expensive than
 * Zf(hash_to_point_vartime)().
 *
 * tmp[] must have 16-bit alignment.
 */
void Zf(hash_to_point_ct)(inner_shake256_context *sc,
	uint16_t *x, unsigned logn, uint8_t *tmp);

/*
 * Tell whether a given vector (2N coordinates, in two halves) is
 * acceptable as a signature. This compares the appropriate norm of the
 * vector with the acceptance bound. Returned value is 1 on success
 * (vector is short enough to be acceptable), 0 otherwise.
 */
int Zf(is_short)(const int16_t *s1, const int16_t *s2, unsigned logn);

/*
 * Tell whether a given vector (2N coordinates, in two halves) is
 * acceptable as a signature. Instead of the first half s1, this
 * function receives the "saturated squared norm" of s1, i.e. the
 * sum of the squares of the coordinates of s1 (saturated at 2^32-1
 * if the sum exceeds 2^31-1).
 *
 * Returned value is 1 on success (vector is short enough to be
 * acceptable), 0 otherwise.
 */
int Zf(is_short_half)(uint32_t sqn, const int16_t *s2, unsigned logn);

/* ==================================================================== */
/*
 * Signature verification functions (vrfy.c).
 */

/*
 * Convert a public key to NTT + Montgomery format. Conversion is done
 * in place.
 */
void Zf(to_ntt_monty)(uint16_t *h, unsigned logn);

/*
 * Internal signature verification code:
 *   c0[]      contains the hashed nonce+message
 *   s2[]      is the decoded signature
 *   h[]       contains the public key, in NTT + Montgomery format
 *   logn      is the degree log
 *   tmp[]     temporary, must have at least 2*2^logn bytes
 * Returned value is 1 on success, 0 on error.
 *
 * tmp[] must have 16-bit alignment.
 */
int Zf(verify_raw)(const uint16_t *c0, const int16_t *s2,
	const uint16_t *h, unsigned logn, uint8_t *tmp);

int Zf(verify_raw_check)(const uint32_t *c0, const int16_t *s2,
	const uint16_t *h, unsigned logn, uint8_t *tmp);

/*
 * Compute the public key h[], given the private key elements f[] and
 * g[]. This computes h = g/f mod phi mod q, where phi is the polynomial
 * modulus. This function returns 1 on success, 0 on error (an error is
 * reported if f is not invertible mod phi mod q).
 *
 * The tmp[] array must have room for at least 2*2^logn elements.
 * tmp[] must have 16-bit alignment.
 */
int Zf(compute_public)(uint16_t *h,
	const int8_t *f, const int8_t *g, unsigned logn, uint8_t *tmp);

int Zf(compute_public_check)(uint32_t *h,
	const int8_t *f, const int8_t *g, unsigned logn, uint8_t *tmp);

/*
 * Recompute the fourth private key element. Private key consists in
 * four polynomials with small coefficients f, g, F and G, which are
 * such that fG - gF = q mod phi; furthermore, f is invertible modulo
 * phi and modulo q. This function recomputes G from f, g and F.
 *
 * The tmp[] array must have room for at least 4*2^logn bytes.
 *
 * Returned value is 1 in success, 0 on error (f not invertible).
 * tmp[] must have 16-bit alignment.
 */
int Zf(complete_private)(int8_t *G,
	const int8_t *f, const int8_t *g, const int8_t *F,
	unsigned logn, uint8_t *tmp);


int Zf(complete_private_check)(int8_t *G,
	const int8_t *f, const int8_t *g, const int8_t *F,
	unsigned logn, uint8_t *tmp);


/*
 * Test whether a given polynomial is invertible modulo phi and q.
 * Polynomial coefficients are small integers.
 *
 * tmp[] must have 16-bit alignment.
 */
int Zf(is_invertible)(
	const int16_t *s2, unsigned logn, uint8_t *tmp);

/*
 * Count the number of elements of value zero in the NTT representation
 * of the given polynomial: this is the number of primitive 2n-th roots
 * of unity (modulo q = 12289) that are roots of the provided polynomial
 * (taken modulo q).
 *
 * tmp[] must have 16-bit alignment.
 */
int Zf(count_nttzero)(const int16_t *sig, unsigned logn, uint8_t *tmp);

/*
 * Internal signature verification with public key recovery:
 *   h[]       receives the public key (NOT in NTT/Montgomery format)
 *   c0[]      contains the hashed nonce+message
 *   s1[]      is the first signature half
 *   s2[]      is the second signature half
 *   logn      is the degree log
 *   tmp[]     temporary, must have at least 2*2^logn bytes
 * Returned value is 1 on success, 0 on error. Success is returned if
 * the signature is a short enough vector; in that case, the public
 * key has been written to h[]. However, the caller must still
 * verify that h[] is the correct value (e.g. with regards to a known
 * hash of the public key).
 *
 * h[] may not overlap with any of the other arrays.
 *
 * tmp[] must have 16-bit alignment.
 */
int Zf(verify_recover)(uint16_t *h,
	const uint16_t *c0, const int16_t *s1, const int16_t *s2,
	unsigned logn, uint8_t *tmp);

/* ==================================================================== */
/*
 * Implementation of floating-point real numbers (fpr.h, fpr.c).
 */

/*
 * Real numbers are implemented by an extra header file, included below.
 * This is meant to support pluggable implementations. The default
 * implementation relies on the C type 'double'.
 *
 * The included file must define the following types, functions and
 * constants:
 *
 *   fpr
 *         type for a real number
 *
 *   fpr fpr_of(int64_t i)
 *         cast an integer into a real number; source must be in the
 *         -(2^63-1)..+(2^63-1) range
 *
 *   fpr fpr_scaled(int64_t i, int sc)
 *         compute i*2^sc as a real number; source 'i' must be in the
 *         -(2^63-1)..+(2^63-1) range
 *
 *   fpr fpr_ldexp(fpr x, int e)
 *         compute x*2^e
 *
 *   int64_t fpr_rint(fpr x)
 *         round x to the nearest integer; x must be in the -(2^63-1)
 *         to +(2^63-1) range
 *
 *   int64_t fpr_trunc(fpr x)
 *         round to an integer; this rounds towards zero; value must
 *         be in the -(2^63-1) to +(2^63-1) range
 *
 *   fpr fpr_add(fpr x, fpr y)
 *         compute x + y
 *
 *   fpr fpr_sub(fpr x, fpr y)
 *         compute x - y
 *
 *   fpr fpr_neg(fpr x)
 *         compute -x
 *
 *   fpr fpr_half(fpr x)
 *         compute x/2
 *
 *   fpr fpr_double(fpr x)
 *         compute x*2
 *
 *   fpr fpr_mul(fpr x, fpr y)
 *         compute x * y
 *
 *   fpr fpr_sqr(fpr x)
 *         compute x * x
 *
 *   fpr fpr_inv(fpr x)
 *         compute 1/x
 *
 *   fpr fpr_div(fpr x, fpr y)
 *         compute x/y
 *
 *   fpr fpr_sqrt(fpr x)
 *         compute the square root of x
 *
 *   int fpr_lt(fpr x, fpr y)
 *         return 1 if x < y, 0 otherwise
 *
 *   uint64_t fpr_expm_p63(fpr x)
 *         return exp(x), assuming that 0 <= x < log(2). Returned value
 *         is scaled to 63 bits (i.e. it really returns 2^63*exp(-x),
 *         rounded to the nearest integer). Computation should have a
 *         precision of at least 45 bits.
 *
 *   const fpr fpr_gm_tab[]
 *         array of constants for FFT / iFFT
 *
 *   const fpr fpr_p2_tab[]
 *         precomputed powers of 2 (by index, 0 to 10)
 *
 * Constants of type 'fpr':
 *
 *   fpr fpr_q                 12289
 *   fpr fpr_inverse_of_q      1/12289
 *   fpr fpr_inv_2sqrsigma0    1/(2*(1.8205^2))
 *   fpr fpr_inv_sigma[]       1/sigma (indexed by logn, 1 to 10)
 *   fpr fpr_sigma_min[]       1/sigma_min (indexed by logn, 1 to 10)
 *   fpr fpr_log2              log(2)
 *   fpr fpr_inv_log2          1/log(2)
 *   fpr fpr_bnorm_max         16822.4121
 *   fpr fpr_zero              0
 *   fpr fpr_one               1
 *   fpr fpr_two               2
 *   fpr fpr_onehalf           0.5
 *   fpr fpr_ptwo31            2^31
 *   fpr fpr_ptwo31m1          2^31-1
 *   fpr fpr_mtwo31m1          -(2^31-1)
 *   fpr fpr_ptwo63m1          2^63-1
 *   fpr fpr_mtwo63m1          -(2^63-1)
 *   fpr fpr_ptwo63            2^63
 */
#include "fpr.h"

/* ==================================================================== */
/*
 * RNG (rng.c).
 *
 * A PRNG based on ChaCha20 is implemented; it is seeded from a SHAKE256
 * context (flipped) and is used for bulk pseudorandom generation.
 * A system-dependent seed generator is also provided.
 */

/*
 * Obtain a random seed from the system RNG.
 *
 * Returned value is 1 on success, 0 on error.
 */
int Zf(get_seed)(void *seed, size_t seed_len);

/*
 * Structure for a PRNG. This includes a large buffer so that values
 * get generated in advance. The 'state' is used to keep the current
 * PRNG algorithm state (contents depend on the selected algorithm).
 *
 * The unions with 'dummy_u64' are there to ensure proper alignment for
 * 64-bit direct access.
 */
typedef struct {
	union {
		uint8_t d[512]; /* MUST be 512, exactly */
		uint64_t dummy_u64;
	} buf;
	size_t ptr;
	union {
		uint8_t d[256];
		uint64_t dummy_u64;
	} state;
	int type;
} prng;

/*
 * Instantiate a PRNG. That PRNG will feed over the provided SHAKE256
 * context (in "flipped" state) to obtain its initial state.
 */
void Zf(prng_init)(prng *p, inner_shake256_context *src);

/*
 * Refill the PRNG buffer. This is normally invoked automatically, and
 * is declared here only so that prng_get_u64() may be inlined.
 */
void Zf(prng_refill)(prng *p);

/*
 * Get some bytes from a PRNG.
 */
void Zf(prng_get_bytes)(prng *p, void *dst, size_t len);

/*
 * Get a 64-bit random value from a PRNG.
 */
static inline uint64_t
prng_get_u64(prng *p)
{
	size_t u;

	/*
	 * If there are less than 9 bytes in the buffer, we refill it.
	 * This means that we may drop the last few bytes, but this allows
	 * for faster extraction code. Also, it means that we never leave
	 * an empty buffer.
	 */
	u = p->ptr;
	if (u >= (sizeof p->buf.d) - 9) {
		Zf(prng_refill)(p);
		u = 0;
	}
	p->ptr = u + 8;

	/*
	 * On systems that use little-endian encoding and allow
	 * unaligned accesses, we can simply read the data where it is.
	 */
#if FALCON_LE && FALCON_UNALIGNED  // yyyLEU+1
	return *(uint64_t *)(p->buf.d + u);
#else  // yyyLEU+0
	return (uint64_t)p->buf.d[u + 0]
		| ((uint64_t)p->buf.d[u + 1] << 8)
		| ((uint64_t)p->buf.d[u + 2] << 16)
		| ((uint64_t)p->buf.d[u + 3] << 24)
		| ((uint64_t)p->buf.d[u + 4] << 32)
		| ((uint64_t)p->buf.d[u + 5] << 40)
		| ((uint64_t)p->buf.d[u + 6] << 48)
		| ((uint64_t)p->buf.d[u + 7] << 56);
#endif  // yyyLEU-
}

/*
 * Get an 8-bit random value from a PRNG.
 */
static inline unsigned
prng_get_u8(prng *p)
{
	unsigned v;

	v = p->buf.d[p->ptr ++];
	if (p->ptr == sizeof p->buf.d) {
		Zf(prng_refill)(p);
	}
	return v;
}

/* ==================================================================== */
/*
 * FFT (falcon-fft.c).
 *
 * A real polynomial is represented as an array of N 'fpr' elements.
 * The FFT representation of a real polynomial contains N/2 complex
 * elements; each is stored as two real numbers, for the real and
 * imaginary parts, respectively. See falcon-fft.c for details on the
 * internal representation.
 */

/*
 * Compute FFT in-place: the source array should contain a real
 * polynomial (N coefficients); its storage area is reused to store
 * the FFT representation of that polynomial (N/2 complex numbers).
 *
 * 'logn' MUST lie between 1 and 10 (inclusive).
 */
void Zf(FFT)(fpr *f, unsigned logn);

/*
 * Compute the inverse FFT in-place: the source array should contain the
 * FFT representation of a real polynomial (N/2 elements); the resulting
 * real polynomial (N coefficients of type 'fpr') is written over the
 * array.
 *
 * 'logn' MUST lie between 1 and 10 (inclusive).
 */
void Zf(iFFT)(fpr *f, unsigned logn);

/*
 * Add polynomial b to polynomial a. a and b MUST NOT overlap. This
 * function works in both normal and FFT representations.
 */
void Zf(poly_add)(fpr* __restrict__a, const fpr* __restrict__b, unsigned logn);

/*
 * Subtract polynomial b from polynomial a. a and b MUST NOT overlap. This
 * function works in both normal and FFT representations.
 */
void Zf(poly_sub)(fpr* __restrict__a, const fpr* __restrict__b, unsigned logn);

/*
 * Negate polynomial a. This function works in both normal and FFT
 * representations.
 */
void Zf(poly_neg)(fpr *a, unsigned logn);

/*
 * Compute adjoint of polynomial a. This function works only in FFT
 * representation.
 */
void Zf(poly_adj_fft)(fpr *a, unsigned logn);

/*
 * Multiply polynomial a with polynomial b. a and b MUST NOT overlap.
 * This function works only in FFT representation.
 */
void Zf(poly_mul_fft)(fpr* __restrict__a, const fpr* __restrict__b, unsigned logn);

/*
 * Multiply polynomial a with the adjoint of polynomial b. a and b MUST NOT
 * overlap. This function works only in FFT representation.
 */
void Zf(poly_muladj_fft)(fpr* __restrict__a, const fpr* __restrict__b, unsigned logn);

/*
 * Multiply polynomial with its own adjoint. This function works only in FFT
 * representation.
 */
void Zf(poly_mulselfadj_fft)(fpr *a, unsigned logn);

/*
 * Multiply polynomial with a real constant. This function works in both
 * normal and FFT representations.
 */
void Zf(poly_mulconst)(fpr *a, fpr x, unsigned logn);

/*
 * Divide polynomial a by polynomial b, modulo X^N+1 (FFT representation).
 * a and b MUST NOT overlap.
 */
void Zf(poly_div_fft)(fpr* __restrict__a, const fpr* __restrict__b, unsigned logn);

/*
 * Given f and g (in FFT representation), compute 1/(f*adj(f)+g*adj(g))
 * (also in FFT representation). Since the result is auto-adjoint, all its
 * coordinates in FFT representation are real; as such, only the first N/2
 * values of d[] are filled (the imaginary parts are skipped).
 *
 * Array d MUST NOT overlap with either a or b.
 */
void Zf(poly_invnorm2_fft)(fpr* __restrict__d,
	const fpr* __restrict__a, const fpr* __restrict__b, unsigned logn);

/*
 * Given F, G, f and g (in FFT representation), compute F*adj(f)+G*adj(g)
 * (also in FFT representation). Destination d MUST NOT overlap with
 * any of the source arrays.
 */
void Zf(poly_add_muladj_fft)(fpr* __restrict__d,
	const fpr* __restrict__F, const fpr* __restrict__G,
	const fpr* __restrict__f, const fpr* __restrict__g, unsigned logn);

/*
 * Multiply polynomial a by polynomial b, where b is autoadjoint. Both
 * a and b are in FFT representation. Since b is autoadjoint, all its
 * FFT coefficients are real, and the array b contains only N/2 elements.
 * a and b MUST NOT overlap.
 */
void Zf(poly_mul_autoadj_fft)(fpr* __restrict__a,
	const fpr* __restrict__b, unsigned logn);

/*
 * Divide polynomial a by polynomial b, where b is autoadjoint. Both
 * a and b are in FFT representation. Since b is autoadjoint, all its
 * FFT coefficients are real, and the array b contains only N/2 elements.
 * a and b MUST NOT overlap.
 */
void Zf(poly_div_autoadj_fft)(fpr* __restrict__a,
	const fpr* __restrict__b, unsigned logn);

/*
 * Perform an LDL decomposition of an auto-adjoint matrix G, in FFT
 * representation. On input, g00, g01 and g11 are provided (where the
 * matrix G = [[g00, g01], [adj(g01), g11]]). On output, the d00, l10
 * and d11 values are written in g00, g01 and g11, respectively
 * (with D = [[d00, 0], [0, d11]] and L = [[1, 0], [l10, 1]]).
 * (In fact, d00 = g00, so the g00 operand is left unmodified.)
 */
void Zf(poly_LDL_fft)(const fpr* __restrict__g00,
	fpr* __restrict__g01, fpr* __restrict__g11, unsigned logn);

/*
 * Perform an LDL decomposition of an auto-adjoint matrix G, in FFT
 * representation. This is identical to poly_LDL_fft() except that
 * g00, g01 and g11 are unmodified; the outputs d11 and l10 are written
 * in two other separate buffers provided as extra parameters.
 */
void Zf(poly_LDLmv_fft)(fpr* __restrict__d11, fpr* __restrict__l10,
	const fpr* __restrict__g00, const fpr* __restrict__g01,
	const fpr* __restrict__g11, unsigned logn);

/*
 * Apply "split" operation on a polynomial in FFT representation:
 * f = f0(x^2) + x*f1(x^2), for half-size polynomials f0 and f1
 * (polynomials modulo X^(N/2)+1). f0, f1 and f MUST NOT overlap.
 */
void Zf(poly_split_fft)(fpr* __restrict__f0, fpr* __restrict__f1,
	const fpr* __restrict__f, unsigned logn);

/*
 * Apply "merge" operation on two polynomials in FFT representation:
 * given f0 and f1, polynomials moduo X^(N/2)+1, this function computes
 * f = f0(x^2) + x*f1(x^2), in FFT representation modulo X^N+1.
 * f MUST NOT overlap with either f0 or f1.
 */
void Zf(poly_merge_fft)(fpr*__restrict__f,
	const fpr*__restrict__f0, const fpr*__restrict__f1, unsigned logn);

/* ==================================================================== */
/*
 * Key pair generation.
 */

/*
 * Required sizes of the temporary buffer (in bytes).
 *
 * This size is 28*2^logn bytes, except for degrees 2 and 4 (logn = 1
 * or 2) where it is slightly greater.
 */
#define FALCON_KEYGEN_TEMP_1      136
#define FALCON_KEYGEN_TEMP_2      272
#define FALCON_KEYGEN_TEMP_3      224
#define FALCON_KEYGEN_TEMP_4      448
#define FALCON_KEYGEN_TEMP_5      896
#define FALCON_KEYGEN_TEMP_6     1792
#define FALCON_KEYGEN_TEMP_7     3584
#define FALCON_KEYGEN_TEMP_8     7168
#define FALCON_KEYGEN_TEMP_9    14336
#define FALCON_KEYGEN_TEMP_10   28672

/*
 * Generate a new key pair. Randomness is extracted from the provided
 * SHAKE256 context, which must have already been seeded and flipped.
 * The tmp[] array must have suitable size (see FALCON_KEYGEN_TEMP_*
 * macros) and be aligned for the uint32_t, uint64_t and fpr types.
 *
 * The private key elements are written in f, g, F and G, and the
 * public key is written in h. Either or both of G and h may be NULL,
 * in which case the corresponding element is not returned (they can
 * be recomputed from f, g and F).
 *
 * tmp[] must have 64-bit alignment.
 * This function uses floating-point rounding (see set_fpu_cw()).
 */
void Zf(keygen)(inner_shake256_context *rng,
	int8_t *f, int8_t *g, int8_t *F, int8_t *G, uint16_t *h,
	unsigned logn, uint8_t *tmp);


void Zf(keygen_check)(inner_shake256_context *rng,
	int8_t *f, int8_t *g, int8_t *F, int8_t *G, uint32_t *h,
	unsigned logn, uint8_t *tmp);

/* ==================================================================== */
/*
 * Signature generation.
 */

/*
 * Expand a private key into the B0 matrix in FFT representation and
 * the LDL tree. All the values are written in 'expanded_key', for
 * a total of (8*logn+40)*2^logn bytes.
 *
 * The tmp[] array must have room for at least 48*2^logn bytes.
 *
 * tmp[] must have 64-bit alignment.
 * This function uses floating-point rounding (see set_fpu_cw()).
 */
void Zf(expand_privkey)(fpr* __restrict__expanded_key,
	const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
	unsigned logn, uint8_t* __restrict__tmp);

/*
 * Compute a signature over the provided hashed message (hm); the
 * signature value is one short vector. This function uses an
 * expanded key (as generated by Zf(expand_privkey)()).
 *
 * The sig[] and hm[] buffers may overlap.
 *
 * On successful output, the start of the tmp[] buffer contains the s1
 * vector (as int16_t elements).
 *
 * The minimal size (in bytes) of tmp[] is 48*2^logn bytes.
 *
 * tmp[] must have 64-bit alignment.
 * This function uses floating-point rounding (see set_fpu_cw()).
 */
void Zf(sign_tree)(int16_t *sig, inner_shake256_context *rng,
	const fpr*__restrict__expanded_key,
	const uint16_t *hm, unsigned logn, uint8_t *tmp);


void Zf(sign_tree_check)(int16_t *sig, inner_shake256_context *rng,
	const fpr*__restrict__expanded_key,
	const uint32_t *hm, unsigned logn, uint8_t *tmp);

/*
 * Compute a signature over the provided hashed message (hm); the
 * signature value is one short vector. This function uses a raw
 * key and dynamically recompute the B0 matrix and LDL tree; this
 * saves RAM since there is no needed for an expanded key, but
 * increases the signature cost.
 *
 * The sig[] and hm[] buffers may overlap.
 *
 * On successful output, the start of the tmp[] buffer contains the s1
 * vector (as int16_t elements).
 *
 * The minimal size (in bytes) of tmp[] is 72*2^logn bytes.
 *
 * tmp[] must have 64-bit alignment.
 * This function uses floating-point rounding (see set_fpu_cw()).
 */
void Zf(sign_dyn)(int16_t *sig, inner_shake256_context *rng,
	const int8_t* __restrict__f, const int8_t* __restrict__g,
	const int8_t* __restrict__F, const int8_t* __restrict__G,
	const uint16_t *hm, unsigned logn, uint8_t *tmp);

/*
 * Internal sampler engine. Exported for tests.
 *
 * sampler_context wraps around a source of random numbers (PRNG) and
 * the sigma_min value (nominally dependent on the degree).
 *
 * sampler() takes as parameters:
 *   ctx      pointer to the sampler_context structure
 *   mu       center for the distribution
 *   isigma   inverse of the distribution standard deviation
 * It returns an integer sampled along the Gaussian distribution centered
 * on mu and of standard deviation sigma = 1/isigma.
 *
 * gaussian0_sampler() takes as parameter a pointer to a PRNG, and
 * returns an integer sampled along a half-Gaussian with standard
 * deviation sigma0 = 1.8205 (center is 0, returned value is
 * nonnegative).
 */

typedef struct {
	prng p;
	fpr sigma_min;
} sampler_context;

TARGET_AVX2
int Zf(sampler)(void *ctx, fpr mu, fpr isigma);

TARGET_AVX2
int Zf(gaussian0_sampler)(prng *p);

/* ==================================================================== */

#endif
