/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden.
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
 *         Negative-path unit tests for the IEEE 802.15.4 framer.
 *
 *         These tests feed deliberately malformed, truncated, and hostile
 *         input to frame802154_parse() and
 *         frame802154e_parse_information_elements() and assert that the
 *         parsers reject it (returning the documented failure value) instead
 *         of reading past the end of the buffer or storing unvalidated state.
 *         They lock in the hardening added to the framer so the guards cannot
 *         be silently regressed.
 *
 *         All test vectors are static const arrays. When this test is built
 *         for the native target with AddressSanitizer
 *         (make TARGET=native CFLAGS+=-fsanitize=address
 *          LDFLAGS+=-fsanitize=address), the global redzones turn any read
 *         past the declared length of a vector into a hard failure; without
 *         ASan the return-value assertions still catch acceptance of bad
 *         frames.
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "contiki.h"
#include "unit-test/unit-test.h"
#include "net/mac/llsec802154.h"
#include "net/mac/framer/frame802154.h"
#include "net/mac/framer/frame802154e-ie.h"

#include <stdio.h>
#include <string.h>

PROCESS(test_process, "framer negative-path test");
AUTOSTART_PROCESSES(&test_process);

/*
 * The framer parsers are pure functions over (buffer, length), so each test
 * vector is just a byte array. The FCF and IE descriptor encodings are
 * little-endian (low byte first), matching READ16() in the framer.
 */

/* ---- frame802154_parse(): truncated headers (hardening #1) -------------- */

/*
 * FCF only: a data frame whose FCF advertises long destination and source
 * addressing plus a security header (~30 bytes of header), but only the two
 * FCF bytes are present. The parser must stop at the missing sequence number.
 */
static const uint8_t frame_fcf_only[] = {
  0x09, /* frame type = data, security enabled */
  0xec, /* dest = long, version = 2015, src = long */
};

/*
 * Same FCF, but now long enough for the sequence number and destination PAN
 * ID, then truncated in the middle of the 8-byte destination address.
 */
static const uint8_t frame_trunc_dest_addr[] = {
  0x09, 0xec,       /* FCF as above */
  0x11,             /* sequence number */
  0xcd, 0xab,       /* destination PAN ID */
};

/* ---- frame802154_parse(): valid baseline (positive control) ------------- */

/*
 * A well-formed frame: data, short dest + short src addressing, PAN ID
 * compression on, version 2015. Header length = 2 (FCF) + 1 (seqno) +
 * 2 (dest PAN) + 2 (dest addr) + 2 (src addr) = 9 bytes.
 */
static const uint8_t frame_valid_short[] = {
  0x41, 0xa8,       /* FCF: data, panid compression, short/short, 2015 */
  0x05,             /* sequence number */
  0xcd, 0xab,       /* destination PAN ID */
  0x02, 0x00,       /* destination short address */
  0x03, 0x00,       /* source short address */
};
#define FRAME_VALID_SHORT_HDR_LEN 9

#if LLSEC802154_USES_AUX_HEADER
/* ---- frame802154_parse(): security-control subfield masking (#5) -------- */

/*
 * A secured frame with no addressing. The security-control octet sets the
 * reserved high bit (0x80) but leaves frame-counter suppression (bit 5) and
 * size (bit 6) clear, so the frame counter is present. Before masking, the
 * bare shift made frame_counter_suppression non-zero and the parser wrongly
 * skipped the 4-byte counter. Header length = 2 + 1 + 1 + 4 = 8.
 */
static const uint8_t frame_sec_reserved_bit[] = {
  0x09, 0x20,             /* FCF: data, security enabled, no addr, 2015 */
  0x42,                   /* sequence number */
  0x81,                   /* SCF: level 1, reserved bit 7 set, suppr/size 0 */
  0xde, 0xad, 0xbe, 0xef, /* 4-byte frame counter */
};
#define FRAME_SEC_RESERVED_HDR_LEN 8
#endif /* LLSEC802154_USES_AUX_HEADER */

/* ---- IE parser: malformed information elements -------------------------- */

/*
 * Header IE "list termination 1" (moves the parser into payload-IE state)
 * followed by an IETF payload IE with length 0. The 6top branch must reject
 * it instead of dereferencing the (absent) Sub-ID and underflowing the
 * content length to 0xffff (hardening #3).
 */
static const uint8_t ie_ietf_zero_len[] = {
  0x00, 0x3f,       /* header IE: list termination 1 */
  0x00, 0xa8,       /* payload IE: IETF group, len 0 */
};

