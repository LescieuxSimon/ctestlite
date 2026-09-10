#ifndef MICROBENCH_H
#define MICROBENCH_H

#define MICROBENCH_MIN_TIME 1.0 /* in seconds */

void microbench_initialize();
void microbench_run(const char *name, void (*fn)(void));

void microbench_no_optim(const void *ptr);
void microbench_clobber();

#endif

#ifdef MICROBENCH_IMPLEMENTATION
#if defined(__GNUC__) || defined(__clang__)
#elif defined(_MSC_VER)
#include <intrin.h>

typedef          __int64 llong;
typedef unsigned __int64 ullong;

static const volatile void *volatile microbench_force_escape;
static unsigned int                  microbench_aux;

void microbench_clobber() { _ReadWriteBarrier(); }
void microbench_no_optim(const void *ptr) {
  microbench_force_escape = (const volatile void *)ptr;
  microbench_clobber();
}

#else
#endif

#if defined(_WIN32)
#include <windows.h>

typedef struct {
  LARGE_INTEGER start, stop, freq;
} microbench_calibration_timer;
static void microbench_calibration_start(microbench_calibration_timer *timer) {
  QueryPerformanceCounter(&timer->start);
}
static void microbench_calibration_stop(microbench_calibration_timer *timer) {
  QueryPerformanceCounter(&timer->stop);
}
static double
microbench_calibration_elapsed(microbench_calibration_timer *timer) {
  QueryPerformanceFrequency(&timer->freq);
  return (double)(timer->stop.QuadPart - timer->start.QuadPart) /
         (double)(timer->freq.QuadPart);
}

#else
#endif

#include <stdio.h>

static double microbench_tsc_freq = 0.0;
typedef struct { ullong start, total; } microbench_timer;
static void microbench_timer_start(microbench_timer *timer) {
  _mm_lfence();
  timer->start = __rdtsc();
  _mm_lfence();
}
static void microbench_timer_stop(microbench_timer *timer) {
  const ullong time = __rdtscp(&microbench_aux);
  _mm_lfence();

  timer->total += time - timer->start;
}
static double microbench_elapsed(microbench_timer *timer) {
  if (microbench_tsc_freq == 0.0) { return -1.0; }
  return (double)timer->total / microbench_tsc_freq;
}

void microbench_initialize() {
  microbench_calibration_timer calibration = {0};
  microbench_timer             timer       = {0};

  printf("Calibrating internal clock...\r\n");
  microbench_calibration_start(&calibration);
  microbench_timer_start(&timer);

  Sleep(1000);

  microbench_calibration_stop(&calibration);
  microbench_timer_stop(&timer);

  microbench_tsc_freq = (double)timer.total / microbench_calibration_elapsed(&calibration);
}

void microbench_run(const char *name, void (*fn)(void)) {
  microbench_timer timer = {0};

  ullong iteration_count = 1000;
  ullong iteration_total = 0;
  double time_so_far     = 0.0;

  printf("%-32s", name);
  do {
    microbench_timer_start(&timer);
    for (unsigned __int64 i = 0; i < iteration_count; ++i) {
      microbench_clobber();
      fn();
    }
    microbench_timer_stop(&timer);

    time_so_far      = microbench_elapsed(&timer);
    iteration_total += iteration_count;

    if (time_so_far / MICROBENCH_MIN_TIME < 0.1) {
      iteration_count = 9 * iteration_count;
    } else {
      const double time_per = time_so_far / (double)iteration_total;
      iteration_count       = (ullong)((MICROBENCH_MIN_TIME - time_so_far) / time_per) + 1;
    }
  } while (time_so_far < MICROBENCH_MIN_TIME);

  double      time_per = time_so_far / (double)iteration_total;
  const char *unit     = "s";

  if (time_per < 0.000001) {
    time_per *= 1000000000.0;
    unit      = "ns";
  } else if (time_per < 0.001) {
    time_per *= 1000000.0;
    unit      = "us";
  } else if (time_per < 1.0) {
    time_per *= 1000.0;
    unit      = "ms";
  }
  printf("(%.2fs): %6.2f%2s %llu\r\n", time_so_far, time_per, unit, iteration_total);
}

#endif