#include "relatorio.h"
#include "coocorrencia.h"
#include "algoritmos_grafo.h"
#include "fila.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─────────────────────────────────────────────
 * Período (usado só na análise temporal)
 * ───────────────────────────────────────────── */
typedef struct {
    char periodo[MAX_PERIODO_LEN];
    int  indices[MAX_DOCS];
    int  n_docs;
} Periodo;

/* ─────────────────────────────────────────────
 * Utilitários de formatação
 * ───────────────────────────────────────────── */

static void separador(char c, int n) {
    for (int i = 0; i < n; i++) putchar(c);
    putchar('\n');
}

static void titulo_secao(int num, const char *nome) {
    printf("\n");
    separador('=', 60);
    printf("  [%02d] %s\n", num, nome);
    separador('=', 60);
}

/* Ordena 'ordem' (0..n_termos-1) por grau decrescente (selection sort). */
static void ordenar_por_grau(const Grafo *g, int *ordem, int n_termos) {
    for (int i = 0; i < n_termos; i++) ordem[i] = i;
    for (int i = 0; i < n_termos - 1; i++) {
        int idx_max = i;
        for (int j = i + 1; j < n_termos; j++)
            if (g->grau[ordem[j]] > g->grau[ordem[idx_max]]) idx_max = j;
        int tmp = ordem[i]; ordem[i] = ordem[idx_max]; ordem[idx_max] = tmp;
    }
}

/* ═══════════════ [01] Grau dos vértices ═══════════════ */
static void sec_grau(const Grafo *g, char termos[][MAX_TOKEN_LEN],
                     int n_termos, const int *ordem) {
    titulo_secao(1, "GRAU DOS VERTICES");
    printf("  %-22s  %s\n", "Termo", "Grau");
    separador('-', 40);

    int exibir = (TOP_GRAU < n_termos) ? TOP_GRAU : n_termos;
    for (int i = 0; i < exibir; i++) {
        int v = ordem[i];
        if (g->grau[v] == 0) break;
        printf("  %-22s  %d\n", termos[v], g->grau[v]);
    }
}

/* ═══════════════ [02] BFS a partir do termo central ═══════════════ */
static void sec_bfs(const Grafo *g, char termos[][MAX_TOKEN_LEN],
                    int n_termos, const int *ordem) {
    titulo_secao(2, "BFS — DISTANCIAS A PARTIR DO TERMO CENTRAL");

    int origem_bfs = ordem[0];
    printf("  Origem: \"%s\" (grau %d)\n\n", termos[origem_bfs], g->grau[origem_bfs]);

    int *dist = malloc(n_termos * sizeof(int));
    for (int i = 0; i < n_termos; i++) dist[i] = -1;

    Fila *fila_bfs = fila_criar(n_termos);
    dist[origem_bfs] = 0;
    fila_enfileirar(fila_bfs, origem_bfs);

    while (!fila_vazia(fila_bfs)) {
        int u = fila_desenfileirar(fila_bfs);
        for (ArestaNo *a = g->adj[u]; a; a = a->prox) {
            if (dist[a->vizinho] == -1) {
                dist[a->vizinho] = dist[u] + 1;
                fila_enfileirar(fila_bfs, a->vizinho);
            }
        }
    }
    fila_liberar(fila_bfs);

    int max_dist = 0;
    for (int i = 0; i < n_termos; i++)
        if (dist[i] > max_dist) max_dist = dist[i];

    for (int d = 0; d <= max_dist; d++) {
        printf("  [%s] ", d == 0 ? "Origem  " : (d == 1 ? "Dist. 1 " :
                                                  d == 2 ? "Dist. 2 " : "Dist. 3+"));
        int primeiro = 1;
        for (int v = 0; v < n_termos; v++) {
            if (dist[v] != d) continue;
            if (!primeiro) printf(", ");
            printf("%s", termos[v]);
            primeiro = 0;
        }
        printf("\n");
    }
    free(dist);
}