/*
 * List termination 1, then an MLME payload IE wrapping a TSCH channel hopping
 * sequence long sub-IE whose length (4) is shorter than the 10-byte fixed
 * header. The parser must reject it rather than reading the sequence-length
 * field at offset 8 past the end of the sub-IE (hardening #2).
 */
static const uint8_t ie_hopping_short[] = {
  0x00, 0x3f,             /* header IE: list termination 1 */
  0x06, 0x88,             /* payload IE: MLME, nested len 6 */
  0x04, 0xc8,             /* long sub-IE: hopping sequence, len 4 */
  0xaa, 0xbb, 0xcc, 0xdd, /* 4 bytes of content (too short) */
};

/*
 * A header IE that declares a length far larger than the bytes that follow.
 * The parser must reject it rather than advancing past the buffer (the
 * per-case bound plus the loop-tail backstop, hardening #4).
 */
static const uint8_t ie_len_overruns_buffer[] = {
  0x64, 0x0f,       /* header IE: ack/nack time correction, len 100 */
};

/* ---- IE parser: valid hopping sequence (positive control) --------------- */

/*
 * List termination 1, MLME payload IE, then a complete TSCH channel hopping
 * sequence sub-IE: 10-byte fixed header (id, page, #channels, phy config,
 * 2-byte sequence length = 4), a 4-byte sequence list, and a 2-byte current
 * hop. Total sub-IE content = 12 + 4 = 16 bytes.
 */
static const uint8_t ie_hopping_valid[] = {
  0x00, 0x3f,             /* header IE: list termination 1 */
  0x12, 0x88,             /* payload IE: MLME, nested len 18 */
  0x10, 0xc8,             /* long sub-IE: hopping sequence, len 16 */
  0x01,                   /* [0]    hopping sequence id */
  0x00,                   /* [1]    channel page */
  0x00, 0x00,             /* [2-3]  number of channels */
  0x00, 0x00, 0x00, 0x00, /* [4-7]  phy configuration */
  0x04, 0x00,             /* [8-9]  sequence length = 4 */
  0x11, 0x22, 0x33, 0x44, /* [10-13] sequence list */
  0x00, 0x00,             /* [14-15] current hop */
};

UNIT_TEST_REGISTER(frame_parse_rejects_truncated,
                   "frame802154_parse rejects truncated headers");
UNIT_TEST_REGISTER(frame_parse_accepts_valid,
                   "frame802154_parse accepts a valid frame");
#if LLSEC802154_USES_AUX_HEADER
UNIT_TEST_REGISTER(frame_parse_masks_security_subfields,
                   "frame802154_parse masks reserved security-control bits");
#endif /* LLSEC802154_USES_AUX_HEADER */
UNIT_TEST_REGISTER(ie_parse_rejects_malformed,
                   "IE parser rejects malformed information elements");
UNIT_TEST_REGISTER(ie_parse_accepts_valid_hopping,
                   "IE parser accepts a valid hopping sequence IE");

void
test_print_report(const unit_test_t *utp)
{
  printf("=check-me= ");
  if(utp->passed == false) {
    printf("FAILED   - %s (exit at %u)\n", utp->descr, utp->exit_line);
  } else {
    printf("SUCCEEDED - %s\n", utp->descr);
  }
}

UNIT_TEST(frame_parse_rejects_truncated)
{
  frame802154_t frame;

  UNIT_TEST_BEGIN();

  /* A frame shorter than the FCF is rejected. */
  UNIT_TEST_ASSERT(frame802154_parse((uint8_t *)frame_fcf_only, 1, &frame) == 0);

  /* The FCF advertises a large header that the 2 available bytes cannot hold. */
  UNIT_TEST_ASSERT(frame802154_parse((uint8_t *)frame_fcf_only,
                                     sizeof(frame_fcf_only), &frame) == 0);

  /* Truncated in the middle of the destination address. */
  UNIT_TEST_ASSERT(frame802154_parse((uint8_t *)frame_trunc_dest_addr,
                                     sizeof(frame_trunc_dest_addr),
                                     &frame) == 0);

  UNIT_TEST_END();
}

