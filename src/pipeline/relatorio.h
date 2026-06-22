#ifndef RELATORIO_H
#define RELATORIO_H

#include "config.h"
#include "grafo.h"

/*
 * Gera o relatório completo no terminal a partir do grafo de coocorrência
 * (já com limiar aplicado): grau, BFS, componentes conexas, MST (Prim),
 * cliques (Bron-Kerbosch), análise temporal e resumo final.
 *
 * Precisa do corpus (docs) para reconstruir grafos por período e dos totais
 * de vocabulário (n_vocab, n_aceitos) apenas para o resumo.
 */
void relatorio_gerar(const Grafo *g, char termos[][MAX_TOKEN_LEN], int n_termos,
                     Documento *docs, int n_docs, double limiar,
                     int n_vocab, int n_aceitos);

#endif
