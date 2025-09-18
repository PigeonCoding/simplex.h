# simplex
a simple lexer written in C

## example on how to use simplex
```c
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

  // if you need to reset the lexer and parse another file
  // instead of creating a new one
  // l_reset(&l); 

  l_free(&l);
}
```
