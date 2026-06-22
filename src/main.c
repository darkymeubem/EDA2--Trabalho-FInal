/*
 * main.c — GraphFinance
 * Análise textual de notícias financeiras via Grafos de Coocorrência
 * UnB 2026
 *
 * Orquestrador fino. As etapas vivem em módulos:
 *   util/        — leitura de CSV, texto, datas
 *   pipeline/    — vocabulário (DF/TF-IDF), coocorrência (PPMI), relatório
 *   estruturas/  — grafo, fila
 *   algoritmos/  — BFS, componentes, Bron-Kerbosch, Prim
 *
 * Pipeline:
 *   1. Leitura do CSV
 *   2. Vocabulário + filtro de DF + seleção TF-IDF
 *   3. Construção do grafo com peso PPMI + limiar de conectividade
 *   4. Relatório: Grau, BFS, Componentes Conexas, MST (Prim),
 *      Cliques (Bron-Kerbosch), Análise temporal
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "config.h"
#include "csv.h"
#include "texto.h"
#include "pipeline/vocabulario.h"
#include "pipeline/coocorrencia.h"
#include "pipeline/relatorio.h"

/* ─────────────────────────────────────────────
 * Leitura do CSV via callback
 * ───────────────────────────────────────────── */

typedef struct {
    Documento *docs;
    int       *n_docs;
} CtxLeitura;

static void callback_csv(const LinhaCSV *linha, void *ctx) {
    CtxLeitura *c = (CtxLeitura *)ctx;
    if (*c->n_docs >= MAX_DOCS) return;
    if (linha->n_campos < 3)    return;
    if (strcmp(linha->campos[0], "data") == 0) return;   /* cabeçalho */

    Documento *doc = &c->docs[*c->n_docs];
    strncpy(doc->data, linha->campos[0], 15);
    doc->data[15] = '\0';

    char texto_bruto[MAX_CAMPO_LEN * 2];
    snprintf(texto_bruto, sizeof(texto_bruto), "%s %s",
             linha->campos[1], linha->campos[2]);

    char texto_norm[MAX_CAMPO_LEN * 2];
    normalizar_texto(texto_bruto, texto_norm);
    doc->n_tokens = tokenizar(texto_norm, doc->tokens, MAX_TOKENS_DOC);

    (*c->n_docs)++;
}

static void linha_de(char c, int n) {
    for (int i = 0; i < n; i++) putchar(c);
    putchar('\n');
}

/* ─────────────────────────────────────────────
 * main
 * ───────────────────────────────────────────── */

int main(int argc, char *argv[]) {
#ifdef _WIN32
    /* Força o console do Windows a interpretar saída como UTF-8,
     * evitando caracteres acentuados corrompidos (mojibake). */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    const char *caminho_csv = "dados/finais/noticias.csv";
    double      limiar      = LIMIAR_PADRAO;
    int         top_n       = TOP_N_TERMOS;

    if (argc >= 2) caminho_csv = argv[1];
    if (argc >= 3) limiar      = atof(argv[2]);
    if (argc >= 4) top_n       = atoi(argv[3]);

    printf("\n");
    linha_de('#', 60);
    printf("  GraphFinance — Grafo de Coocorrência Financeiro\n");
    printf("  Corpus : %s\n", caminho_csv);
    printf("  Limiar PPMI : %.2f  |  Termos TF-IDF: %d\n", limiar, top_n);
    linha_de('#', 60);

    /* ── 1. Leitura do CSV ── */
    Documento *docs   = calloc(MAX_DOCS, sizeof(Documento));
    int        n_docs = 0;

    CtxLeitura ctx = { docs, &n_docs };
    csv_ler_arquivo(caminho_csv, callback_csv, &ctx);

    printf("\n[1/6] Corpus carregado       : %d documentos\n", n_docs);
    if (n_docs == 0) {
        fprintf(stderr, "Erro: nenhum documento carregado.\n");
        free(docs); return 1;
    }

    /* ── 2. Vocabulário + filtro DF + TF-IDF ── */
    vocab_construir(docs, n_docs);
    int n_vocab = vocab_tamanho();
    printf("[2/6] Vocabulário (busca bin.): %d termos únicos\n", n_vocab);

    int n_aceitos = vocab_contar_aceitos(n_docs);
    printf("[3/6] Após filtro de DF       : %d termos (min=%.0f%%, max=%.0f%%)\n",
           n_aceitos, DF_MIN_FRAC * 100, DF_MAX_FRAC * 100);

    char termos[TOP_N_TERMOS][MAX_TOKEN_LEN];
    int  n_termos = selecionar_termos(docs, n_docs, termos,
                                      top_n < TOP_N_TERMOS ? top_n : TOP_N_TERMOS);
    printf("[4/6] Termos selecionados TF-IDF: %d\n", n_termos);

    /* Ordenar termos para habilitar busca binária na construção do grafo */
    ordenar_termos(termos, n_termos);

    /* ── 3. Construção do grafo com PPMI + limiar ── */
    Grafo *g_completo = construir_grafo_ppmi(docs, n_docs, termos, n_termos);
    if (!g_completo) {
        fprintf(stderr, "Erro ao construir grafo.\n");
        free(docs); return 1;
    }
    printf("[5/6] Grafo PPMI completo     : %d vértices, %d arestas\n",
           g_completo->n_vert, g_completo->n_arest);

    Grafo *g = aplicar_limiar(g_completo, limiar);
    printf("[6/6] Após limiar PPMI %.2f   : %d arestas restantes\n\n",
           limiar, g->n_arest);
    grafo_liberar(g_completo);

    /* ── 4. Relatório ── */
    relatorio_gerar(g, termos, n_termos, docs, n_docs, limiar,
                    n_vocab, n_aceitos);

    printf("\n");
    linha_de('#', 60);
    printf("  GraphFinance · UnB 2026\n");
    linha_de('#', 60);
    printf("\n");

    grafo_liberar(g);
    free(docs);
    return 0;
}
