/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *         Unit tests for the dbg-io library.
 *
 *         Part 1: Regression tests for specific bugs that were fixed.
 *         Part 2: General unit test suite for the formatting engine.
 *         Part 3: Tests for printf, sprintf, puts, and putchar via
 *                 stub dbg_send_bytes/dbg_putchar backend.
 */

#include "contiki.h"
#include "unit-test.h"
#include "lib/dbg-io/dbg.h"
#include <strformat.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
/*---------------------------------------------------------------------------*/
/*
 * Stub dbg backend. When capture is enabled, output goes to dbg_buf.
 * When capture is disabled, output goes to stdout via fwrite so that
 * the unit-test framework's printf output remains visible.
 */
/*---------------------------------------------------------------------------*/
#define DBG_BUF_SIZE 512
static char dbg_buf[DBG_BUF_SIZE];
static unsigned int dbg_buf_pos;
static int dbg_capture_enabled;
/*---------------------------------------------------------------------------*/
unsigned int
dbg_send_bytes(const unsigned char *seq, unsigned int len)
{
  if(dbg_capture_enabled) {
    if(dbg_buf_pos + len < DBG_BUF_SIZE) {
      memcpy(dbg_buf + dbg_buf_pos, seq, len);
      dbg_buf_pos += len;
    }
  } else {
    fwrite(seq, 1, len, stdout);
  }
  return len;
}
/*---------------------------------------------------------------------------*/
int
dbg_putchar(int c)
{
  unsigned char ch = c;
  dbg_send_bytes(&ch, 1);
  return c;
}
/*---------------------------------------------------------------------------*/
static void
dbg_capture_reset(void)
{
  dbg_buf_pos = 0;
  memset(dbg_buf, 0, DBG_BUF_SIZE);
}
/*---------------------------------------------------------------------------*/
static const char *
dbg_capture_get(void)
{
  dbg_buf[dbg_buf_pos] = '\0';
  return dbg_buf;
}
/*---------------------------------------------------------------------------*/
/* Test output buffer and write callback for format_str */
#define TEST_BUF_SIZE 512
static char test_buf[TEST_BUF_SIZE];
static unsigned int test_buf_pos;
/*---------------------------------------------------------------------------*/
static strformat_result
test_write_str(void *user_data, const char *data, unsigned int len)
{
  if(test_buf_pos + len < TEST_BUF_SIZE) {
    memcpy(test_buf + test_buf_pos, data, len);
    test_buf_pos += len;
  }
  return STRFORMAT_OK;
}
/*---------------------------------------------------------------------------*/
static const strformat_context_t test_ctxt = {
  test_write_str,
  NULL
};
/*---------------------------------------------------------------------------*/
static void
reset_buf(void)
{
  test_buf_pos = 0;
  memset(test_buf, 0, TEST_BUF_SIZE);
}
/*---------------------------------------------------------------------------*/
static int
test_format(const char *expected, const char *fmt, ...)
{
  int ret;
  va_list ap;

  reset_buf();
  va_start(ap, fmt);
  ret = format_str_v(&test_ctxt, fmt, ap);
  va_end(ap);
  test_buf[test_buf_pos] = '\0';

  if(strcmp(test_buf, expected) != 0) {
    printf("  MISMATCH: expected \"%s\", got \"%s\"\n", expected, test_buf);
    return 0;
  }
  if(ret != (int)strlen(expected)) {
    printf("  LENGTH: expected %d, got %d for \"%s\"\n",
           (int)strlen(expected), ret, expected);
    return 0;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS(run_tests, "dbg-io unit tests");
AUTOSTART_PROCESSES(&run_tests);
/*---------------------------------------------------------------------------*/
/*
 * Part 1: Regression tests for fixed bugs
 */
/*---------------------------------------------------------------------------*/

/*
 * Bug fix: snprintf buffer overflow when size == 0.
 */
UNIT_TEST_REGISTER(test_snprintf_size_zero,
                   "snprintf with size 0 must not crash or write");
UNIT_TEST(test_snprintf_size_zero)
{
  char buf[16];
  int ret;

  UNIT_TEST_BEGIN();

  memset(buf, 0xAA, sizeof(buf));

  ret = snprintf(buf, 0, "%d", 12345);
  UNIT_TEST_ASSERT(ret == 5);
  UNIT_TEST_ASSERT((unsigned char)buf[0] == 0xAA);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Bug fix: snprintf with size == 1 should write only a NUL terminator.
 */
UNIT_TEST_REGISTER(test_snprintf_size_one,
                   "snprintf with size 1 writes only NUL");
UNIT_TEST(test_snprintf_size_one)
{
  char buf[16];
  int ret;

  UNIT_TEST_BEGIN();

  memset(buf, 0xAA, sizeof(buf));

  ret = snprintf(buf, 1, "%d", 12345);
  UNIT_TEST_ASSERT(ret == 5);
  UNIT_TEST_ASSERT(buf[0] == '\0');
  UNIT_TEST_ASSERT((unsigned char)buf[1] == 0xAA);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Bug fix: snprintf truncation must NUL-terminate correctly.
 */
UNIT_TEST_REGISTER(test_snprintf_truncation,
                   "snprintf truncation NUL-terminates correctly");
UNIT_TEST(test_snprintf_truncation)
{
  char buf[8];
  int ret;

  UNIT_TEST_BEGIN();

  memset(buf, 0xAA, sizeof(buf));

  ret = snprintf(buf, 4, "hello");
  UNIT_TEST_ASSERT(ret == 5);
  UNIT_TEST_ASSERT(buf[0] == 'h');
  UNIT_TEST_ASSERT(buf[1] == 'e');
  UNIT_TEST_ASSERT(buf[2] == 'l');
  UNIT_TEST_ASSERT(buf[3] == '\0');
  UNIT_TEST_ASSERT((unsigned char)buf[4] == 0xAA);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Bug fix: negative '*' width used to wrap to ~4 billion.
 */
UNIT_TEST_REGISTER(test_negative_star_width,
                   "Negative * width means left-justify with |w|");
UNIT_TEST(test_negative_star_width)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("42   ", "%*d", -5, 42));
  UNIT_TEST_ASSERT(test_format("hello     ", "%*s", -10, "hello"));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Bug fix: CONV_CHAR double-counted field_fill for right-justified %c.
 */
UNIT_TEST_REGISTER(test_char_field_fill,
                   "%%c return value correct with width");
UNIT_TEST(test_char_field_fill)
{
  int ret;

  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("    x", "%5c", 'x'));
  UNIT_TEST_ASSERT(test_format("x    ", "%-5c", 'x'));

  reset_buf();
  ret = format_str(&test_ctxt, "%c", 'A');
  test_buf[test_buf_pos] = '\0';
  UNIT_TEST_ASSERT(ret == 1);
  UNIT_TEST_ASSERT(test_buf[0] == 'A');

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Bug fix: %n format specifier disabled.
 */
UNIT_TEST_REGISTER(test_percent_n_disabled,
                   "%%n is disabled and does not write");
UNIT_TEST(test_percent_n_disabled)
{
  int written = -1;

  UNIT_TEST_BEGIN();

  {
    char fmt[] = { 'a', 'b', 'c', '%', 'n', 'd', 'e', 'f', '\0' };
    reset_buf();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
    format_str(&test_ctxt, fmt);
#pragma GCC diagnostic pop
  }
  test_buf[test_buf_pos] = '\0';

  UNIT_TEST_ASSERT(strcmp(test_buf, "abcdef") == 0);
  UNIT_TEST_ASSERT(written == -1);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Bug fix: %n is disabled, but it must still consume its pointer argument
 * so that subsequent arguments stay aligned in the va_list.
 */
UNIT_TEST_REGISTER(test_percent_n_va_list_consumed,
                   "%%n consumes pointer arg without corrupting va_list");
UNIT_TEST(test_percent_n_va_list_consumed)
{
  int sentinel = 12345;

  UNIT_TEST_BEGIN();

  /* %n writes nothing, but must consume the int* so the trailing %d reads
     42 rather than misinterpreting the pointer as the integer argument. */
  UNIT_TEST_ASSERT(test_format("42", "%n%d", &sentinel, 42));
  /* The pointer target must remain untouched (no write-through). */
  UNIT_TEST_ASSERT(sentinel == 12345);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Bug fix: negative dynamic precision via * must be treated as omitted.
 */
UNIT_TEST_REGISTER(test_negative_star_precision,
                   "Negative * precision treated as omitted (C99)");
UNIT_TEST(test_negative_star_precision)
{
  UNIT_TEST_BEGIN();

  /* Negative precision => treat as no precision; default integer precision 1 */
  UNIT_TEST_ASSERT(test_format("42", "%.*d", -5, 42));
  /* Negative precision => string not truncated */
  UNIT_TEST_ASSERT(test_format("hello", "%.*s", -3, "hello"));
  /* Zero precision with zero value => empty output */
  UNIT_TEST_ASSERT(test_format("", "%.*d", 0, 0));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Bug fix: float format specifiers must consume the double argument
 * even though formatting is not implemented, to keep va_list aligned.
 */
UNIT_TEST_REGISTER(test_float_va_list_consumed,
                   "Float specifiers consume argument without corrupting va_list");
UNIT_TEST(test_float_va_list_consumed)
{
  UNIT_TEST_BEGIN();

  /* %f is not rendered, but the double must be consumed so that %d reads 42 */
  UNIT_TEST_ASSERT(test_format("42", "%f%d", 3.14, 42));
  UNIT_TEST_ASSERT(test_format("hello", "%e%s", 2.718, "hello"));
  /* Multiple floats followed by an integer */
  UNIT_TEST_ASSERT(test_format("99", "%g%f%d", 1.0, 2.0, 99));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Bug fix: %p must output "0x..." not "#0x...".
 */
UNIT_TEST_REGISTER(test_pointer_format,
                   "%%p outputs 0x prefix without spurious #");
UNIT_TEST(test_pointer_format)
{
  char buf[32];
  int ret;

  UNIT_TEST_BEGIN();

  ret = snprintf(buf, sizeof(buf), "%p", (void *)0);
  /* NULL pointer should be "0x0" */
  UNIT_TEST_ASSERT(strcmp(buf, "0x0") == 0);
  UNIT_TEST_ASSERT(ret == 3);

  ret = snprintf(buf, sizeof(buf), "%p", (void *)0xff);
  UNIT_TEST_ASSERT(strcmp(buf, "0xff") == 0);
  UNIT_TEST_ASSERT(ret == 4);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/*
 * Part 2: General unit test suite for the formatting engine
 */
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_integer_formats,
                   "Basic integer formatting");
UNIT_TEST(test_integer_formats)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("0", "%d", 0));
  UNIT_TEST_ASSERT(test_format("42", "%d", 42));
  UNIT_TEST_ASSERT(test_format("-42", "%d", -42));
  UNIT_TEST_ASSERT(test_format("2147483647", "%d", 2147483647));
  UNIT_TEST_ASSERT(test_format("-2147483648", "%d", (int)-2147483648LL));
  UNIT_TEST_ASSERT(test_format("42", "%i", 42));
  UNIT_TEST_ASSERT(test_format("0", "%u", 0));
  UNIT_TEST_ASSERT(test_format("4294967295", "%u", 4294967295U));
  UNIT_TEST_ASSERT(test_format("0", "%o", 0));
  UNIT_TEST_ASSERT(test_format("52", "%o", 42));
  UNIT_TEST_ASSERT(test_format("377", "%o", 255));
  UNIT_TEST_ASSERT(test_format("0", "%x", 0));
  UNIT_TEST_ASSERT(test_format("2a", "%x", 42));
  UNIT_TEST_ASSERT(test_format("ff", "%x", 255));
  UNIT_TEST_ASSERT(test_format("deadbeef", "%x", 0xDEADBEEF));
  UNIT_TEST_ASSERT(test_format("DEADBEEF", "%X", 0xDEADBEEF));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_integer_width_padding,
                   "Integer width and padding");
UNIT_TEST(test_integer_width_padding)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("   42", "%5d", 42));
  UNIT_TEST_ASSERT(test_format("42   ", "%-5d", 42));
  UNIT_TEST_ASSERT(test_format("00042", "%05d", 42));
  UNIT_TEST_ASSERT(test_format("-0042", "%05d", -42));
  UNIT_TEST_ASSERT(test_format("12345", "%3d", 12345));
  UNIT_TEST_ASSERT(test_format("  ff", "%4x", 255));
  UNIT_TEST_ASSERT(test_format("00ff", "%04x", 255));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_integer_sign_flags,
                   "Integer sign flags (+ and space)");
UNIT_TEST(test_integer_sign_flags)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("+42", "%+d", 42));
  UNIT_TEST_ASSERT(test_format("-42", "%+d", -42));
  UNIT_TEST_ASSERT(test_format("+0", "%+d", 0));
  UNIT_TEST_ASSERT(test_format(" 42", "% d", 42));
  UNIT_TEST_ASSERT(test_format("-42", "% d", -42));
  UNIT_TEST_ASSERT(test_format("+42", "%+ d", 42));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_alternate_form,
                   "Alternate form flag (#)");
UNIT_TEST(test_alternate_form)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("0x2a", "%#x", 42));
  UNIT_TEST_ASSERT(test_format("0XFF", "%#X", 255));
  UNIT_TEST_ASSERT(test_format("0", "%#x", 0));
  UNIT_TEST_ASSERT(test_format("052", "%#o", 42));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_integer_precision,
                   "Integer precision");
