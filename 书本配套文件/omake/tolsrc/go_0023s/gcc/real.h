/* Definitions of floating-point access for GNU compiler.
   Copyright (C) 1989, 1991, 1994, 1996, 1997, 1998,
   1999, 2000, 2002 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#ifndef GCC_REAL_H
#define GCC_REAL_H

/* Define codes for all the float formats that we know of.  */
#define UNKNOWN_FLOAT_FORMAT 0
#define IEEE_FLOAT_FORMAT 1
#define VAX_FLOAT_FORMAT 2
#define IBM_FLOAT_FORMAT 3
#define C4X_FLOAT_FORMAT 4

/* Default to IEEE float if not specified.  Nearly all machines use it.  */

#ifndef TARGET_FLOAT_FORMAT
#define	TARGET_FLOAT_FORMAT	IEEE_FLOAT_FORMAT
#endif

#ifndef HOST_FLOAT_FORMAT
#define	HOST_FLOAT_FORMAT	IEEE_FLOAT_FORMAT
#endif

#ifndef INTEL_EXTENDED_IEEE_FORMAT
#define INTEL_EXTENDED_IEEE_FORMAT 0
#endif

#if TARGET_FLOAT_FORMAT == IEEE_FLOAT_FORMAT
#define REAL_INFINITY
#endif

/* If FLOAT_WORDS_BIG_ENDIAN and HOST_FLOAT_WORDS_BIG_ENDIAN are not defined
   in the header files, then this implies the word-endianness is the same as
   for integers.  */

/* This is defined 0 or 1, like WORDS_BIG_ENDIAN.  */
#ifndef FLOAT_WORDS_BIG_ENDIAN
#define FLOAT_WORDS_BIG_ENDIAN WORDS_BIG_ENDIAN
#endif

/* This is defined 0 or 1, unlike HOST_WORDS_BIG_ENDIAN.  */
#ifndef HOST_FLOAT_WORDS_BIG_ENDIAN
#ifdef HOST_WORDS_BIG_ENDIAN
#define HOST_FLOAT_WORDS_BIG_ENDIAN 1
#else
#define HOST_FLOAT_WORDS_BIG_ENDIAN 0
#endif
#endif

/* Defining REAL_ARITHMETIC invokes a floating point emulator
   that can produce a target machine format differing by more
   than just endian-ness from the host's format.  The emulator
   is also used to support extended real XFmode.  */
#ifndef LONG_DOUBLE_TYPE_SIZE
#define LONG_DOUBLE_TYPE_SIZE 64
#endif
/* MAX_LONG_DOUBLE_TYPE_SIZE is a constant tested by #if.
   LONG_DOUBLE_TYPE_SIZE can vary at compiler run time.
   So long as macros like REAL_VALUE_TO_TARGET_LONG_DOUBLE cannot
   vary too, however, then XFmode and TFmode long double
   cannot both be supported at the same time.  */
#ifndef MAX_LONG_DOUBLE_TYPE_SIZE
#define MAX_LONG_DOUBLE_TYPE_SIZE LONG_DOUBLE_TYPE_SIZE
#endif
#if (MAX_LONG_DOUBLE_TYPE_SIZE == 96) || (MAX_LONG_DOUBLE_TYPE_SIZE == 128)
#ifndef REAL_ARITHMETIC
#define REAL_ARITHMETIC
#endif
#endif
#ifdef REAL_ARITHMETIC
/* **** Start of software floating point emulator interface macros **** */

/* Support 80-bit extended real XFmode if LONG_DOUBLE_TYPE_SIZE
   has been defined to be 96 in the tm.h machine file.  */
#if (MAX_LONG_DOUBLE_TYPE_SIZE == 96)
#define REAL_IS_NOT_DOUBLE
#define REAL_ARITHMETIC
typedef struct {
  HOST_WIDE_INT r[(11 + sizeof (HOST_WIDE_INT))/(sizeof (HOST_WIDE_INT))];
} realvaluetype;
#define REAL_VALUE_TYPE realvaluetype

#else /* no XFmode support */

#if (MAX_LONG_DOUBLE_TYPE_SIZE == 128)

#define REAL_IS_NOT_DOUBLE
#define REAL_ARITHMETIC
typedef struct {
  HOST_WIDE_INT r[(19 + sizeof (HOST_WIDE_INT))/(sizeof (HOST_WIDE_INT))];
} realvaluetype;
#define REAL_VALUE_TYPE realvaluetype

#else /* not TFmode */

