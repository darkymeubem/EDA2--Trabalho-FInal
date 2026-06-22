#ifndef TEXTO_H
#define TEXTO_H

#define MAX_TOKEN_LEN 64

void normalizar_texto(const char *entrada, char *saida);

int eh_stopword(const char *palavra);

int tokenizar(const char *texto_normalizado, char tokens[][MAX_TOKEN_LEN],
              int max_tokens);

#endif
