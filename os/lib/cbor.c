/*
 * Copyright (c) 2018, SICS, RISE AB
 * Copyright (c) 2023, Uppsala universitet
 * Copyright (c) 2024, Siemens AG
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
 */

/**
 * \addtogroup cbor
 * @{
 * \file
 *         CBOR implementation.
 */

#include "lib/cbor.h"
#include <string.h>

/*---------------------------------------------------------------------------*/
void
cbor_init_writer(cbor_writer_state_t *state,
                 uint8_t *buffer, size_t buffer_size)
{
  state->buffer_head = buffer;
  state->buffer = buffer;
  state->buffer_size = buffer_size;
  state->nesting_depth = CBOR_MAX_NESTING;
}
/*---------------------------------------------------------------------------*/
size_t
cbor_end_writer(cbor_writer_state_t *state)
{
  return (state->nesting_depth == CBOR_MAX_NESTING) && state->buffer
         ? (size_t)(state->buffer - state->buffer_head)
         : 0;
}
/*---------------------------------------------------------------------------*/
void
cbor_break_writer(cbor_writer_state_t *state)
{
  state->buffer = NULL;
  state->buffer_size = 0;
}
/*---------------------------------------------------------------------------*/
static void
increment(cbor_writer_state_t *state)
{
  if(state->nesting_depth == CBOR_MAX_NESTING) {
    return;
  }
  state->records[state->nesting_depth].objects++;
}
/*---------------------------------------------------------------------------*/
static void
write_first_byte(cbor_writer_state_t *state, uint_fast8_t value)
{
  if(!state->buffer_size) {
    cbor_break_writer(state);
    return;
  }
  *state->buffer++ = value;
  state->buffer_size--;
  increment(state);
}
/*---------------------------------------------------------------------------*/
static void
write_object(cbor_writer_state_t *state,
             const void *object, size_t object_size)
{
  if(!object_size) {
    return;
  }
  if(state->buffer_size < object_size) {
    cbor_break_writer(state);
    return;
  }
  memcpy(state->buffer, object, object_size);
  state->buffer += object_size;
  state->buffer_size -= object_size;
}
/*---------------------------------------------------------------------------*/
void
cbor_write_object(cbor_writer_state_t *state,
                  const void *object, size_t object_size)
{
  if(!object_size) {
    return;
  }
  write_object(state, object, object_size);
  increment(state);
}
/*---------------------------------------------------------------------------*/
static void
insert_unsigned(cbor_writer_state_t *state,
                uint8_t *const destination,
                uint64_t value)
{
  size_t length_to_copy;

  if(!state->buffer) {
    return;
  }

  /* update additional information */
  if(value < CBOR_SIZE_1) {
    destination[-1] |= value;
    return;
  }
  if(value <= UINT8_MAX) {
    length_to_copy = 1;
    destination[-1] |= CBOR_SIZE_1;
  } else if(value <= UINT16_MAX) {
    length_to_copy = 2;
    destination[-1] |= CBOR_SIZE_2;
  } else if(value <= UINT32_MAX) {
    length_to_copy = 4;
    destination[-1] |= CBOR_SIZE_4;
  } else {
    length_to_copy = 8;
    destination[-1] |= CBOR_SIZE_8;
  }

  /* check if there is enough space left */
  if(state->buffer_size < length_to_copy) {
    cbor_break_writer(state);
    return;
  }

  /* move when inserting */
  memmove(destination + length_to_copy,
          destination,
          state->buffer - destination);

  /* update buffer variables */
  state->buffer += length_to_copy;
  state->buffer_size -= length_to_copy;

  /* serialize value */
  while(length_to_copy--) {
    destination[length_to_copy] = value;
    value >>= 8;
  }
}
/*---------------------------------------------------------------------------*/
void
cbor_write_unsigned(cbor_writer_state_t *state, uint64_t value)
{
  write_first_byte(state, CBOR_MAJOR_TYPE_UNSIGNED);
  insert_unsigned(state, state->buffer, value);
}
/*---------------------------------------------------------------------------*/
void
cbor_write_signed(cbor_writer_state_t *state, int64_t value)
{
  if(value >= 0) {
    cbor_write_unsigned(state, (uint64_t)value);
  } else {
    write_first_byte(state, CBOR_MAJOR_TYPE_SIGNED);
    insert_unsigned(state, state->buffer, (uint64_t)(-1 - value));
  }
}
/*---------------------------------------------------------------------------*/
void
cbor_write_data(cbor_writer_state_t *state,
                const uint8_t *data, size_t data_size)
{
  write_first_byte(state, CBOR_MAJOR_TYPE_BYTE_STRING);
  insert_unsigned(state, state->buffer, data_size);
  write_object(state, data, data_size);
}
/*---------------------------------------------------------------------------*/
void
cbor_write_text(cbor_writer_state_t *state,
                const char *text, size_t text_size)
{
  write_first_byte(state, CBOR_MAJOR_TYPE_TEXT_STRING);
  insert_unsigned(state, state->buffer, text_size);
  write_object(state, text, text_size);
}
/*---------------------------------------------------------------------------*/
void
cbor_write_null(cbor_writer_state_t *state)
{
  write_first_byte(state, CBOR_SIMPLE_VALUE_NULL);
}
/*---------------------------------------------------------------------------*/
void
cbor_write_undefined(cbor_writer_state_t *state)
{
  write_first_byte(state, CBOR_SIMPLE_VALUE_UNDEFINED);
}
/*---------------------------------------------------------------------------*/
void
cbor_write_bool(cbor_writer_state_t *state, bool boolean)
{
  write_first_byte(state,
                   boolean ? CBOR_SIMPLE_VALUE_TRUE : CBOR_SIMPLE_VALUE_FALSE);
}
/*---------------------------------------------------------------------------*/
static void
generic_open(cbor_writer_state_t *state, cbor_major_type_t major_type)
{
  if(!state->nesting_depth) {
    cbor_break_writer(state);
    return;
  }
  write_first_byte(state, major_type);
  state->records[--state->nesting_depth].start = state->buffer;
  state->records[state->nesting_depth].objects = 0;
}
/*---------------------------------------------------------------------------*/
static void
generic_close(cbor_writer_state_t *state, size_t value)
{
  insert_unsigned(state, state->records[state->nesting_depth].start, value);
  state->nesting_depth++;
}
/*---------------------------------------------------------------------------*/
void
cbor_open_data(cbor_writer_state_t *state)
{
  generic_open(state, CBOR_MAJOR_TYPE_BYTE_STRING);
}
/*---------------------------------------------------------------------------*/
void
cbor_close_data(cbor_writer_state_t *state)
{
  if(state->nesting_depth == CBOR_MAX_NESTING) {
    cbor_break_writer(state);
    return;
  }
  generic_close(state,
                state->buffer - state->records[state->nesting_depth].start);
}
/*---------------------------------------------------------------------------*/
void
cbor_open_array(cbor_writer_state_t *state)
{
  generic_open(state, CBOR_MAJOR_TYPE_ARRAY);
}
/*---------------------------------------------------------------------------*/
void
cbor_close_array(cbor_writer_state_t *state)
{
  if(state->nesting_depth == CBOR_MAX_NESTING) {
    cbor_break_writer(state);
    return;
  }
  generic_close(state, state->records[state->nesting_depth].objects);
}
/*---------------------------------------------------------------------------*/
void
cbor_open_map(cbor_writer_state_t *state)
{
  generic_open(state, CBOR_MAJOR_TYPE_MAP);
}
/*---------------------------------------------------------------------------*/
void
cbor_close_map(cbor_writer_state_t *state)
{
  if((state->nesting_depth == CBOR_MAX_NESTING)
     || (state->records[state->nesting_depth].objects & 1)) {
    cbor_break_writer(state);
    return;
  }
  generic_close(state, state->records[state->nesting_depth].objects >> 1);
}
/*---------------------------------------------------------------------------*/
void
cbor_init_reader(cbor_reader_state_t *state,
                 const uint8_t *cbor, size_t cbor_size)
{
  state->cbor = cbor;
  state->cbor_size = cbor_size;
}
/*---------------------------------------------------------------------------*/
cbor_major_type_t
cbor_peek_next(cbor_reader_state_t *state)
{
  if(!state->cbor_size) {
    return CBOR_MAJOR_TYPE_NONE;
  }
  return *state->cbor & 0xE0;
}
/*---------------------------------------------------------------------------*/
bool
cbor_end_reader(cbor_reader_state_t *state)
{
  return state->cbor_size == 0;
}
/*---------------------------------------------------------------------------*/
cbor_size_t
cbor_read_unsigned(cbor_reader_state_t *state, uint64_t *value)
{
  size_t bytes_to_read;

  if(!state->cbor_size) {
    return CBOR_SIZE_NONE;
  }
  cbor_size_t size = *state->cbor & ~0xE0;
  state->cbor++;
  state->cbor_size--;

  if(size < CBOR_SIZE_1) {
    *value = size;
    return CBOR_SIZE_1;
  }

  switch(size) {
  case CBOR_SIZE_1:
    bytes_to_read = 1;
    break;
  case CBOR_SIZE_2:
    bytes_to_read = 2;
    break;
  case CBOR_SIZE_4:
    bytes_to_read = 4;
    break;
  case CBOR_SIZE_8:
    bytes_to_read = 8;
    break;
  case CBOR_SIZE_NONE:
  default:
    return CBOR_SIZE_NONE;
  }

  if(bytes_to_read > state->cbor_size) {
    return CBOR_SIZE_NONE;
  }
  state->cbor_size -= bytes_to_read;

  *value = 0;
  while(bytes_to_read--) {
    *value <<= 8;
    *value += *state->cbor++;
  }
  return size;
}
/*---------------------------------------------------------------------------*/
cbor_size_t
cbor_read_signed(cbor_reader_state_t *state, int64_t *value)
{
  uint64_t unsigned_value;
  cbor_major_type_t type = cbor_peek_next(state);
  cbor_size_t size = cbor_read_unsigned(state, &unsigned_value);
  if(size == CBOR_SIZE_NONE || unsigned_value > INT64_MAX) {
    return CBOR_SIZE_NONE;
  }

  switch(type) {
  case CBOR_MAJOR_TYPE_UNSIGNED:
    *value = (int64_t)unsigned_value;
    return size;
  case CBOR_MAJOR_TYPE_SIGNED:
    *value = -(int64_t)(unsigned_value + 1);
    return size;
  default:
    return CBOR_SIZE_NONE;
  }
}
/*---------------------------------------------------------------------------*/
static const uint8_t *
read_byte_or_text_string(cbor_reader_state_t *state, size_t *size)
{
  uint64_t value;

  if((CBOR_SIZE_NONE == cbor_read_unsigned(state, &value))
     || (state->cbor_size < value)) {
    return NULL;
  }
  *size = value;
  const uint8_t *beginning = state->cbor;
  state->cbor += *size;
  state->cbor_size -= *size;
  return beginning;
}
/*---------------------------------------------------------------------------*/
const uint8_t *
cbor_read_data(cbor_reader_state_t *state, size_t *data_size)
{
  if(cbor_peek_next(state) != CBOR_MAJOR_TYPE_BYTE_STRING) {
    return NULL;
  }
  return read_byte_or_text_string(state, data_size);
}
/*---------------------------------------------------------------------------*/
const char *
cbor_read_text(cbor_reader_state_t *state, size_t *text_size)
{
  if(cbor_peek_next(state) != CBOR_MAJOR_TYPE_TEXT_STRING) {
    return NULL;
  }
  return (const char *)read_byte_or_text_string(state, text_size);
}
/*---------------------------------------------------------------------------*/
cbor_simple_value_t
cbor_read_simple(cbor_reader_state_t *state)
{
  if(!state->cbor_size) {
    return CBOR_SIMPLE_VALUE_NONE;
  }
  state->cbor_size--;
  return *state->cbor++;
}
/*---------------------------------------------------------------------------*/
static size_t
read_array_or_map(cbor_reader_state_t *state)
{
  uint64_t value;

  if((CBOR_SIZE_NONE == cbor_read_unsigned(state, &value))
     || (value >= SIZE_MAX)) {
    return SIZE_MAX;
  }
  return value;
}
/*---------------------------------------------------------------------------*/
size_t
cbor_read_array(cbor_reader_state_t *state)
{
  if(cbor_peek_next(state) != CBOR_MAJOR_TYPE_ARRAY) {
    return SIZE_MAX;
  }
  return read_array_or_map(state);
}
/*---------------------------------------------------------------------------*/
size_t
cbor_read_map(cbor_reader_state_t *state)
{
  if(cbor_peek_next(state) != CBOR_MAJOR_TYPE_MAP) {
    return SIZE_MAX;
  }
  return read_array_or_map(state);
}
/*---------------------------------------------------------------------------*/

/** @} */
