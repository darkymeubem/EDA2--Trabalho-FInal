#include "./../src/texto.h"
#include "testutil.h"
#include <string.h>

static void testar_normalizar_texto(void) {
  char saida[256];

  normalizar_texto("Hello World!", saida);
  CHECK(strcmp(saida, "hello world") == 0,
        "normalizar: minusculas e pontuacao removida");

  normalizar_texto("THE FED RAISES RATES.", saida);
  CHECK(strcmp(saida, "the fed raises rates") == 0,
        "normalizar: maiusculas viram minusculas");

  normalizar_texto("inflation,  rates...   and bonds!!", saida);
  CHECK(strcmp(saida, "inflation rates and bonds") == 0,
        "normalizar: pontuacao e espacos multiplos colapsam em um espaco");

  normalizar_texto("  leading and trailing  ", saida);
  CHECK(strcmp(saida, "leading and trailing") == 0,
        "normalizar: espacos no inicio/fim sao removidos");

  normalizar_texto("inflação e crédito", saida);
  CHECK(strcmp(saida, "inflacao e credito") == 0,
        "normalizar: acentos (UTF-8 Latin-1) removidos corretamente");

  normalizar_texto("123 rates up 50%", saida);
  CHECK(strcmp(saida, "123 rates up 50") == 0,
        "normalizar: digitos mantidos, simbolo % removido");

  normalizar_texto("", saida);
  CHECK(strcmp(saida, "") == 0, "normalizar: string vazia permanece vazia");
}

static void testar_eh_stopword(void) {
  CHECK(eh_stopword("the") == 1, "'the' eh stopword");
  CHECK(eh_stopword("and") == 1, "'and' eh stopword");
  CHECK(eh_stopword("of") == 1, "'of' eh stopword");
  CHECK(eh_stopword("inflation") == 0, "'inflation' nao eh stopword");
  CHECK(eh_stopword("fed") == 0, "'fed' nao eh stopword");
  CHECK(eh_stopword("THE") == 0,
        "busca eh case-sensitive: 'THE' (maiusculo) nao bate na lista");
  CHECK(eh_stopword("") == 0, "string vazia nao eh stopword");
}

static void testar_tokenizar(void) {
  char tokens[32][MAX_TOKEN_LEN];
  int n;

  n = tokenizar("the fed raises interest rates", tokens, 32);
  CHECK(n == 4, "tokenizar: stopword 'the' removida, restam 4 tokens");
  CHECK(strcmp(tokens[0], "fed") == 0, "tokenizar: primeiro token correto");
  CHECK(strcmp(tokens[1], "raises") == 0, "tokenizar: segundo token correto");
  CHECK(strcmp(tokens[2], "interest") == 0,
        "tokenizar: terceiro token correto");
  CHECK(strcmp(tokens[3], "rates") == 0, "tokenizar: quarto token correto");

  n = tokenizar("the of and", tokens, 32);
  CHECK(n == 0, "tokenizar: texto so com stopwords resulta em 0 tokens");

  n = tokenizar("inflation rate hike", tokens, 2);
  CHECK(n == 2,
        "tokenizar: respeita max_tokens mesmo com mais palavras disponiveis");

  n = tokenizar("", tokens, 32);
  CHECK(n == 0, "tokenizar: string vazia resulta em 0 tokens");
}

int main(void) {
  testar_normalizar_texto();
  testar_eh_stopword();
  testar_tokenizar();
  TEST_SUMMARY();
}
