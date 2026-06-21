#include "./../src/fila.h"
#include "testutil.h"

int main(void) {
  /* --- criação --- */
  Fila *f = fila_criar(3);
  CHECK(f != NULL, "fila_criar retorna nao-NULL");
  CHECK(f->capacidade == 3, "capacidade correta apos criar");
  CHECK(f->tamanho == 0, "tamanho == 0 apos criar");
  CHECK(fila_vazia(f) == 1, "fila_vazia == 1 apos criar");

  /* --- enfileirar / desenfileirar simples (FIFO) --- */
  fila_enfileirar(f, 10);
  fila_enfileirar(f, 20);
  CHECK(f->tamanho == 2, "tamanho == 2 apos 2 enfileirar");
  CHECK(fila_vazia(f) == 0, "fila_vazia == 0 com elementos");

  int v1 = fila_desenfileirar(f);
  CHECK(v1 == 10, "primeiro a sair eh o primeiro a entrar (FIFO)");
  CHECK(f->tamanho == 1, "tamanho == 1 apos desenfileirar");

  int v2 = fila_desenfileirar(f);
  CHECK(v2 == 20, "segundo valor desenfileirado correto");
  CHECK(fila_vazia(f) == 1, "fila_vazia == 1 apos esvaziar");

  /* --- wraparound (testa indices circulares dando a volta no array) --- */
  fila_enfileirar(f, 1);
  fila_enfileirar(f, 2);
  fila_enfileirar(f, 3);
  CHECK(f->tamanho == 3, "tamanho == 3 (fila cheia, capacidade 3)");

  CHECK(fila_desenfileirar(f) == 1, "wraparound: primeiro valor correto");
  fila_enfileirar(f, 4); /* 'fim' deve dar a volta no array aqui */
  CHECK(f->tamanho == 3, "tamanho == 3 apos wraparound");
  CHECK(fila_desenfileirar(f) == 2, "wraparound: segundo valor correto");
  CHECK(fila_desenfileirar(f) == 3, "wraparound: terceiro valor correto");
  CHECK(fila_desenfileirar(f) == 4,
        "wraparound: quarto valor (deu a volta) correto");
  CHECK(fila_vazia(f) == 1, "fila_vazia == 1 apos esvaziar de novo");

  fila_liberar(f);

  /* --- fila de capacidade 0 (caso extremo) --- */
  Fila *vazia = fila_criar(0);
  CHECK(vazia != NULL, "fila_criar(0) retorna nao-NULL");
  CHECK(fila_vazia(vazia) == 1, "fila_vazia(0) == 1");
  fila_liberar(vazia);

  TEST_SUMMARY();
}
