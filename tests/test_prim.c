#include "./../src/algoritmos_grafo.h"
#include "./../src/grafo.h"
#include "testutil.h"

static void testar_prim_maximo_basico(void) {
  /* grafo conexo com pesos variados, testando se a arvore prioriza pesos altos
   */
  Grafo *g = grafo_criar(4);
  grafo_adicionar_aresta(g, 0, 1, 1.0);
  grafo_adicionar_aresta(g, 0, 2, 5.0); /* peso alto */
  grafo_adicionar_aresta(g, 1, 2, 2.0);
  grafo_adicionar_aresta(g, 2, 3, 4.0); /* peso alto */
  grafo_adicionar_aresta(g, 1, 3, 0.5);

  ArvoreGeradora *arv = prim_maximo(g, 0);

  CHECK(arv != NULL, "prim_maximo retorna nao-NULL");
  CHECK(arv->n_arestas == 3,
        "arvore geradora maxima tem n-1 arestas (grafo conexo)");

  double peso_total = 0.0;
  for (int i = 0; i < arv->n_arestas; i++)
    peso_total += arv->arestas[i].peso;
  /* arvore otima usa as arestas 0-2 (5.0), 2-3 (4.0) e 1-2 (2.0) = 11.0 */
  CHECK(peso_total == 11.0,
        "prim_maximo: soma dos pesos da arvore eh a maxima possivel");

  arvore_geradora_liberar(arv);
  grafo_liberar(g);
}

static void testar_prim_maximo_componente_desconexa(void) {
  /* duas componentes: {0,1} e {2,3} */
  Grafo *g = grafo_criar(4);
  grafo_adicionar_aresta(g, 0, 1, 3.0);
  grafo_adicionar_aresta(g, 2, 3, 7.0);

  ArvoreGeradora *arv = prim_maximo(g, 0);

  CHECK(arv->n_arestas == 1,
        "prim_maximo a partir da origem so alcanca a propria componente");
  CHECK(arv->arestas[0].peso == 3.0,
        "unica aresta da arvore pertence a componente de origem");

  arvore_geradora_liberar(arv);
  grafo_liberar(g);
}

static void testar_prim_maximo_vertice_unico(void) {
  Grafo *g = grafo_criar(1);

  ArvoreGeradora *arv = prim_maximo(g, 0);

  CHECK(arv != NULL, "prim_maximo com grafo de 1 vertice retorna nao-NULL");
  CHECK(arv->n_arestas == 0, "arvore com 1 vertice nao tem arestas");

  arvore_geradora_liberar(arv);
  grafo_liberar(g);
}

int main(void) {
  testar_prim_maximo_basico();
  testar_prim_maximo_componente_desconexa();
  testar_prim_maximo_vertice_unico();
  TEST_SUMMARY();
}
