#include "algoritmos_grafo.h"
#include "fila.h"
#include "grafo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bfs(const Grafo *g, int origem, int *visitado) {
  Fila *f = fila_criar(g->n_vert);
  visitado[origem] = 1;
  fila_enfileirar(f, origem);
  while (!fila_vazia(f)) {
    int u = fila_desenfileirar(f);
    for (ArestaNo *e = g->adj[u]; e; e = e->prox) {
      int v = e->vizinho;
      if (!visitado[v]) {
        visitado[v] = 1;
        fila_enfileirar(f, v);
      }
    }
  }
  fila_liberar(f);
}

int componentes_conexas(const Grafo *g, int *componente) {
  int *visitado = calloc(g->n_vert, sizeof(int));
  if (g->n_vert > 0 && !visitado) {
    perror("calloc");
    exit(1);
  }
  for (int i = 0; i < g->n_vert; i++)
    componente[i] = -1;
  int id_componente = 0;
  for (int i = 0; i < g->n_vert; i++) {
    if (!visitado[i]) {
      bfs(g, i, visitado);
      for (int j = 0; j < g->n_vert; j++) {
        if (visitado[j] && componente[j] == -1)
          componente[j] = id_componente;
      }
      id_componente++;
    }
  }
  free(visitado);
  return id_componente;
}

static ListaCliques *lista_cliques_criar(void) {
  ListaCliques *lc = malloc(sizeof(ListaCliques));
  if (!lc) {
    perror("malloc");
    exit(1);
  }
  lc->capacidade = 8;
  lc->n_cliques = 0;
  lc->itens = malloc(lc->capacidade * sizeof(Clique));
  if (!lc->itens) {
    perror("malloc");
    exit(1);
  }
  return lc;
}

static void lista_cliques_adicionar(ListaCliques *lc, const int *vertices,
                                    int n) {
  if (lc->n_cliques == lc->capacidade) {
    lc->capacidade *= 2;
    Clique *novo = realloc(lc->itens, lc->capacidade * sizeof(Clique));
    if (!novo) {
      perror("realloc");
      exit(1);
    }
    lc->itens = novo;
  }
  int *copia = malloc(n * sizeof(int));
  if (n > 0 && !copia) {
    perror("malloc");
    exit(1);
  }
  memcpy(copia, vertices, n * sizeof(int));
  lc->itens[lc->n_cliques].vertices = copia;
  lc->itens[lc->n_cliques].n = n;
  lc->n_cliques++;
}

void lista_cliques_liberar(ListaCliques *lc) {
  if (!lc)
    return;
  for (int i = 0; i < lc->n_cliques; i++)
    free(lc->itens[i].vertices);
  free(lc->itens);
  free(lc);
}

/* Variante de Tomita: escolhe um pivô u em P∪X que maximiza |P ∩ N(u)| e
 * itera apenas sobre P \ N(u). Não altera o conjunto de cliques maximais —
 * apenas poda ramos redundantes da recursão, evitando o custo exponencial em
 * grafos densos. */
static void bron_kerbosch_rec(const Grafo *g, int *r, int n_r, int *p, int n_p,
                              int *x, int n_x, ListaCliques *lc) {
  if (n_p == 0 && n_x == 0) {
    lista_cliques_adicionar(lc, r, n_r);
    return;
  }

  int n_vert = g->n_vert;

  /* Seleção do pivô: dentre P∪X, o que tem mais vizinhos em P. */
  int pivo = -1;
  int melhor_cont = -1;
  for (int origem = 0; origem < 2; origem++) {
    int *base = origem == 0 ? p : x;
    int n_base = origem == 0 ? n_p : n_x;
    for (int i = 0; i < n_base; i++) {
      int u = base[i];
      int cont = 0;
      for (int j = 0; j < n_p; j++)
        if (grafo_tem_aresta(g, u, p[j]))
          cont++;
      if (cont > melhor_cont) {
        melhor_cont = cont;
        pivo = u;
      }
    }
  }

  /* Candidatos a expandir: P \ N(pivo) (snapshot, pois P muda no laço). */
  int *cand = malloc(n_p * sizeof(int));
  if (n_p > 0 && !cand) {
    perror("malloc");
    exit(1);
  }
  int n_cand = 0;
  for (int j = 0; j < n_p; j++)
    if (!grafo_tem_aresta(g, pivo, p[j]))
      cand[n_cand++] = p[j];

  for (int c = 0; c < n_cand; c++) {
    int v = cand[c];

    int *novo_r = malloc((n_r + 1) * sizeof(int));
    int *novo_p = malloc(n_vert * sizeof(int));
    int *novo_x = malloc(n_vert * sizeof(int));
    if (!novo_r || !novo_p || !novo_x) {
      perror("malloc");
      exit(1);
    }
    memcpy(novo_r, r, n_r * sizeof(int));
    novo_r[n_r] = v;

    int n_novo_p = 0;
    for (int j = 0; j < n_p; j++)
      if (p[j] != v && grafo_tem_aresta(g, v, p[j]))
        novo_p[n_novo_p++] = p[j];

    int n_novo_x = 0;
    for (int j = 0; j < n_x; j++)
      if (grafo_tem_aresta(g, v, x[j]))
        novo_x[n_novo_x++] = x[j];

    bron_kerbosch_rec(g, novo_r, n_r + 1, novo_p, n_novo_p, novo_x, n_novo_x,
                      lc);

    free(novo_r);
    free(novo_p);
    free(novo_x);

    /* Move v de P para X. */
    for (int j = 0; j < n_p; j++)
      if (p[j] == v) {
        memmove(p + j, p + j + 1, (n_p - j - 1) * sizeof(int));
        n_p--;
        break;
      }
    x[n_x++] = v;
  }

  free(cand);
}

