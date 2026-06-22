#include "coocorrencia.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ─────────────────────────────────────────────
 * Mapeamento termo -> índice de vértice
 * (busca binária no array de termos selecionados)
 * ───────────────────────────────────────────── */

static int termo_para_vertice(const char termos[][MAX_TOKEN_LEN],
                              int n_termos, const char *termo) {
    int lo = 0, hi = n_termos - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int cmp = strcmp(termo, termos[mid]);
        if (cmp == 0) return mid;
        if (cmp < 0)  hi = mid - 1;
        else          lo = mid + 1;
    }
    return -1;
}

void ordenar_termos(char termos[][MAX_TOKEN_LEN], int n) {
    for (int i = 0; i < n - 1; i++) {
        int idx_min = i;
        for (int j = i + 1; j < n; j++)
            if (strcmp(termos[j], termos[idx_min]) < 0) idx_min = j;
        if (idx_min != i) {
            char tmp[MAX_TOKEN_LEN];
            strcpy(tmp, termos[i]);
            strcpy(termos[i], termos[idx_min]);
            strcpy(termos[idx_min], tmp);
        }
    }
}

/* ─────────────────────────────────────────────
 * PPMI — Positive Pointwise Mutual Information
 *
 * PPMI(u,v) = max(0, log( P(u,v) / (P(u) * P(v)) ))
 *
 * cooc(u,v) e freq_doc(u) são ambos contados POR DOCUMENTO, então as três
 * probabilidades vivem no mesmo espaço amostral (documentos) e usam n_docs
 * como denominador comum:
 *   P(u,v) = cooc(u,v)  / n_docs
 *   P(u)   = freq_doc(u) / n_docs
 *   P(v)   = freq_doc(v) / n_docs
 * ───────────────────────────────────────────── */

static double calcular_ppmi(double cooc_uv, double freq_u, double freq_v,
                            double n_docs) {
    if (cooc_uv <= 0.0 || freq_u <= 0.0 || freq_v <= 0.0) return 0.0;

    double p_uv = cooc_uv / n_docs;
    double p_u  = freq_u  / n_docs;
    double p_v  = freq_v  / n_docs;

    double pmi = log(p_uv / (p_u * p_v));
    return pmi > 0.0 ? pmi : 0.0;
}

/* ─────────────────────────────────────────────
 * Construção do grafo com peso PPMI
 * ───────────────────────────────────────────── */

Grafo *construir_grafo_ppmi(Documento *docs, int n_docs,
                            char termos[][MAX_TOKEN_LEN], int n_termos) {

    Grafo *g = grafo_criar(n_termos);
    if (!g) return NULL;

    /* Matriz de coocorrência (array 2D plano) */
    double *cooc = calloc((size_t)n_termos * n_termos, sizeof(double));
    /* Frequência de documento por vértice */
    double *freq = calloc((size_t)n_termos, sizeof(double));

    if (!cooc || !freq) {
        free(cooc); free(freq);
        grafo_liberar(g);
        return NULL;
    }

    double total_pares = 0.0;

    for (int d = 0; d < n_docs; d++) {
        /* Coleta vértices presentes neste doc (sem duplicata) */
        int presentes[TOP_N_TERMOS];
        int n_pres = 0;

        for (int t = 0; t < docs[d].n_tokens && n_pres < TOP_N_TERMOS; t++) {
            int v = termo_para_vertice(
                        (const char (*)[MAX_TOKEN_LEN])termos,
                        n_termos, docs[d].tokens[t]);
            if (v < 0) continue;
            int ja = 0;
            for (int k = 0; k < n_pres; k++)
                if (presentes[k] == v) { ja = 1; break; }
            if (!ja) {
                presentes[n_pres++] = v;
                freq[v] += 1.0;
            }
        }

        /* Incrementa coocorrência para todos os pares do doc */
        for (int i = 0; i < n_pres; i++)
            for (int j = i + 1; j < n_pres; j++) {
                cooc[(size_t)presentes[i] * n_termos + presentes[j]] += 1.0;
                total_pares += 1.0;
            }
    }

    /* Calcula PPMI e adiciona arestas */
    if (total_pares > 0.0) {
        for (int i = 0; i < n_termos; i++) {
            for (int j = i + 1; j < n_termos; j++) {
                double c = cooc[(size_t)i * n_termos + j];
                if (c <= 0.0) continue;
                double ppmi = calcular_ppmi(c, freq[i], freq[j],
                                            (double)n_docs);
                if (ppmi > 0.0)
                    grafo_adicionar_aresta(g, i, j, ppmi);
            }
        }
    }

    free(cooc);
    free(freq);
    return g;
}

/* ─────────────────────────────────────────────
 * Aplicação do limiar de conectividade
 * ───────────────────────────────────────────── */

Grafo *aplicar_limiar(const Grafo *g, double limiar) {
    Grafo *gl = grafo_criar(g->n_vert);
    if (!gl) return NULL;
    for (int u = 0; u < g->n_vert; u++) {
        ArestaNo *a = g->adj[u];
        while (a) {
            if (a->vizinho > u && a->peso >= limiar)
                grafo_adicionar_aresta(gl, u, a->vizinho, a->peso);
            a = a->prox;
        }
    }
    return gl;
}
