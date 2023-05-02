#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define	READLINE_BUFFER_INIT_SIZE	 	1000
#define READLINE_BUFFER_GROWTH_FACTOR	0.5f
#define TOKENS_BUFFER_INIT_SIZE			16

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
			alloc_len += alloc_len * READLINE_BUFFER_GROWTH_FACTOR;
			if((tmp = realloc(buffer, alloc_len)) == NULL) return;
			buffer = tmp;
		}

		buffer[(*len)++] = c;
	}
	buffer[*len] = '\0';
	if (c == EOF) run = false;
}

void buffer_split(char *b, size_t *len, char ***tokens, size_t *tokens_count) {
	if(b == NULL) return;

	size_t token_alloc;
	char *start, **tmp;

	start = b;
	*tokens_count = 0;

	if(*tokens != NULL) return; 

	*tokens = malloc((sizeof(char *) * TOKENS_BUFFER_INIT_SIZE));
	token_alloc = TOKENS_BUFFER_INIT_SIZE;
	if (*tokens == NULL) return;

	(*tokens)[*tokens_count] = strtok(b, " ");
	while((*tokens)[*tokens_count] != NULL) {
		(*tokens)[++(*tokens_count)] = strtok(NULL, " ");
		if (*tokens_count == token_alloc) {
			token_alloc += TOKENS_BUFFER_INIT_SIZE;
			tmp = realloc(*tokens, sizeof(char *) * token_alloc);
			if(tmp == NULL) return;
			*tokens = tmp;
		}
	}

	b = start;
}

void msh_loop(void) {
	char *buffer;
	size_t len;

	len = 0;
	buffer = malloc(alloc_len = READLINE_BUFFER_INIT_SIZE);

	while (run) {
		buffer_clear(buffer, &len);

		printf("> ");

		buffer_read_line(buffer, &len);

		/* printf("%s\n", buffer); */

		char **tokens = NULL;
		size_t tokens_count = 0;
		buffer_split(buffer, &len, &tokens, &tokens_count);

		printf("tokens_count: %ld\n", tokens_count);
		for(size_t t = 0; t < tokens_count; t++)
			printf("%s\n", tokens[t]);
	}
}

int main (void) {
	/* load config files */

	msh_loop();

	/* cleanup */
	return 0;
}
