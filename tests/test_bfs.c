#include "./../src/algoritmos_grafo.h"
#include "./../src/grafo.h"
#include "testutil.h"

/* --- bfs --- */

static void testar_bfs_caminho(void) {
  /* grafo: 0-1-2-3 em cadeia, vertice 4 isolado */
  Grafo *g = grafo_criar(5);
  grafo_adicionar_aresta(g, 0, 1, 1.0);
  grafo_adicionar_aresta(g, 1, 2, 1.0);
  grafo_adicionar_aresta(g, 2, 3, 1.0);

  int visitado[5] = {0};
  bfs(g, 0, visitado);

  CHECK(visitado[0] == 1, "bfs: origem marcada como visitada");
  CHECK(visitado[1] == 1, "bfs: vizinho a 1 salto visitado");
  CHECK(visitado[2] == 1, "bfs: vizinho a 2 saltos visitado");
  CHECK(visitado[3] == 1, "bfs: vizinho a 3 saltos visitado");
  CHECK(visitado[4] == 0, "bfs: vertice isolado nao visitado");

  grafo_liberar(g);
}

static void testar_bfs_a_partir_de_isolado(void) {
  Grafo *g = grafo_criar(3);
  grafo_adicionar_aresta(g, 0, 1, 1.0);
  /* vertice 2 fica isolado */

  int visitado[3] = {0};
  bfs(g, 2, visitado);

  CHECK(visitado[2] == 1, "bfs a partir de isolado: ele mesmo visitado");
  CHECK(visitado[0] == 0, "bfs a partir de isolado: nao alcanca outros (0)");
  CHECK(visitado[1] == 0, "bfs a partir de isolado: nao alcanca outros (1)");

  grafo_liberar(g);
}

/* --- componentes_conexas --- */

static void testar_componentes_duas_componentes(void) {
  /* duas componentes: {0,1,2} e {3,4} */
  Grafo *g = grafo_criar(5);
  grafo_adicionar_aresta(g, 0, 1, 1.0);
  grafo_adicionar_aresta(g, 1, 2, 1.0);
  grafo_adicionar_aresta(g, 3, 4, 1.0);

  int componente[5];
  int n_comp = componentes_conexas(g, componente);

  CHECK(n_comp == 2, "componentes_conexas: duas componentes encontradas");
  CHECK(componente[0] == componente[1], "0 e 1 na mesma componente");
  CHECK(componente[1] == componente[2], "1 e 2 na mesma componente");
  CHECK(componente[3] == componente[4], "3 e 4 na mesma componente");
  CHECK(componente[0] != componente[3],
        "componentes {0,1,2} e {3,4} sao distintas");

  grafo_liberar(g);
}

static void testar_componentes_grafo_conexo(void) {
  Grafo *g = grafo_criar(4);
  grafo_adicionar_aresta(g, 0, 1, 1.0);
  grafo_adicionar_aresta(g, 1, 2, 1.0);
  grafo_adicionar_aresta(g, 2, 3, 1.0);

  int componente[4];
  int n_comp = componentes_conexas(g, componente);

  CHECK(n_comp == 1, "grafo totalmente conexo: 1 unica componente");

  grafo_liberar(g);
}

static void testar_componentes_grafo_sem_arestas(void) {
  Grafo *g = grafo_criar(3); /* sem nenhuma aresta */

  int componente[3];
  int n_comp = componentes_conexas(g, componente);

  CHECK(n_comp == 3,
        "grafo sem arestas: cada vertice eh sua propria componente");
  CHECK(componente[0] != componente[1],
        "vertices isolados em componentes distintas (0,1)");
  CHECK(componente[1] != componente[2],
        "vertices isolados em componentes distintas (1,2)");

  grafo_liberar(g);
}

int main(void) {
  testar_bfs_caminho();
  testar_bfs_a_partir_de_isolado();
  testar_componentes_duas_componentes();
  testar_componentes_grafo_conexo();
  testar_componentes_grafo_sem_arestas();
  TEST_SUMMARY();
}
