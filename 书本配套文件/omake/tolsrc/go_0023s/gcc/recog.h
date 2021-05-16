/* Declarations for interface to insn recognizer and insn-output.c.
   Copyright (C) 1987, 1996, 1997, 1998, 1999, 2000, 2001
   Free Software Foundation, Inc.

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

/* Random number that should be large enough for all purposes.  */
#define MAX_RECOG_ALTERNATIVES 30
#define recog_memoized(I) (INSN_CODE (I) >= 0 \
			   ? INSN_CODE (I) : recog_memoized_1 (I))

/* Types of operands.  */
enum op_type {
  OP_IN,
  OP_OUT,
  OP_INOUT
};

struct operand_alternative
{
  /* Pointer to the beginning of the constraint string for this alternative,
     for easier access by alternative number.  */
  const char *constraint;

  /* The register class valid for this alternative (possibly NO_REGS).  */
  enum reg_class class;

  /* "Badness" of this alternative, computed from number of '?' and '!'
     characters in the constraint string.  */
  unsigned int reject;

  /* -1 if no matching constraint was found, or an operand number.  */
  int matches;
  /* The same information, but reversed: -1 if this operand is not
     matched by any other, or the operand number of the operand that
     matches this one.  */
  int matched;

  /* Nonzero if '&' was found in the constraint string.  */
  unsigned int earlyclobber:1;
  /* Nonzero if 'm' was found in the constraint string.  */
  unsigned int memory_ok:1;  
  /* Nonzero if 'o' was found in the constraint string.  */
  unsigned int offmem_ok:1;  
  /* Nonzero if 'V' was found in the constraint string.  */
  unsigned int nonoffmem_ok:1;
  /* Nonzero if '<' was found in the constraint string.  */
  unsigned int decmem_ok:1;
  /* Nonzero if '>' was found in the constraint string.  */
  unsigned int incmem_ok:1;
  /* Nonzero if 'p' was found in the constraint string.  */
  unsigned int is_address:1;
  /* Nonzero if 'X' was found in the constraint string, or if the constraint
     string for this alternative was empty.  */
  unsigned int anything_ok:1;
};


extern void init_recog			PARAMS ((void));
extern void init_recog_no_volatile	PARAMS ((void));
extern int recog_memoized_1		PARAMS ((rtx));
extern int check_asm_operands		PARAMS ((rtx));
extern int asm_operand_ok		PARAMS ((rtx, const char *));
extern int validate_change		PARAMS ((rtx, rtx *, rtx, int));
extern int insn_invalid_p		PARAMS ((rtx));
extern int apply_change_group		PARAMS ((void));
extern int num_validated_changes	PARAMS ((void));
extern void cancel_changes		PARAMS ((int));
extern int constrain_operands		PARAMS ((int));
extern int constrain_operands_cached	PARAMS ((int));
extern int memory_address_p		PARAMS ((enum machine_mode, rtx));
extern int strict_memory_address_p	PARAMS ((enum machine_mode, rtx));
extern int validate_replace_rtx_subexp	PARAMS ((rtx, rtx, rtx, rtx *));
extern int validate_replace_rtx		PARAMS ((rtx, rtx, rtx));
extern void validate_replace_rtx_group	PARAMS ((rtx, rtx, rtx));
extern int validate_replace_src		PARAMS ((rtx, rtx, rtx));
#ifdef HAVE_cc0
extern int next_insn_tests_no_inequality PARAMS ((rtx));
#endif
extern int reg_fits_class_p		PARAMS ((rtx, enum reg_class, int,
					       enum machine_mode));
extern rtx *find_single_use		PARAMS ((rtx, rtx, rtx *));

extern int general_operand		PARAMS ((rtx, enum machine_mode));
extern int address_operand		PARAMS ((rtx, enum machine_mode));
extern int register_operand		PARAMS ((rtx, enum machine_mode));
extern int pmode_register_operand	PARAMS ((rtx, enum machine_mode));
extern int scratch_operand		PARAMS ((rtx, enum machine_mode));
extern int immediate_operand		PARAMS ((rtx, enum machine_mode));
extern int const_int_operand		PARAMS ((rtx, enum machine_mode));
extern int const_double_operand		PARAMS ((rtx, enum machine_mode));
extern int nonimmediate_operand		PARAMS ((rtx, enum machine_mode));
extern int nonmemory_operand		PARAMS ((rtx, enum machine_mode));
extern int push_operand			PARAMS ((rtx, enum machine_mode));
extern int pop_operand			PARAMS ((rtx, enum machine_mode));
extern int memory_operand		PARAMS ((rtx, enum machine_mode));
extern int indirect_operand		PARAMS ((rtx, enum machine_mode));
extern int mode_independent_operand	PARAMS ((rtx, enum machine_mode));
extern int comparison_operator		PARAMS ((rtx, enum machine_mode));

extern int offsettable_memref_p		PARAMS ((rtx));
extern int offsettable_nonstrict_memref_p	PARAMS ((rtx));
extern int offsettable_address_p	PARAMS ((int, enum machine_mode, rtx));
extern int mode_dependent_address_p	PARAMS ((rtx));

