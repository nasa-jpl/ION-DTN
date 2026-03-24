/*****************************************************************************
 **
 ** File Name: entropy_src.c
 **
 ** Description: Cross-platform, high-reliability entropy source polling, with
 **              multi-layer fallbacks (for modern and legacy systems).
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **
 **  12/11/24  S. DeBaun      Initial implementation
 **  06/13/25  S. DeBaun      Revised for improved hwrng polling and fallback
 **                           behavior.
 *****************************************************************************/


#include "entropy_src.h"

/* Platform-specific includes */
#if defined(__linux__) || defined(__sun) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <sys/random.h>
#include <errno.h>
#endif

#include <fcntl.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <Security/Security.h>
#endif

/*============================================================================
 * get_error_message
 *==========================================================================*/
const char* get_error_message(ErrorCode code)
{
	/* See entropy_src.h for documentation */
	switch (code)
	{
	case INVALID_ARGUMENTS:
		return "Invalid arguments";
	case ERROR_GETRANDOM_FAILED:
		return "getrandom() failed (Linux)";
	case ERROR_BCRYPT_FAILED:
		return "BCryptGenRandom failed";
	case ERROR_SECRANDOM_FAILED:
		return "SecRandomCopyBytes failed (macOS)";
	case ERROR_OPENING_ENTROPY_SOURCE:
		return "Could not open entropy device file";
	case ERROR_READING_ENTROPY_SOURCE:
		return "Could not read from entropy device file";
	default:
		return "Unknown error";
	}
}


#if !defined(__FreeBSD__) && !defined(__OpenBSD__)
/*============================================================================
 * poll_from_device_file (static helper)
 *==========================================================================*/
/**
 * @brief [FALLBACK] Reads entropy from a device file like /dev/urandom.
 * @details This is the final fallback method for Unix-like systems. It
 * prefers /dev/urandom but falls back to /dev/random if urandom is
 * unavailable or (on older Linux) not safely seeded.
 */
static int poll_from_device_file(unsigned char *output, size_t ilen, size_t *olen)
{
	int fd = -1;

	/* Try /dev/urandom, the preferred non-blocking source. */
	fd = open("/dev/urandom", O_RDONLY);
	if (fd >= 0)
	{
#if defined(__linux__)
		/*
		 * EXTRA PRECAUTION: Check if the entropy pool is seeded.
		 * This function is the fallback for when getrandom() is unavailable, so
		 * this check is critical for safety on those legacy systems.
		 */
		int random_fd = open("/dev/random", O_RDONLY | O_NONBLOCK);
		if (random_fd >= 0)
		{
			char	dummy_buf;
			ssize_t ret = read(random_fd, &dummy_buf, 1);
			close(random_fd);

			/* If the non-blocking read fails with anything other than EAGAIN,
			 * it implies the pool is not yet initialized. Invalidate the
			 * /dev/urandom file descriptor to force a fallback to /dev/random. */
			if (ret < 0 && errno != EAGAIN)
			{
				close(fd); /* Close the /dev/urandom handle */
				fd = -1; /* Invalidate fd to force the fallback */
				putErrmsg("[!] poll_from_device_file:", "urandom not ready, falling back to /dev/random.");
			}
		}
#endif /* defined(__linux__) */
	}

	/*
	 * If /dev/urandom was unavailable, unready, or failed to open, fall back
	 * to /dev/random. This is the last resort.
	 */
	if (fd < 0)
	{
		fd = open("/dev/random", O_RDONLY);
		if (fd < 0)
		{
			putErrmsg("[!] poll_from_device_file:", "could not open any entropy source device.");
			return ERROR_OPENING_ENTROPY_SOURCE;
		}
	}

	/* Now, read from the file descriptor that was successfully opened. */
	ssize_t read_bytes = read(fd, output, ilen);
	close(fd);

	if (read_bytes < 0)
	{
		putErrmsg("[!] poll_from_device_file:", "error reading from entropy source.");
		*olen = 0;
		return ERROR_READING_ENTROPY_SOURCE;
	}

	*olen = (size_t) read_bytes;
	return 0; /* Success */
}
#endif


