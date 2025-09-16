#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define STR_BUFF_SZ 1024

static bool tmp_bool = false;

enum CLEX {
  oof,
  CLEX_eof,
  CLEX_unknown,
  CLEX_parse_error,
  CLEX_intlit,
  CLEX_floatlit,
  CLEX_id,
  CLEX_dqstring,
  CLEX_punct,
  CLEX_charlit,
  CLEX_eq,
  CLEX_noteq,
  CLEX_lesseq,
  CLEX_greatereq,
  CLEX_andand,
  CLEX_oror,
  CLEX_shl,
  CLEX_shr,
  CLEX_plusplus,
  CLEX_minusminus,
  CLEX_pluseq,
  CLEX_minuseq,
  CLEX_muleq,
  CLEX_diveq,
  CLEX_modeq,
  CLEX_andeq,
  CLEX_oreq,
  CLEX_xoreq,
  CLEX_arrow,
  CLEX_eqarrow,
  CLEX_shleq,
  CLEX_shreq,
};

typedef struct {
  enum CLEX type;
  char charlit;
  long intlit;
  double floatlit;
  // this should be null terminated
  Nob_String_Builder str;

  size_t col;
  size_t row;
} token_t;

typedef struct {
  const char *file;
  char *content;
  size_t content_size;
  size_t i_col;
  size_t i_row;

  size_t cursor;

  token_t token;

} lexer_t;

int l_init(const char *file, lexer_t *l) {
  Nob_String_Builder sb = {0};
  if (!read_entire_file(file, &sb))
    return 1;
  l->file = file;
  l->content = sb.items;
  l->content_size = sb.count;

  da_append(&sb, '\0');
  return 0;
}

char char_to_num(char c) {
  switch (c) {
  case '1':
    return 1;
    break;
  case '2':
    return 2;
    break;
  case '3':
    return 3;
    break;
  case '4':
    return 4;
    break;
  case '5':
    return 5;
    break;
  case '6':
    return 6;
    break;
  case '7':
    return 7;
    break;
  case '8':
    return 8;
    break;
  case '9':
    return 9;
    break;
  case '0':
    return 0;
    break;
  default:
    printf("wtf is %c\n", c);
  }

  UNREACHABLE("char char_to_num()");
}

#define is_alphanumerical(c)                                                   \
  ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_')
#define is_numerical(c) (c >= '0' && c <= '9')

// current char
#define CC(l) (l)->content[(l)->cursor]

#define loc "%s:%ld:%ld"
#define loc_ptr(l) (l)->file, (l)->token.row, (l)->token.col
#define prt_assert(cond, msg, ...)                                             \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, msg, __VA_ARGS__);                                       \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define INC_CURSOR(l)                                                          \
  do {                                                                         \
    (l)->cursor++;                                                             \
    (l)->i_col++;                                                              \
    if (CC(l) == '\n') {                                                       \
      (l)->i_col = 0;                                                          \
      (l)->i_row++;                                                            \
    }                                                                          \
  } while (0)

// returns true if a token is found
bool get_token(lexer_t *l) {

  l->token.type = 0;
  l->token.str.count = 0;
  l->token.charlit = 0;
  l->token.floatlit = 0;
  l->token.intlit = 0;

  while (CC(l) == ' ' || CC(l) == '\t') {
    INC_CURSOR(l);
  }

  if (l->cursor >= l->content_size)
    return false;

  if (is_alphanumerical(CC(l))) {
    l->token.col = l->i_col;
    l->token.row = l->i_row;

    while (is_alphanumerical(CC(l))) {
      da_append(&l->token.str, CC(l));
      INC_CURSOR(l);
    }

    da_append(&l->token.str, '\0');
    INC_CURSOR(l);
    l->token.type = CLEX_id;
  } else if (CC(l) == '"') {
    l->token.col = l->i_col;
    l->token.row = l->i_row;

    INC_CURSOR(l);

    while (CC(l) != '"') {
      if (CC(l) == '\\') {
        // TODO: do things here
        da_append(&l->token.str, CC(l));
        INC_CURSOR(l);
        da_append(&l->token.str, CC(l));
      } else {
        da_append(&l->token.str, CC(l));
      }
      INC_CURSOR(l);
    }
    INC_CURSOR(l);
    da_append(&l->token.str, '\0');
    l->token.type = CLEX_dqstring;

  } else if (is_numerical(CC(l))) {
    l->token.col = l->i_col;
    l->token.row = l->i_row;

    l->token.type = CLEX_intlit;
    while (is_numerical(CC(l))) {
      l->token.intlit *= 10;
      l->token.intlit += char_to_num(CC(l));
      INC_CURSOR(l);
    }
    if (CC(l) == '.') {
      // TODO: optimize it maybe
      l->token.type = CLEX_floatlit;
      l->token.floatlit = l->token.intlit;
      l->token.intlit = 0;

      INC_CURSOR(l);

      while (is_numerical(CC(l))) {
        l->token.intlit *= 10;
        l->token.intlit += char_to_num(CC(l));
        INC_CURSOR(l);
      }

      float f = l->token.intlit;
      while (abs((int)floor(f)) > 1)
        f /= 10;

      l->token.floatlit += f;
      l->token.intlit = 0;
    }

  } else if (CC(l) == '\'') {
    l->token.col = l->i_col;
    l->token.row = l->i_row;

    INC_CURSOR(l);
    l->token.charlit = CC(l);
    l->token.type = CLEX_charlit;
    INC_CURSOR(l);

    prt_assert(CC(l) == '\'',
               loc " single quote strings are not supported, "
                   "either use double quote or fix your char\n",
               loc_ptr(l));
    INC_CURSOR(l);
  } else {
    l->token.col = l->i_col;
    l->token.row = l->i_row;
    l->token.charlit = CC(l);
    l->token.type = CLEX_punct;
    INC_CURSOR(l);
  }

  return true;
}

