
// exmaple on how to use simplex
/*
  #define SIMPLEX_IMPLEMENTATION
  #include "simplex.h"

  int main() {
    lexer_t l = {0};
    l_init("./file.txt", &l);

    while (get_token(&l)) {
      switch (l.token.type) {
      // do your stuff here
      }
    }

    l_reset(&l); // if you need to reset the lexer and parse another file instead of creating a new one

    l_free(&l);

  }

*/

#ifndef SIMPLEX_H_
#define SIMPLEX_H_

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// this part is taken from nob - v1.23.0 - Public Domain -
// https://github.com/tsoding/nob.h just renamed as to not cause problems
// with using both simplex and nob at the same time

#ifdef __cplusplus
#define __DECLTYPE_CAST(T) (decltype(T))
#else
#define __DECLTYPE_CAST(T)
#endif // __cplusplus

#ifndef SPX_ASSERT
#include <assert.h>
#define SPX_ASSERT assert
#endif /* SPX_ASSERT */

#ifndef SPX_REALLOC
#include <stdlib.h>
#define SPX_REALLOC realloc
#endif /* SPX_REALLOC */

#ifndef SPX_FREE
#include <stdlib.h>
#define SPX_FREE free
#endif /* SPX_FREE */

#define spx_da_reserve(da, expected_capacity)                                  \
  do {                                                                         \
    if ((expected_capacity) > (da)->capacity) {                                \
      if ((da)->capacity == 0) {                                               \
        (da)->capacity = 128;                                                  \
      }                                                                        \
      while ((expected_capacity) > (da)->capacity) {                           \
        (da)->capacity *= 2;                                                   \
      }                                                                        \
      (da)->items = __DECLTYPE_CAST((da)->items)                               \
          SPX_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));     \
      SPX_ASSERT((da)->items != NULL && "Buy more RAM lol");                   \
    }                                                                          \
  } while (0)

#define spx_da_append(da, item)                                                \
  do {                                                                         \
    spx_da_reserve((da), (da)->count + 1);                                     \
    (da)->items[(da)->count++] = (item);                                       \
  } while (0)

#define spx_da_free(da) SPX_FREE((da).items)

typedef struct {
  char *items;
  size_t count;
  size_t capacity;
} SPX_String_Builder;
typedef struct {
  const char **items;
  size_t count;
  size_t capacity;
} SPX_File_Paths;

#define __return_defer(value)                                                  \
  do {                                                                         \
    result = (value);                                                          \
    goto defer;                                                                \
  } while (0)

bool spx_read_entire_file(const char *path, SPX_String_Builder *sb);

// -----------------------------------------------

#define MINUS_ATTACHED

static bool tmp_bool = false;

enum SPX {
  SPX_eof,
  SPX_intlit,
  SPX_floatlit,
  SPX_id,
  SPX_dqstring,
  SPX_punct,
  SPX_charlit,
};

typedef struct {
  enum SPX type;
  char charlit;
  long intlit;
  double floatlit;
  // this is null terminated
  SPX_String_Builder str;

  uint16_t col;
  uint16_t row;
} token_t;

typedef struct {
  const char *file;
  SPX_String_Builder content;

  uint32_t cursor;
  uint16_t _col;
  uint16_t _row;

  token_t token;

} lexer_t;

int l_init(const char *file, lexer_t *l);
bool get_token(lexer_t *l);
bool check_puncts(lexer_t *l, int count, ...);
void l_reset(lexer_t *l);
void l_free(lexer_t *l);

