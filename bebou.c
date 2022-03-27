#include "poc.h"
#include <stdint.h>
#include <stdio.h>

int main(void) {
  struct poc_parser_interface *parser =
      poc_and_then(poc_char('('),
                   poc_and_then(poc_str(poc_slice("bebou", 5)), poc_char(')')));

  struct poc_slice input = poc_slice_auto("(bebou)");
  struct poc_result result = parser->parser(parser, &input);
  if (poc_is_ok(&result)) {
    struct poc_destructable_ptr data = result.data.ok.parsed;
    struct poc_tuple *tuple_f = data.ptr;
    struct poc_tuple *tuple_s = tuple_f->snd.ptr;
    struct poc_slice *parsed = tuple_s->fst.ptr;

    printf("%*s\n", (int)parsed->len, parsed->ptr);
  }
}
