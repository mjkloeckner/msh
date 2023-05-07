#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define	READLINE_BUFFER_INIT_SIZE	128
#define TOKENS_BUFFER_INIT_SIZE		8

static bool run = true;
static size_t buffer_alloc;
static size_t tokens_alloc;

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
			goto end;

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
end:
	s[used_size] = '\0';
	if (c == EOF) run = false;
	return s;
}

char **buffer_split_delim(char *b, char **t) {
	char *p, **aux;
	size_t tokens_count;

	tokens_count = 0;
	for(p = b; (p = strtok(p, " ")); p = NULL) {
		if((tokens_count + 1) == tokens_alloc) {
			tokens_alloc += tokens_alloc;
			if(!(aux = realloc(t, sizeof(char*) * tokens_alloc))) {
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
		free(buffer)
		exit(EXIT_FAILURE);
	}

	while (run) {
		/* make cusor blinking vertical bar */
		printf("\033[5 q$ ");
		buffer = buffer_read_line(buffer);
		tokens = buffer_split_delim(buffer, tokens);

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
