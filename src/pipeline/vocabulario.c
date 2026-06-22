#include "vocabulario.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ─────────────────────────────────────────────
 * Estado interno do vocabulário
 * ───────────────────────────────────────────── */

typedef struct {
    char termo[MAX_TOKEN_LEN];
    int  df;          /* document frequency: em quantos docs aparece */
    int  idx_orig;    /* índice original antes de ordenar            */
} EntradaVocab;

typedef struct {
    char   termo[MAX_TOKEN_LEN];
    double score;
} TermoScore;

static EntradaVocab vocab[MAX_VOCAB];
static int          n_vocab = 0;

/* Busca linear — usada durante a construção (vocab ainda cresce) */
static int vocab_buscar_linear(const char *termo) {
    for (int i = 0; i < n_vocab; i++)
        if (strcmp(vocab[i].termo, termo) == 0) return i;
    return -1;
}

/* Inserção — mantém idx_orig igual à posição de inserção */
static int vocab_inserir(const char *termo) {
    int idx = vocab_buscar_linear(termo);
    if (idx >= 0) return idx;
    if (n_vocab >= MAX_VOCAB) return -1;
    strncpy(vocab[n_vocab].termo, termo, MAX_TOKEN_LEN - 1);
    vocab[n_vocab].termo[MAX_TOKEN_LEN - 1] = '\0';
    vocab[n_vocab].df      = 0;
    vocab[n_vocab].idx_orig = n_vocab;
    return n_vocab++;
}

/* Ordena o vocab alfabeticamente (selection sort) para habilitar busca binária */
static void vocab_ordenar(void) {
    for (int i = 0; i < n_vocab - 1; i++) {
        int idx_min = i;
        for (int j = i + 1; j < n_vocab; j++)
            if (strcmp(vocab[j].termo, vocab[idx_min].termo) < 0)
                idx_min = j;
        if (idx_min != i) {
            EntradaVocab tmp = vocab[i];
            vocab[i]         = vocab[idx_min];
            vocab[idx_min]   = tmp;
        }
    }
}

/* ─────────────────────────────────────────────
 * Construção do vocabulário com DF
 * ───────────────────────────────────────────── */

void vocab_construir(Documento *docs, int n_docs) {
    /* Flag de aparição por documento (sem hash: array de flags).
     * Alocado no heap — MAX_VOCAB é grande demais para a pilha. */
    char *visto = malloc(MAX_VOCAB);
    if (!visto) { perror("malloc"); exit(1); }

    for (int d = 0; d < n_docs; d++) {
        memset(visto, 0, MAX_VOCAB);

        for (int t = 0; t < docs[d].n_tokens; t++) {
            int idx = vocab_inserir(docs[d].tokens[t]);
            if (idx < 0) continue;
            if (!visto[idx]) {
                vocab[idx].df++;
                visto[idx] = 1;
            }
        }
    }
    free(visto);
    /* Após construir todo o vocab, ordena para habilitar busca binária */
    vocab_ordenar();
}

int vocab_tamanho(void) {
    return n_vocab;
}

/* ─────────────────────────────────────────────
 * Filtro de DF — remove termos raros e genéricos
 *
 * Preenche 'aceito[i]' = 1 se vocab[i] passa no filtro de DF.
 * df_min = máximo de (1, floor(DF_MIN_FRAC * n_docs))
 * df_max = ceil(DF_MAX_FRAC * n_docs)
 * ───────────────────────────────────────────── */
static void filtrar_df(int n_docs, int *aceito) {
    int df_min = (int)(DF_MIN_FRAC * n_docs);
    if (df_min < 1) df_min = 1;
    int df_max = (int)(DF_MAX_FRAC * n_docs);
    if (df_max < df_min) df_max = n_docs;

    for (int i = 0; i < n_vocab; i++)
        aceito[i] = (vocab[i].df >= df_min && vocab[i].df <= df_max);
}

int vocab_contar_aceitos(int n_docs) {
    int *aceito = malloc(MAX_VOCAB * sizeof(int));
    if (!aceito) { perror("malloc"); exit(1); }
    filtrar_df(n_docs, aceito);
    int n = 0;
    for (int i = 0; i < n_vocab; i++) n += aceito[i];
    free(aceito);
    return n;
}

/* ─────────────────────────────────────────────
 * TF-IDF com filtro de DF
 * ───────────────────────────────────────────── */

static double calcular_tfidf(const Documento *doc, int vocab_idx, int n_docs) {
    int tf_raw = 0;
    for (int t = 0; t < doc->n_tokens; t++)
        if (strcmp(doc->tokens[t], vocab[vocab_idx].termo) == 0) tf_raw++;
    if (tf_raw == 0) return 0.0;

    double tf  = (double)tf_raw / (doc->n_tokens > 0 ? doc->n_tokens : 1);
    double idf = log((double)(n_docs + 1) / (vocab[vocab_idx].df + 1)) + 1.0;
    return tf * idf;
}

int selecionar_termos(Documento *docs, int n_docs,
                      char selecionados[][MAX_TOKEN_LEN], int top_n) {
    int *aceito = malloc(MAX_VOCAB * sizeof(int));
    TermoScore *scores = malloc(MAX_VOCAB * sizeof(TermoScore));
    if (!aceito || !scores) { perror("malloc"); exit(1); }
    filtrar_df(n_docs, aceito);

    int        n_scores = 0;

    for (int i = 0; i < n_vocab; i++) {
        if (!aceito[i]) continue;
        double soma = 0.0;
        for (int d = 0; d < n_docs; d++)
            soma += calcular_tfidf(&docs[d], i, n_docs);
        scores[n_scores].score = soma / n_docs;
        strncpy(scores[n_scores].termo, vocab[i].termo, MAX_TOKEN_LEN - 1);
        scores[n_scores].termo[MAX_TOKEN_LEN - 1] = '\0';
        n_scores++;
    }

    /* Selection sort decrescente por score */
    for (int i = 0; i < n_scores - 1; i++) {
        int idx_max = i;
        for (int j = i + 1; j < n_scores; j++)
            if (scores[j].score > scores[idx_max].score) idx_max = j;
        TermoScore tmp  = scores[i];
        scores[i]       = scores[idx_max];
        scores[idx_max] = tmp;
    }

    int n = (top_n < n_scores) ? top_n : n_scores;
    for (int i = 0; i < n; i++) {
        strncpy(selecionados[i], scores[i].termo, MAX_TOKEN_LEN - 1);
        selecionados[i][MAX_TOKEN_LEN - 1] = '\0';
    }

    free(aceito);
    free(scores);
    return n;
}
