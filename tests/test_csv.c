#include "./../src/csv.h"
#include "testutil.h"
#include <stdio.h>
#include <string.h>

/* --- csv_parse_linha (parsing de uma linha ja em memoria) --- */

static void testar_parse_linha(void) {
  LinhaCSV lc;

  /* caso simples, sem aspas */
  csv_parse_linha("2023-01-05,Fed raises rates,Markets react", &lc);
  CHECK(lc.n_campos == 3, "campos simples: n_campos == 3");
  CHECK(strcmp(lc.campos[0], "2023-01-05") == 0, "campo 0 correto (sem aspas)");
  CHECK(strcmp(lc.campos[1], "Fed raises rates") == 0,
        "campo 1 correto (sem aspas)");
  CHECK(strcmp(lc.campos[2], "Markets react") == 0,
        "campo 2 correto (sem aspas)");

  /* campo entre aspas com virgula interna */
  csv_parse_linha("2023-02-10,\"Inflation, rates, and the Fed\",summary", &lc);
  CHECK(lc.n_campos == 3, "campo com virgula entre aspas: n_campos == 3");
  CHECK(strcmp(lc.campos[1], "Inflation, rates, and the Fed") == 0,
        "campo com virgula interna preservado corretamente");

  /* aspas escapadas dentro de campo entre aspas */
  csv_parse_linha("1,\"He said \"\"rates up\"\" today\",2", &lc);
  CHECK(lc.n_campos == 3, "aspas escapadas: n_campos == 3");
  CHECK(strcmp(lc.campos[1], "He said \"rates up\" today") == 0,
        "aspas escapadas viram aspas literal no campo");

  /* campo vazio no meio */
  csv_parse_linha("a,,c", &lc);
  CHECK(lc.n_campos == 3, "campo vazio no meio: n_campos == 3");
  CHECK(strcmp(lc.campos[1], "") == 0, "campo vazio fica string vazia");

  /* linha vazia */
  csv_parse_linha("", &lc);
  CHECK(lc.n_campos == 1, "linha vazia: n_campos == 1 (um campo vazio)");
  CHECK(strcmp(lc.campos[0], "") == 0, "linha vazia: campo 0 eh string vazia");
}

/* --- csv_ler_arquivo (le um arquivo real do disco) --- */

#define MAX_LINHAS_LIDAS 16

typedef struct {
  LinhaCSV linhas[MAX_LINHAS_LIDAS];
  int n;
} ContextoTeste;

static void callback_coletar(const LinhaCSV *linha, void *ctx_void) {
  ContextoTeste *ctx = (ContextoTeste *)ctx_void;
  if (ctx->n < MAX_LINHAS_LIDAS) {
    ctx->linhas[ctx->n] = *linha;
    ctx->n++;
  }
}

static void testar_ler_arquivo(void) {
  const char *caminho = "tests/tmp_test_csv.csv";

  FILE *f = fopen(caminho, "w");
  CHECK(f != NULL, "conseguiu criar arquivo CSV temporario para teste");
  if (!f)
    return;

  fprintf(f, "date,headline,summary\n"); /* cabecalho, deve ser pulado */
  fprintf(f, "2023-01-05,Fed raises rates,Markets react\n");
  fprintf(f, "2023-02-10,\"Inflation, rates rise\",details\n");
  fprintf(f, "\n"); /* linha em branco, deve ser pulada */
  fprintf(f, "2023-03-01,SVB collapse,Banking crisis\n");
  fclose(f);

  ContextoTeste ctx;
  ctx.n = 0;
  csv_ler_arquivo(caminho, callback_coletar, &ctx);

  CHECK(ctx.n == 3, "csv_ler_arquivo: 3 linhas de dados lidas (cabecalho e "
                    "linha em branco pulados)");
  if (ctx.n >= 1)
    CHECK(strcmp(ctx.linhas[0].campos[1], "Fed raises rates") == 0,
          "primeira linha de dados lida corretamente");
  if (ctx.n >= 2)
    CHECK(strcmp(ctx.linhas[1].campos[1], "Inflation, rates rise") == 0,
          "segunda linha (com virgula entre aspas) lida corretamente");
  if (ctx.n >= 3)
    CHECK(strcmp(ctx.linhas[2].campos[1], "SVB collapse") == 0,
          "terceira linha lida corretamente");

  remove(caminho);
}

int main(void) {
  testar_parse_linha();
  testar_ler_arquivo();
  TEST_SUMMARY();
}
