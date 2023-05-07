#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define	READLINE_BUFFER_INIT_SIZE	2
#define TOKENS_BUFFER_INIT_SIZE		16

static bool run = true;
static size_t alloc_len;

void buffer_clear(char *buf) {
	size_t len = strlen(buf);
	for (size_t i = 0; i < len; i++)
		buf[i] = '\0';
}

char *buffer_read_line(void) {
	int c;
	char *s, *aux;
	size_t used_size;

	if(!(s = malloc(alloc_len = READLINE_BUFFER_INIT_SIZE)))
		return NULL;

	used_size = 0;
	while (1) {
		c = getchar();
		if((c == '\n') || (c == EOF)) break;

		if(used_size == (alloc_len - 1)) {
			if(!(aux = realloc(s, alloc_len += alloc_len))) {
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

char **buffer_split_delim(char *b, size_t *t) {
	char *p, **tokens, **aux;
	size_t tokens_count, tokens_alloc;

	tokens_alloc = TOKENS_BUFFER_INIT_SIZE;
	tokens = malloc(sizeof(char*) * TOKENS_BUFFER_INIT_SIZE);

	tokens_count = 0;
	for(p = b; (p = strtok(p, " ")); p = NULL) {
		if((tokens_count + 1) == tokens_alloc) {
			if(!(aux = realloc(tokens, tokens_alloc += tokens_alloc))) {
				perror("msh");
				for(size_t i = 0; i < tokens_count; i++)
					free(tokens[i]);
				free(tokens);
				return NULL;
			}
			tokens = aux;
		}

		tokens[tokens_count] = calloc(strlen(p) + 1, sizeof(char));
		strcpy(tokens[tokens_count++], p);
	}

	*t = tokens_count;
	return tokens;
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
	size_t tokens_count;

	while (run) {
		printf("> ");
		buffer = buffer_read_line();
		/* printf("%s\n", buffer); */
		tokens = buffer_split_delim(buffer, &tokens_count);

		msh_execute(tokens);
		for (size_t token = 0; token < tokens_count; token++)
			free(tokens[token]);
		free(buffer);
	}

	/* for (size_t token = 0; token < tokens_count; token++) */
	/* 	free(tokens[token]); */

	/* free(tokens); */
	/* free(buffer); */
}

int main (void) {
	msh_loop();
	return 0;
}