#if HOST_FLOAT_FORMAT != TARGET_FLOAT_FORMAT
/* If no XFmode support, then a REAL_VALUE_TYPE is 64 bits wide
   but it is not necessarily a host machine double.  */
#define REAL_IS_NOT_DOUBLE
typedef struct {
  HOST_WIDE_INT r[(7 + sizeof (HOST_WIDE_INT))/(sizeof (HOST_WIDE_INT))];
} realvaluetype;
#define REAL_VALUE_TYPE realvaluetype
#else
/* If host and target formats are compatible, then a REAL_VALUE_TYPE
   is actually a host machine double.  */
#define REAL_VALUE_TYPE double
#endif

#endif /* no TFmode support */
#endif /* no XFmode support */

extern unsigned int significand_size	PARAMS ((enum machine_mode));

/* If emulation has been enabled by defining REAL_ARITHMETIC or by
   setting LONG_DOUBLE_TYPE_SIZE to 96 or 128, then define macros so that
   they invoke emulator functions. This will succeed only if the machine
   files have been updated to use these macros in place of any
   references to host machine `double' or `float' types.  */
#ifdef REAL_ARITHMETIC
#undef REAL_ARITHMETIC
#define REAL_ARITHMETIC(value, code, d1, d2) \
  earith (&(value), (code), &(d1), &(d2))

/* Declare functions in real.c.  */
extern void earith		PARAMS ((REAL_VALUE_TYPE *, int,
				       REAL_VALUE_TYPE *, REAL_VALUE_TYPE *));
extern REAL_VALUE_TYPE etrunci	PARAMS ((REAL_VALUE_TYPE));
extern REAL_VALUE_TYPE etruncui	PARAMS ((REAL_VALUE_TYPE));
extern REAL_VALUE_TYPE ereal_negate PARAMS ((REAL_VALUE_TYPE));
extern HOST_WIDE_INT efixi	PARAMS ((REAL_VALUE_TYPE));
extern unsigned HOST_WIDE_INT efixui PARAMS ((REAL_VALUE_TYPE));
extern void ereal_from_int	PARAMS ((REAL_VALUE_TYPE *,
				       HOST_WIDE_INT, HOST_WIDE_INT,
				       enum machine_mode));
extern void ereal_from_uint	PARAMS ((REAL_VALUE_TYPE *,
				       unsigned HOST_WIDE_INT,
				       unsigned HOST_WIDE_INT,
				       enum machine_mode));
extern void ereal_to_int	PARAMS ((HOST_WIDE_INT *, HOST_WIDE_INT *,
				       REAL_VALUE_TYPE));
extern REAL_VALUE_TYPE ereal_ldexp PARAMS ((REAL_VALUE_TYPE, int));

extern void etartdouble		PARAMS ((REAL_VALUE_TYPE, long *));
extern void etarldouble		PARAMS ((REAL_VALUE_TYPE, long *));
extern void etardouble		PARAMS ((REAL_VALUE_TYPE, long *));
extern long etarsingle		PARAMS ((REAL_VALUE_TYPE));
extern void ereal_to_decimal	PARAMS ((REAL_VALUE_TYPE, char *));
extern int ereal_cmp		PARAMS ((REAL_VALUE_TYPE, REAL_VALUE_TYPE));
extern int ereal_isneg		PARAMS ((REAL_VALUE_TYPE));
extern REAL_VALUE_TYPE ereal_unto_float PARAMS ((long));
extern REAL_VALUE_TYPE ereal_unto_double PARAMS ((long *));
extern REAL_VALUE_TYPE ereal_from_float PARAMS ((HOST_WIDE_INT));
extern REAL_VALUE_TYPE ereal_from_double PARAMS ((HOST_WIDE_INT *));

#define REAL_VALUES_EQUAL(x, y) (ereal_cmp ((x), (y)) == 0)
/* true if x < y : */
#define REAL_VALUES_LESS(x, y) (ereal_cmp ((x), (y)) == -1)
#define REAL_VALUE_LDEXP(x, n) ereal_ldexp (x, n)

/* These return REAL_VALUE_TYPE: */
#define REAL_VALUE_RNDZINT(x) (etrunci (x))
#define REAL_VALUE_UNSIGNED_RNDZINT(x) (etruncui (x))
#define REAL_VALUE_TRUNCATE(mode, x)  real_value_truncate (mode, x)

/* These return HOST_WIDE_INT: */
/* Convert a floating-point value to integer, rounding toward zero.  */
#define REAL_VALUE_FIX(x) (efixi (x))
/* Convert a floating-point value to unsigned integer, rounding
   toward zero.  */
#define REAL_VALUE_UNSIGNED_FIX(x) (efixui (x))

