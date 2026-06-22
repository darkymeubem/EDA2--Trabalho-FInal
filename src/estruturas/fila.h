#ifndef FILA_H
#define FILA_H

typedef struct {
  int *dados;
  int capacidade, inicio, fim, tamanho;
} Fila;

Fila *fila_criar(int capacidade);
void fila_enfileirar(Fila *f, int valor);
int fila_desenfileirar(Fila *f);
int fila_vazia(const Fila *f);
void fila_liberar(Fila *f);

#endif
