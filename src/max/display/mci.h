#ifndef __MAX_MCI_H_DEFINED
#define __MAX_MCI_H_DEFINED

#include <stddef.h>
#include <stdint.h>

#define MCI_PARSE_PIPE_COLORS 0x00000001UL
#define MCI_PARSE_MCI_CODES   0x00000002UL
#define MCI_PARSE_FORMAT_OPS  0x00000004UL

/** @brief All parse flags combined (pipe colors + MCI codes + format ops). */
#define MCI_PARSE_ALL         (MCI_PARSE_PIPE_COLORS | MCI_PARSE_MCI_CODES | MCI_PARSE_FORMAT_OPS)

/**
 * @brief Strip pipe color sequences like @c |00..|31.
 */
#define MCI_STRIP_COLORS      0x00000001UL

/**
 * @brief Strip information MCI sequences like @c |UN, |BN, etc.
 */
#define MCI_STRIP_INFO        0x00000002UL

/**
 * @brief Strip formatting operators like @c $C##, $D##C, and @c |PD.
 */
#define MCI_STRIP_FORMAT      0x00000004UL

extern unsigned long g_mci_parse_flags;

/**
 * @brief Positional parameter bindings for |!N expansion.
 *
 * When non-NULL, MciExpand() replaces |!1..|!F with the pre-formatted
 * string values in this struct.  Set by LangPrintf() before calling Puts(),
 * cleared immediately after.
 */
typedef struct {
    const char *values[15];  /**< Pre-formatted string values for |!1..|!F */
    int count;               /**< Number of bound parameters */
} MciLangParams;

extern MciLangParams *g_lang_params;

/**
 * @brief Set the MCI parse flags.
 *
 * @param flags New flags value.
 */
void MciSetParseFlags(unsigned long flags);

/**
 * @brief Get the current MCI parse flags.
 *
 * @return Current flags value.
 */
unsigned long MciGetParseFlags(void);

/**
 * @brief Push a new set of parse flags onto the stack.
 *
 * @param mask Mask of flags to change.
 * @param values New values for the masked flags.
 */
void MciPushParseFlags(unsigned long mask, unsigned long values);

/**
 * @brief Pop the top set of parse flags from the stack.
 */
void MciPopParseFlags(void);

/**
 * @brief Expand Mystic-style MCI codes and formatting operators in a string.
 *
 * Pipe colors @c |00..|31 are left intact for the output-layer parser.
 *
 * @param in Input string.
 * @param out Output buffer.
 * @param out_size Size of @p out in bytes.
 * @return Number of bytes written to @p out (excluding terminating NUL).
 */
size_t MciExpand(const char *in, char *out, size_t out_size);

/**
 * @brief Strip MCI-related sequences from a string.
 *
 * This is intended for sanitizing user-supplied input where MCI injection
 * should be prevented.
 *
 * @param in Input string.
 * @param out Output buffer.
 * @param out_size Size of @p out in bytes.
 * @param strip_flags Bitmask of @c MCI_STRIP_COLORS, @c MCI_STRIP_INFO, @c MCI_STRIP_FORMAT.
 * @return Number of bytes written to @p out (excluding terminating NUL).
 */
size_t MciStrip(const char *in, char *out, size_t out_size, unsigned long strip_flags);

#endif /* __MAX_MCI_H_DEFINED */
