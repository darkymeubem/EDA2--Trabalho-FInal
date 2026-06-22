#ifndef ALGORITMOS_GRAFO_H
#define ALGORITMOS_GRAFO_H

#include "grafo.h"

void bfs(const Grafo *g, int origem, int *visitado);

int componentes_conexas(const Grafo *g, int *componente);

typedef struct {
  int *vertices;
  int n;
} Clique;

typedef struct {
  Clique *itens;
  int n_cliques;
  int capacidade;
} ListaCliques;

ListaCliques *bron_kerbosch(const Grafo *g);
void lista_cliques_liberar(ListaCliques *lc);

typedef struct {
  int u, v;
  double peso;
} ArestaMST;

typedef struct {
  ArestaMST *arestas;
  int n_arestas;
} ArvoreGeradora;

ArvoreGeradora *prim_maximo(const Grafo *g, int origem);
void arvore_geradora_liberar(ArvoreGeradora *arv);

#endif