/* ═══════════════ [03] Componentes conexas ═══════════════ */
static void sec_componentes(const Grafo *g, char termos[][MAX_TOKEN_LEN],
                            int n_termos) {
    titulo_secao(3, "COMPONENTES CONEXAS (BFS)");

    int *componente = malloc(n_termos * sizeof(int));
    int  n_comps    = componentes_conexas(g, componente);

    int *tam_comp = calloc(n_comps, sizeof(int));
    for (int v = 0; v < n_termos; v++) tam_comp[componente[v]]++;

    /* Ordem dos componentes por tamanho decrescente (selection sort) */
    int *ord_comp = malloc(n_comps * sizeof(int));
    for (int i = 0; i < n_comps; i++) ord_comp[i] = i;
    for (int i = 0; i < n_comps - 1; i++) {
        int idx_max = i;
        for (int j = i + 1; j < n_comps; j++)
            if (tam_comp[ord_comp[j]] > tam_comp[ord_comp[idx_max]]) idx_max = j;
        int tmp = ord_comp[i]; ord_comp[i] = ord_comp[idx_max]; ord_comp[idx_max] = tmp;
    }

    int singletons    = 0;
    int exibidos_comp = 0;
    for (int i = 0; i < n_comps; i++) {
        int c = ord_comp[i];
        if (tam_comp[c] == 1) { singletons++; continue; }
        if (exibidos_comp < 8) {
            printf("  Componente #%d (%d termos): ", exibidos_comp + 1, tam_comp[c]);
            int primeiro = 1;
            for (int v = 0; v < n_termos; v++) {
                if (componente[v] != c) continue;
                if (!primeiro) printf(", ");
                printf("%s", termos[v]);
                primeiro = 0;
            }
            printf("\n");
        }
        exibidos_comp++;
    }
    if (exibidos_comp == 0)
        printf("  Nenhum componente com 2+ termos encontrado.\n");
    printf("\n  Total de componentes: %d  |  Isolados: %d\n", n_comps, singletons);

    free(componente);
    free(tam_comp);
    free(ord_comp);
}

/* ═══════════════ [04] MST — Prim máximo ═══════════════ */
static void sec_mst(const Grafo *g, char termos[][MAX_TOKEN_LEN],
                    const int *ordem) {
    titulo_secao(4, "MST — ARVORE DE MAXIMO PESO PPMI (Prim)");

    ArvoreGeradora *mst = prim_maximo(g, ordem[0]);
    if (mst) {
        printf("  %-22s  %-22s  %s\n", "Termo A", "Termo B", "PPMI");
        separador('-', 55);
        for (int i = 0; i < mst->n_arestas; i++) {
            printf("  %-22s  %-22s  %.4f\n",
                   termos[mst->arestas[i].u],
                   termos[mst->arestas[i].v],
                   mst->arestas[i].peso);
        }
        printf("\n  Total de arestas na MST: %d\n", mst->n_arestas);
        arvore_geradora_liberar(mst);
    }
}

/* ═══════════════ [05] Cliques — Bron-Kerbosch ═══════════════ */
static void sec_cliques(const Grafo *g, char termos[][MAX_TOKEN_LEN]) {
    titulo_secao(5, "CLIQUES MAXIMAIS (Bron-Kerbosch, tamanho >= 3)");

    ListaCliques *lc = bron_kerbosch(g);
    if (lc) {
        /* Selection sort decrescente por tamanho */
        for (int i = 0; i < lc->n_cliques - 1; i++) {
            int idx_max = i;
            for (int j = i + 1; j < lc->n_cliques; j++)
                if (lc->itens[j].n > lc->itens[idx_max].n) idx_max = j;
            Clique tmp         = lc->itens[i];
            lc->itens[i]       = lc->itens[idx_max];
            lc->itens[idx_max] = tmp;
        }

        int exibidos = 0;
        for (int i = 0; i < lc->n_cliques && exibidos < 10; i++) {
            if (lc->itens[i].n < 3) continue;
            printf("  Clique #%d (%d termos): ", ++exibidos, lc->itens[i].n);
            for (int j = 0; j < lc->itens[i].n; j++) {
                if (j > 0) printf(", ");
                printf("%s", termos[lc->itens[i].vertices[j]]);
            }
            printf("\n");
        }
        if (exibidos == 0)
            printf("  Nenhum clique de tamanho >= 3 com este limiar.\n");
        printf("\n  Total de cliques encontrados: %d\n", lc->n_cliques);
        lista_cliques_liberar(lc);
    }
}

/* ═══════════════ [06] Análise temporal por mês ═══════════════ */
static void extrair_periodo(const char *data, char *periodo) {
    strncpy(periodo, data, 7);
    periodo[7] = '\0';
}

static int buscar_periodo(Periodo *periodos, int n, const char *p) {
    for (int i = 0; i < n; i++)
        if (strcmp(periodos[i].periodo, p) == 0) return i;
    return -1;
}

