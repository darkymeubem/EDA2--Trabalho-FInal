# FinGraph Analytics 📈

Trabalho final da disciplina **FGA0030 - Estruturas de Dados 2 (EDA2)**.

## 🎯 Objetivo Principal
Identificar os **temas dominantes** em notícias financeiras (2018–2020) construindo um
**grafo de coocorrência** de termos. As arestas são ponderadas por **PPMI** (Positive
Pointwise Mutual Information) e os algoritmos de grafo (grau, BFS, componentes conexas,
cliques de Bron-Kerbosch e árvore geradora máxima de Prim) revelam os grupos de termos
que formam cada tema. O resultado é exportável como um **grafo interativo no navegador**.

## 🚀 Como rodar

Pré-requisitos: `gcc` (C99) e `make`.

```bash
make fingraph                      # compila o executável principal
./fingraph                         # usa dados/finais/noticias.csv (padrão)
./fingraph dados/convertidos/cnbc_convertido.csv        # corpus completo (53k docs)
./fingraph <csv> <limiar> <top_n>  # argumentos opcionais
```

Argumentos posicionais (todos opcionais):
| Posição | Significado | Padrão |
|---|---|---|
| 1 | caminho do CSV (`data,titulo,resumo`) | `dados/finais/noticias.csv` |
| 2 | limiar PPMI (mantém arestas com peso ≥ limiar) | `2.0` |
| 3 | nº de termos no grafo (filtro TF-IDF) | `600` |

> Dica: limiar maior (ex. `3.0`) deixa menos arestas e grupos mais separados;
> menor (ex. `1.0`) deixa o grafo mais denso.

## 🌐 Visualização (HTML interativo)

A flag `--html` gera uma página **autocontida** que abre em qualquer navegador — sem
instalar nada:

```bash
./fingraph dados/convertidos/cnbc_convertido.csv 2.0 --html grafo.html
```

Abra o `grafo.html`. Você verá:
- **cor do nó** = grupo/tema (componente conexa);
- **tamanho do nó** = relevância do termo (grau);
- **espessura e rótulo da aresta** = peso PPMI (grafo ponderado);
- **painel lateral** com os grupos/temas nomeados e busca de termos;
- layout de força que separa os temas automaticamente (arrasta, zoom, hover).

> A página usa a biblioteca `vis-network` via CDN (precisa de internet na 1ª abertura).

### Exportar para o Gephi (opcional)
Para análise de redes mais avançada, a flag `--gexf` gera um arquivo do Gephi
(nós já coloridos por grupo e dimensionados por grau):

```bash
./fingraph dados/convertidos/cnbc_convertido.csv 2.0 --gexf grafo.gexf
```

## 🧪 Como rodar os testes

```bash
make test     # compila e executa todos os testes unitários
```

Cobre as estruturas (grafo, fila), os algoritmos (BFS, componentes, cliques, Prim),
os utilitários (texto, CSV, datas) e o núcleo analítico (**coocorrência/PPMI**).

## 🗂️ Estrutura

```
src/
  main.c              # orquestrador
  config.h            # constantes globais + struct Documento
  estruturas/         # grafo, fila
  algoritmos/         # BFS, componentes, Bron-Kerbosch, Prim
  util/               # csv, texto (tokenização/stopwords), datas
  pipeline/           # vocabulario (DF/TF-IDF), coocorrencia (PPMI),
                      #   relatorio (saída), gexf, html
  ferramentas/        # converter (unifica as fontes brutas)
dados/
  brutos/             # manchetes originais (fontes do converter)
  convertidos/        # corpus unificado (data,titulo,resumo)
  finais/             # CSVs prontos para análise
tests/                # testes unitários
```

### Gerar o corpus a partir das fontes brutas (opcional)
```bash
make converter
./converter > dados/finais/noticias.csv
```

## 👥 Membros (Grupo 2 - Temática A)
* Caio Vilas Boas Miranda (232001380)
* Carlos Henrique de Paiva Munis (221022480)
* Davi Rodrigues Nunes (232014413)
* Felipe Lopes Pedroza (231026330)
* Gustavo Oki de Freitas Rodrigues Leite (231034716)
