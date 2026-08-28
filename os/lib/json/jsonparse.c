/*
 * Copyright (c) 2011-2012, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

#include "jsonparse.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*--------------------------------------------------------------------*/
static bool
push(struct jsonparse_state *state, char c)
{
  if(state->depth < JSONPARSE_MAX_DEPTH) {
    state->stack[state->depth] = c;
    state->depth++;
    state->vtype = 0;
    return true;
  } else {
    return false;
  }
}
/*--------------------------------------------------------------------*/
static bool
modify(struct jsonparse_state *state, char c)
{
  if(state->depth > 0) {
    state->stack[state->depth - 1] = c;
    return true;
  } else {
    return false;
  }
}
/*--------------------------------------------------------------------*/
static bool
pop(struct jsonparse_state *state)
{
  if(state->depth == 0) {
    return false;
  }
  state->depth--;
  state->vtype = state->stack[state->depth];
  return true;
}
/*--------------------------------------------------------------------*/
static bool
is_whitespace(char c)
{
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}
/*--------------------------------------------------------------------*/
/* will pass by the value and store the start and length of the value for
   atomic types */
/*--------------------------------------------------------------------*/
static char
atomic(struct jsonparse_state *state, char type)
{
  char c;
  const char *str;
  int len;

  state->vstart = state->pos;
  /* Cleared here so that an error return below cannot leave the length of
     the previous value behind. */
  state->vlen = 0;
  if(type == JSON_TYPE_STRING || type == JSON_TYPE_PAIR_NAME) {
    c = 0;
    while(state->pos < state->len) {
      c = state->json[state->pos++];
      if(c == '\0' || c == '"') {
        break;
      }
      if(c == '\\') {
        if(state->pos == state->len) {
          /* Nothing left for the escape to consume: the string is
             unterminated. */
          c = 0;
          break;
        }
        state->pos++;           /* skip the escaped char */
      }
    }
    if(c != '"') {
      state->error = JSON_ERROR_SYNTAX;
      return JSON_TYPE_ERROR;
    }
    state->vlen = state->pos - state->vstart - 1;
  } else if(type == JSON_TYPE_NUMBER) {
    while(state->pos < state->len) {
      c = state->json[state->pos];
      if((c < '0' || c > '9') && c != '.') {
        break;
      }
      state->pos++;
    }
    /* need to back one step since first char is already gone */
    state->vstart--;
    state->vlen = state->pos - state->vstart;
  } else if(type == JSON_TYPE_NULL || type == JSON_TYPE_TRUE || type == JSON_TYPE_FALSE) {
    state->vstart--;
    switch (type) {
    case JSON_TYPE_NULL:  str = "null";  break;
    case JSON_TYPE_TRUE:  str = "true";  break;
    case JSON_TYPE_FALSE: str = "false"; break;
    default:              str = "";      break;
    }

    while(state->pos < state->len) {
      c = state->json[state->pos];
      if(c == '\0' || is_whitespace(c) ||
         c == ',' || c == ']' || c == '}') {
        break;
      }
      state->pos++;
    }

    state->vlen = state->pos - state->vstart;
    len = strlen(str);

    /* Require an exact-length match. Comparing over the longer of the two
       lengths, as was done before, reads past the end of the input when the
       input holds a truncated literal. */
    if(state->vlen != len ||
       strncmp(str, &state->json[state->vstart], len) != 0) {
      state->error = JSON_ERROR_SYNTAX;
      return JSON_TYPE_ERROR;
    }
  }

  state->vtype = type;
  return state->vtype;
}
/*--------------------------------------------------------------------*/
static void
skip_ws(struct jsonparse_state *state)
{
  while(state->pos < state->len &&
        is_whitespace(state->json[state->pos])) {
    state->pos++;
  }
}
/*--------------------------------------------------------------------*/
static bool
is_atomic(struct jsonparse_state *state)
{
  char v = state->vtype;
  return v == 'N' || v == '"' || v == '0' || v == 'n' || v == 't' || v == 'f';
}
/*--------------------------------------------------------------------*/
void
jsonparse_setup(struct jsonparse_state *state, const char *json, int len)
{
  state->json = json;
  state->len = len;
  state->pos = 0;
  state->depth = 0;
  state->error = 0;
  state->vtype = 0;
  state->vstart = 0;
  state->vlen = 0;
  state->stack[0] = 0;
}
/*--------------------------------------------------------------------*/
int
jsonparse_next(struct jsonparse_state *state)
{
  char c;
  char s;
  char v;
  bool ret;

  skip_ws(state);
  s = jsonparse_get_type(state);
  v = state->vtype;

  /* Treat the end of the input like the NUL terminator handled below,
     rather than reading, and stepping over, the byte just past it. */
  if(state->pos < state->len) {
    c = state->json[state->pos];
    state->pos++;
  } else {
    c = 0;
  }

  switch(c) {
  case '{':
    if((s == 0 && v == 0) || s == '[' || s == ':') {
      if(push(state, c)) {
        return c;
      }
    }
    state->error = JSON_ERROR_UNEXPECTED_OBJECT;
    return JSON_TYPE_ERROR;
  case '}':
    if((s == ':' && v != ',' && v != 0 ) || (s == '{' && v == 0)) {
      if(pop(state)) {
        return c;
      }
    }
    state->error = JSON_ERROR_UNEXPECTED_END_OF_OBJECT;
    return JSON_TYPE_ERROR;
  case ']':
    if(s == '[' && v != ',') {
      if(pop(state)) {
        return c;
      }
    }
    state->error = JSON_ERROR_UNEXPECTED_END_OF_ARRAY;
    return JSON_TYPE_ERROR;
  case ':':
    if(s == '{' && v == 'N') {
      ret = modify(state, ':');
      state->vtype = 0;
      if(ret) {
        return jsonparse_next(state);
      }
    }
    state->error = JSON_ERROR_SYNTAX;
    return JSON_TYPE_ERROR;
  case ',':
    if(s == ':' && v != 0) {
      ret = modify(state, '{');
      state->vtype = c;
      if(ret) {
        return c;
      }
    } else if(s == '[') {
      state->vtype = c;
      return c;
    }
    state->error = JSON_ERROR_SYNTAX;
    return JSON_TYPE_ERROR;
  case '"':
    if((s == 0 && v == 0) || s == '{' || s == '[' || s == ':') {
      return atomic(state, c = (s == '{' ? JSON_TYPE_PAIR_NAME : c));
    } else {
      state->error = JSON_ERROR_UNEXPECTED_STRING;
      return JSON_TYPE_ERROR;
    }
    return c;
  case '[':
    if((s == 0 && v == 0) || s == '[' || s == ':') {
      if(push(state, c)) {
        return c;
      }
    }
    state->error = JSON_ERROR_UNEXPECTED_ARRAY;
    return JSON_TYPE_ERROR;
  case 0:
    if(v == 0 || state->depth > 0) {
      state->error = JSON_ERROR_SYNTAX;
    }
    return JSON_TYPE_ERROR;
  default:
    if(s == 0 || s == ':' || s == '[') {
      if (v != 0 && v != ',') {
        state->error = JSON_ERROR_SYNTAX;
        return JSON_TYPE_ERROR;
      }
      if(c == '-' || (c <= '9' && c >= '0')) {
        return atomic(state, JSON_TYPE_NUMBER);
      } else if(c == 'n') {
        return atomic(state, JSON_TYPE_NULL);
      } else if(c == 't') {
        return atomic(state, JSON_TYPE_TRUE);
      } else if(c == 'f') {
        return atomic(state, JSON_TYPE_FALSE);
      } else {
        state->error = JSON_ERROR_SYNTAX;
        return JSON_TYPE_ERROR;
      }
    } else if(s == '{') {
      state->error = JSON_ERROR_SYNTAX;
      return JSON_TYPE_ERROR;
    }
  }
  return 0;
}
/*--------------------------------------------------------------------*/
/* get the json value of the current position
 * works only on "atomic" values such as string, number, null, false, true
 */