UNIT_TEST(frame_parse_accepts_valid)
{
  frame802154_t frame;
  int ret;

  UNIT_TEST_BEGIN();

  ret = frame802154_parse((uint8_t *)frame_valid_short,
                          sizeof(frame_valid_short), &frame);
  UNIT_TEST_ASSERT(ret == FRAME_VALID_SHORT_HDR_LEN);
  UNIT_TEST_ASSERT(frame.seq == 0x05);
  UNIT_TEST_ASSERT(frame.fcf.frame_type == FRAME802154_DATAFRAME);

  UNIT_TEST_END();
}

#if LLSEC802154_USES_AUX_HEADER
UNIT_TEST(frame_parse_masks_security_subfields)
{
  frame802154_t frame;
  int ret;

  UNIT_TEST_BEGIN();

  memset(&frame, 0, sizeof(frame));
  ret = frame802154_parse((uint8_t *)frame_sec_reserved_bit,
                          sizeof(frame_sec_reserved_bit), &frame);

  /* The reserved high bit must not be interpreted as suppression/size, so the
     frame counter is parsed and the full header is consumed. */
  UNIT_TEST_ASSERT(ret == FRAME_SEC_RESERVED_HDR_LEN);
  UNIT_TEST_ASSERT(frame.aux_hdr.security_control.frame_counter_suppression == 0);
  UNIT_TEST_ASSERT(frame.aux_hdr.security_control.frame_counter_size == 0);
  UNIT_TEST_ASSERT(frame.aux_hdr.security_control.security_level == 1);
  UNIT_TEST_ASSERT(frame.aux_hdr.frame_counter.u8[0] == 0xde);
  UNIT_TEST_ASSERT(frame.aux_hdr.frame_counter.u8[3] == 0xef);

  UNIT_TEST_END();
}
#endif /* LLSEC802154_USES_AUX_HEADER */

UNIT_TEST(ie_parse_rejects_malformed)
{
  struct ieee802154_ies ies;

  UNIT_TEST_BEGIN();

  /* IETF 6top IE with zero length: rejected, content length left untouched. */
  memset(&ies, 0, sizeof(ies));
  UNIT_TEST_ASSERT(frame802154e_parse_information_elements(ie_ietf_zero_len,
                       sizeof(ie_ietf_zero_len), &ies) == -1);
#if TSCH_WITH_SIXTOP
  UNIT_TEST_ASSERT(ies.sixtop_ie_content_len == 0);
#endif /* TSCH_WITH_SIXTOP */

  /* Hopping sequence sub-IE shorter than its fixed header: rejected, and the
     advertised sequence length is not stored from a frame we did not copy. */
  memset(&ies, 0, sizeof(ies));
  UNIT_TEST_ASSERT(frame802154e_parse_information_elements(ie_hopping_short,
                       sizeof(ie_hopping_short), &ies) == -1);
  UNIT_TEST_ASSERT(ies.ie_hopping_sequence_len == 0);

  /* IE that declares more bytes than the buffer holds: rejected. */
  memset(&ies, 0, sizeof(ies));
  UNIT_TEST_ASSERT(frame802154e_parse_information_elements(ie_len_overruns_buffer,
                       sizeof(ie_len_overruns_buffer), &ies) == -1);

  UNIT_TEST_END();
}

UNIT_TEST(ie_parse_accepts_valid_hopping)
{
  struct ieee802154_ies ies;
  int ret;

  UNIT_TEST_BEGIN();

  memset(&ies, 0, sizeof(ies));
  ret = frame802154e_parse_information_elements(ie_hopping_valid,
                                                sizeof(ie_hopping_valid), &ies);
  UNIT_TEST_ASSERT(ret > 0);
  UNIT_TEST_ASSERT(ies.ie_channel_hopping_sequence_id == 0x01);
  UNIT_TEST_ASSERT(ies.ie_hopping_sequence_len == 4);
  UNIT_TEST_ASSERT(ies.ie_hopping_sequence_list[0] == 0x11);
  UNIT_TEST_ASSERT(ies.ie_hopping_sequence_list[3] == 0x44);

  UNIT_TEST_END();
}

PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(frame_parse_rejects_truncated);
  UNIT_TEST_RUN(frame_parse_accepts_valid);
#if LLSEC802154_USES_AUX_HEADER
  UNIT_TEST_RUN(frame_parse_masks_security_subfields);
#endif /* LLSEC802154_USES_AUX_HEADER */
  UNIT_TEST_RUN(ie_parse_rejects_malformed);
  UNIT_TEST_RUN(ie_parse_accepts_valid_hopping);

  printf("=check-me= DONE\n");

  PROCESS_END();
}