/* Convert ASCII string S to floating point in mode M.
   Decimal input uses ATOF.  Hexadecimal uses HTOF.  */
#define REAL_VALUE_ATOF(s,m) ereal_atof(s,m)
#define REAL_VALUE_HTOF(s,m) ereal_atof(s,m)

#define REAL_VALUE_NEGATE ereal_negate

#define REAL_VALUE_MINUS_ZERO(x) \
 ((ereal_cmp (x, dconst0) == 0) && (ereal_isneg (x) != 0 ))

#define REAL_VALUE_TO_INT ereal_to_int

/* Here the cast to HOST_WIDE_INT sign-extends arguments such as ~0.  */
#define REAL_VALUE_FROM_INT(d, lo, hi, mode) \
  ereal_from_int (&d, (HOST_WIDE_INT) (lo), (HOST_WIDE_INT) (hi), mode)

#define REAL_VALUE_FROM_UNSIGNED_INT(d, lo, hi, mode) \
  ereal_from_uint (&d, lo, hi, mode)

/* IN is a REAL_VALUE_TYPE.  OUT is an array of longs.  */
#define REAL_VALUE_TO_TARGET_LONG_DOUBLE(IN, OUT) 		\
   (LONG_DOUBLE_TYPE_SIZE == 64 ? etardouble ((IN), (OUT))	\
    : LONG_DOUBLE_TYPE_SIZE == 96 ? etarldouble ((IN), (OUT))	\
    : LONG_DOUBLE_TYPE_SIZE == 128 ? etartdouble ((IN), (OUT))  \
    : abort ())
#define REAL_VALUE_TO_TARGET_DOUBLE(IN, OUT) (etardouble ((IN), (OUT)))

/* IN is a REAL_VALUE_TYPE.  OUT is a long.  */
#define REAL_VALUE_TO_TARGET_SINGLE(IN, OUT) ((OUT) = etarsingle ((IN)))

/* Inverse of REAL_VALUE_TO_TARGET_DOUBLE.  */
#define REAL_VALUE_UNTO_TARGET_DOUBLE(d)  (ereal_unto_double (d))

/* Inverse of REAL_VALUE_TO_TARGET_SINGLE.  */
#define REAL_VALUE_UNTO_TARGET_SINGLE(f)  (ereal_unto_float (f))

/* d is an array of HOST_WIDE_INT that holds a double precision
   value in the target computer's floating point format.  */
#define REAL_VALUE_FROM_TARGET_DOUBLE(d)  (ereal_from_double (d))

/* f is a HOST_WIDE_INT containing a single precision target float value.  */
#define REAL_VALUE_FROM_TARGET_SINGLE(f)  (ereal_from_float (f))

/* Conversions to decimal ASCII string.  */
#define REAL_VALUE_TO_DECIMAL(r, fmt, s) (ereal_to_decimal (r, s))

#endif /* REAL_ARITHMETIC defined */

/* **** End of software floating point emulator interface macros **** */
#else /* No XFmode or TFmode and REAL_ARITHMETIC not defined */

/* old interface */
#ifdef REAL_ARITHMETIC
/* Defining REAL_IS_NOT_DOUBLE breaks certain initializations
   when REAL_ARITHMETIC etc. are not defined.  */

/* Now see if the host and target machines use the same format. 
   If not, define REAL_IS_NOT_DOUBLE (even if we end up representing
   reals as doubles because we have no better way in this cross compiler.)
   This turns off various optimizations that can happen when we know the
   compiler's float format matches the target's float format.
   */
#if HOST_FLOAT_FORMAT != TARGET_FLOAT_FORMAT
#define	REAL_IS_NOT_DOUBLE
#ifndef REAL_VALUE_TYPE
typedef struct {
    HOST_WIDE_INT r[sizeof (double)/sizeof (HOST_WIDE_INT)];
  } realvaluetype;
#define REAL_VALUE_TYPE realvaluetype
#endif /* no REAL_VALUE_TYPE */
#endif /* formats differ */
#endif /* 0 */

#endif /* emulator not used */

/* If we are not cross-compiling, use a `double' to represent the
   floating-point value.  Otherwise, use some other type
   (probably a struct containing an array of longs).  */
#ifndef REAL_VALUE_TYPE
#define REAL_VALUE_TYPE double
#else
#define REAL_IS_NOT_DOUBLE
#endif

#if HOST_FLOAT_FORMAT == TARGET_FLOAT_FORMAT

/* Convert a type `double' value in host format first to a type `float'
   value in host format and then to a single type `long' value which
   is the bitwise equivalent of the `float' value.  */
