#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <ctype.h>

#include "tui.h"

#define	TAB_SIZE	4

#define	READLINE_BUFFER_INIT_SIZE	128
#define TOKENS_BUFFER_INIT_SIZE		8
#define TOKENS_DELIM				" |"

static bool run = true;
static size_t buffer_alloc;
static size_t tokens_alloc;

void buffer_print_slice(const char *buf, size_t from, size_t to) {
	for(size_t i = from; i < to; i++)
		printf("%c", buf[i]);
}

void buffer_clear(char *buf) {
	size_t len = strlen(buf);
	for (size_t i = 0; i < len; i++)
		buf[i] = '\0';
}

char *buffer_read_line(char *s) {
	int c;
	char *aux;
	size_t used_size;

	used_size = 0;
	while (1) {
		c = getchar();
		if((c == '\n') || (c == EOF))
			break;

		if(used_size == (buffer_alloc - 1)) {
			if(!(aux = realloc(s, buffer_alloc += buffer_alloc))) {
				perror("msh");
				free(s);
				return NULL;
			}
			s = aux;
		}
		s[used_size++] = c;
	}

	s[used_size] = '\0';
	if (c == EOF) run = false;
	return s;
}

char **buffer_split(char *b, char **t) {
	char *p, **aux;
	size_t tokens_count;

	tokens_count = 0;
	for(p = b; (p = strtok(p, TOKENS_DELIM)); p = NULL) {
		if((tokens_count + 1) == tokens_alloc) {
			if(!(aux = realloc(t, sizeof(char*) * (tokens_alloc *= 2)))) {
				perror("msh");
				for(size_t i = 0; i < tokens_count; i++)
					free(t[i]);
				free(t);
				return NULL;
			}
			t = aux;
		}
		t[tokens_count++] = p;
	}
	t[tokens_count] = NULL;
	return t;
}

char *editor_read_line(char *s) {
	int c;
	char *aux;
	size_t line_len, cursor_pos;

	line_len = cursor_pos = 0;
	while(1) {
		c = getchar();

		/* Ctrl-D */
		if((c == 4) || (c == EOF)) {
			run = false;
			buffer_clear(s);
			break;
		}

		if(c == 3) {
			buffer_clear(s);
			break;
		}

		if((c == '\r') || (c == '\n'))
			break;

		/* ESC */
		if(c == 27) {
			getchar(); /* skip '[' */
			switch (getchar()) {
			case 'A': /* up */
				break;
			case 'B': /* down */
				break;
			case 'D': /* left */
				if(cursor_pos > 0) {
					printf("\033[1D");
					cursor_pos--;
				}
				break;
			case 'C': /* right */
				if(cursor_pos < line_len) {
					printf("\033[1C");
					cursor_pos++;
				}
				break;
			default:
				break;
			}
			continue;
		}

		/* Backspace */
		if(c == 0x7f) {
			if (line_len > 0) {
				line_len -= 1;
				cursor_pos -= 1;
				printf("\b \b");
				if (cursor_pos < line_len) {
					memmove(s + cursor_pos, s + cursor_pos + 1, line_len - cursor_pos + 1);
					buffer_print_slice(s, cursor_pos, line_len);
					putchar(' ');
					printf("\033[%ldD", line_len - cursor_pos + 1);
				}
			}
			continue;
		}

		/* Delete */
		if(c == 0x7e) {
			if ((line_len > 0) && (cursor_pos < line_len)) {
				line_len -= 1;
				memmove(s + cursor_pos, s + cursor_pos + 1, line_len - cursor_pos);
				buffer_print_slice(s, cursor_pos, line_len);
				putchar(' ');
				printf("\033[%ldD", line_len - cursor_pos + 1);
			}
			continue;
		}


		if(c == '\t') {
			for(size_t i = 0; i < TAB_SIZE; i++)
				putchar(' ');

			line_len += TAB_SIZE;
			continue;
		}

		if(line_len == (buffer_alloc - 1)) {
			if(!(aux = realloc(s, buffer_alloc += buffer_alloc))) {
				perror("msh");
				free(s);
				return NULL;
			}
			s = aux;
		}

		/* only print printable caracters */
		if (isprint(c)) {
			if (cursor_pos < line_len) {
				memmove(s + cursor_pos + 1, s + cursor_pos, line_len - cursor_pos);
				putchar(c);
				s[cursor_pos++] = c;
				line_len += 1;

				buffer_print_slice(s, cursor_pos, line_len);

				/* move left the amount buffer_print_slice moved the cursor */
				printf("\033[%ldD", line_len - cursor_pos);
			} else {
				putchar(c);
				s[line_len++] = c;
				cursor_pos++;
			}
		}
	}
	printf("\r\n");
	s[line_len] = '\0';
	return s;
}


void msh_execute(char **argv) {
	int status;
	pid_t pid;

	pid = fork();
	if (pid == 0) {
		/* child process */
		if(execvp(argv[0], argv) < 0)
			perror(argv[0]);

		exit(EXIT_FAILURE);
	}
	else if (pid < 0) {
		perror("msh: could not fork");
	}
	else {
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
}

void msh_loop(void) {
	char *buffer, **tokens;

	buffer_alloc = READLINE_BUFFER_INIT_SIZE;
	if(!(buffer = calloc(buffer_alloc, sizeof(char))))
		exit(EXIT_FAILURE);

	tokens_alloc = TOKENS_BUFFER_INIT_SIZE;
	if(!(tokens = malloc(sizeof(char*) * tokens_alloc))) {
		free(buffer);
		exit(EXIT_FAILURE);
	}

	while (run) {
		tui_set_input_mode();

		/* make cusor blinking vertical bar */
		printf("\r\033[5 q$ ");
		buffer = editor_read_line(buffer);

		tui_reset_input_mode();

		tokens = buffer_split(buffer, tokens);

		/* skip execute if buffer is empty */
		if(strlen(buffer) > 0)
			msh_execute(tokens);
		
		buffer_clear(buffer);
	}
	free(tokens);
	free(buffer);
}

int main (void) {
	msh_loop();
	return 0;
}
