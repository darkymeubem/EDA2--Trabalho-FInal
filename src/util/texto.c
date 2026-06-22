#include "texto.h"
#include <ctype.h>
#include <string.h>

static const char TABELA_LATIN1[64] = {
    /* 0xC3 0x80-0x8F (À-Ï) */
    'A', 'A', 'A', 'A', 'A', 'A', 0, 'C', 'E', 'E', 'E', 'E', 'I', 'I', 'I',
    'I',
    /* 0xC3 0x90-0x9F (Ð-ß) */
    'D', 'N', 'O', 'O', 'O', 'O', 'O', 0, 'O', 'U', 'U', 'U', 'U', 'Y', 'T',
    's',
    /* 0xC3 0xA0-0xAF (à-ï) */
    'a', 'a', 'a', 'a', 'a', 'a', 0, 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i',
    'i',
    /* 0xC3 0xB0-0xBF (ð-ÿ) */
    'd', 'n', 'o', 'o', 'o', 'o', 'o', 0, 'o', 'u', 'u', 'u', 'u', 'y', 't',
    'y'};

void normalizar_texto(const char *entrada, char *saida) {
  int i = 0, j = 0;
  int ultimo_eh_espaco = 1;

  while (entrada[i] != '\0') {
    unsigned char c = (unsigned char)entrada[i];

    if (c < 0x80) {
      if (isalnum(c)) {
        saida[j++] = (char)tolower(c);
        ultimo_eh_espaco = 0;
      } else if (!ultimo_eh_espaco) {
        saida[j++] = ' ';
        ultimo_eh_espaco = 1;
      }
      i++;

    } else if (c == 0xC3 && (unsigned char)entrada[i + 1] >= 0x80 &&
               (unsigned char)entrada[i + 1] <= 0xBF) {
      unsigned char cont = (unsigned char)entrada[i + 1];
      char mapeado = TABELA_LATIN1[cont - 0x80];

      if (mapeado != 0) {
        saida[j++] = (char)tolower((unsigned char)mapeado);
        ultimo_eh_espaco = 0;
      } else if (!ultimo_eh_espaco) {
        saida[j++] = ' ';
        ultimo_eh_espaco = 1;
      }
      i += 2;

    } else {
      int tam = 1;
      if ((c & 0xE0) == 0xC0)
        tam = 2;
      else if ((c & 0xF0) == 0xE0)
        tam = 3;
      else if ((c & 0xF8) == 0xF0)
        tam = 4;

      if (!ultimo_eh_espaco) {
        saida[j++] = ' ';
        ultimo_eh_espaco = 1;
      }
      i += tam;
    }
  }

  while (j > 0 && saida[j - 1] == ' ')
    j--; /* remove espaço final */
  saida[j] = '\0';
}

static const char *STOPWORDS[] = {
    "a",       "about",  "above",   "after",      "again",    "against",
    "all",     "am",     "an",      "and",        "any",      "are",
    "as",      "at",     "be",      "because",    "been",     "before",
    "being",   "below",  "between", "both",       "but",      "by",
    "can",     "could",  "did",     "do",         "does",     "doing",
    "down",    "during", "each",    "few",        "for",      "from",
    "further", "had",    "has",     "have",       "having",   "he",
    "her",     "here",   "hers",    "herself",    "him",      "himself",
    "his",     "how",    "i",       "if",         "in",       "into",
    "is",      "it",     "its",     "itself",     "just",     "me",
    "more",    "most",   "my",      "myself",     "no",       "nor",
    "not",     "now",    "of",      "off",        "on",       "once",
    "only",    "or",     "other",   "our",        "ours",     "ourselves",
    "out",     "over",   "own",     "same",       "she",      "should",
    "so",      "some",   "such",    "than",       "that",     "the",
    "their",   "theirs", "them",    "themselves", "then",     "there",
    "these",   "they",   "this",    "those",      "through",  "to",
    "too",     "under",  "until",   "up",         "very",     "was",
    "we",      "were",   "what",    "when",       "where",    "which",
    "while",   "who",    "whom",    "why",        "will",     "with",
    "would",   "you",    "your",    "yours",      "yourself", "yourselves"};

int eh_stopword(const char *palavra) {
  int n = sizeof(STOPWORDS) / sizeof(STOPWORDS[0]);
  for (int i = 0; i < n; i++) {
    if (strcmp(palavra, STOPWORDS[i]) == 0)
      return 1;
  }
  return 0;
}

/* Token puramente numérico (ex: "2008", "3000", "10") não carrega
 * significado temático para o grafo de coocorrência — é ruído. */
static int eh_numerico(const char *palavra) {
  if (palavra[0] == '\0') return 0;
  for (const char *p = palavra; *p != '\0'; p++)
    if (!isdigit((unsigned char)*p)) return 0;
  return 1;
}

int tokenizar(const char *texto_normalizado, char tokens[][MAX_TOKEN_LEN],
              int max_tokens) {
  int n_tokens = 0;
  const char *p = texto_normalizado;

  while (*p != '\0' && n_tokens < max_tokens) {
    while (*p == ' ')
      p++;
    if (*p == '\0')
      break;

    const char *inicio = p;
    while (*p != ' ' && *p != '\0')
      p++;

    int len = (int)(p - inicio);
    if (len >= MAX_TOKEN_LEN)
      len = MAX_TOKEN_LEN - 1;

    char palavra[MAX_TOKEN_LEN];
    memcpy(palavra, inicio, len);
    palavra[len] = '\0';

    if (!eh_stopword(palavra) && !eh_numerico(palavra)) {
      strcpy(tokens[n_tokens], palavra);
      n_tokens++;
    }
  }

  return n_tokens;
}
