#include "datas.h"
#include <ctype.h>
#include <string.h>

/* Retorna 1..12 se o token começa com um nome de mês em inglês (3+ letras),
 * ou 0 caso contrário. Compara apenas os 3 primeiros caracteres (case-insensitive),
 * o que cobre tanto a forma curta ("Jul") quanto a extensa ("July"). */
static int mes_para_num(const char *tok) {
    static const char *meses[12] = {
        "jan", "feb", "mar", "apr", "may", "jun",
        "jul", "aug", "sep", "oct", "nov", "dec"};
    char p[4];
    int i = 0;
    for (; i < 3 && tok[i] != '\0'; i++)
        p[i] = (char)tolower((unsigned char)tok[i]);
    p[i] = '\0';
    if (i < 3) return 0;
    for (int m = 0; m < 12; m++)
        if (strcmp(p, meses[m]) == 0) return m + 1;
    return 0;
}

#define MAX_TOKENS_DATA 32

int data_para_iso(const char *bruto, char *saida) {
    saida[0] = '\0';
    if (!bruto) return 0;

    /* Tokeniza em sequências alfanuméricas, registrando posição e tipo. */
    int  tok_num[MAX_TOKENS_DATA];   /* valor numérico (-1 se for alfabético) */
    int  tok_len[MAX_TOKENS_DATA];   /* nº de dígitos (só p/ numéricos)        */
    int  tok_mes[MAX_TOKENS_DATA];   /* 1..12 se token é nome de mês, senão 0  */
    int  n_tok = 0;

    int i = 0;
    while (bruto[i] != '\0' && n_tok < MAX_TOKENS_DATA) {
        if (!isalnum((unsigned char)bruto[i])) { i++; continue; }

        int inicio = i;
        int eh_num = 1;
        while (bruto[i] != '\0' && isalnum((unsigned char)bruto[i])) {
            if (!isdigit((unsigned char)bruto[i])) eh_num = 0;
            i++;
        }
        int len = i - inicio;

        if (eh_num) {
            int val = 0;
            for (int k = inicio; k < inicio + len; k++)
                val = val * 10 + (bruto[k] - '0');
            tok_num[n_tok] = val;
            tok_len[n_tok] = len;
            tok_mes[n_tok] = 0;
        } else {
            char palavra[16];
            int  pl = len < 15 ? len : 15;
            memcpy(palavra, bruto + inicio, pl);
            palavra[pl] = '\0';
            tok_num[n_tok] = -1;
            tok_len[n_tok] = 0;
            tok_mes[n_tok] = mes_para_num(palavra);
        }
        n_tok++;
    }

    /* Localiza o token de mês. */
    int idx_mes = -1, mes = 0;
    for (int t = 0; t < n_tok; t++)
        if (tok_mes[t] > 0) { idx_mes = t; mes = tok_mes[t]; break; }
    if (idx_mes < 0) return 0;

    /* Dia: numérico mais próximo ANTES do mês; se não houver, o primeiro DEPOIS. */
    int dia = -1;
    for (int t = idx_mes - 1; t >= 0; t--)
        if (tok_num[t] >= 0 && tok_len[t] <= 2) { dia = tok_num[t]; break; }

    /* Ano: prefere numérico de 4 dígitos depois do mês; senão 2 dígitos (+2000). */
    int ano = -1;
    int idx_dia_pos = -1;  /* índice usado como dia, se veio depois do mês */
    if (dia < 0) {
        for (int t = idx_mes + 1; t < n_tok; t++)
            if (tok_num[t] >= 0 && tok_len[t] <= 2) {
                dia = tok_num[t];
                idx_dia_pos = t;
                break;
            }
    }
    for (int t = idx_mes + 1; t < n_tok; t++) {
        if (t == idx_dia_pos) continue;
        if (tok_num[t] >= 0 && tok_len[t] == 4) { ano = tok_num[t]; break; }
    }
    if (ano < 0) {
        for (int t = idx_mes + 1; t < n_tok; t++) {
            if (t == idx_dia_pos) continue;
            if (tok_num[t] >= 0 && tok_len[t] <= 2) { ano = tok_num[t] + 2000; break; }
        }
    }

    if (dia < 1 || dia > 31 || ano < 0) return 0;

    saida[0] = (char)('0' + (ano / 1000) % 10);
    saida[1] = (char)('0' + (ano / 100) % 10);
    saida[2] = (char)('0' + (ano / 10) % 10);
    saida[3] = (char)('0' + ano % 10);
    saida[4] = '-';
    saida[5] = (char)('0' + (mes / 10) % 10);
    saida[6] = (char)('0' + mes % 10);
    saida[7] = '-';
    saida[8] = (char)('0' + (dia / 10) % 10);
    saida[9] = (char)('0' + dia % 10);
    saida[10] = '\0';
    return 1;
}
