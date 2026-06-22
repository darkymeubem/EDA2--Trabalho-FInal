#ifndef CONFIG_H
#define CONFIG_H

#include "texto.h"   /* MAX_TOKEN_LEN */

/* ─────────────────────────────────────────────
 * Constantes de configuração globais do pipeline
 * ───────────────────────────────────────────── */
#define MAX_DOCS        60000   /* 1 documento = 1 manchete (granularidade original) */
#define MAX_TOKENS_DOC  256
#define MAX_VOCAB       60000
#define TOP_N_TERMOS    600     /* teto de termos selecionados (válvula de segurança; filtro de DF já faz o corte principal) */
#define LIMIAR_PADRAO   2.0     /* limiar PPMI para manter aresta            */
#define TOP_GRAU        15      /* top-N vértices por grau a exibir          */
#define MAX_PERIODO_LEN 8       /* "YYYY-MM\0"                               */
#define MAX_PERIODOS    24
#define DF_MIN_FRAC     0.003   /* ignora termos em < 0,3% dos docs (raros)  */
#define DF_MAX_FRAC     0.85    /* ignora termos em > 85% dos docs (gerais)  */

/* Um documento já tokenizado (1 manchete). */
typedef struct {
    char data[16];
    char tokens[MAX_TOKENS_DOC][MAX_TOKEN_LEN];
    int  n_tokens;
} Documento;

#endif
