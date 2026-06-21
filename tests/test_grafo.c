#include "./../src/grafo.h"
#include "testutil.h"

int main(void) {
  /* --- criação básica --- */
  Grafo *g = grafo_criar(5);
  CHECK(g != NULL, "grafo_criar retorna nao-NULL");
  CHECK(g->n_vert == 5, "n_vert correto apos criar");
  CHECK(g->n_arest == 0, "n_arest == 0 apos criar");
  for (int i = 0; i < 5; i++)
    CHECK(g->grau[i] == 0, "grau[i] == 0 apos criar");

  /* --- adicionar aresta --- */
  grafo_adicionar_aresta(g, 0, 1, 2.5);
  CHECK(g->n_arest == 1, "n_arest == 1 apos adicionar aresta");
  CHECK(g->grau[0] == 1, "grau[0] incrementado");
  CHECK(g->grau[1] == 1, "grau[1] incrementado");
  CHECK(g->grau[2] == 0, "grau[2] nao afetado");

  /* --- tem_aresta (nao-direcionado) --- */
  CHECK(grafo_tem_aresta(g, 0, 1) == 1, "tem_aresta(0,1) == 1");
  CHECK(grafo_tem_aresta(g, 1, 0) == 1,
        "tem_aresta(1,0) == 1 (nao-direcionado)");
  CHECK(grafo_tem_aresta(g, 0, 2) == 0, "tem_aresta(0,2) == 0 (nao existe)");

  /* --- peso_aresta --- */
  CHECK(grafo_peso_aresta(g, 0, 1) == 2.5, "peso_aresta(0,1) correto");
  CHECK(grafo_peso_aresta(g, 1, 0) == 2.5,
        "peso_aresta(1,0) correto (simetrico)");
  CHECK(grafo_peso_aresta(g, 0, 2) == -1.0, "peso_aresta inexistente == -1.0");

  /* --- aresta duplicada nao incrementa --- */
  grafo_adicionar_aresta(g, 0, 1, 9.9);
  CHECK(g->n_arest == 1, "aresta duplicada nao incrementa n_arest");
  CHECK(g->grau[0] == 1, "grau[0] inalterado apos duplicata");
  CHECK(grafo_peso_aresta(g, 0, 1) == 2.5,
        "peso original preservado em duplicata");

  /* --- aresta reflexiva ignorada --- */
  grafo_adicionar_aresta(g, 2, 2, 1.0);
  CHECK(g->n_arest == 1, "aresta reflexiva nao incrementa n_arest");
  CHECK(g->grau[2] == 0, "grau[2] inalterado apos aresta reflexiva");

  /* --- multiplas arestas --- */
  grafo_adicionar_aresta(g, 1, 2, 0.8);
  grafo_adicionar_aresta(g, 2, 3, 1.2);
  grafo_adicionar_aresta(g, 3, 4, 0.5);
  CHECK(g->n_arest == 4, "n_arest == 4 apos 4 arestas");
  CHECK(g->grau[2] == 2, "grau[2] == 2 (vizinhos 1 e 3)");

  grafo_liberar(g);

  /* --- grafo vazio --- */
  Grafo *vazio = grafo_criar(0);
  CHECK(vazio != NULL, "grafo_criar(0) retorna nao-NULL");
  CHECK(vazio->n_vert == 0, "n_vert == 0");
  CHECK(vazio->n_arest == 0, "n_arest == 0");
  grafo_liberar(vazio);

  TEST_SUMMARY();
}
