#ifndef HTML_H
#define HTML_H

#include "config.h"
#include "grafo.h"

/*
 * Exporta o grafo de coocorrência para uma página HTML autocontida e
 * interativa (usa a biblioteca vis-network via CDN; basta abrir no navegador).
 *
 * - Nós isolados (grau 0) são omitidos.
 * - Cor do nó = componente conexa (grupo/tema); tamanho = grau.
 * - Layout de força (ForceAtlas2) roda no navegador: os temas se separam
 *   sozinhos. Arrastar / zoom / hover já funcionam.
 *
 * Retorna o número de nós escritos (>= 0), ou -1 em caso de erro de E/S.
 */
int exportar_html(const char *caminho, const Grafo *g,
                  char termos[][MAX_TOKEN_LEN], int n_termos);

#endif