#define is_letter(c)                                                           \
  (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'
#define is_numerical(c) (c >= '0' && c <= '9')
#define is_alphanumerical(c) (is_letter(c) || is_numerical(c))

#define _CC(l) (l)->content.items[(l)->cursor]

#define LOC "%s:%d:%d"
#define LOC_PRT(l) (l)->file, (l)->token.row, (l)->token.col
#define _PRT_ASSERT(cond, msg, ...)                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, msg, __VA_ARGS__);                                       \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define _INC_CURSOR(l, nl)                                                     \
  do {                                                                         \
    if (_CC(l) == '\n' && nl) {                                                \
      (l)->_col = 0;                                                           \
      (l)->_row++;                                                             \
      (l)->cursor++;                                                           \
    } else {                                                                   \
      (l)->cursor++;                                                           \
      (l)->_col++;                                                             \
    }                                                                          \
  } while (0)

// TODO: find a way to do it without tmp_bool
#define check_puncts_n_skip(l, count, ...)                                     \
  do {                                                                         \
    tmp_bool = check_puncts(l, count, __VA_ARGS__);                            \
    if (tmp_bool) {                                                            \
      for (int i = 1; i < count; i++) {                                        \
        get_token(l);                                                          \
      }                                                                        \
    }                                                                          \
  } while (0);                                                                 \
  if (tmp_bool)

#define get_token_and_expect(l, type)                                          \
  get_token((l)) && (l)->token.type == (type)
#define get_token_and_expect_punct(l, punct)                                   \
  get_token_and_expect(l, SPX_punct) && (l)->token.charlit == (punct)

// this function converts chars '0'-'9' to numbers 0-9
#define _char_to_nm(c)                                                         \
  (((c) == '1') * 1 + ((c) == '2') * 2 + ((c) == '3') * 3 + ((c) == '4') * 4 + \
   ((c) == '5') * 5 + ((c) == '6') * 6 + ((c) == '7') * 7 + ((c) == '8') * 8 + \
   ((c) == '9') * 9)

#endif // SIMPLEX_H_

#ifdef SIMPLEX_IMPLEMENTATION

int l_init(const char *file, lexer_t *l) {
  l->content = (SPX_String_Builder){.capacity = 0, .count = 0, .items = NULL};
  if (!spx_read_entire_file(file, &l->content)) {
    fprintf(stderr, "could not read file %s\n", file);
    return 1;
  }
  l->file = file;

  spx_da_append(&l->content, '\0');
  return 0;
}

// returns true if a token is found
bool get_token(lexer_t *l) {

  l->token.type = 0;
  l->token.str.count = 0;
  l->token.charlit = 0;
  l->token.floatlit = 0;
  l->token.intlit = 0;
  l->token.col = 0;
  l->token.row = 0;

  while (_CC(l) == ' ' || _CC(l) == '\t' || _CC(l) == '\r' || _CC(l) == '\n') {
    _INC_CURSOR(l, true);
  }

  if (l->cursor >= l->content.count) {
    l->token.type = SPX_eof;
    return false;
  }

  if (is_letter(_CC(l))) {
    l->token.col = l->_col + 1;
    l->token.row = l->_row + 1;

    while (is_alphanumerical(_CC(l))) {
      spx_da_append(&l->token.str, _CC(l));
      _INC_CURSOR(l, false);
    }

    spx_da_append(&l->token.str, '\0');
    l->token.type = SPX_id;
  } else if (_CC(l) == '"') {
    l->token.col = l->_col + 1;
    l->token.row = l->_row + 1;

    _INC_CURSOR(l, false);

    while (_CC(l) != '"') {
      if (_CC(l) == '\\') {
        uint16_t row = l->_row + 1;
        uint16_t col = l->_col + 1;
        _INC_CURSOR(l, false);
        switch (_CC(l)) {
        case 'b':
          spx_da_append(&l->token.str, '\b');
          break;
        case 'f':
          spx_da_append(&l->token.str, '\f');
          break;
        case 'n':
          spx_da_append(&l->token.str, '\n');
          break;
        case 'r':
          spx_da_append(&l->token.str, '\r');
          break;
        case 't':
          spx_da_append(&l->token.str, '\t');
          break;
        case 'v':
          spx_da_append(&l->token.str, '\v');
          break;
        case '\\':
          spx_da_append(&l->token.str, '\\');
          break;
        case '\'':
          break;
        case '"':
          spx_da_append(&l->token.str, '"');
          break;
        case '0':
          spx_da_append(&l->token.str, '\0');
          break;

        default:
          printf(LOC " unknown escape code here\n", l->file, row, col);
        }
        // spx_da_append(&l->token.str, _CC(l));
      } else {
        spx_da_append(&l->token.str, _CC(l));
      }
      _INC_CURSOR(l, false);
    }
    spx_da_append(&l->token.str, '\0');
    l->token.type = SPX_dqstring;
    _INC_CURSOR(l, true);

  } else if (is_numerical(_CC(l))) {
    l->token.col = l->_col + 1;
    l->token.row = l->_row + 1;

    l->token.type = SPX_intlit;
    while (is_numerical(_CC(l))) {
      l->token.intlit *= 10;
      l->token.intlit += _char_to_nm(_CC(l));
      _INC_CURSOR(l, false);
    }

    if (l->token.intlit != 0 && _CC(l) == '#') {
      int base = l->token.intlit;
      l->token.intlit = 0;
      _INC_CURSOR(l, false);
      const char *current = l->content.items + l->cursor;

      while (is_alphanumerical(_CC(l))) {
        l->token.intlit *= 10;
        l->token.intlit += _char_to_nm(_CC(l));
        _INC_CURSOR(l, false);
      }

      char tmp = _CC(l);
      _CC(l) = '\0';
      l->token.intlit = strtoull(current, NULL, base);
      _CC(l) = tmp;
    } else if (l->token.intlit == 0 && _CC(l) == 'x') {
      int base = 16;
      // l->token.intlit = 0;
      _INC_CURSOR(l, false);
      const char *current = l->content.items + l->cursor;

      while (is_alphanumerical(_CC(l))) {
        l->token.intlit *= 10;
        l->token.intlit += _char_to_nm(_CC(l));
        _INC_CURSOR(l, false);
      }

      char tmp = _CC(l);
      _CC(l) = '\0';
      l->token.intlit = strtoull(current, NULL, base);
      _CC(l) = tmp;
    } else if (l->token.intlit == 0 && _CC(l) == 'o') {
      int base = 8;
      // l->token.intlit = 0;
      _INC_CURSOR(l, false);
      const char *current = l->content.items + l->cursor;

      while (is_alphanumerical(_CC(l))) {
        l->token.intlit *= 10;
        l->token.intlit += _char_to_nm(_CC(l));
        _INC_CURSOR(l, false);
      }

      char tmp = _CC(l);
      _CC(l) = '\0';
      l->token.intlit = strtoull(current, NULL, base);
      _CC(l) = tmp;
    } else if (l->token.intlit == 0 && _CC(l) == 'b') {
      int base = 2;
      // l->token.intlit = 0;
      _INC_CURSOR(l, false);
      const char *current = l->content.items + l->cursor;

      while (is_alphanumerical(_CC(l))) {
        l->token.intlit *= 10;
        l->token.intlit += _char_to_nm(_CC(l));
        _INC_CURSOR(l, false);
      }

      char tmp = _CC(l);
      _CC(l) = '\0';
      l->token.intlit = strtoull(current, NULL, base);
      _CC(l) = tmp;
    }

    if (_CC(l) == '.') {
      // TODO: optimize it maybe
      l->token.type = SPX_floatlit;
      l->token.floatlit = l->token.intlit;
      l->token.intlit = 1;

      _INC_CURSOR(l, false);

      while (is_numerical(_CC(l))) {
        l->token.intlit *= 10;
        l->token.intlit += _char_to_nm(_CC(l));
        _INC_CURSOR(l, false);
      }

      float f = l->token.intlit;
      while (abs((int)(f)) > 1)
        f /= 10;

      l->token.floatlit += f - 1;
      l->token.intlit = 0;
    }

  } else if (_CC(l) == '\'') {
    l->token.col = l->_col + 1;
    l->token.row = l->_row + 1;

    _INC_CURSOR(l, false);
    l->token.charlit = _CC(l);
    l->token.type = SPX_charlit;
    _INC_CURSOR(l, false);

    _PRT_ASSERT(_CC(l) == '\'',
                LOC " single quote strings are not supported, "
                    "either use double quote or fix your char\n",
                LOC_PRT(l));
    _INC_CURSOR(l, true);
  } else {
    if (_CC(l) == 0)
      return false;
    bool yes = false;

    if (_CC(l) == '-') {
      uint16_t t = l->cursor;
      _INC_CURSOR(l, false);

      if (
#ifdef MINUS_ATTACHED
          is_numerical(_CC(l)) &&
#endif
          get_token(l) &&
          (l->token.type == SPX_floatlit || l->token.type == SPX_intlit)) {
        yes = true;
        if (l->token.type == SPX_intlit) {
          l->token.intlit = -l->token.intlit;
        } else if (l->token.type == SPX_floatlit) {
          l->token.floatlit = -l->token.floatlit;
        }
        l->token.col -= 1;
      } else {
        l->cursor = t;
      }
    }

    if (!yes) {
      l->token.col = l->_col + 1;
      l->token.row = l->_row + 1;
      l->token.charlit = _CC(l);
      l->token.type = SPX_punct;
      _INC_CURSOR(l, true);
    }
  }

  return true;
}

// this function does not modify the lexer
bool check_puncts(lexer_t *l, int count, ...) {

  token_t token = l->token;
  size_t _row = l->_row;
  size_t _col = l->_col;
  size_t cursor = l->cursor;

  va_list args;
  va_start(args, count);

  for (int i = 0; i < count; i++) {
    if (l->token.charlit == va_arg(args, int)) {
      get_token(l);
      continue;
    } else {

      l->token = token;
      l->_row = _row;
      l->_col = _col;
      l->cursor = cursor;

      va_end(args);
      return false;
    }
  }

  l->token = token;
  l->_row = _row;
  l->_col = _col;
  l->cursor = cursor;

  va_end(args);
  return true;
}

void l_reset(lexer_t *l) {
  l->content.count = 0;
  l->cursor = 0;
  if (l->file)
    free((void *)l->file);
  l->file = NULL;
  l->_col = 0;
  l->_row = 0;

  l->token.type = 0;
  l->token.str.count = 0;
  l->token.charlit = 0;
  l->token.floatlit = 0;
  l->token.intlit = 0;
  l->token.col = 0;
  l->token.row = 0;
}

void l_free(lexer_t *l) {
  l_reset(l);

  spx_da_free(l->content);
  l->content.items = NULL;
  l->content.capacity = 0;
  l->content.count = 0;

  spx_da_free(l->token.str);
  l->token.str.items = NULL;
  l->token.str.capacity = 0;
  l->token.str.count = 0;
}

// taken from nob,h
bool spx_read_entire_file(const char *path, SPX_String_Builder *sb) {
  bool result = true;

  FILE *f = fopen(path, "rb");
  size_t new_count = 0;
  long long m = 0;
  if (f == NULL)
    __return_defer(false);
  if (fseek(f, 0, SEEK_END) < 0)
    __return_defer(false);
#ifndef _WIN32
  m = ftell(f);
#else
  m = _ftelli64(f);
#endif
  if (m < 0)
    __return_defer(false);
  if (fseek(f, 0, SEEK_SET) < 0)
    __return_defer(false);

  new_count = sb->count + m;
  if (new_count > sb->capacity) {
    sb->items = __DECLTYPE_CAST(sb->items) SPX_REALLOC(sb->items, new_count);
    SPX_ASSERT(sb->items != NULL && "Buy more RAM lool!!");
    sb->capacity = new_count;
  }

  fread(sb->items + sb->count, m, 1, f);
  if (ferror(f)) {
    // TODO: Afaik, ferror does not set errno. So the error reporting in defer
    // is not correct in this case.
    __return_defer(false);
  }
  sb->count = new_count;

defer:
  if (!result)
    fprintf(stderr, "Could not read file %s: %s", path, strerror(errno));
  if (f)
    fclose(f);
  return result;
}

#endif // SIMPLEX_IMPLEMENTATION