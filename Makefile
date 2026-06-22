CC     = gcc
CFLAGS = -std=c99 -Wall -Wextra -Isrc -Itests

SRC_FINGRAPH = src/main.c src/grafo.c src/algoritmos_grafo.c src/csv.c src/texto.c src/fila.c

fingraph: $(SRC_FINGRAPH)
	$(CC) $(CFLAGS) -O2 -o fingraph $(SRC_FINGRAPH) -lm

converter: src/converter.c src/csv.c src/datas.c
	$(CC) $(CFLAGS) -O2 -o converter src/converter.c src/csv.c src/datas.c -lm

test_datas: tests/test_datas.c src/datas.c src/datas.h
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_datas tests/test_datas.c src/datas.c -lm

test_grafo: tests/test_grafo.c src/grafo.c src/grafo.h
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_grafo tests/test_grafo.c src/grafo.c -lm

test_fila: tests/test_fila.c src/fila.c src/fila.h
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_fila tests/test_fila.c src/fila.c -lm

test_bfs: tests/test_bfs.c src/grafo.c src/fila.c src/algoritmos_grafo.c src/algoritmos_grafo.h
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_bfs tests/test_bfs.c src/grafo.c src/fila.c src/algoritmos_grafo.c -lm

test_cliques: tests/test_cliques.c src/grafo.c src/fila.c src/algoritmos_grafo.c src/algoritmos_grafo.h
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_cliques tests/test_cliques.c src/grafo.c src/fila.c src/algoritmos_grafo.c -lm

test_prim: tests/test_prim.c src/grafo.c src/fila.c src/algoritmos_grafo.c src/algoritmos_grafo.h
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_prim tests/test_prim.c src/grafo.c src/fila.c src/algoritmos_grafo.c -lm

test_texto: tests/test_texto.c src/texto.c src/texto.h
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_texto tests/test_texto.c src/texto.c -lm

test_csv: tests/test_csv.c src/csv.c src/csv.h
	$(CC) $(CFLAGS) -fsanitize=address -o tests/test_csv tests/test_csv.c src/csv.c -lm

test: test_grafo test_fila test_bfs test_cliques test_prim test_texto test_csv test_datas
	./tests/test_grafo
	./tests/test_fila
	./tests/test_bfs
	./tests/test_cliques
	./tests/test_prim
	./tests/test_texto
	./tests/test_csv
	./tests/test_datas

clean:
	rm -f tests/test_grafo tests/test_fila tests/test_bfs tests/test_cliques tests/test_prim tests/test_texto tests/test_csv tests/test_datas fingraph converter

.PHONY: test clean
