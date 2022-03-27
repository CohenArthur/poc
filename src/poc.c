#include "poc.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct poc_slice poc_slice(const char *ptr, size_t len) {
  return (struct poc_slice){.ptr = ptr, .len = len};
}

struct poc_slice poc_slice_auto(const char *ptr) {
  return (struct poc_slice){.ptr = ptr, .len = strlen(ptr)};
}

struct poc_destructable_ptr poc_destructable_ptr(void *ptr,
                                                 void (*free_fn)(void *ptr)) {
  return (struct poc_destructable_ptr){
      .ptr = ptr,
      .free_fn = free_fn,
  };
}

void poc_destruct(struct poc_destructable_ptr *ptr) {
  if (ptr->free_fn)
    ptr->free_fn(ptr->ptr);
}

static const char *slice_at(const struct poc_slice *slice, size_t idx) {
  if (!slice->ptr || idx >= slice->len)
    return NULL;

  return slice->ptr + idx;
}

static const struct poc_slice slice_advance(const struct poc_slice *slice,
                                            size_t amount) {
  struct poc_slice new_slice = {
      .ptr = NULL,
      .len = 0,
  };

  if (!slice->ptr || amount >= slice->len)
    return new_slice;

  new_slice.len -= amount;
  new_slice.ptr = slice->ptr + amount;

  return new_slice;
}

struct poc_char {
  struct poc_parser_interface interface;
  char character;
};

static struct poc_result
poc_char_parser(const struct poc_parser_interface *self_void,
                const struct poc_slice *input) {
  const struct poc_char *self = (const struct poc_char *)self_void;

  const char *first_char = slice_at(input, 0);
  if (!first_char)
    return poc_err(INPUT);

  if (*first_char != self->character)
    return poc_err(FAILURE);

  return poc_ok(poc_destructable_ptr((void *)(size_t)self->character, NULL),
                slice_advance(input, 1));
}

struct poc_parser_interface *poc_char(char character) {
  struct poc_char *parser = malloc(sizeof(*parser));
  if (!parser)
    return NULL;

  parser->character = character;
  parser->interface = (struct poc_parser_interface){poc_char_parser};

  return &parser->interface;
}

struct poc_str {
  struct poc_parser_interface interface;
  struct poc_slice str;
};

static struct poc_result
poc_str_parser(const struct poc_parser_interface *self_void,
               const struct poc_slice *input) {
  const struct poc_str *self = (const struct poc_str *)self_void;

  const char *start = slice_at(input, 0);
  const char *end = slice_at(input, self->str.len);

  if (!start || !end)
    return poc_err(INPUT);

  if (strncmp(input->ptr, self->str.ptr, self->str.len))
    return poc_err(FAILURE);

  return poc_ok(poc_destructable_ptr((void *)&self->str, NULL),
                slice_advance(input, self->str.len));
}

struct poc_parser_interface *poc_str(struct poc_slice str) {
  struct poc_str *parser = malloc(sizeof(*parser));
  if (!parser)
    return NULL;

  parser->str = str;
  parser->interface = (struct poc_parser_interface){poc_str_parser};

  return &parser->interface;
}

struct poc_and_then {
  struct poc_parser_interface interface;
  struct poc_parser_interface *first;
  struct poc_parser_interface *second;
};

static struct poc_result
poc_and_then_parser(const struct poc_parser_interface *self_void,
                    const struct poc_slice *input) {
  const struct poc_and_then *self = (const struct poc_and_then *)self_void;

  struct poc_result first = poc_parse(self->first, input);
  if (poc_is_err(&first))
    return first;

  struct poc_result second = poc_parse(self->second, &first.data.ok.remaining);
  if (poc_is_err(&second))
    return second;

  struct poc_tuple *result = malloc(sizeof(*result));
  result->fst = first.data.ok.parsed;
  result->snd = second.data.ok.parsed;

  return poc_ok(poc_destructable_ptr(result, poc_tuple_destruct),
                second.data.ok.remaining);
}

struct poc_parser_interface *poc_and_then(struct poc_parser_interface *first,
                                          struct poc_parser_interface *second) {
  struct poc_and_then *parser = malloc(sizeof(*parser));

  parser->first = first;
  parser->second = second;
  parser->interface = (struct poc_parser_interface){poc_and_then_parser};

  return &parser->interface;
}

struct poc_result poc_parse(struct poc_parser_interface *parser,
                            const struct poc_slice *input) {
  return parser->parser(parser, input);
}

struct poc_result poc_ok(struct poc_destructable_ptr parsed,
                         struct poc_slice remaining) {
  return (struct poc_result){.tag = OK,
                             .data.ok = {
                                 .parsed = parsed,
                                 .remaining = remaining,
                             }};
}

struct poc_result poc_err(enum error_reason reason) {
  return (struct poc_result){
      .tag = ERR,
      .data.error = reason,
  };
}

bool poc_is_err(struct poc_result *result) { return result->tag == ERR; }
bool poc_is_ok(struct poc_result *result) { return result->tag == OK; }