// this function does not modify the lexer
bool check_puncts(lexer_t *l, int count, ...) {

  token_t token = l->token;
  size_t i_row = l->i_row;
  size_t i_col = l->i_col;
  size_t cursor = l->cursor;

  va_list args;
  va_start(args, count);

  for (int i = 0; i < count; i++) {
    if (l->token.charlit == va_arg(args, int)) {
      get_token(l);
      continue;
    } else {

      l->token = token;
      l->i_row = i_row;
      l->i_col = i_col;
      l->cursor = cursor;

      va_end(args);
      return false;
    }
  }

  l->token = token;
  l->i_row = i_row;
  l->i_col = i_col;
  l->cursor = cursor;

  va_end(args);
  return true;
}

// TODO: find a way to do it without tmp_bool
#define check_puncts_n_skip(l, count, ...)                                     \
  tmp_bool = check_puncts(l, count, __VA_ARGS__);                              \
  if (tmp_bool) {                                                              \
    for (int i = 1; i < count; i++) {                                          \
      get_token(l);                                                            \
    }                                                                          \
  }                                                                            \
  if (tmp_bool)


int main() {
  lexer_t l = {0};
  l_init("./test.te", &l);

  printf("%s\n", l.content);

  while (get_token(&l)) {
    switch (l.token.type) {
    case CLEX_id:
      printf(loc " id: '%s'\n", loc_ptr(&l), l.token.str.items);
      break;
    case CLEX_dqstring:
      printf(loc " dqstring: \"%s\"\n", loc_ptr(&l), l.token.str.items);
      break;
    case CLEX_charlit:
      printf(loc " charlit: '%c'\n", loc_ptr(&l), l.token.charlit);
      break;
    case CLEX_intlit:
      printf(loc " intlit: %ld\n", loc_ptr(&l), l.token.intlit);
      break;
    case CLEX_floatlit:
      printf(loc " floatlit: %f\n", loc_ptr(&l), l.token.floatlit);
      break;
    case CLEX_punct:
      check_puncts_n_skip(&l, 2, '+', '+') {
        printf(loc " spec punct: ++\n", loc_ptr(&l));
      } else {
        printf(loc " punct: '%c'\n", loc_ptr(&l), l.token.charlit);
      }
      break;

    case CLEX_eq:
    case CLEX_noteq:
    case CLEX_lesseq:
    case CLEX_greatereq:
    case CLEX_andand:
    case CLEX_oror:
    case CLEX_shl:
    case CLEX_shr:
    case CLEX_plusplus:
    case CLEX_minusminus:
    case CLEX_pluseq:
    case CLEX_minuseq:
    case CLEX_muleq:
    case CLEX_diveq:
    case CLEX_modeq:
    case CLEX_andeq:
    case CLEX_oreq:
    case CLEX_xoreq:
    case CLEX_arrow:
    case CLEX_eqarrow:
    case CLEX_shleq:
    case CLEX_shreq:

    case oof:
    case CLEX_eof:
    case CLEX_unknown:
    case CLEX_parse_error:
    default:
      break;
    }
  }

  return 0;
}