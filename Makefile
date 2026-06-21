CC     = gcc
CFLAGS = -std=c99 -Wall -Wextra -Isrc -Itests

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

test: test_grafo test_fila test_bfs test_cliques test_prim test_texto test_csv
	./tests/test_grafo
	./tests/test_fila
	./tests/test_bfs
	./tests/test_cliques
	./tests/test_prim
	./tests/test_texto
	./tests/test_csv

clean:
	rm -f tests/test_grafo tests/test_fila tests/test_bfs tests/test_cliques tests/test_prim tests/test_texto tests/test_csv

.PHONY: test clean
