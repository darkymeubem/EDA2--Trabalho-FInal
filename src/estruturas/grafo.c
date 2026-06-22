#include "grafo.h"
#include <stdlib.h>
#include <stdio.h>

Grafo *grafo_criar(int n_vert) {
    Grafo *g = malloc(sizeof(Grafo));
    if (!g) { perror("malloc"); exit(1); }
    g->n_vert  = n_vert;
    g->n_arest = 0;
    g->adj  = calloc(n_vert, sizeof(ArestaNo *));
    g->grau = calloc(n_vert, sizeof(int));
    if (n_vert > 0 && (!g->adj || !g->grau)) { perror("calloc"); exit(1); }
    return g;
}

static void inserir_no(Grafo *g, int u, int v, double peso) {
    ArestaNo *no = malloc(sizeof(ArestaNo));
    if (!no) { perror("malloc"); exit(1); }
    no->vizinho = v;
    no->peso    = peso;
    no->prox    = g->adj[u];
    g->adj[u]   = no;
    g->grau[u]++;
}

void grafo_adicionar_aresta(Grafo *g, int u, int v, double peso) {
    if (u == v) return;
    if (grafo_tem_aresta(g, u, v)) return;
    inserir_no(g, u, v, peso);
    inserir_no(g, v, u, peso);
    g->n_arest++;
}

double grafo_peso_aresta(const Grafo *g, int u, int v) {
    for (ArestaNo *e = g->adj[u]; e; e = e->prox)
        if (e->vizinho == v) return e->peso;
    return -1.0;
}

int grafo_tem_aresta(const Grafo *g, int u, int v) {
    for (ArestaNo *e = g->adj[u]; e; e = e->prox)
        if (e->vizinho == v) return 1;
    return 0;
}

void grafo_liberar(Grafo *g) {
    if (!g) return;
    for (int i = 0; i < g->n_vert; i++) {
        ArestaNo *cur = g->adj[i];
        while (cur) {
            ArestaNo *prox = cur->prox;
            free(cur);
            cur = prox;
        }
    }
    free(g->adj);
    free(g->grau);
    free(g);
}
