#ifndef CTESTLITE_H
#define CTESTLITE_H
#include <stdio.h>
#include <stdlib.h>

#if defined(_MSC_VER)
__pragma(warning(disable : 5045)) /**/
__pragma(warning(disable : 5247)) /**/
__pragma(warning(disable : 5248)) /**/

#pragma section(".CRT$XCU", read)
#define CTESTLITE_REGISTER     __declspec(allocate(".CRT$XCU"))
#define CTESTLITE_REFERENCE(x) __pragma(comment(linker, "/include:" CTESTLITE_TOSTR(x)))

#else
#error "Compiler not not supported!"
#endif

#define CTESTLITE_CODE_SUCCESS 0
#define CTESTLITE_CODE_FAILURE 1

#define CTESTLITE_TOSTR_IMPL(x)  #x
#define CTESTLITE_TOSTR(x)       CTESTLITE_TOSTR_IMPL(x)
#define CTESTLITE_CAT_IMPL(a, b) a##b
#define CTESTLITE_CAT(a, b)      CTESTLITE_CAT_IMPL(a, b)
#define CTESTLITE_ANON(x)        CTESTLITE_CAT(x, __LINE__)

#define CTESTLITE_TEST_CASE(f, name)                                           \
  static void f(ctestlite_context *);                                          \
  static void CTESTLITE_CAT(f, _REG)(void) {                                   \
    ctestlite_context ctx = {0};                                               \
    ctx.test_function     = (f);                                               \
    ctx.test_name         = (name);                                            \
    ctestlite_contexts_add(ctx);                                               \
  }                                                                            \
  extern "C" {                                                                 \
  CTESTLITE_REGISTER                                                           \
  CTESTLITE_REFERENCE(CTESTLITE_CAT(f, _PTR))                                  \
  void (*CTESTLITE_CAT(f, _PTR))(void) = CTESTLITE_CAT(f, _REG);               \
  }                                                                            \
  static void f(ctestlite_context *ctx)
#define TEST_CASE(name)                                                        \
  CTESTLITE_TEST_CASE(CTESTLITE_ANON(CTESTLITE_TEST_), name)

#define CTESTLITE_TEST(expr, str, line)                                        \
  if (!(expr)) {                                                               \
    ctx->test_expr        = (str);                                             \
    ctx->test_result_code = CTESTLITE_CODE_FAILURE;                            \
    ctx->test_result_line = (line);                                            \
    return;                                                                    \
  }
#define TEST(expr)                                                             \
  do {                                                                         \
    CTESTLITE_TEST(expr, CTESTLITE_TOSTR(expr), __LINE__)                      \
  } while (0);

typedef struct ctestlite_context {
  void      (*test_function)(ctestlite_context *);
  const char *test_name;
  const char *test_expr;
  int         test_result_code;
  int         test_result_line;
} ctestlite_context;

static struct {
  ctestlite_context *contexts;
  ctestlite_context *capacity;
  ctestlite_context *last;
} ctestlite_contexts = {0};

static unsigned int ctestlite_contexts_size() {
  return (unsigned int)(ctestlite_contexts.last - ctestlite_contexts.contexts);
}
static void ctestlite_contexts_add(ctestlite_context ctx) {
  if (ctestlite_contexts.last >= ctestlite_contexts.capacity) {
    const unsigned int old_size = ctestlite_contexts_size();
    const unsigned int new_size = old_size ? old_size * 2 : 16;

    ctestlite_context *new_array = (ctestlite_context *)realloc(
        ctestlite_contexts.contexts, new_size * sizeof(ctestlite_context));
    if (new_array == NULL) {
      printf("[ ERROR ] Failed to register test \'%s\'\n", ctx.test_name);
      return;
    }

    ctestlite_contexts.contexts = new_array;
    ctestlite_contexts.capacity = new_array + new_size;
    ctestlite_contexts.last     = new_array + old_size;
  }
  *(ctestlite_contexts.last++) = ctx;
  return;
}

int main() {
  int counter_failure = 0;

  const unsigned int test_count = ctestlite_contexts_size();
  for (unsigned int i = 0; i < test_count; ++i) {
    ctestlite_context *const ctx = &ctestlite_contexts.contexts[i];

    ctx->test_result_code = 0;
    ctx->test_result_line = 0;
    ctx->test_function(ctx);

    switch (ctx->test_result_code) {
    case CTESTLITE_CODE_SUCCESS:
      printf("[SUCCESS] Test: %s\n", ctx->test_name);
      break;
    case CTESTLITE_CODE_FAILURE:
      counter_failure += 1;
      printf("[FAILURE] Test: %s\n", ctx->test_name);
      printf("\tassertion \'%s\' failed on line %d\n", ctx->test_expr,
             ctx->test_result_line);
      break;
    }
  }

  free(ctestlite_contexts.contexts);
  return counter_failure ? EXIT_FAILURE : EXIT_SUCCESS;
}

#endif
