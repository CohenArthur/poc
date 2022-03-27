#ifndef POC_H
#define POC_H

#include <stdbool.h>
#include <stddef.h>

struct poc_slice {
  const char *ptr;
  size_t len;
};

struct poc_slice poc_slice(const char *ptr, size_t len);
struct poc_slice poc_slice_auto(const char *ptr);

struct poc_destructable_ptr {
  void *ptr;
  void (*free_fn)(void *ptr);
};

struct poc_destructable_ptr poc_destructable_ptr(void *ptr,
                                                 void (*free_fn)(void *ptr));
void poc_destruct(struct poc_destructable_ptr *ptr);

struct poc_result {
  enum { OK, ERR } tag;

  union {
    struct {
      struct poc_destructable_ptr parsed;
      struct poc_slice remaining;
    } ok;
    enum error_reason {
      FAILURE,
      INPUT,
      IO_ERROR,
    } error;
  } data;
};

struct poc_tuple {
  struct poc_destructable_ptr fst;
  struct poc_destructable_ptr snd;
};

void poc_tuple_destruct(void *t_void) {
  struct poc_tuple *t = t_void;

  poc_destruct(&t->fst);
  poc_destruct(&t->snd);
}

struct poc_result poc_ok(struct poc_destructable_ptr parsed,
                         struct poc_slice remaining);
struct poc_result poc_err(enum error_reason reason);

bool poc_is_ok(struct poc_result *result);
bool poc_is_err(struct poc_result *result);

struct poc_parser_interface;

typedef struct poc_result (*poc_parse_fn)(
    const struct poc_parser_interface *self_interface,
    const struct poc_slice *input);

struct poc_parser_interface {
  poc_parse_fn parser;
};

struct poc_result poc_parse(struct poc_parser_interface *parser,
                            const struct poc_slice *input);

struct poc_parser_interface *poc_and_then(struct poc_parser_interface *first,
                                          struct poc_parser_interface *second);
struct poc_parser_interface *poc_char(char character);
struct poc_parser_interface *poc_str(struct poc_slice str);

#endif /* !POC_H */
