#ifndef GEXF_H
#define GEXF_H

#include "config.h"
#include "grafo.h"

/*
 * Exporta o grafo de coocorrência para um arquivo GEXF (formato do Gephi).
 *
 * - Nós isolados (grau 0) são omitidos.
 * - Cada nó recebe cor por componente conexa (os "grupos"/temas) e tamanho
 *   proporcional ao grau, via namespace viz — o Gephi já abre colorido.
 * - Atributos 'grau' e 'componente' ficam disponíveis para filtro no Gephi.
 * - Arestas carregam o peso PPMI.
 *
 * Retorna o número de nós escritos (>= 0), ou -1 em caso de erro de E/S.
 */
int exportar_gexf(const char *caminho, const Grafo *g,
                  char termos[][MAX_TOKEN_LEN], int n_termos);

#endif
