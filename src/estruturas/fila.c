#include "fila.h"
#include <stdio.h>
#include <stdlib.h>

Fila *fila_criar(int capacidade) {
  Fila *f = malloc(sizeof(Fila));
  if (!f) {
    perror("malloc");
    exit(1);
  }

  f->dados = malloc(capacidade * sizeof(int));
  if (capacidade > 0 && !f->dados) {
    perror("malloc");
    exit(1);
  }

  f->capacidade = capacidade;
  f->inicio = 0;
  f->fim = 0;
  f->tamanho = 0;

  return f;
}

void fila_enfileirar(Fila *f, int valor) {
  if (f->tamanho == f->capacidade) {
    fprintf(stderr, "fila_enfileirar: fila cheia\n");
    exit(1);
  }
  f->dados[f->fim] = valor;
  f->fim = (f->fim + 1) % f->capacidade;
  f->tamanho++;
}

int fila_desenfileirar(Fila *f) {
  if (fila_vazia(f)) {
    fprintf(stderr, "fila_desenfileirar: fila vazia\n");
    exit(1);
  }
  int valor = f->dados[f->inicio];
  f->inicio = (f->inicio + 1) % f->capacidade;
  f->tamanho--;
  return valor;
}

int fila_vazia(const Fila *f) { return f->tamanho == 0; }

void fila_liberar(Fila *f) {
  if (!f)
    return;
  free(f->dados);
  free(f);
}