extern int recog			PARAMS ((rtx, rtx, int *));
extern void add_clobbers		PARAMS ((rtx, int));
extern int added_clobbers_hard_reg_p	PARAMS ((int));
extern void insn_extract		PARAMS ((rtx));
extern void extract_insn		PARAMS ((rtx));
extern void extract_constrain_insn_cached PARAMS ((rtx));
extern void extract_insn_cached		PARAMS ((rtx));
extern void preprocess_constraints	PARAMS ((void));
extern rtx peep2_next_insn		PARAMS ((int));
extern int peep2_regno_dead_p		PARAMS ((int, int));
extern int peep2_reg_dead_p		PARAMS ((int, rtx));
#ifdef CLEAR_HARD_REG_SET
extern rtx peep2_find_free_register	PARAMS ((int, int, const char *,
						 enum machine_mode,
						 HARD_REG_SET *));
#endif
extern void peephole2_optimize		PARAMS ((FILE *));
extern rtx peephole2_insns		PARAMS ((rtx, rtx, int *));

/* Nonzero means volatile operands are recognized.  */
extern int volatile_ok;

/* Set by constrain_operands to the number of the alternative that
   matched.  */
extern int which_alternative;

/* The following vectors hold the results from insn_extract.  */

struct recog_data
{
  /* It is very tempting to make the 5 operand related arrays into a
     structure and index on that.  However, to be source compatible
     with all of the existing md file insn constraints and output
     templates, we need `operand' as a flat array.  Without that
     member, making an array for the rest seems pointless.  */

  /* Gives value of operand N.  */
  rtx operand[MAX_RECOG_OPERANDS];

  /* Gives location where operand N was found.  */
  rtx *operand_loc[MAX_RECOG_OPERANDS];

  /* Gives the constraint string for operand N.  */
  const char *constraints[MAX_RECOG_OPERANDS];

  /* Gives the mode of operand N.  */
  enum machine_mode operand_mode[MAX_RECOG_OPERANDS];

  /* Gives the type (in, out, inout) for operand N.  */
  enum op_type operand_type[MAX_RECOG_OPERANDS];

  /* Gives location where the Nth duplicate-appearance of an operand
     was found.  This is something that matched MATCH_DUP.  */
  rtx *dup_loc[MAX_DUP_OPERANDS];

  /* Gives the operand number that was duplicated in the Nth
     duplicate-appearance of an operand.  */
  char dup_num[MAX_DUP_OPERANDS];

  /* ??? Note that these are `char' instead of `unsigned char' to (try to)
     avoid certain lossage from K&R C, wherein `unsigned char' default 
     promotes to `unsigned int' instead of `int' as in ISO C.  As of 1999,
     the most common places to bootstrap from K&R C are SunOS and HPUX,
     both of which have signed characters by default.  The only other
     supported natives that have both K&R C and unsigned characters are
     ROMP and Irix 3, and neither have been seen for a while, but do
     continue to consider unsignedness when performing arithmetic inside
     a comparison.  */

  /* The number of operands of the insn.  */
  char n_operands;

  /* The number of MATCH_DUPs in the insn.  */
  char n_dups;

  /* The number of alternatives in the constraints for the insn.  */
  char n_alternatives;

  /* In case we are caching, hold insn data was generated for.  */
  rtx insn;
};

extern struct recog_data recog_data;

/* Contains a vector of operand_alternative structures for every operand.
   Set up by preprocess_constraints.  */
extern struct operand_alternative recog_op_alt[MAX_RECOG_OPERANDS][MAX_RECOG_ALTERNATIVES];

/* A table defined in insn-output.c that give information about
   each insn-code value.  */

typedef int (*insn_operand_predicate_fn) PARAMS ((rtx, enum machine_mode));
typedef const char * (*insn_output_fn) PARAMS ((rtx *, rtx));
typedef rtx (*insn_gen_fn) PARAMS ((rtx, ...));

struct insn_operand_data
{
  const insn_operand_predicate_fn predicate;

  const char *const constraint;

  const ENUM_BITFIELD(machine_mode) mode : 16;

  const char strict_low;

  const char eliminable;
};

/* Legal values for insn_data.output_format.  Indicate what type of data
   is stored in insn_data.output.  */
#define INSN_OUTPUT_FORMAT_NONE		0	/* abort */
#define INSN_OUTPUT_FORMAT_SINGLE	1	/* const char * */
#define INSN_OUTPUT_FORMAT_MULTI	2	/* const char * const * */
#define INSN_OUTPUT_FORMAT_FUNCTION	3	/* const char * (*)(...) */

struct insn_data
{
  const char *const name;
  const PTR output;
  const insn_gen_fn genfun;
  const struct insn_operand_data *const operand;

  const char n_operands;
  const char n_dups;
  const char n_alternatives;
  const char output_format;
};

extern const struct insn_data insn_data[];
