#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define	READLINE_BUFFER_INIT_SIZE	 2
#define TOKENS_BUFFER_INIT_SIZE		16

static bool run = true;
static size_t alloc_len;

void buffer_clear(char *buf, size_t *len) {
	for (size_t i = 0; i < *len; i++)
		buf[i] = '\0';

	*len = 0;
}

void buffer_read_line(char *buffer, size_t *len) {
	int c;
	char *tmp;

	/* read line into buffer */
	while(1) {
		c = fgetc(stdin);
		if((c == '\n') || (c == EOF)) break;

		if((*len + 2 == alloc_len)) {
			alloc_len += alloc_len;
			if((tmp = realloc(buffer, alloc_len)) == NULL) return;
			buffer = tmp;
		}

		buffer[(*len)++] = c;
	}
	buffer[*len] = '\0';
	if (c == EOF) run = false;
}

void buffer_split(char *b, char ***tokens, size_t *tokens_count) {
	if(b == NULL) return;

	char **tmp;
	size_t token_alloc;

	*tokens_count = 0;

	if(*tokens != NULL) return; 

	*tokens = malloc((sizeof(char *) * TOKENS_BUFFER_INIT_SIZE));
	token_alloc = TOKENS_BUFFER_INIT_SIZE;
	if (*tokens == NULL) return;

	(*tokens)[*tokens_count] = strtok(b, " ");
	while((*tokens)[*tokens_count] != NULL) {
		(*tokens)[++(*tokens_count)] = strtok(NULL, " ");
		if (*tokens_count == token_alloc) {
			token_alloc += token_alloc;
			tmp = realloc(*tokens, sizeof(char *) * token_alloc);
			if(tmp == NULL) return;
			*tokens = tmp;
		}
	}
}

void msh_execute(char **argv) {
	int status;
	pid_t pid;

	pid = fork();
	if (pid == 0) {
		/* child process */
		if(execvp(argv[0], argv) < 0) perror("msh");

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
	size_t len, tokens_count;

	buffer = malloc(alloc_len = READLINE_BUFFER_INIT_SIZE);

	len = 0;
	while (run) {
		buffer_clear(buffer, &len);

		printf("> ");

		buffer_read_line(buffer, &len);

		tokens = NULL;
		tokens_count = 0;
		buffer_split(buffer, &tokens, &tokens_count);

		msh_execute(tokens);
	}

	for (size_t token = 0; token < tokens_count; token++)
		free(tokens[token]);

	free(tokens);
	free(buffer);
}

int main (void) {
	msh_loop();
	return 0;
}