UNIT_TEST(test_integer_precision)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("00042", "%.5d", 42));
  UNIT_TEST_ASSERT(test_format("  00042", "%7.5d", 42));
  UNIT_TEST_ASSERT(test_format("", "%.0d", 0));
  UNIT_TEST_ASSERT(test_format("12345", "%.3d", 12345));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_size_modifiers,
                   "Size modifiers (hh, h, l, ll, z)");
UNIT_TEST(test_size_modifiers)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("-1", "%hhd", (int)(signed char)-1));
  UNIT_TEST_ASSERT(test_format("-1", "%hd", (int)(short)-1));
  UNIT_TEST_ASSERT(test_format("42", "%ld", 42L));
  UNIT_TEST_ASSERT(test_format("42", "%lld", 42LL));
  UNIT_TEST_ASSERT(test_format("42", "%zu", (size_t)42));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_string_format,
                   "String formatting (%%s)");
UNIT_TEST(test_string_format)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("hello", "%s", "hello"));
  UNIT_TEST_ASSERT(test_format("(null)", "%s", (char *)NULL));
  UNIT_TEST_ASSERT(test_format("", "%s", ""));
  UNIT_TEST_ASSERT(test_format("     hello", "%10s", "hello"));
  UNIT_TEST_ASSERT(test_format("hello     ", "%-10s", "hello"));
  UNIT_TEST_ASSERT(test_format("hel", "%.3s", "hello"));
  UNIT_TEST_ASSERT(test_format("   hel", "%6.3s", "hello"));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_char_format,
                   "Character formatting (%%c)");
