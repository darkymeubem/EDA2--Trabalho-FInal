/*
 * main.c — GraphFinance
 * Análise textual de notícias financeiras via Grafos de Coocorrência
 * UnB 2026
 *
 * Pipeline:
 *   1. Leitura do CSV
 *   2. Tokenização e pré-processamento
 *   3. Vocabulário com array ordenado + busca binária
 *   4. Filtro de DF (remove termos muito raros ou muito frequentes)
 *   5. TF-IDF para seleção dos termos mais relevantes
 *   6. Construção do grafo com peso PPMI
 *   7. Aplicação do limiar de conectividade
 *   8. Algoritmos: Grau, BFS, Componentes Conexas, MST (Prim), Cliques (Bron-Kerbosch)
 *   9. Análise temporal por período
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "grafo.h"
#include "algoritmos_grafo.h"
#include "csv.h"
#include "texto.h"
#include "fila.h"

/* ─────────────────────────────────────────────
 * Constantes de configuração
 * ───────────────────────────────────────────── */
#define MAX_DOCS        60000   /* 1 documento = 1 manchete (granularidade original) */
#define MAX_TOKENS_DOC  256
#define MAX_VOCAB       60000
#define MAX_TOKEN_LEN   64
#define TOP_N_TERMOS    600     /* teto de termos selecionados (válvula de segurança; filtro de DF já faz o corte principal) */
#define LIMIAR_PADRAO   2.0     /* limiar PPMI para manter aresta            */
#define TOP_GRAU        15      /* top-N vértices por grau a exibir          */
#define MAX_PERIODO_LEN 8       /* "YYYY-MM\0"                               */
#define MAX_PERIODOS    24
#define DF_MIN_FRAC     0.003   /* ignora termos em < 0,3% dos docs (raros)  */
#define DF_MAX_FRAC     0.85    /* ignora termos em > 85% dos docs (gerais)  */

/* ─────────────────────────────────────────────
 * Estruturas auxiliares
 * ───────────────────────────────────────────── */

typedef struct {
    char data[16];
    char tokens[MAX_TOKENS_DOC][MAX_TOKEN_LEN];
    int  n_tokens;
} Documento;

typedef struct {
    char   termo[MAX_TOKEN_LEN];
    double score;
} TermoScore;

typedef struct {
    char periodo[MAX_PERIODO_LEN];
    int  indices[MAX_DOCS];
    int  n_docs;
} Periodo;

/* ─────────────────────────────────────────────
 * Vocabulário — array ordenado + busca binária
 * ───────────────────────────────────────────── */

typedef struct {
    char termo[MAX_TOKEN_LEN];
    int  df;          /* document frequency: em quantos docs aparece */
    int  idx_orig;    /* índice original antes de ordenar            */
} EntradaVocab;

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

/* ─────────────────────────────────────────────
 * Construção do vocabulário com DF
 * ───────────────────────────────────────────── */

