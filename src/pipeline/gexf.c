#include "gexf.h"
#include "algoritmos_grafo.h"
#include <stdio.h>
#include <stdlib.h>

/* Paleta de cores (estilo categórico) para colorir por componente/grupo. */
static const int PALETA[][3] = {
    { 31, 119, 180}, {255, 127,  14}, { 44, 160,  44}, {214,  39,  40},
    {148, 103, 189}, {140,  86,  75}, {227, 119, 194}, {127, 127, 127},
    {188, 189,  34}, { 23, 190, 207}, {174, 199, 232}, {255, 187, 120},
    {152, 223, 138}, {255, 152, 150}, {197, 176, 213}, {196, 156, 148},
};
static const int N_PALETA = (int)(sizeof(PALETA) / sizeof(PALETA[0]));

/* Escreve uma string como texto XML, escapando os caracteres especiais. */
static void escrever_escapado(FILE *f, const char *s) {
    for (const char *p = s; *p; p++) {
        switch (*p) {
            case '&':  fputs("&amp;",  f); break;
            case '<':  fputs("&lt;",   f); break;
            case '>':  fputs("&gt;",   f); break;
            case '"':  fputs("&quot;", f); break;
            case '\'': fputs("&apos;", f); break;
            default:   fputc(*p, f);       break;
        }
    }
}

int exportar_gexf(const char *caminho, const Grafo *g,
                  char termos[][MAX_TOKEN_LEN], int n_termos) {
    FILE *f = fopen(caminho, "w");
    if (!f) { perror("fopen"); return -1; }

    /* Componentes conexas = os grupos/temas que coloriremos. */
    int *componente = malloc(n_termos * sizeof(int));
    if (n_termos > 0 && !componente) { perror("malloc"); fclose(f); return -1; }
    componentes_conexas(g, componente);

    fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n", f);
    fputs("<gexf xmlns=\"http://www.gexf.net/1.2draft\" "
          "xmlns:viz=\"http://www.gexf.net/1.2draft/viz\" version=\"1.2\">\n", f);
    fputs("  <meta>\n"
          "    <creator>GraphFinance</creator>\n"
          "    <description>Grafo de coocorrencia (peso PPMI); "
          "cor por componente, tamanho por grau</description>\n"
          "  </meta>\n", f);
    fputs("  <graph mode=\"static\" defaultedgetype=\"undirected\">\n", f);
    fputs("    <attributes class=\"node\">\n"
          "      <attribute id=\"0\" title=\"grau\" type=\"integer\"/>\n"
          "      <attribute id=\"1\" title=\"componente\" type=\"integer\"/>\n"
          "    </attributes>\n", f);

    /* ── Nós (omite isolados) ── */
    fputs("    <nodes>\n", f);
    int n_nos = 0;
    for (int v = 0; v < n_termos; v++) {
        if (g->grau[v] == 0) continue;   /* pula isolados */
        int comp = componente[v];
        const int *cor = PALETA[((comp % N_PALETA) + N_PALETA) % N_PALETA];

        fprintf(f, "      <node id=\"%d\" label=\"", v);
        escrever_escapado(f, termos[v]);
        fputs("\">\n", f);
        fprintf(f, "        <attvalues>\n"
                   "          <attvalue for=\"0\" value=\"%d\"/>\n"
                   "          <attvalue for=\"1\" value=\"%d\"/>\n"
                   "        </attvalues>\n", g->grau[v], comp);
        fprintf(f, "        <viz:size value=\"%d\"/>\n", g->grau[v]);
        fprintf(f, "        <viz:color r=\"%d\" g=\"%d\" b=\"%d\"/>\n",
                cor[0], cor[1], cor[2]);
        fputs("      </node>\n", f);
        n_nos++;
    }
    fputs("    </nodes>\n", f);

    /* ── Arestas (cada par uma vez: vizinho > u) ── */
    fputs("    <edges>\n", f);
    int id = 0;
    for (int u = 0; u < g->n_vert; u++) {
        for (ArestaNo *a = g->adj[u]; a; a = a->prox) {
            if (a->vizinho > u) {
                fprintf(f, "      <edge id=\"%d\" source=\"%d\" target=\"%d\" "
                           "weight=\"%.4f\"/>\n", id++, u, a->vizinho, a->peso);
            }
        }
    }
    fputs("    </edges>\n", f);

    fputs("  </graph>\n</gexf>\n", f);

    free(componente);
    fclose(f);
    return n_nos;
}