UNIT_TEST(test_char_format)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("A", "%c", 'A'));

  reset_buf();
  format_str(&test_ctxt, "%c", '\0');
  UNIT_TEST_ASSERT(test_buf_pos == 1);
  UNIT_TEST_ASSERT(test_buf[0] == '\0');

  UNIT_TEST_ASSERT(test_format(" ", "%c", ' '));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_percent_literal,
                   "Percent literal (%%)");
UNIT_TEST(test_percent_literal)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("%", "%%"));
  UNIT_TEST_ASSERT(test_format("100%", "100%%"));
  UNIT_TEST_ASSERT(test_format("%%", "%%%%"));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_dynamic_width,
                   "Dynamic width with *");
UNIT_TEST(test_dynamic_width)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("   42", "%*d", 5, 42));
  UNIT_TEST_ASSERT(test_format("42", "%*d", 0, 42));
  UNIT_TEST_ASSERT(test_format("42   ", "%*d", -5, 42));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_dynamic_precision,
                   "Dynamic precision with *");
UNIT_TEST(test_dynamic_precision)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("00042", "%.*d", 5, 42));
  UNIT_TEST_ASSERT(test_format("hel", "%.*s", 3, "hello"));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_snprintf_basic,
                   "snprintf with adequate buffer");
UNIT_TEST(test_snprintf_basic)
{
  char buf[64];
  int ret;

  UNIT_TEST_BEGIN();

  ret = snprintf(buf, sizeof(buf), "hello %d", 42);
  UNIT_TEST_ASSERT(ret == 8);
  UNIT_TEST_ASSERT(strcmp(buf, "hello 42") == 0);

  ret = snprintf(buf, sizeof(buf), "%s=%d", "key", 123);
  UNIT_TEST_ASSERT(ret == 7);
  UNIT_TEST_ASSERT(strcmp(buf, "key=123") == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_mixed_formats,
                   "Mixed format specifiers");
UNIT_TEST(test_mixed_formats)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("A 42 hello", "%c %d %s", 'A', 42, "hello"));
  UNIT_TEST_ASSERT(test_format("ff/255/377", "%x/%u/%o", 255, 255, 255));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