static void construir_vocab(Documento *docs, int n_docs) {
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

/* ─────────────────────────────────────────────
 * Filtro de DF — remove termos raros e genéricos
 * ───────────────────────────────────────────── */

/*
 * Preenche 'aceito[i]' = 1 se vocab[i] passa no filtro de DF.
 * df_min = máximo de (1, floor(DF_MIN_FRAC * n_docs))
 * df_max = ceil(DF_MAX_FRAC * n_docs)
 */
static void filtrar_df(int n_docs, int *aceito) {
    int df_min = (int)(DF_MIN_FRAC * n_docs);
    if (df_min < 1) df_min = 1;
    int df_max = (int)(DF_MAX_FRAC * n_docs);
    if (df_max < df_min) df_max = n_docs;

    for (int i = 0; i < n_vocab; i++)
        aceito[i] = (vocab[i].df >= df_min && vocab[i].df <= df_max);
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

/*
 * Seleciona os top_n termos por TF-IDF médio dentre os que passam no filtro de DF.
 * Preenche 'selecionados' com os nomes e retorna quantos foram selecionados.
 */
static int selecionar_termos(Documento *docs, int n_docs,
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

/* Ordena o array de termos selecionados (selection sort) */
static void ordenar_termos(char termos[][MAX_TOKEN_LEN], int n) {
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

static Grafo *construir_grafo_ppmi(
        Documento *docs, int n_docs,
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

static Grafo *aplicar_limiar(const Grafo *g, double limiar) {
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

/* ─────────────────────────────────────────────
 * Análise temporal
 * ───────────────────────────────────────────── */

static void extrair_periodo(const char *data, char *periodo) {
    strncpy(periodo, data, 7);
    periodo[7] = '\0';
}

static int buscar_periodo(Periodo *periodos, int n, const char *p) {
    for (int i = 0; i < n; i++)
        if (strcmp(periodos[i].periodo, p) == 0) return i;
    return -1;
}

/* ─────────────────────────────────────────────
 * Saída — utilitários de formatação
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

    const char *caminho_csv = "dados/noticias.csv";
    double      limiar      = LIMIAR_PADRAO;
    int         top_n       = TOP_N_TERMOS;

    if (argc >= 2) caminho_csv = argv[1];
    if (argc >= 3) limiar      = atof(argv[2]);
    if (argc >= 4) top_n       = atoi(argv[3]);

    printf("\n");
    separador('#', 60);
    printf("  GraphFinance — Grafo de Coocorrência Financeiro\n");
    printf("  Corpus : %s\n", caminho_csv);
    printf("  Limiar PPMI : %.2f  |  Termos TF-IDF: %d\n", limiar, top_n);
    separador('#', 60);

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
    construir_vocab(docs, n_docs);
    printf("[2/6] Vocabulário (busca bin.): %d termos únicos\n", n_vocab);

    /* Contar quantos passam no filtro de DF */
    int *aceito_df = malloc(MAX_VOCAB * sizeof(int));
    if (!aceito_df) { perror("malloc"); exit(1); }
    filtrar_df(n_docs, aceito_df);
    int n_aceitos = 0;
    for (int i = 0; i < n_vocab; i++) n_aceitos += aceito_df[i];
    free(aceito_df);
    printf("[3/6] Após filtro de DF       : %d termos (min=%.0f%%, max=%.0f%%)\n",
           n_aceitos, DF_MIN_FRAC * 100, DF_MAX_FRAC * 100);

    /* Selecionar top_n por TF-IDF */
    char termos[TOP_N_TERMOS][MAX_TOKEN_LEN];
    int  n_termos = selecionar_termos(docs, n_docs, termos,
                                      top_n < TOP_N_TERMOS ? top_n : TOP_N_TERMOS);
    printf("[4/6] Termos selecionados TF-IDF: %d\n", n_termos);

    /* Ordenar termos para habilitar busca binária em termo_para_vertice */
    ordenar_termos(termos, n_termos);

    /* ── 3. Construção do grafo com PPMI ── */
    Grafo *g_completo = construir_grafo_ppmi(docs, n_docs, termos, n_termos);
    if (!g_completo) {
        fprintf(stderr, "Erro ao construir grafo.\n");
        free(docs); return 1;
    }
    printf("[5/6] Grafo PPMI completo     : %d vértices, %d arestas\n",
           g_completo->n_vert, g_completo->n_arest);

    /* ── 4. Aplicar limiar PPMI ── */
    Grafo *g = aplicar_limiar(g_completo, limiar);
    printf("[6/6] Após limiar PPMI %.2f   : %d arestas restantes\n\n",
           limiar, g->n_arest);
    grafo_liberar(g_completo);

    /* ── Ordenação por grau — usada nas seções seguintes ── */
    int ordem[TOP_N_TERMOS];
    for (int i = 0; i < n_termos; i++) ordem[i] = i;
    for (int i = 0; i < n_termos - 1; i++) {
        int idx_max = i;
        for (int j = i + 1; j < n_termos; j++)
            if (g->grau[ordem[j]] > g->grau[ordem[idx_max]]) idx_max = j;
        int tmp = ordem[i]; ordem[i] = ordem[idx_max]; ordem[idx_max] = tmp;
    }

    /* ═══════════════════════════════════════════
     * [01] Grau dos vértices
     * ═══════════════════════════════════════════ */
    titulo_secao(1, "GRAU DOS VERTICES");
    printf("  %-22s  %s\n", "Termo", "Grau");
    separador('-', 40);

    int exibir = (TOP_GRAU < n_termos) ? TOP_GRAU : n_termos;
    for (int i = 0; i < exibir; i++) {
        int v = ordem[i];
        if (g->grau[v] == 0) break;
        printf("  %-22s  %d\n", termos[v], g->grau[v]);
    }

    /* ═══════════════════════════════════════════
     * [02] BFS — distâncias a partir do termo mais central
     * ═══════════════════════════════════════════ */
    titulo_secao(2, "BFS — DISTANCIAS A PARTIR DO TERMO CENTRAL");

    int origem_bfs = ordem[0];
    printf("  Origem: \"%s\" (grau %d)\n\n", termos[origem_bfs], g->grau[origem_bfs]);

    /* BFS usando o módulo de fila (fila_criar/enfileirar/desenfileirar) */
    int  *dist = malloc(n_termos * sizeof(int));
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

    /* ═══════════════════════════════════════════
     * [03] Componentes conexas (BFS)
     *
     * O grafo de coocorrência é não-direcionado, então a noção relevante
     * é a de componentes conexas (não SCC, que pressupõe arestas dirigidas).
     * Reutiliza componentes_conexas() do módulo algoritmos_grafo.
     * ═══════════════════════════════════════════ */
    titulo_secao(3, "COMPONENTES CONEXAS (BFS)");

    int *componente = malloc(n_termos * sizeof(int));
    int  n_comps    = componentes_conexas(g, componente);

    /* Tamanho de cada componente */
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

    int singletons   = 0;
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

    /* ═══════════════════════════════════════════
     * [04] MST — Prim máximo (sem heap)
     * ═══════════════════════════════════════════ */
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

    /* ═══════════════════════════════════════════
     * [05] Cliques — Bron-Kerbosch
     * ═══════════════════════════════════════════ */
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

    /* ═══════════════════════════════════════════
     * [06] Análise temporal por mês
     * ═══════════════════════════════════════════ */
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
        for (int i = 0; i < n_termos; i++) ord_p[i] = i;
        for (int i = 0; i < n_termos - 1; i++) {
            int im = i;
            for (int j = i + 1; j < n_termos; j++)
                if (gp_lim->grau[ord_p[j]] > gp_lim->grau[ord_p[im]]) im = j;
            int tmp = ord_p[i]; ord_p[i] = ord_p[im]; ord_p[im] = tmp;
        }

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

    /* ── Resumo final ── */
    titulo_secao(7, "RESUMO FINAL");
    printf("  Documentos analisados    : %d\n", n_docs);
    printf("  Vocabulário total        : %d termos\n", n_vocab);
    printf("  Após filtro de DF        : %d termos\n", n_aceitos);
    printf("  Termos no grafo (TF-IDF) : %d\n", n_termos);
    printf("  Arestas (pós-limiar PPMI): %d\n", g->n_arest);
    printf("  Limiar PPMI aplicado     : %.2f\n", limiar);
    printf("  Períodos detectados      : %d\n\n", n_periodos);
    separador('#', 60);
    printf("  GraphFinance · UnB 2026\n");
    separador('#', 60);
    printf("\n");

    grafo_liberar(g);
    free(periodos);
    free(docs);
    return 0;
}