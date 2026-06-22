#ifndef GRAFO_H
#define GRAFO_H

typedef struct ArestaNo {
    int             vizinho;
    double          peso;
    struct ArestaNo *prox;
} ArestaNo;

typedef struct {
    ArestaNo **adj;
    int       *grau;
    int        n_vert;
    int        n_arest;
} Grafo;

Grafo *grafo_criar(int n_vert);
void   grafo_adicionar_aresta(Grafo *g, int u, int v, double peso);
double grafo_peso_aresta(const Grafo *g, int u, int v);
int    grafo_tem_aresta(const Grafo *g, int u, int v);
void   grafo_liberar(Grafo *g);

#endif
