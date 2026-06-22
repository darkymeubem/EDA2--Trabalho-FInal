#ifndef DATAS_H
#define DATAS_H

/*
 * Normaliza uma data textual heterogênea para o formato ISO "YYYY-MM-DD".
 *
 * Formatos suportados (observados nas fontes brutas):
 *   "Jul 18 2020"                  (reuters)   -> 2020-07-18
 *   "17 July 2020"                 (cnbc, parte da data) -> 2020-07-17
 *   "18-Jul-20"                    (guardian)  -> 2020-07-18
 *   " 7:51  PM ET Fri, 17 July 2020" (cnbc completo) -> 2020-07-17
 *
 * 'saida' deve ter espaço para ao menos 11 bytes ("YYYY-MM-DD\0").
 * Retorna 1 em sucesso (mês e dia identificados) e 0 caso contrário;
 * em falha, 'saida' recebe string vazia.
 */
int data_para_iso(const char *bruto, char *saida);

#endif