static void sec_temporal(char termos[][MAX_TOKEN_LEN], int n_termos,
                         Documento *docs, int n_docs, double limiar) {
    titulo_secao(6, "ANALISE TEMPORAL (por mes)");

    Periodo *periodos = malloc(MAX_PERIODOS * sizeof(Periodo));
    if (!periodos) { perror("malloc"); exit(1); }
    int     n_periodos = 0;

    for (int d = 0; d < n_docs; d++) {
        char p[MAX_PERIODO_LEN];
        extrair_periodo(docs[d].data, p);
        int idx = buscar_periodo(periodos, n_periodos, p);
        if (idx < 0) {
            if (n_periodos >= MAX_PERIODOS) continue;
            strncpy(periodos[n_periodos].periodo, p, MAX_PERIODO_LEN - 1);
            periodos[n_periodos].periodo[MAX_PERIODO_LEN - 1] = '\0';
            periodos[n_periodos].n_docs = 0;
            idx = n_periodos++;
        }
        if (periodos[idx].n_docs < MAX_DOCS)
            periodos[idx].indices[periodos[idx].n_docs++] = d;
    }

    /* Ordenar períodos cronologicamente (selection sort) */
    for (int i = 0; i < n_periodos - 1; i++) {
        int idx_min = i;
        for (int j = i + 1; j < n_periodos; j++)
            if (strcmp(periodos[j].periodo, periodos[idx_min].periodo) < 0)
                idx_min = j;
        Periodo tmp       = periodos[i];
        periodos[i]       = periodos[idx_min];
        periodos[idx_min] = tmp;
    }

    for (int p = 0; p < n_periodos; p++) {
        int n_sub = periodos[p].n_docs;

        Documento *sub = malloc(n_sub * sizeof(Documento));
        for (int i = 0; i < n_sub; i++)
            sub[i] = docs[periodos[p].indices[i]];

        Grafo *gp      = construir_grafo_ppmi(sub, n_sub, termos, n_termos);
        Grafo *gp_lim  = aplicar_limiar(gp, limiar);

        /* Top termos por grau no período */
        int ord_p[TOP_N_TERMOS];
        ordenar_por_grau(gp_lim, ord_p, n_termos);

        printf("\n  %s  (%d docs | %d arestas)\n",
               periodos[p].periodo, n_sub, gp_lim->n_arest);
        printf("  Termos: ");
        int mostrados = 0;
        for (int i = 0; i < n_termos && mostrados < 6; i++) {
            int v = ord_p[i];
            if (gp_lim->grau[v] == 0) break;
            if (mostrados > 0) printf(", ");
            printf("%s(grau=%d)", termos[v], gp_lim->grau[v]);
            mostrados++;
        }
        printf("\n");

        grafo_liberar(gp);
        grafo_liberar(gp_lim);
        free(sub);
    }

    free(periodos);
}

/* ═══════════════ [07] Resumo final ═══════════════ */
static void sec_resumo(const Grafo *g, int n_docs, double limiar,
                       int n_vocab, int n_aceitos, int n_termos) {
    titulo_secao(7, "RESUMO FINAL");
    printf("  Documentos analisados    : %d\n", n_docs);
    printf("  Vocabulário total        : %d termos\n", n_vocab);
    printf("  Após filtro de DF        : %d termos\n", n_aceitos);
    printf("  Termos no grafo (TF-IDF) : %d\n", n_termos);
    printf("  Arestas (pós-limiar PPMI): %d\n", g->n_arest);
    printf("  Limiar PPMI aplicado     : %.2f\n", limiar);
}

/* ─────────────────────────────────────────────
 * Orquestração do relatório
 * ───────────────────────────────────────────── */
void relatorio_gerar(const Grafo *g, char termos[][MAX_TOKEN_LEN], int n_termos,
                     Documento *docs, int n_docs, double limiar,
                     int n_vocab, int n_aceitos) {
    int *ordem = malloc(n_termos * sizeof(int));
    if (!ordem) { perror("malloc"); exit(1); }
    ordenar_por_grau(g, ordem, n_termos);

    sec_grau(g, termos, n_termos, ordem);
    sec_bfs(g, termos, n_termos, ordem);
    sec_componentes(g, termos, n_termos);
    sec_mst(g, termos, ordem);
    sec_cliques(g, termos);
    sec_temporal(termos, n_termos, docs, n_docs, limiar);
    sec_resumo(g, n_docs, limiar, n_vocab, n_aceitos, n_termos);

    free(ordem);
}