/*--------------------------------------------------------------------*/
int
jsonparse_copy_value(struct jsonparse_state *state, char *str, int size)
{
  int i, o;
  char c;

  if(!is_atomic(state)) {
    return 0;
  }
  for(i = 0, o = 0; i < state->vlen && o < size - 1; i++) {
    c = state->json[state->vstart + i];
    if(c == '\\') {
      if(i + 1 >= state->vlen) {
        break;
      }
      i++;
      switch(state->json[state->vstart + i]) {
      case '"':  str[o++] = '"';  break;
      case '\\': str[o++] = '\\'; break;
      case '/':  str[o++] = '/';  break;
      case 'b':  str[o++] = '\b'; break;
      case 'f':  str[o++] = '\f'; break;
      case 'n':  str[o++] = '\n'; break;
      case 'r':  str[o++] = '\r'; break;
      case 't':  str[o++] = '\t'; break;
      }
      continue;
    }
    str[o++] = c;
  }
  str[o] = 0;
  return state->vtype;
}
/*--------------------------------------------------------------------*/
/* Copy the current number into a NUL-terminated buffer. The input is not
   required to be NUL-terminated, so atoi() and atol() must not be pointed
   into it directly.
*/
/*--------------------------------------------------------------------*/
static bool
copy_number(struct jsonparse_state *state, char *buf, int size)
{
  if(state->vtype != JSON_TYPE_NUMBER ||
     state->vlen < 0 || state->vlen >= size) {
    return false;
  }
  memcpy(buf, &state->json[state->vstart], state->vlen);
  buf[state->vlen] = '\0';
  return true;
}
/*--------------------------------------------------------------------*/
int
jsonparse_get_value_as_int(struct jsonparse_state *state)
{
  char buf[JSONPARSE_NUMBER_MAX_LEN + 1];

  if(!copy_number(state, buf, sizeof(buf))) {
    return 0;
  }
  return atoi(buf);
}
/*--------------------------------------------------------------------*/
long
jsonparse_get_value_as_long(struct jsonparse_state *state)
{
  char buf[JSONPARSE_NUMBER_MAX_LEN + 1];

  if(!copy_number(state, buf, sizeof(buf))) {
    return 0;
  }
  return atol(buf);
}
/*--------------------------------------------------------------------*/
/* strcmp - assume no strange chars that needs to be stuffed in string... */
/*--------------------------------------------------------------------*/
int
jsonparse_strcmp_value(struct jsonparse_state *state, const char *str)
{
  if(!is_atomic(state)) {
    return -1;
  }
  return strncmp(str, &state->json[state->vstart], state->vlen);
}
/*--------------------------------------------------------------------*/
int
jsonparse_get_len(struct jsonparse_state *state)
{
  return state->vlen;
}
/*--------------------------------------------------------------------*/
int
jsonparse_get_type(struct jsonparse_state *state)
{
  if(state->depth == 0) {
    return 0;
  }
  return state->stack[state->depth - 1];
}
/*--------------------------------------------------------------------*/
int
jsonparse_has_next(struct jsonparse_state *state)
{
  return state->pos < state->len;
}
/*--------------------------------------------------------------------*/
