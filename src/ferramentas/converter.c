/*
 * converter.c — unifica as fontes brutas de manchetes em um único CSV no
 * schema esperado pelo pipeline: "data,titulo,resumo" (data em ISO YYYY-MM-DD).
 *
 * Uso:
 *   ./converter > dados/finais/noticias.csv
 *
 * Lê os arquivos de fonte conhecidos em dados/, normaliza a data e descarta
 * linhas sem data reconhecível ou sem título. Reutiliza o parser de csv.c e o
 * normalizador de datas.c.
 */
#include "csv.h"
#include "datas.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    int   col_data;
    int   col_titulo;
    int   col_resumo;   /* -1 se a fonte não tem resumo */
    FILE *out;
    long  ok;
    long  descartadas;
} CtxConv;

/* Emite um campo CSV entre aspas, duplicando aspas internas. */
static void emitir_campo(FILE *out, const char *s) {
    fputc('"', out);
    for (const char *p = s; *p; p++) {
        if (*p == '"') fputc('"', out);
        fputc(*p, out);
    }
    fputc('"', out);
}

static void callback(const LinhaCSV *linha, void *ctx) {
    CtxConv *c = (CtxConv *)ctx;

    int n = linha->n_campos;
    if (c->col_data >= n || c->col_titulo >= n) { c->descartadas++; return; }

    const char *titulo = linha->campos[c->col_titulo];
    const char *resumo = (c->col_resumo >= 0 && c->col_resumo < n)
                             ? linha->campos[c->col_resumo] : "";

    if (titulo[0] == '\0') { c->descartadas++; return; }

    char iso[16];
    if (!data_para_iso(linha->campos[c->col_data], iso)) { c->descartadas++; return; }

    fprintf(c->out, "%s,", iso);
    emitir_campo(c->out, titulo);
    fputc(',', c->out);
    emitir_campo(c->out, resumo);
    fputc('\n', c->out);
    c->ok++;
}

typedef struct {
    const char *caminho;
    int col_data, col_titulo, col_resumo;
} Fonte;

int main(void) {
    /* Mapeamento de colunas por fonte (índices após csv_parse_linha). */
    Fonte fontes[] = {
        /* reuters:  Headlines, Time, Description */
        {"dados/brutos/reuters_headlines.csv",  1, 0, 2},
        /* cnbc:     Headlines, Time, Description */
        {"dados/brutos/cnbc_headlines.csv",     1, 0, 2},
        /* guardian: Time, Headlines (sem resumo)  */
        {"dados/brutos/guardian_headlines.csv", 0, 1, -1},
    };
    int n_fontes = (int)(sizeof(fontes) / sizeof(fontes[0]));

    printf("data,titulo,resumo\n");

    long total_ok = 0, total_desc = 0;
    for (int f = 0; f < n_fontes; f++) {
        CtxConv ctx = {fontes[f].col_data, fontes[f].col_titulo,
                       fontes[f].col_resumo, stdout, 0, 0};
        csv_ler_arquivo(fontes[f].caminho, callback, &ctx);
        fprintf(stderr, "%-32s : %ld linhas, %ld descartadas\n",
                fontes[f].caminho, ctx.ok, ctx.descartadas);
        total_ok += ctx.ok;
        total_desc += ctx.descartadas;
    }
    fprintf(stderr, "TOTAL: %ld linhas convertidas, %ld descartadas\n",
            total_ok, total_desc);
    return 0;
}
