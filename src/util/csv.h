#ifndef CSV_H
#define CSV_H

#define MAX_CAMPOS 16
#define MAX_CAMPO_LEN 4096

typedef struct {
  char campos[MAX_CAMPOS][MAX_CAMPO_LEN];
  int n_campos;
} LinhaCSV;

int csv_parse_linha(const char *linha, LinhaCSV *saida);

typedef void (*CsvCallback)(const LinhaCSV *linha, void *ctx);

void csv_ler_arquivo(const char *caminho, CsvCallback callback, void *ctx);

#endif
