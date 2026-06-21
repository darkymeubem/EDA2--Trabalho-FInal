#include "./../src/algoritmos_grafo.h"
#include "./../src/grafo.h"
#include "testutil.h"

/* OBS: estes testes dependem de bron_kerbosch(), que ainda nao foi
 * implementada (so existem os helpers internos de ListaCliques).
 * Este arquivo serve como especificacao do comportamento esperado
 * e so vai compilar/linkar quando a implementacao estiver pronta. */

static int clique_contem(const Clique *c, int vertice) {
  for (int i = 0; i < c->n; i++)
    if (c->vertices[i] == vertice)
      return 1;
  return 0;
}

static int existe_clique_com_vertices(const ListaCliques *lc,
                                      const int *esperado, int n_esperado) {
  for (int i = 0; i < lc->n_cliques; i++) {
    const Clique *c = &lc->itens[i];
    if (c->n != n_esperado)
      continue;

    int todos_presentes = 1;
    for (int j = 0; j < n_esperado; j++) {
      if (!clique_contem(c, esperado[j])) {
        todos_presentes = 0;
        break;
      }
    }
    if (todos_presentes)
      return 1;
  }
  return 0;
}

static void testar_clique_triangulo(void) {
  /* triangulo 0-1-2 totalmente conectado: deve ser um unico clique maximal */
  Grafo *g = grafo_criar(3);
  grafo_adicionar_aresta(g, 0, 1, 1.0);
  grafo_adicionar_aresta(g, 0, 2, 1.0);
  grafo_adicionar_aresta(g, 1, 2, 1.0);

  ListaCliques *lc = bron_kerbosch(g);

  CHECK(lc != NULL, "bron_kerbosch retorna nao-NULL");
  CHECK(lc->n_cliques == 1, "triangulo completo: exatamente 1 clique maximal");

  int esperado[3] = {0, 1, 2};
  CHECK(existe_clique_com_vertices(lc, esperado, 3),
        "clique maximal contem os 3 vertices do triangulo");

  lista_cliques_liberar(lc);
  grafo_liberar(g);
}

static void testar_clique_sem_arestas(void) {
  /* sem nenhuma aresta: cada vertice eh um clique maximal isolado de tamanho 1
   */
  Grafo *g = grafo_criar(3);

  ListaCliques *lc = bron_kerbosch(g);

  CHECK(lc->n_cliques == 3,
        "grafo sem arestas: 3 cliques maximais (um por vertice)");
  for (int i = 0; i < lc->n_cliques; i++)
    CHECK(lc->itens[i].n == 1, "cada clique isolado tem tamanho 1");

  lista_cliques_liberar(lc);
  grafo_liberar(g);
}

static void testar_clique_dois_grupos_conectados(void) {
  /* 0-1-2 forma triangulo; 2-3 conecta a um vertice fora do triangulo */
  Grafo *g = grafo_criar(4);
  grafo_adicionar_aresta(g, 0, 1, 1.0);
  grafo_adicionar_aresta(g, 0, 2, 1.0);
  grafo_adicionar_aresta(g, 1, 2, 1.0);
  grafo_adicionar_aresta(g, 2, 3, 1.0);

  ListaCliques *lc = bron_kerbosch(g);

  /* esperado: clique {0,1,2} (tamanho 3) e clique {2,3} (tamanho 2) */
  CHECK(lc->n_cliques == 2, "dois cliques maximais distintos encontrados");

  int triangulo[3] = {0, 1, 2};
  int aresta[2] = {2, 3};
  CHECK(existe_clique_com_vertices(lc, triangulo, 3),
        "clique do triangulo {0,1,2} encontrado");
  CHECK(existe_clique_com_vertices(lc, aresta, 2),
        "clique da aresta extra {2,3} encontrado");

  lista_cliques_liberar(lc);
  grafo_liberar(g);
}

int main(void) {
  testar_clique_triangulo();
  testar_clique_sem_arestas();
  testar_clique_dois_grupos_conectados();
  TEST_SUMMARY();
}