#ifndef REAL_VALUE_TO_TARGET_SINGLE
#define REAL_VALUE_TO_TARGET_SINGLE(IN, OUT)		\
do {							\
  union {						\
    float f;						\
    HOST_WIDE_INT l;					\
  } u;							\
  if (sizeof(HOST_WIDE_INT) < sizeof(float))		\
    abort ();						\
  u.l = 0;						\
  u.f = (IN);						\
  (OUT) = u.l;						\
} while (0)
#endif

/* Convert a type `double' value in host format to a pair of type `long'
   values which is its bitwise equivalent, but put the two words into
   proper word order for the target.  */
#ifndef REAL_VALUE_TO_TARGET_DOUBLE
#define REAL_VALUE_TO_TARGET_DOUBLE(IN, OUT)				\
do {									\
  union {								\
    REAL_VALUE_TYPE f;							\
    HOST_WIDE_INT l[2];							\
  } u;									\
  if (sizeof(HOST_WIDE_INT) * 2 < sizeof(REAL_VALUE_TYPE))		\
    abort ();								\
  u.l[0] = u.l[1] = 0;							\
  u.f = (IN);								\
  if (HOST_FLOAT_WORDS_BIG_ENDIAN == FLOAT_WORDS_BIG_ENDIAN)		\
    (OUT)[0] = u.l[0], (OUT)[1] = u.l[1];				\
  else									\
    (OUT)[1] = u.l[0], (OUT)[0] = u.l[1];				\
} while (0)
#endif
#endif /* HOST_FLOAT_FORMAT == TARGET_FLOAT_FORMAT */

/* In this configuration, double and long double are the same.  */
#ifndef REAL_VALUE_TO_TARGET_LONG_DOUBLE
#define REAL_VALUE_TO_TARGET_LONG_DOUBLE(a, b) REAL_VALUE_TO_TARGET_DOUBLE (a, b)
#endif

/* Compare two floating-point objects for bitwise identity.
   This is not the same as comparing for equality on IEEE hosts:
   -0.0 equals 0.0 but they are not identical, and conversely
   two NaNs might be identical but they cannot be equal.  */
#define REAL_VALUES_IDENTICAL(x, y) \
  (!memcmp ((char *) &(x), (char *) &(y), sizeof (REAL_VALUE_TYPE)))

/* Compare two floating-point values for equality.  */
#ifndef REAL_VALUES_EQUAL
#define REAL_VALUES_EQUAL(x, y) ((x) == (y))
#endif

/* Compare two floating-point values for less than.  */
#ifndef REAL_VALUES_LESS
#define REAL_VALUES_LESS(x, y) ((x) < (y))
#endif

/* Truncate toward zero to an integer floating-point value.  */
#ifndef REAL_VALUE_RNDZINT
#define REAL_VALUE_RNDZINT(x) ((double) ((int) (x)))
#endif

/* Truncate toward zero to an unsigned integer floating-point value.  */
#ifndef REAL_VALUE_UNSIGNED_RNDZINT
#define REAL_VALUE_UNSIGNED_RNDZINT(x) ((double) ((unsigned int) (x)))
#endif

/* Convert a floating-point value to integer, rounding toward zero.  */
#ifndef REAL_VALUE_FIX
#define REAL_VALUE_FIX(x) ((int) (x))
#endif

/* Convert a floating-point value to unsigned integer, rounding
   toward zero.  */
#ifndef REAL_VALUE_UNSIGNED_FIX
#define REAL_VALUE_UNSIGNED_FIX(x) ((unsigned int) (x))
#endif

/* Scale X by Y powers of 2.  */
#ifndef REAL_VALUE_LDEXP
#define REAL_VALUE_LDEXP(x, y) ldexp (x, y)
extern double ldexp PARAMS ((double, int));
#endif

/* Convert the string X to a floating-point value.  */
#ifndef REAL_VALUE_ATOF
#if 1
/* Use real.c to convert decimal numbers to binary, ...  */
#define REAL_VALUE_ATOF(x, s) ereal_atof (x, s)
/* Could use ereal_atof here for hexadecimal floats too, but real_hex_to_f
   is OK and it uses faster native fp arithmetic.  */
/* #define REAL_VALUE_HTOF(x, s) ereal_atof (x, s) */
#else
/* ... or, if you like the host computer's atof, go ahead and use it: */
#define REAL_VALUE_ATOF(x, s) atof (x)
#if defined (MIPSEL) || defined (MIPSEB)
/* MIPS compiler can't handle parens around the function name.
   This problem *does not* appear to be connected with any
   macro definition for atof.  It does not seem there is one.  */