/*============================================================================
 * poll_entropy_src
 *==========================================================================*/
int poll_entropy_src(void *data, unsigned char *output, size_t ilen, size_t *olen)
{
	/* See entropy_src.h for documentation */
	(void) data;

	if (!output || !olen || ilen == 0)
	{
		if (olen) *olen = 0;
		return INVALID_ARGUMENTS;
	}
	*olen = 0;

#if defined(__linux__) || defined(__sun)
	/*
	 * TIER 1: Use the getrandom() glibc wrapper (Linux) or syscall (Solaris).
	 * This is the most performant and secure method on modern Linux. It will
	 * use a vDSO implementation if available (kernel >= 6.6, glibc >= 2.36),
	 * avoiding a context switch. The wrapper also handles the fallback to
	 * the getrandom() syscall or /dev/urandom for older systems.
	 */
	ssize_t bytes_read = 0;
	while ((size_t)bytes_read < ilen)
	{
		ssize_t ret = getrandom(output + bytes_read, ilen - bytes_read, 0);

		if (ret > 0)
		{
			bytes_read += ret;
		} else {
			/* If the call was interrupted by a signal, retry. */
			if (errno == EINTR)
			{
				continue;
			}

			/*
			 * For any other failure, even with the glibc wrapper, we fall back
			 * to the device file polling method for constrained or unusual systems.
			 */
			putErrmsg("[!] poll_entropy_src:", "getrandom() failed, attempting device file fallback.");
			return poll_from_device_file(output, ilen, olen);
		}
	}
	*olen = bytes_read;
	return 0; /* Success */

#elif defined(__APPLE__)
	/* TIER 1: Use arc4random_buf() on macOS 10.12+ */
	#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 101200
	{
		arc4random_buf(output, ilen);
		*olen = ilen;
		return 0; /* Success */
	}

	/* If arc4random_buf() failed or was unavailable, fall through to Tier 2... */
	#endif

	/* TIER 2: Use SecRandomCopyBytes as the established API */
	if (SecRandomCopyBytes(kSecRandomDefault, ilen, output) == errSecSuccess)
	{
		*olen = ilen;
		return 0; /* Success */
	}

	/* TIER 3: If both modern methods fail, use the device file fallback. */
	return poll_from_device_file(output, ilen, olen);


#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	/*
	 * On BSD systems, arc4random_buf() is the recommended, high-level
	 * interface for applications. It is fast, automatically seeded with a
	 * strong entropy source, and cannot fail.
	 */
	arc4random_buf(output, ilen);
	*olen = ilen;
	return 0; /* Success */


#elif defined(FREERTOS)
	/**************************************************************************
	 * IMPORTANT: This is a placeholder for a future hardware-specific
	 * implementation. FreeRTOS itself does not provide an entropy source;
	 * you must call the random number generator function provided by the
	 * microcontroller's SDK/HAL (e.g., for STM32, ESP32, etc.).
	 *
	 * Remove this #error directive once you have implemented and
	 * thoroughly tested the correct hardware-specific function call below.
	 **************************************************************************/
	#error "ENTROPY_SRC: No hardware-specific implementation for this FreeRTOS target. See comments."

	/*
	 * EXAMPLE for a hypothetical platform:
	 *
	 * if (vendor_hal_get_random(output, ilen) == VENDOR_SUCCESS)
	 * {
	 * *olen = ilen;
	 * return 0;
	 * }
	 * else
	 * {
	 * return -1; // Define a new ErrorCode for this case
	 * }
	 */

#else
	/* TIER 3: Fallback for other generic Unix-like systems. */
	return poll_from_device_file(output, ilen, olen);

#endif
}
