#include "config.h"
#include "coocorrencia.h"
#include "grafo.h"
#include "testutil.h"
#include <math.h>
#include <string.h>

/* Preenche um documento com a lista de tokens dada. */
static void doc_set(Documento *d, const char *toks[], int n) {
    d->data[0] = '\0';
    d->n_tokens = n;
    for (int i = 0; i < n; i++) {
        strncpy(d->tokens[i], toks[i], MAX_TOKEN_LEN - 1);
        d->tokens[i][MAX_TOKEN_LEN - 1] = '\0';
    }
}

/*
 * PPMI = log( cooc(u,v) * n_docs / (freq_u * freq_v) ), com cooc e freq
 * contados POR DOCUMENTO (mesmo espaço amostral => denominador n_docs).
 *
 * Caso forte: "a" e "b" coocorrem em 3 de 4 docs; "c" isolado.
 *   freq_a = 3, freq_b = 3, cooc_ab = 3, n_docs = 4
 *   PPMI(a,b) = log(3 * 4 / (3*3)) = log(12/9) ≈ 0.287682
 *
 * Este é o caso que pega o bug antigo: usar total_pares (=3) no lugar de
 * n_docs daria log(1/0.5625) ≈ 0.575 — valor diferente, teste falharia.
 */
static void testar_ppmi_associacao_forte(void) {
    char termos[3][MAX_TOKEN_LEN] = {"a", "b", "c"};  /* já ordenado */

    Documento docs[4];
    const char *d0[] = {"a", "b"};
    const char *d1[] = {"a", "b"};
    const char *d2[] = {"a", "b"};
    const char *d3[] = {"c"};
    doc_set(&docs[0], d0, 2);
    doc_set(&docs[1], d1, 2);
    doc_set(&docs[2], d2, 2);
    doc_set(&docs[3], d3, 1);

    Grafo *g = construir_grafo_ppmi(docs, 4, termos, 3);
    CHECK(g != NULL, "construir_grafo_ppmi retorna nao-NULL");
    CHECK(g->n_arest == 1, "apenas a aresta a-b existe (c eh isolado)");
    CHECK(g->grau[2] == 0, "vertice 'c' fica isolado (grau 0)");

    double esperado = log(12.0 / 9.0);  /* ≈ 0.287682 */
    double peso = grafo_peso_aresta(g, 0, 1);
    CHECK(fabs(peso - esperado) < 1e-6,
          "PPMI(a,b) usa n_docs como denominador (≈0.2877, nao 0.575)");

    grafo_liberar(g);
}

/*
 * Caso de independência: observado == esperado => PPMI = 0 => sem aresta.
 *   a em 2 docs, b em 2 docs, coocorrem em 1, n_docs = 4
 *   esperado = freq_a*freq_b/n_docs = 2*2/4 = 1 == cooc => log(1) = 0
 */
static void testar_ppmi_independencia_sem_aresta(void) {
    char termos[3][MAX_TOKEN_LEN] = {"a", "b", "c"};

    Documento docs[4];
    const char *d0[] = {"a", "b"};
    const char *d1[] = {"a"};
    const char *d2[] = {"b"};
    const char *d3[] = {"c"};
    doc_set(&docs[0], d0, 2);
    doc_set(&docs[1], d1, 1);
    doc_set(&docs[2], d2, 1);
    doc_set(&docs[3], d3, 1);

    Grafo *g = construir_grafo_ppmi(docs, 4, termos, 3);
    CHECK(g->n_arest == 0,
          "sob independencia (PPMI<=0) nenhuma aresta eh criada");

    grafo_liberar(g);
}

/*
 * Associação mais forte deve gerar PPMI maior que uma mais fraca — com AMBAS
 * as arestas existindo. Docs "x" (token fora do vocabulário) só elevam N sem
 * alterar freq/cooc, garantindo que os dois pares fiquem positivos.
 *   fa=3, fb=2, fc=2, cooc_ab=2, cooc_ac=1, N=10
 *   PPMI(a,b)=log(20/6)≈1.204   PPMI(a,c)=log(10/6)≈0.511
 */
static void testar_ppmi_monotonia(void) {
    char termos[3][MAX_TOKEN_LEN] = {"a", "b", "c"};

    Documento docs[10];
    const char *d0[] = {"a", "b"};
    const char *d1[] = {"a", "b"};
    const char *d2[] = {"a", "c"};
    const char *d3[] = {"c"};
    const char *dx[] = {"x"};
    doc_set(&docs[0], d0, 2);
    doc_set(&docs[1], d1, 2);
    doc_set(&docs[2], d2, 2);
    doc_set(&docs[3], d3, 1);
    for (int i = 4; i < 10; i++) doc_set(&docs[i], dx, 1);  /* fillers: só elevam N */

    Grafo *g = construir_grafo_ppmi(docs, 10, termos, 3);
    double peso_ab = grafo_peso_aresta(g, 0, 1);
    double peso_ac = grafo_peso_aresta(g, 0, 2);
    CHECK(peso_ab > 0.0, "par fortemente associado (a,b) tem PPMI positivo");
    CHECK(peso_ac > 0.0, "par fracamente associado (a,c) tambem tem PPMI positivo");
    CHECK(peso_ab > peso_ac,
          "associacao mais forte (a,b) > associacao mais fraca (a,c)");
    CHECK(fabs(peso_ab - log(20.0 / 6.0)) < 1e-6, "PPMI(a,b) = log(20/6)");
    CHECK(fabs(peso_ac - log(10.0 / 6.0)) < 1e-6, "PPMI(a,c) = log(10/6)");

    grafo_liberar(g);
}

static void testar_grafo_vertice_unico(void) {
    char termos[1][MAX_TOKEN_LEN] = {"a"};
    Documento docs[1];
    const char *d0[] = {"a"};
    doc_set(&docs[0], d0, 1);

    Grafo *g = construir_grafo_ppmi(docs, 1, termos, 1);
    CHECK(g != NULL, "grafo de 1 termo nao quebra");
    CHECK(g->n_arest == 0, "1 vertice nao tem arestas");
    grafo_liberar(g);
}

int main(void) {
    testar_ppmi_associacao_forte();
    testar_ppmi_independencia_sem_aresta();
    testar_ppmi_monotonia();
    testar_grafo_vertice_unico();
    TEST_SUMMARY();
}
