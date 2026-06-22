#ifndef VOCABULARIO_H
#define VOCABULARIO_H

#include "config.h"

/*
 * Vocabulário do corpus — array ordenado + busca binária (sem hash).
 * O estado é interno ao módulo; use as funções abaixo.
 */

/* Constrói o vocabulário e a frequência de documento (DF) a partir do corpus.
 * Ao final, ordena o vocabulário para habilitar busca binária. */
void vocab_construir(Documento *docs, int n_docs);

/* Número de termos únicos no vocabulário. */
int vocab_tamanho(void);

/* Quantos termos passam no filtro de DF (raros/genéricos removidos). */
int vocab_contar_aceitos(int n_docs);

/* Seleciona os top_n termos por TF-IDF médio dentre os que passam no filtro
 * de DF. Preenche 'selecionados' com os nomes e retorna quantos foram. */
int selecionar_termos(Documento *docs, int n_docs,
                      char selecionados[][MAX_TOKEN_LEN], int top_n);

#endif