UNIT_TEST_REGISTER(test_edge_cases,
                   "Format string edge cases");
UNIT_TEST(test_edge_cases)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(test_format("", ""));
  UNIT_TEST_ASSERT(test_format("hello world", "hello world"));

  {
    char trailing_pct[] = { 'a', 'b', 'c', '%', '\0' };
    reset_buf();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
    format_str(&test_ctxt, trailing_pct);
#pragma GCC diagnostic pop
    test_buf[test_buf_pos] = '\0';
    UNIT_TEST_ASSERT(strcmp(test_buf, "abc") == 0);
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/*
 * Part 3: Tests for printf, sprintf, puts, and putchar
 *         via the stub dbg_send_bytes/dbg_putchar backend.
 */
/*---------------------------------------------------------------------------*/

/*
 * Test printf: output goes through dbg_send_bytes.
 */
UNIT_TEST_REGISTER(test_printf_via_dbg,
                   "printf output via dbg backend");
UNIT_TEST(test_printf_via_dbg)
{
  int ret;

  UNIT_TEST_BEGIN();

  /* Enable capture so printf output goes to dbg_buf. */
  dbg_capture_reset();
  dbg_capture_enabled = 1;

  ret = printf("hello %d", 42);

  dbg_capture_enabled = 0;

  UNIT_TEST_ASSERT(ret == 8);
  UNIT_TEST_ASSERT(strcmp(dbg_capture_get(), "hello 42") == 0);

  /* Test printf with multiple format specifiers. */
  dbg_capture_reset();
  dbg_capture_enabled = 1;

  ret = printf("%s=%d (0x%x)", "val", 255, 255);

  dbg_capture_enabled = 0;

  UNIT_TEST_ASSERT(ret == 14);
  UNIT_TEST_ASSERT(strcmp(dbg_capture_get(), "val=255 (0xff)") == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Test sprintf: output written to caller buffer with no bounds.
 */
UNIT_TEST_REGISTER(test_sprintf_basic,
                   "sprintf writes to caller buffer");
UNIT_TEST(test_sprintf_basic)
{
  char buf[64];
  int ret;

  UNIT_TEST_BEGIN();

  ret = sprintf(buf, "hello %d", 42);
  UNIT_TEST_ASSERT(ret == 8);
  UNIT_TEST_ASSERT(strcmp(buf, "hello 42") == 0);

  ret = sprintf(buf, "%s=%d", "key", 123);
  UNIT_TEST_ASSERT(ret == 7);
  UNIT_TEST_ASSERT(strcmp(buf, "key=123") == 0);

  /* Empty format string. */
  ret = sprintf(buf, "");
  UNIT_TEST_ASSERT(ret == 0);
  UNIT_TEST_ASSERT(buf[0] == '\0');

  /* Just a string. */
  ret = sprintf(buf, "abc");
  UNIT_TEST_ASSERT(ret == 3);
  UNIT_TEST_ASSERT(strcmp(buf, "abc") == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Test puts: outputs string followed by newline via dbg backend.
 */
UNIT_TEST_REGISTER(test_puts_via_dbg,
                   "puts output via dbg backend");
UNIT_TEST(test_puts_via_dbg)
{
  UNIT_TEST_BEGIN();

  dbg_capture_reset();
  dbg_capture_enabled = 1;

  puts("hello world");

  dbg_capture_enabled = 0;

  UNIT_TEST_ASSERT(strcmp(dbg_capture_get(), "hello world\n") == 0);
  UNIT_TEST_ASSERT(dbg_buf_pos == 12);

  /* Empty string: should produce just a newline. */
  dbg_capture_reset();
  dbg_capture_enabled = 1;

  puts("");

  dbg_capture_enabled = 0;

  UNIT_TEST_ASSERT(strcmp(dbg_capture_get(), "\n") == 0);
  UNIT_TEST_ASSERT(dbg_buf_pos == 1);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Test putchar: outputs a single character via dbg backend.
 */
UNIT_TEST_REGISTER(test_putchar_via_dbg,
                   "putchar output via dbg backend");
UNIT_TEST(test_putchar_via_dbg)
{
  int ret;

  UNIT_TEST_BEGIN();

  dbg_capture_reset();
  dbg_capture_enabled = 1;

  ret = putchar('A');

  dbg_capture_enabled = 0;

  UNIT_TEST_ASSERT(ret == 'A');
  UNIT_TEST_ASSERT(dbg_buf_pos == 1);
  UNIT_TEST_ASSERT(dbg_buf[0] == 'A');

  /* Test newline character. */
  dbg_capture_reset();
  dbg_capture_enabled = 1;

  ret = putchar('\n');

  dbg_capture_enabled = 0;

  UNIT_TEST_ASSERT(ret == '\n');
  UNIT_TEST_ASSERT(dbg_buf_pos == 1);
  UNIT_TEST_ASSERT(dbg_buf[0] == '\n');

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Test printf with successive calls accumulate in the dbg backend.
 */
UNIT_TEST_REGISTER(test_printf_multiple_calls,
                   "printf multiple calls accumulate");
UNIT_TEST(test_printf_multiple_calls)
{
  UNIT_TEST_BEGIN();

  dbg_capture_reset();
  dbg_capture_enabled = 1;

  printf("aaa");
  printf("bbb");
  printf("ccc");

  dbg_capture_enabled = 0;

  UNIT_TEST_ASSERT(strcmp(dbg_capture_get(), "aaabbbccc") == 0);
  UNIT_TEST_ASSERT(dbg_buf_pos == 9);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Test snprintf with various truncation boundaries.
 */
UNIT_TEST_REGISTER(test_snprintf_boundaries,
                   "snprintf truncation at various boundaries");
UNIT_TEST(test_snprintf_boundaries)
{
  char buf[32];
  int ret;

  UNIT_TEST_BEGIN();

  /* Exact fit: "hello" (5 chars) + NUL = 6 bytes needed. */
  memset(buf, 0xAA, sizeof(buf));
  ret = snprintf(buf, 6, "hello");
  UNIT_TEST_ASSERT(ret == 5);
  UNIT_TEST_ASSERT(strcmp(buf, "hello") == 0);
  UNIT_TEST_ASSERT((unsigned char)buf[6] == 0xAA);

  /* One byte short: only "hell" fits. */
  memset(buf, 0xAA, sizeof(buf));
  ret = snprintf(buf, 5, "hello");
  UNIT_TEST_ASSERT(ret == 5);
  UNIT_TEST_ASSERT(strcmp(buf, "hell") == 0);
  UNIT_TEST_ASSERT((unsigned char)buf[5] == 0xAA);

  /* Truncation with format specifiers. */
  memset(buf, 0xAA, sizeof(buf));
  ret = snprintf(buf, 6, "%d-%d", 123, 456);
  /* "123-456" = 7 chars, truncated to "123-4\0" */
  UNIT_TEST_ASSERT(ret == 7);
  UNIT_TEST_ASSERT(strcmp(buf, "123-4") == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Test sprintf with format specifiers that exercise all conversions.
 */
UNIT_TEST_REGISTER(test_sprintf_all_conversions,
                   "sprintf exercises all conversion types");
UNIT_TEST(test_sprintf_all_conversions)
{
  char buf[128];

  UNIT_TEST_BEGIN();

  /* Integer conversions */
  sprintf(buf, "%d %u %o %x %X", -42, 42U, 42, 42, 42);
  UNIT_TEST_ASSERT(strcmp(buf, "-42 42 52 2a 2A") == 0);

  /* String and char */
  sprintf(buf, "[%s] [%c]", "test", 'Z');
  UNIT_TEST_ASSERT(strcmp(buf, "[test] [Z]") == 0);

  /* Percent */
  sprintf(buf, "100%%");
  UNIT_TEST_ASSERT(strcmp(buf, "100%") == 0);

  /* Width and padding */
  sprintf(buf, "[%8d] [%-8d] [%08d]", 42, 42, 42);
  UNIT_TEST_ASSERT(strcmp(buf, "[      42] [42      ] [00000042]") == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(run_tests, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("\nRunning dbg-io unit tests\n");

  /* Part 1: Bug regression tests */
  printf("\n--- Bug regression tests ---\n");
  UNIT_TEST_RUN(test_snprintf_size_zero);
  UNIT_TEST_RUN(test_snprintf_size_one);
  UNIT_TEST_RUN(test_snprintf_truncation);
  UNIT_TEST_RUN(test_negative_star_width);
  UNIT_TEST_RUN(test_char_field_fill);
  UNIT_TEST_RUN(test_percent_n_disabled);
  UNIT_TEST_RUN(test_percent_n_va_list_consumed);
  UNIT_TEST_RUN(test_negative_star_precision);
  UNIT_TEST_RUN(test_float_va_list_consumed);
  UNIT_TEST_RUN(test_pointer_format);

  /* Part 2: General formatting tests */
  printf("\n--- General formatting tests ---\n");
  UNIT_TEST_RUN(test_integer_formats);
  UNIT_TEST_RUN(test_integer_width_padding);
  UNIT_TEST_RUN(test_integer_sign_flags);
  UNIT_TEST_RUN(test_alternate_form);
  UNIT_TEST_RUN(test_integer_precision);
  UNIT_TEST_RUN(test_size_modifiers);
  UNIT_TEST_RUN(test_string_format);
  UNIT_TEST_RUN(test_char_format);
  UNIT_TEST_RUN(test_percent_literal);
  UNIT_TEST_RUN(test_dynamic_width);
  UNIT_TEST_RUN(test_dynamic_precision);
  UNIT_TEST_RUN(test_snprintf_basic);
  UNIT_TEST_RUN(test_mixed_formats);
  UNIT_TEST_RUN(test_edge_cases);

  /* Part 3: printf, sprintf, puts, putchar via dbg backend */
  printf("\n--- Backend integration tests ---\n");
  UNIT_TEST_RUN(test_printf_via_dbg);
  UNIT_TEST_RUN(test_sprintf_basic);
  UNIT_TEST_RUN(test_puts_via_dbg);
  UNIT_TEST_RUN(test_putchar_via_dbg);
  UNIT_TEST_RUN(test_printf_multiple_calls);
  UNIT_TEST_RUN(test_snprintf_boundaries);
  UNIT_TEST_RUN(test_sprintf_all_conversions);

  if(!UNIT_TEST_PASSED(test_snprintf_size_zero) ||
     !UNIT_TEST_PASSED(test_snprintf_size_one) ||
     !UNIT_TEST_PASSED(test_snprintf_truncation) ||
     !UNIT_TEST_PASSED(test_negative_star_width) ||
     !UNIT_TEST_PASSED(test_char_field_fill) ||
     !UNIT_TEST_PASSED(test_percent_n_disabled) ||
     !UNIT_TEST_PASSED(test_negative_star_precision) ||
     !UNIT_TEST_PASSED(test_float_va_list_consumed) ||
     !UNIT_TEST_PASSED(test_pointer_format) ||
     !UNIT_TEST_PASSED(test_integer_formats) ||
     !UNIT_TEST_PASSED(test_integer_width_padding) ||
     !UNIT_TEST_PASSED(test_integer_sign_flags) ||
     !UNIT_TEST_PASSED(test_alternate_form) ||
     !UNIT_TEST_PASSED(test_integer_precision) ||
     !UNIT_TEST_PASSED(test_size_modifiers) ||
     !UNIT_TEST_PASSED(test_string_format) ||
     !UNIT_TEST_PASSED(test_char_format) ||
     !UNIT_TEST_PASSED(test_percent_literal) ||
     !UNIT_TEST_PASSED(test_dynamic_width) ||
     !UNIT_TEST_PASSED(test_dynamic_precision) ||
     !UNIT_TEST_PASSED(test_snprintf_basic) ||
     !UNIT_TEST_PASSED(test_mixed_formats) ||
     !UNIT_TEST_PASSED(test_edge_cases) ||
     !UNIT_TEST_PASSED(test_printf_via_dbg) ||
     !UNIT_TEST_PASSED(test_sprintf_basic) ||
     !UNIT_TEST_PASSED(test_puts_via_dbg) ||
     !UNIT_TEST_PASSED(test_putchar_via_dbg) ||
     !UNIT_TEST_PASSED(test_printf_multiple_calls) ||
     !UNIT_TEST_PASSED(test_snprintf_boundaries) ||
     !UNIT_TEST_PASSED(test_sprintf_all_conversions)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
