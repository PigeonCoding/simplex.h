#define SIMPLEX_IMPLEMENTATION
#include "simplex.h"

int main() {

  lexer_t l = {0};
  if (!spx_init("./test.txt", &l))
    return 1;

  l.slash_comments = true;
  l.pound_comments = true;

  printf("%s\n", l.content.items);
  printf("----------------------------\n");

  while (spx_get_token(&l)) {
    switch (l.token.type) {
    case SPX_id:
      printf(LOC " id: '%s'\n", LOC_PRT(&l), l.token.str.items);
      break;
    case SPX_dqstring:
      printf(LOC " dqstring: \"%s\"\n", LOC_PRT(&l), l.token.str.items);
      break;
    case SPX_charlit:
      printf(LOC " charlit: '%c'\n", LOC_PRT(&l), l.token.charlit);
      break;
    case SPX_intlit:
      printf(LOC " intlit: %ld\n", LOC_PRT(&l), l.token.intlit);
      break;
    case SPX_floatlit:
      printf(LOC " floatlit: %f\n", LOC_PRT(&l), l.token.floatlit);
      break;
    case SPX_punct:
      spx_check_puncts_n_skip(&l, 2, '+', '+') {
        printf(LOC " spec punct: ++\n", LOC_PRT(&l));
      }
      else {
        printf(LOC " punct: '%c'\n", LOC_PRT(&l), l.token.charlit);
      }
      break;

    case SPX_eof:
      break;
    }
  }

  spx_free(&l);

  return 0;
}