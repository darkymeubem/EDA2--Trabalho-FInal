#include "html.h"
#include "algoritmos_grafo.h"
#include <stdio.h>
#include <stdlib.h>

/* Mesma paleta categórica do exportador GEXF (cor por componente/grupo). */
static const int PALETA[][3] = {
    { 31, 119, 180}, {255, 127,  14}, { 44, 160,  44}, {214,  39,  40},
    {148, 103, 189}, {140,  86,  75}, {227, 119, 194}, {127, 127, 127},
    {188, 189,  34}, { 23, 190, 207}, {174, 199, 232}, {255, 187, 120},
    {152, 223, 138}, {255, 152, 150}, {197, 176, 213}, {196, 156, 148},
};
static const int N_PALETA = (int)(sizeof(PALETA) / sizeof(PALETA[0]));

static const int *cor_de(int comp) {
    return PALETA[((comp % N_PALETA) + N_PALETA) % N_PALETA];
}

/* Escapa uma string para uso dentro de uma string JS entre aspas duplas. */
static void escrever_js(FILE *f, const char *s) {
    for (const char *p = s; *p; p++) {
        if (*p == '\\' || *p == '"') fputc('\\', f);
        fputc(*p, f);
    }
}

int exportar_html(const char *caminho, const Grafo *g,
                  char termos[][MAX_TOKEN_LEN], int n_termos) {
    FILE *f = fopen(caminho, "w");
    if (!f) { perror("fopen"); return -1; }

    int *componente = malloc(n_termos * sizeof(int));
    if (n_termos > 0 && !componente) { perror("malloc"); fclose(f); return -1; }
    int n_comps = componentes_conexas(g, componente);

    int n_nos = 0;
    for (int v = 0; v < n_termos; v++) if (g->grau[v] > 0) n_nos++;

    /* Maior clique (Bron-Kerbosch) — destacável por um botão na página. */
    ListaCliques *lc = bron_kerbosch(g);
    int *clq = NULL, clq_n = 0;
    if (lc) {
        int melhor = -1;
        for (int i = 0; i < lc->n_cliques; i++)
            if (lc->itens[i].n > clq_n) { clq_n = lc->itens[i].n; melhor = i; }
        if (melhor >= 0) {
            clq = malloc(clq_n * sizeof(int));
            for (int i = 0; i < clq_n; i++) clq[i] = lc->itens[melhor].vertices[i];
        }
        lista_cliques_liberar(lc);
    }

    /* ── Cabeçalho HTML + estilo ── */
    fputs("<!DOCTYPE html>\n<html lang=\"pt-br\">\n<head>\n", f);
    fputs("<meta charset=\"utf-8\">\n", f);
    fputs("<title>GraphFinance — Temas (grafo de coocorrência)</title>\n", f);
    fputs("<script src=\"https://unpkg.com/vis-network/standalone/umd/"
          "vis-network.min.js\"></script>\n", f);
    fputs("<style>\n"
          "  html,body{margin:0;height:100%;font-family:system-ui,sans-serif;"
          "background:#0e1116;color:#e6e6e6}\n"
          "  #grafo{width:100vw;height:100vh}\n"
          "  #painel{position:absolute;top:12px;left:12px;z-index:10;"
          "background:rgba(20,24,31,.9);padding:10px 14px;border-radius:8px;"
          "font-size:13px;line-height:1.5;max-width:300px;"
          "max-height:92vh;overflow:auto}\n"
          "  #painel b{font-size:15px}\n"
          "  #busca{margin:6px 0;width:100%;padding:5px;border-radius:5px;"
          "border:1px solid #333;background:#0e1116;color:#e6e6e6}\n"
          "  .grupo{margin:5px 0;font-size:12px;line-height:1.4}\n"
          "  .bola{display:inline-block;width:10px;height:10px;border-radius:50%;"
          "margin-right:6px;vertical-align:middle}\n"
          "  #grupos b{font-size:13px}\n"
          "</style>\n</head>\n<body>\n", f);

    fprintf(f,
        "<div id=\"painel\">\n"
        "  <b>GraphFinance — Temas</b><br>\n"
        "  %d termos · %d conexões ponderadas (PPMI) · %d grupos<br>\n"
        "  <span style=\"color:#9aa\">cor = grupo · tamanho = relevância (grau) · "
        "espessura/rótulo da aresta = peso PPMI</span>\n"
        "  <input id=\"busca\" placeholder=\"buscar termo…\">\n",
        n_nos, g->n_arest, n_comps);

    fputs("  <button id=\"btnClique\" style=\"margin:4px 0;width:100%;padding:7px;"
          "border:0;border-radius:5px;background:#36c98a;color:#06120a;"
          "font-weight:600;cursor:pointer\">★ Destacar maior clique</button>\n"
          "  <div id=\"statusClique\" style=\"font-size:12px;color:#9aa\"></div>\n", f);

    /* Painel de grupos (gerado aqui pois precisa de 'termos'). */
    {
        int *tam = calloc(n_comps, sizeof(int));
        for (int v = 0; v < n_termos; v++)
            if (g->grau[v] > 0) tam[componente[v]]++;
        int *ord = malloc(n_comps * sizeof(int));
        for (int i = 0; i < n_comps; i++) ord[i] = i;
        for (int i = 0; i < n_comps - 1; i++) {
            int mx = i;
            for (int j = i + 1; j < n_comps; j++)
                if (tam[ord[j]] > tam[ord[mx]]) mx = j;
            int t = ord[i]; ord[i] = ord[mx]; ord[mx] = t;
        }

        fputs("  <div id=\"grupos\"><b>Grupos / temas</b>\n", f);
        int mostrados = 0;
        for (int k = 0; k < n_comps && mostrados < 12; k++) {
            int c = ord[k];
            if (tam[c] < 3) continue;
            int *membros = malloc(tam[c] * sizeof(int));
            int nm = 0;
            for (int v = 0; v < n_termos; v++)
                if (g->grau[v] > 0 && componente[v] == c) membros[nm++] = v;
            for (int i = 0; i < nm - 1; i++) {
                int mx = i;
                for (int j = i + 1; j < nm; j++)
                    if (g->grau[membros[j]] > g->grau[membros[mx]]) mx = j;
                int t = membros[i]; membros[i] = membros[mx]; membros[mx] = t;
            }
            const int *cor = cor_de(c);
            fprintf(f, "    <div class=\"grupo\"><span class=\"bola\" "
                       "style=\"background:rgb(%d,%d,%d)\"></span>",
                    cor[0], cor[1], cor[2]);
            fprintf(f, "<b>%d termos:</b> ", tam[c]);
            int lim = nm < 12 ? nm : 12;
            for (int i = 0; i < lim; i++) {
                if (i) fputs(", ", f);
                escrever_js(f, termos[membros[i]]);  /* só alfanumérico: seguro como texto */
            }
            if (nm > lim) fputs("…", f);
            fputs("</div>\n", f);
            free(membros);
            mostrados++;
        }
        fputs("  </div>\n", f);
        free(tam);
        free(ord);
    }

    fputs("</div>\n<div id=\"grafo\"></div>\n<script>\n", f);

    /* ── Nós ── */
    fputs("const nodes = new vis.DataSet([\n", f);
    for (int v = 0; v < n_termos; v++) {
        if (g->grau[v] == 0) continue;
        const int *cor = cor_de(componente[v]);
        fprintf(f, "{id:%d,value:%d,color:\"rgb(%d,%d,%d)\",label:\"",
                v, g->grau[v], cor[0], cor[1], cor[2]);
        escrever_js(f, termos[v]);
        fputs("\",title:\"", f);
        escrever_js(f, termos[v]);
        fprintf(f, " — grau %d, grupo %d\"},\n", g->grau[v], componente[v]);
    }
    fputs("]);\n", f);

    /* ── Arestas (grafo PONDERADO: largura = PPMI, valor no tooltip) ── */
    fputs("const edges = new vis.DataSet([\n", f);
    for (int u = 0; u < g->n_vert; u++)
        for (ArestaNo *a = g->adj[u]; a; a = a->prox)
            if (a->vizinho > u)
                fprintf(f, "{from:%d,to:%d,value:%.4f,"
                           "title:\"PPMI %.4f\"},\n",
                        u, a->vizinho, a->peso, a->peso);
    fputs("]);\n", f);

    /* ── Maior clique (ids + termos) para o botão ── */
    fputs("const maxClique = [", f);
    for (int i = 0; i < clq_n; i++) { if (i) fputc(',', f); fprintf(f, "%d", clq[i]); }
    fputs("];\n", f);
    fputs("const maxCliqueTermos = [", f);
    for (int i = 0; i < clq_n; i++) {
        if (i) fputc(',', f);
        fputc('"', f); escrever_js(f, termos[clq[i]]); fputc('"', f);
    }
    fputs("];\n", f);

    /* ── Configuração da rede + interações ── */
    fputs(
        "const container = document.getElementById('grafo');\n"
        "const network = new vis.Network(container, {nodes, edges}, {\n"
        /* labels de nó só aparecem quando o nó for grande o suficiente (zoom in) */
        "  nodes:{shape:'dot',scaling:{min:6,max:36,"
        "label:{enabled:true,min:11,max:20,drawThreshold:8,maxVisible:30}},"
        "font:{color:'#e6e6e6',strokeWidth:2,strokeColor:'#0e1116'}},\n"
        /* sem label nas arestas: era o maior gargalo de rendering (361 textos/frame) */
        "  edges:{scaling:{min:1,max:6},color:{color:'#5a6472',opacity:0.45},"
        "smooth:false},\n"
        "  physics:{solver:'forceAtlas2Based',"
        "stabilization:{iterations:150,fit:true},"
        "forceAtlas2Based:{gravitationalConstant:-50,springLength:80,"
        "damping:0.6}},\n"
        "  interaction:{hover:true,tooltipDelay:80,"
        "hideEdgesOnDrag:true,hideEdgesOnZoom:true,zoomSpeed:0.5}\n"
        "});\n"
        "network.once('stabilizationIterationsDone', () =>"
        " network.setOptions({physics:false}));\n"
        "document.getElementById('busca').addEventListener('input', e => {\n"
        "  const q = e.target.value.trim().toLowerCase();\n"
        "  if(!q) return;\n"
        "  const hit = nodes.get().find(n => n.label.toLowerCase().includes(q));\n"
        "  if(hit){ network.focus(hit.id,{scale:1.2,animation:true});"
        " network.selectNodes([hit.id]); }\n"
        "});\n"
        /* toggle: 1º clique isola o clique (oculta resto das arestas para nao lentificar),
           2º clique restaura o grafo completo */
        "let _cliqueMode=false;\n"
        "document.getElementById('btnClique').addEventListener('click', () => {\n"
        "  if(!maxClique.length) return;\n"
        "  if(_cliqueMode){\n"
        "    edges.update(edges.get().map(e=>({id:e.id,hidden:false})));\n"
        "    network.selectNodes([]);\n"
        "    network.fit({animation:{duration:400,easingFunction:'easeInOutQuad'}});\n"
        "    document.getElementById('btnClique').textContent='★ Destacar maior clique';\n"
        "    document.getElementById('statusClique').textContent='';\n"
        "    _cliqueMode=false;\n"
        "  } else {\n"
        "    const s=new Set(maxClique);\n"
        "    edges.update(edges.get().map(e=>({\n"
        "      id:e.id, hidden:!(s.has(e.from)&&s.has(e.to))\n"
        "    })));\n"
        "    network.selectNodes(maxClique);\n"
        "    network.fit({nodes:maxClique,animation:{duration:400,easingFunction:'easeInOutQuad'}});\n"
        "    document.getElementById('btnClique').textContent='✕ Restaurar grafo';\n"
        "    document.getElementById('statusClique').textContent=\n"
        "      'Maior clique ('+maxClique.length+'): '+maxCliqueTermos.join(', ');\n"
        "    _cliqueMode=true;\n"
        "  }\n"
        "});\n"
        "</script>\n</body>\n</html>\n", f);

    free(clq);
    free(componente);
    fclose(f);
    return n_nos;
}