extern double atof ();
#else
extern double (atof) ();
#endif
#endif
#endif

/* Hexadecimal floating constant input for use with host computer's
   fp arithmetic.  */
#ifndef REAL_VALUE_HTOF
extern REAL_VALUE_TYPE real_hex_to_f PARAMS ((const char *,
					      enum machine_mode));
#define REAL_VALUE_HTOF(s,m) real_hex_to_f(s,m)
#endif

/* Negate the floating-point value X.  */
#ifndef REAL_VALUE_NEGATE
#define REAL_VALUE_NEGATE(x) (- (x))
#endif

/* Truncate the floating-point value X to mode MODE.  This is correct only
   for the most common case where the host and target have objects of the same
   size and where `float' is SFmode.  */

/* Don't use REAL_VALUE_TRUNCATE directly--always call real_value_truncate.  */
extern REAL_VALUE_TYPE real_value_truncate PARAMS ((enum machine_mode,
						  REAL_VALUE_TYPE));

#ifndef REAL_VALUE_TRUNCATE
#define REAL_VALUE_TRUNCATE(mode, x) \
 (GET_MODE_BITSIZE (mode) == sizeof (float) * HOST_BITS_PER_CHAR	\
  ? (float) (x) : (x))
#endif

/* Determine whether a floating-point value X is infinite.  */
#ifndef REAL_VALUE_ISINF
#define REAL_VALUE_ISINF(x) (target_isinf (x))
#endif

/* Determine whether a floating-point value X is a NaN.  */
#ifndef REAL_VALUE_ISNAN
#define REAL_VALUE_ISNAN(x) (target_isnan (x))
#endif

/* Determine whether a floating-point value X is negative.  */
#ifndef REAL_VALUE_NEGATIVE
#define REAL_VALUE_NEGATIVE(x) (target_negative (x))
#endif

/* Determine whether a floating-point value X is minus 0.  */
#ifndef REAL_VALUE_MINUS_ZERO
#define REAL_VALUE_MINUS_ZERO(x) ((x) == 0 && REAL_VALUE_NEGATIVE (x))
#endif

/* Constant real values 0, 1, 2, and -1.  */

extern REAL_VALUE_TYPE dconst0;
extern REAL_VALUE_TYPE dconst1;
extern REAL_VALUE_TYPE dconst2;
extern REAL_VALUE_TYPE dconstm1;

/* Union type used for extracting real values from CONST_DOUBLEs
   or putting them in.  */

union real_extract 
{
  REAL_VALUE_TYPE d;
  HOST_WIDE_INT i[sizeof (REAL_VALUE_TYPE) / sizeof (HOST_WIDE_INT)];
};

/* Given a CONST_DOUBLE in FROM, store into TO the value it represents.  */
/* Function to return a real value (not a tree node)
   from a given integer constant.  */
union tree_node;
REAL_VALUE_TYPE real_value_from_int_cst	PARAMS ((union tree_node *,
						union tree_node *));

#define REAL_VALUE_FROM_CONST_DOUBLE(to, from)		\
do { union real_extract u;				\
     memcpy (&u, &CONST_DOUBLE_LOW ((from)), sizeof u); \
     to = u.d; } while (0)

/* Return a CONST_DOUBLE with value R and mode M.  */

#define CONST_DOUBLE_FROM_REAL_VALUE(r, m) immed_real_const_1 (r,  m)
extern struct rtx_def *immed_real_const_1	PARAMS ((REAL_VALUE_TYPE,
						       enum machine_mode));


/* Convert a floating point value `r', that can be interpreted
   as a host machine float or double, to a decimal ASCII string `s'
   using printf format string `fmt'.  */
#ifndef REAL_VALUE_TO_DECIMAL
#define REAL_VALUE_TO_DECIMAL(r, fmt, s) (sprintf (s, fmt, r))
#endif

/* Replace R by 1/R in the given machine mode, if the result is exact.  */
extern int exact_real_inverse	PARAMS ((enum machine_mode, REAL_VALUE_TYPE *));
extern int target_isnan		PARAMS ((REAL_VALUE_TYPE));
extern int target_isinf		PARAMS ((REAL_VALUE_TYPE));
extern int target_negative	PARAMS ((REAL_VALUE_TYPE));
extern void debug_real		PARAMS ((REAL_VALUE_TYPE));
extern REAL_VALUE_TYPE ereal_atof PARAMS ((const char *, enum machine_mode));

#endif /* ! GCC_REAL_H */
