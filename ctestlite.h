#ifndef CTESTLITE_H
#define CTESTLITE_H
#include <stdio.h>
#include <stdlib.h>

#define CTESTLITE_CODE_SUCCESS 0
#define CTESTLITE_CODE_FAILURE 1

#define CTESTLITE_CAT_IMPL(a, b) a##b
#define CTESTLITE_CAT(a, b)      CTESTLITE_CAT_IMPL(a, b)
#define CTESTLITE_ANON(x)        CTESTLITE_CAT(x, __COUNTER__)

#define CTESTLITE_TEST_CASE(f, name)                                           \
  static void f(ctestlite_context *);                                          \
  static int CTESTLITE_ANON(CTESTLITE_ANON_VAR_) =                             \
      ctestlite_contexts_add({f, name});                                       \
  static void f(ctestlite_context *ctx)
#define TEST_CASE(name)                                                        \
  CTESTLITE_TEST_CASE(CTESTLITE_ANON(CTESTLITE_ANON_FUNC_), name)

#define CTESTLITE_TEST_TRUE(expr, expr_str, line)                              \
  if ((expr) == 0) {                                                           \
    ctx->test_expr        = expr_str;                                          \
    ctx->test_result_code = CTESTLITE_CODE_FAILURE;                            \
    ctx->test_result_line = line;                                              \
    return;                                                                    \
  }
#define CTESTLITE_TEST_FALSE(expr, expr_str, line)                             \
  if ((expr) != 0) {                                                           \
    ctx->test_expr        = expr_str;                                          \
    ctx->test_result_code = CTESTLITE_CODE_FAILURE;                            \
    ctx->test_result_line = line;                                              \
    return;                                                                    \
  }

#define TEST_TRUE(expr)  CTESTLITE_TEST_TRUE((expr), #expr, __LINE__)
#define TEST_FALSE(expr) CTESTLITE_TEST_FALSE((expr), #expr, __LINE__)

#define TEST_EQ(a, b) TEST_TRUE(a == b)
#define TEST_NE(a, b) TEST_TRUE(a != b)
#define TEST_LE(a, b) TEST_TRUE(a <= b)
#define TEST_QE(a, b) TEST_TRUE(a >= b)
#define TEST_LT(a, b) TEST_TRUE(a < b)
#define TEST_QT(a, b) TEST_TRUE(a > b)

typedef struct ctestlite_context {
  void      (*test_function)(ctestlite_context *);
  const char *test_name;
  const char *test_expr;
  int         test_result_code;
  int         test_result_line;
} ctestlite_context;

struct {
  ctestlite_context *contexts;
  ctestlite_context *capacity;
  ctestlite_context *last;
} ctestlite_contexts = {0};

static unsigned int ctestlite_contexts_size() {
  return (unsigned int)(ctestlite_contexts.last - ctestlite_contexts.contexts);
}
static ctestlite_context *ctestlite_contexts_get(unsigned int n) {
  return &ctestlite_contexts.contexts[n];
}
static unsigned int ctestlite_contexts_add(ctestlite_context ctx) {
  if (ctestlite_contexts.last >= ctestlite_contexts.capacity) {
    const unsigned int old_size = ctestlite_contexts_size();
    const unsigned int new_size = old_size ? old_size * 2 : 16;

    ctestlite_context *new_array = (ctestlite_context *)realloc(
        ctestlite_contexts.contexts, new_size * sizeof(ctestlite_context));
    ctestlite_contexts.contexts = new_array;
    ctestlite_contexts.capacity = new_array + new_size;
    ctestlite_contexts.last = new_array + old_size;
  }
  *(ctestlite_contexts.last++) = ctx;
  return 0;
}

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef LARGE_INTEGER ctestlite_time;
static ctestlite_time ctestlite_timestamp() {
  LARGE_INTEGER value;
  QueryPerformanceCounter(&value);
  return value;
}
static double ctestlite_timesince(ctestlite_time stamp) {
  LARGE_INTEGER freq, time;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&time);

  return (double)(time.QuadPart - stamp.QuadPart) / (double)freq.QuadPart;
}
#endif



int main() {
  int counter_success = 0;
  int counter_failure = 0;

  const unsigned int test_count = ctestlite_contexts_size();
  for (unsigned int i = 0; i < test_count; ++i) {
    ctestlite_context *const ctx = ctestlite_contexts_get(i);

    const ctestlite_time perf_start = ctestlite_timestamp();
    ctx->test_result_code = 0;
    ctx->test_result_line = 0;
    ctx->test_function(ctx);
    const double perf_time = ctestlite_timesince(perf_start);

    switch (ctx->test_result_code) {
    case CTESTLITE_CODE_SUCCESS:
      counter_success += 1;
      printf("[SUCCESS] Test: %s (%06.3fms)\n", ctx->test_name,
             1000.0 * perf_time);
      break;
    case CTESTLITE_CODE_FAILURE:
      counter_failure += 1;
      printf("[FAILURE] Test: %s (%06.3fms)\n", ctx->test_name,
             1000.0 * perf_time);
      printf("\tassertion \'%s\' failed on line %d\n", ctx->test_expr,
             ctx->test_result_line);
      break;
    }
  }
  return counter_failure;
}

#endif
