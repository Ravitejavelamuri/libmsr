#include <sys/types.h>
#include <sys/fcntl.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <err.h>

#include "libmsr.h"

/*
 * Serial I/O routines.
 */
static int msr_serial_setup (int fd, speed_t baud);

/*
 * Read a character from the serial port. Note that this
 * routine will block until a valid character is read.
 */
int msr_serial_readchar (int fd, uint8_t * c)
{
	char b;
	int	r;

	while ((r = read (fd, &b, 1)) == -1)
		;

	if (r != -1) {
		*c = b;
#ifdef MSR_DEBUG
		printf ("[0x%x]\n", b);
#endif
	}

	return (r);
}

/*
 * Read a series of characters from the serial port. This
 * routine will block until the desired number of characters
 * is read.
 */
int msr_serial_read (int fd, void * buf, size_t len)
{
	size_t i;
	uint8_t b, *p;

	p = buf;

#ifdef SERIAL_DEBUG
	printf("[RX %.3lu]", len);
#endif
	for (i = 0; i < len; i++) {
		msr_serial_readchar (fd, &b);
#ifdef SERIAL_DEBUG
		printf(" %.2x", b);
#endif
		p[i] = b;
	}
#ifdef SERIAL_DEBUG
	printf("\n");
#endif

	return LIBMSR_ERR_OK;
}

int msr_serial_write (int fd, void * buf, size_t len)
{
	return (write (fd, buf, len));
}

/*
 * Set serial line options. We need to set the baud rate and
 * turn off most of the internal processing in the tty layer in
 * order to avoid having some of the output from the card reader
 * interpreted as control characters and swallowed.
 */

static int
msr_serial_setup (int fd, speed_t baud)
{
    struct termios options;


	if (tcgetattr(fd, &options) == -1) {
		return LIBMSR_ERR_SERIAL;
	}

	/* Set baud rate */
	cfsetispeed(&options, baud);
	cfsetospeed(&options, baud);

	/* Control modes */

	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

	/*
     * Local modes
     * We have to clear the ISIG flag to defeat signal
	 * processing in order to see the file separator
	 * character (0x1C) which the device will send as
	 * part of its end of record markers.
	 */

	options.c_lflag &= ~ICANON;
	options.c_lflag &= ~ECHO;
	options.c_lflag &= ~ECHONL;
	options.c_lflag &= ~ISIG;
	options.c_lflag &= ~IEXTEN;

	/* Input modes */

	options.c_iflag &= ~ICRNL;
	options.c_iflag &= ~IXON;
	options.c_iflag &= ~IXOFF;
	options.c_iflag &= ~IXANY;
	options.c_iflag |= IGNBRK;
	options.c_iflag |= IGNPAR;

	/* Output modes */

	options.c_oflag &= ~OPOST;

	if (tcsetattr(fd, TCSANOW, &options) == -1) {
		return LIBMSR_ERR_SERIAL;
	}

	return LIBMSR_ERR_OK;
}

int msr_serial_open(char *path, int * fd, int blocking, speed_t baud)
{
	int	f;

	f = open(path, blocking | O_RDWR | O_FSYNC);

	if (f == -1) {
		return LIBMSR_ERR_SERIAL;
	}

	if (msr_serial_setup (f, baud) != LIBMSR_ERR_OK) {
		close (f);
		return LIBMSR_ERR_SERIAL;
	}

	*fd = f;

	return LIBMSR_ERR_OK;
}

int msr_serial_close(int fd)
{
	close (fd);
	return LIBMSR_ERR_OK;
}
