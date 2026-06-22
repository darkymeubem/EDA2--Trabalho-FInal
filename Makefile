CC     = gcc
INCS   = -Isrc -Isrc/estruturas -Isrc/algoritmos -Isrc/util -Isrc/pipeline -Itests
CFLAGS = -std=c99 -Wall -Wextra $(INCS)

# Módulos por responsabilidade
ESTRUTURAS = src/estruturas/grafo.c src/estruturas/fila.c
ALGORITMOS = src/algoritmos/algoritmos_grafo.c
UTIL       = src/util/csv.c src/util/texto.c src/util/datas.c
PIPELINE   = src/pipeline/vocabulario.c src/pipeline/coocorrencia.c \
             src/pipeline/relatorio.c src/pipeline/gexf.c src/pipeline/html.c

SRC_FINGRAPH = src/main.c $(PIPELINE) src/estruturas/grafo.c src/estruturas/fila.c \
               src/algoritmos/algoritmos_grafo.c src/util/csv.c src/util/texto.c

fingraph: $(SRC_FINGRAPH)
	$(CC) $(CFLAGS) -O2 -o fingraph $(SRC_FINGRAPH) -lm

converter: src/ferramentas/converter.c src/util/csv.c src/util/datas.c
	$(CC) $(CFLAGS) -O2 -o converter src/ferramentas/converter.c src/util/csv.c src/util/datas.c -lm

test_datas: tests/test_datas.c src/util/datas.c
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_datas tests/test_datas.c src/util/datas.c -lm

test_grafo: tests/test_grafo.c src/estruturas/grafo.c
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_grafo tests/test_grafo.c src/estruturas/grafo.c -lm

test_fila: tests/test_fila.c src/estruturas/fila.c
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_fila tests/test_fila.c src/estruturas/fila.c -lm

test_bfs: tests/test_bfs.c src/estruturas/grafo.c src/estruturas/fila.c src/algoritmos/algoritmos_grafo.c
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_bfs tests/test_bfs.c src/estruturas/grafo.c src/estruturas/fila.c src/algoritmos/algoritmos_grafo.c -lm

test_cliques: tests/test_cliques.c src/estruturas/grafo.c src/estruturas/fila.c src/algoritmos/algoritmos_grafo.c
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_cliques tests/test_cliques.c src/estruturas/grafo.c src/estruturas/fila.c src/algoritmos/algoritmos_grafo.c -lm

test_prim: tests/test_prim.c src/estruturas/grafo.c src/estruturas/fila.c src/algoritmos/algoritmos_grafo.c
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_prim tests/test_prim.c src/estruturas/grafo.c src/estruturas/fila.c src/algoritmos/algoritmos_grafo.c -lm

test_coocorrencia: tests/test_coocorrencia.c src/pipeline/coocorrencia.c src/estruturas/grafo.c
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_coocorrencia tests/test_coocorrencia.c src/pipeline/coocorrencia.c src/estruturas/grafo.c -lm

test_texto: tests/test_texto.c src/util/texto.c
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_texto tests/test_texto.c src/util/texto.c -lm

test_csv: tests/test_csv.c src/util/csv.c
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_csv tests/test_csv.c src/util/csv.c -lm

test: test_grafo test_fila test_bfs test_cliques test_prim test_coocorrencia test_texto test_csv test_datas
	./tests/test_grafo
	./tests/test_fila
	./tests/test_bfs
	./tests/test_cliques
	./tests/test_prim
	./tests/test_coocorrencia
	./tests/test_texto
	./tests/test_csv
	./tests/test_datas

clean:
	rm -f tests/test_grafo tests/test_fila tests/test_bfs tests/test_cliques tests/test_prim tests/test_coocorrencia tests/test_texto tests/test_csv tests/test_datas fingraph converter

.PHONY: test clean
