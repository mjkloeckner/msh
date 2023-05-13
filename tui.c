#include "tui.h"

#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

#define	TAB_SIZE	4

static int is_terminal;
static struct termios default_attributes;

void tui_reset_input_mode (void) {
	tcsetattr(STDIN_FILENO, TCSANOW, &default_attributes);
}

void tui_set_input_mode (void) {
	struct termios tattr;

	/* Make sure stdin is a terminal. */
	if (!isatty(STDIN_FILENO)) {
		is_terminal = 0;
		return;
	} else {
		is_terminal = 1;
	}

	/* Save the terminal attributes so we can restore them later. */
	tcgetattr(STDIN_FILENO, &default_attributes);
	atexit(tui_reset_input_mode);

	/* Set the terminal modes. */
	tcgetattr(STDIN_FILENO, &tattr);

	tattr.c_iflag &= ~(PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tattr.c_oflag &= ~OPOST;
	tattr.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tattr.c_cflag &= ~(CSIZE | PARENB);
	tattr.c_cflag |= CS8;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}
