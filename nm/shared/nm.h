#ifndef __NM__
#define __NM__

// Preprocessor magic helper
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
/** AMP Protocol Version
 * STATUS: Currently supports version 6 and version 7/8 controllable via this define.
 * NOTICE: Backwards compatibility will be removed in the near future.
 */
#ifndef AMP_VERSION
#define AMP_VERSION 8
#endif

// IETF URL Must be 0-padded
#if AMP_VERSION < 10
#define AMP_VERSION_PAD "0" STR(AMP_VERSION)
#else
#define AMP_VERSION_PAD STR(AMP_VERSION)
#endif

#define AMP_PROTOCOL_URL_BASE "https://datatracker.ietf.org/doc/draft-birrane-dtn-amp"
#define AMP_PROTOCOL_URL      AMP_PROTOCOL_URL_BASE "/" AMP_VERSION_PAD
#define AMP_VERSION_STR       STR(AMP_VERSION) " - " AMP_PROTOCOL_URL


#endif
