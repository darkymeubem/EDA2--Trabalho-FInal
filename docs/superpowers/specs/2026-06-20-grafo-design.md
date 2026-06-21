# Estrutura de Grafo — Design

**Escopo:** Módulo standalone `src/grafo.h` + `src/grafo.c` com a estrutura de dados do grafo ponderado não-direcionado (lista de adjacência). Sem dependências de outros módulos do projeto.

**Contexto:** Base estrutural do FinGraph Analytics. Representa o grafo de coocorrência de termos financeiros, onde vértices são termos (identificados por índice inteiro) e arestas têm peso double (PPMI futuramente; peso manual no modo standalone).

---

## Estruturas de Dados

```c
/* Nó de aresta na lista encadeada */
typedef struct ArestaNo {
    int             vizinho;  /* índice do vértice destino */
    double          peso;     /* peso da aresta */
    struct ArestaNo *prox;    /* próximo nó na lista */
} ArestaNo;

/* Grafo não-direcionado ponderado, lista de adjacência */
typedef struct {
    ArestaNo **adj;    /* adj[v] → cabeça da lista de vizinhos de v */
    int       *grau;   /* grau[v] = número de vizinhos de v */
    int        n_vert; /* número de vértices (fixo na criação) */
    int        n_arest;/* número de arestas (contado 1x por aresta) */
} Grafo;
```

**Decisões de design:**
- `n_vert` é fixado em `grafo_criar` — vértices não são adicionados depois. Coerente com o pipeline: seleção de termos precede construção de arestas.
- Vértices identificados por índice inteiro (0 a n_vert-1). O mapeamento índice → string (termo) é responsabilidade do chamador.
- Grafo não-direcionado: cada aresta (u,v) armazena dois nós de lista (u→v e v→u).

---

## Interface Pública

```c
/* Cria grafo com n_vert vértices, sem arestas. */
Grafo *grafo_criar(int n_vert);

/* Adiciona aresta não-direcionada (u, v) com peso.
   Ignora se u == v ou se a aresta já existe. */
void   grafo_adicionar_aresta(Grafo *g, int u, int v, double peso);

/* Retorna peso da aresta (u,v), ou -1.0 se não existe. */
double grafo_peso_aresta(const Grafo *g, int u, int v);

/* Retorna 1 se existe aresta entre u e v, 0 caso contrário. */
int    grafo_tem_aresta(const Grafo *g, int u, int v);

/* Libera toda a memória do grafo. */
void   grafo_liberar(Grafo *g);
```

**Funções ausentes por YAGNI:** remoção de aresta, adição de vértice após criação, cópia de grafo.

---

## Comportamento em Casos de Borda

| Situação | Comportamento |
|----------|---------------|
| `grafo_criar(0)` | Retorna grafo válido com `n_vert=0` |
| Aresta reflexiva `u == v` | Ignorada silenciosamente |
| Aresta duplicada `(u,v)` já existente | Ignorada (não duplica, não atualiza peso) |
| Índice fora de `[0, n_vert)` | Comportamento indefinido — caller é responsável |
| `malloc` falha | `perror` + `exit(1)` |

---

## Arquivo de Teste

`tests/test_grafo.c` — `main()` próprio, usa `tests/testutil.h`.

Casos cobertos:
1. Criar grafo → `n_vert` correto, `n_arest == 0`, todos os graus zero
2. Adicionar aresta (u,v,w) → `grau[u]` e `grau[v]` incrementados, `n_arest == 1`
3. `grafo_tem_aresta(u,v)` e `grafo_tem_aresta(v,u)` retornam 1
4. `grafo_peso_aresta(u,v)` retorna o peso correto
5. Aresta inexistente → `grafo_tem_aresta` retorna 0, `grafo_peso_aresta` retorna -1.0
6. Aresta duplicada não incrementa `n_arest`
7. Aresta reflexiva `(u,u)` é ignorada
8. `grafo_liberar` sem crash (verificado com `-fsanitize=address`)

Compilação do teste:
```bash
gcc -std=c99 -Wall -Wextra -fsanitize=address -o tests/test_grafo tests/test_grafo.c src/grafo.c -lm
./tests/test_grafo
```

---

## O Que Este Módulo NÃO É

- Não calcula PPMI (responsabilidade futura do módulo de construção)
- Não conhece termos/strings (o chamador mapeia índice → termo)
- Não implementa algoritmos (BFS, componentes, cliques, MST — tasks futuras)
