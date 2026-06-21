#include "csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINHA_LEN 8192

int csv_parse_linha(const char *linha, LinhaCSV *saida) {
  int campo = 0;
  int pos = 0;
  int dentro_aspas = 0;
  int i = 0;

  saida->n_campos = 0;

  while (1) {
    char c = linha[i];

    if (dentro_aspas) {
      if (c == '\0') {
        break;
      } else if (c == '"') {
        if (linha[i + 1] == '"') {
          if (pos < MAX_CAMPO_LEN - 1)
            saida->campos[campo][pos++] = '"';
          i += 2;
        } else {
          dentro_aspas = 0;
          i++;
        }
      } else {
        if (pos < MAX_CAMPO_LEN - 1)
          saida->campos[campo][pos++] = c;
        i++;
      }

    } else {
      if (c == '"') {
        dentro_aspas = 1;
        i++;
      } else if (c == ',') {
        saida->campos[campo][pos] = '\0';
        campo++;
        pos = 0;
        i++;
        if (campo >= MAX_CAMPOS) {
          saida->n_campos = campo;
          return saida->n_campos;
        }
      } else if (c == '\0') {
        break;
      } else {
        if (pos < MAX_CAMPO_LEN - 1)
          saida->campos[campo][pos++] = c;
        i++;
      }
    }
  }

  saida->campos[campo][pos] = '\0';
  saida->n_campos = campo + 1;

  return saida->n_campos;
}

void csv_ler_arquivo(const char *caminho, CsvCallback callback, void *ctx) {
  FILE *f = fopen(caminho, "r");
  if (!f) {
    perror("fopen");
    exit(1);
  }

  char linha[MAX_LINHA_LEN];
  int primeira_linha = 1;

  while (fgets(linha, sizeof(linha), f)) {
    size_t len = strlen(linha);
    while (len > 0 && (linha[len - 1] == '\n' || linha[len - 1] == '\r')) {
      linha[--len] = '\0';
    }

    if (primeira_linha) {
      primeira_linha = 0;
      continue;
    }
    if (len == 0)
      continue;

    LinhaCSV lc;
    csv_parse_linha(linha, &lc);
    callback(&lc, ctx);
  }

  fclose(f);
}
