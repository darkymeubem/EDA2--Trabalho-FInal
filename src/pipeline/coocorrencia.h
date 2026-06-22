#ifndef COOCORRENCIA_H
#define COOCORRENCIA_H

#include "config.h"
#include "grafo.h"

/* Ordena (alfabeticamente) o array de termos selecionados, habilitando a
 * busca binária usada na construção do grafo. Chamar antes de construir. */
void ordenar_termos(char termos[][MAX_TOKEN_LEN], int n);

/* Constrói o grafo de coocorrência com peso PPMI sobre os termos selecionados.
 * 'termos' deve estar ordenado (ver ordenar_termos). */
Grafo *construir_grafo_ppmi(Documento *docs, int n_docs,
                            char termos[][MAX_TOKEN_LEN], int n_termos);

/* Retorna uma cópia do grafo mantendo apenas as arestas com peso >= limiar. */
Grafo *aplicar_limiar(const Grafo *g, double limiar);

#endif