ListaCliques *bron_kerbosch(const Grafo *g) {
  ListaCliques *lc = lista_cliques_criar();

  if (g->n_vert == 0)
    return lc;

  int *r = malloc(g->n_vert * sizeof(int));
  int *p = malloc(g->n_vert * sizeof(int));
  int *x = malloc(g->n_vert * sizeof(int));
  if (!r || !p || !x) {
    perror("malloc");
    exit(1);
  }

  int n_p = g->n_vert;
  for (int i = 0; i < n_p; i++)
    p[i] = i;

  bron_kerbosch_rec(g, r, 0, p, n_p, x, 0, lc);

  free(r);
  free(p);
  free(x);

  return lc;
}

ArvoreGeradora *prim_maximo(const Grafo *g, int origem) {
  int n = g->n_vert;
  int *na_arvore = calloc(n, sizeof(int));
  double *melhor_peso = malloc(n * sizeof(double));
  int *melhor_de = malloc(n * sizeof(int));
  if (n > 0 && (!na_arvore || !melhor_peso || !melhor_de)) {
    perror("malloc");
    exit(1);
  }
  for (int i = 0; i < n; i++) {
    melhor_peso[i] = -1.0;
    melhor_de[i] = -1;
  }
  ArvoreGeradora *arv = malloc(sizeof(ArvoreGeradora));
  if (!arv) {
    perror("malloc");
    exit(1);
  }
  arv->arestas = (n > 1) ? malloc((n - 1) * sizeof(ArestaMST)) : NULL;
  if (n > 1 && !arv->arestas) {
    perror("malloc");
    exit(1);
  }
  arv->n_arestas = 0;
  na_arvore[origem] = 1;
  for (ArestaNo *e = g->adj[origem]; e; e = e->prox) {
    if (e->peso > melhor_peso[e->vizinho]) {
      melhor_peso[e->vizinho] = e->peso;
      melhor_de[e->vizinho] = origem;
    }
  }
  for (int passo = 1; passo < n; passo++) {
    int u = -1;
    double maior = -1.0;
    for (int i = 0; i < n; i++) {
      if (!na_arvore[i] && melhor_peso[i] > maior) {
        maior = melhor_peso[i];
        u = i;
      }
    }
    if (u == -1)
      break;
    na_arvore[u] = 1;
    arv->arestas[arv->n_arestas].u = melhor_de[u];
    arv->arestas[arv->n_arestas].v = u;
    arv->arestas[arv->n_arestas].peso = melhor_peso[u];
    arv->n_arestas++;
    for (ArestaNo *e = g->adj[u]; e; e = e->prox) {
      int v = e->vizinho;
      if (!na_arvore[v] && e->peso > melhor_peso[v]) {
        melhor_peso[v] = e->peso;
        melhor_de[v] = u;
      }
    }
  }
  free(na_arvore);
  free(melhor_peso);
  free(melhor_de);
  return arv;
}
void arvore_geradora_liberar(ArvoreGeradora *arv) {
  if (!arv)
    return;
  free(arv->arestas);
  free(arv);
}