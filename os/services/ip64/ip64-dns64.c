/*
 * Copyright (c) 2014, Thingsquare, http://www.thingsquare.com/.
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
 *
 */

#include "ip64/ip64.h"
#include "ipv6/ip64-addr.h"
#include "ip64/ip64-dns64.h"

/*---------------------------------------------------------------------------*/

#include "sys/log.h"
#define LOG_MODULE  "IP64"
#define LOG_LEVEL   LOG_LEVEL_IP64

/*---------------------------------------------------------------------------*/

struct dns_hdr {
  uint8_t id[2];
  uint8_t flags1, flags2;
#define DNS_FLAG1_RESPONSE        0x80
#define DNS_FLAG1_OPCODE_STATUS   0x10
#define DNS_FLAG1_OPCODE_INVERSE  0x08
#define DNS_FLAG1_OPCODE_STANDARD 0x00
#define DNS_FLAG1_AUTHORATIVE     0x04
#define DNS_FLAG1_TRUNC           0x02
#define DNS_FLAG1_RD              0x01
#define DNS_FLAG2_RA              0x80
#define DNS_FLAG2_ERR_MASK        0x0f
#define DNS_FLAG2_ERR_NONE        0x00
#define DNS_FLAG2_ERR_NAME        0x03
  uint8_t numquestions[2];
  uint8_t numanswers[2];
  uint8_t numauthrr[2];
  uint8_t numextrarr[2];
};

/* A question, which follows the name it asks about. */
#define DNS_QUESTION_TYPE0  0
#define DNS_QUESTION_TYPE1  1
#define DNS_QUESTION_CLASS0 2
#define DNS_QUESTION_CLASS1 3
#define DNS_QUESTION_SIZE   4

/*
 * The fixed fields of a resource record, which follow the name the record is
 * about: a type and a class of two bytes each, a TTL of four, and then the
 * length of the data that follows the fields themselves.
 */
#define DNS_ANSWER_TYPE0    0
#define DNS_ANSWER_TYPE1    1
#define DNS_ANSWER_LEN0     8
#define DNS_ANSWER_LEN1     9
#define DNS_ANSWER_SIZE    10

#define DNS_TYPE_A      1
#define DNS_TYPE_AAAA  28

#define DNS_CLASS_IN    1
#define DNS_CLASS_ANY 255

/* The two high bits of a label length mark a compression pointer instead. */
#define DNS_NAME_POINTER 0xc0

/*---------------------------------------------------------------------------*/
/*
 * A name is a sequence of labels, each one a length byte followed by that
 * many bytes, ending either with a zero-length label or with a two-byte
 * pointer to a name earlier in the message. Returns the offset of the byte
 * that follows the name, or a negative value if the name does not end
 * within the message.
 */
static int
skip_name(const uint8_t *data, int datalen, int offset)
{
  int len;

  while(offset < datalen) {
    len = data[offset];
    if(len & DNS_NAME_POINTER) {
      /* A pointer is the last thing in a name. */
      return offset <= datalen - 2 ? offset + 2 : -1;
    }
    offset += 1 + len;
    if(len == 0) {
      return offset;
    }
  }

  return -1;
}
/*---------------------------------------------------------------------------*/
void
ip64_dns64_6to4(const uint8_t *ipv6data, int ipv6datalen,
                uint8_t *ipv4data, int ipv4datalen)
{
  int i;
  int offset;
  uint8_t *q;
  const struct dns_hdr *hdr;

  if(ipv4datalen < (int)sizeof(struct dns_hdr)) {
    LOG_WARN("dns64_6to4: message ended while parsing the header\n");
    return;
  }

  hdr = (const struct dns_hdr *)ipv4data;
  LOG_DBG("dns64_6to4 id: %02x%02x\n", hdr->id[0], hdr->id[1]);
  LOG_DBG("dns64_6to4 flags1: 0x%02x\n", hdr->flags1);
  LOG_DBG("dns64_6to4 flags2: 0x%02x\n", hdr->flags2);
  LOG_DBG("dns64_6to4 numquestions: 0x%02x\n",
      ((hdr->numquestions[0] << 8) + hdr->numquestions[1]));
  LOG_DBG("dns64_6to4 numanswers: 0x%02x\n",
      ((hdr->numanswers[0] << 8) + hdr->numanswers[1]));
  LOG_DBG("dns64_6to4 numauthrr: 0x%02x\n",
      ((hdr->numauthrr[0] << 8) + hdr->numauthrr[1]));
  LOG_DBG("dns64_6to4 numextrarr: 0x%02x\n",
      ((hdr->numextrarr[0] << 8) + hdr->numextrarr[1]));

  /* Find the DNS question header by scanning through the question
     labels. */
  offset = sizeof(struct dns_hdr);
  for(i = 0; i < ((hdr->numquestions[0] << 8) + hdr->numquestions[1]); i++) {
    offset = skip_name(ipv4data, ipv4datalen, offset);
    if(offset < 0 || offset > ipv4datalen - DNS_QUESTION_SIZE) {
      LOG_WARN("dns64_6to4: message ended while parsing a question\n");
      return;
    }

    q = &ipv4data[offset];
    if(q[DNS_QUESTION_CLASS0] == 0 && q[DNS_QUESTION_CLASS1] == DNS_CLASS_IN &&
       q[DNS_QUESTION_TYPE0] == 0 && q[DNS_QUESTION_TYPE1] == DNS_TYPE_AAAA) {
      q[DNS_QUESTION_TYPE1] = DNS_TYPE_A;
    }

    offset += DNS_QUESTION_SIZE;
  }
}
/*---------------------------------------------------------------------------*/
int
ip64_dns64_4to6(const uint8_t *ipv4data, int ipv4datalen,
                uint8_t *ipv6data, int ipv6capacity)
{
  int i;
  /* Offsets of the next byte to read, and of the next one to write. */
  int in, out;
  int end, len, tail;
  uint8_t *q;
  const struct dns_hdr *hdr;

  if(ipv4datalen < (int)sizeof(struct dns_hdr)) {
    LOG_WARN("dns64_4to6: message ended while parsing the header\n");
    return IP64_DNS64_DROP;
  }

  /*
   * ip64_4to6() copies the message into the packet going out before calling
   * this function, so everything up to the first record that grows is
   * already in place there.
   */
  if(ipv6capacity < ipv4datalen) {
    LOG_WARN("dns64_4to6: no room for the message that came in\n");
    return IP64_DNS64_DROP;
  }

  hdr = (const struct dns_hdr *)ipv4data;
  LOG_DBG("dns64_4to6 id: %02x%02x\n", hdr->id[0], hdr->id[1]);
  LOG_DBG("dns64_4to6 flags1: 0x%02x\n", hdr->flags1);
  LOG_DBG("dns64_4to6 flags2: 0x%02x\n", hdr->flags2);
  LOG_DBG("dns64_4to6 numquestions: 0x%02x\n",
      ((hdr->numquestions[0] << 8) + hdr->numquestions[1]));
  LOG_DBG("dns64_4to6 numanswers: 0x%02x\n",
      ((hdr->numanswers[0] << 8) + hdr->numanswers[1]));
  LOG_DBG("dns64_4to6 numauthrr: 0x%02x\n",
      ((hdr->numauthrr[0] << 8) + hdr->numauthrr[1]));
  LOG_DBG("dns64_4to6 numextrarr: 0x%02x\n",
      ((hdr->numextrarr[0] << 8) + hdr->numextrarr[1]));

  /*
   * Find the DNS answer header by scanning through the question labels.
   * Nothing has grown yet, so a question sits at the same offset in both
   * messages and is rewritten where the copy left it.
   */
  in = sizeof(struct dns_hdr);
  for(i = 0; i < ((hdr->numquestions[0] << 8) + hdr->numquestions[1]); i++) {
    in = skip_name(ipv4data, ipv4datalen, in);
    if(in < 0 || in > ipv4datalen - DNS_QUESTION_SIZE) {
      LOG_WARN("dns64_4to6: message ended while parsing a question\n");
      return IP64_DNS64_DROP;
    }

    q = &ipv6data[in];
    /*
     * The question is the one that went out, which ip64_dns64_6to4() rewrote
     * from AAAA to A. Put it back, so that the reply carries the question
     * that was asked.
     */
    if(q[DNS_QUESTION_CLASS0] == 0 && q[DNS_QUESTION_CLASS1] == DNS_CLASS_IN &&
       q[DNS_QUESTION_TYPE0] == 0 && q[DNS_QUESTION_TYPE1] == DNS_TYPE_A) {
      q[DNS_QUESTION_TYPE1] = DNS_TYPE_AAAA;
    }

    in += DNS_QUESTION_SIZE;
  }

  /*
   * Go through the answers and turn every A record into a AAAA record. Each
   * one that is translated grows by 12 bytes, so the two offsets part
   * company and everything that follows has to be written to its new place.
   */
  out = in;
  for(i = 0; i < ((hdr->numanswers[0] << 8) + hdr->numanswers[1]); i++) {
    /* The name the record is about, which is copied as it stands. */
    end = skip_name(ipv4data, ipv4datalen, in);
    if(end < 0) {
      LOG_WARN("dns64_4to6: message ended while parsing a record name\n");
      return IP64_DNS64_DROP;
    }
    if(end - in > ipv6capacity - out) {
      LOG_WARN("dns64_4to6: no room for a record name\n");
      return IP64_DNS64_DROP;
    }
    memcpy(&ipv6data[out], &ipv4data[in], end - in);
    out += end - in;
    in = end;

    if(in > ipv4datalen - DNS_ANSWER_SIZE) {
      LOG_WARN("dns64_4to6: message ended while parsing a record\n");
      return IP64_DNS64_DROP;
    }
    len = (ipv4data[in + DNS_ANSWER_LEN0] << 8) + ipv4data[in + DNS_ANSWER_LEN1];
    if(len > ipv4datalen - DNS_ANSWER_SIZE - in) {
      LOG_WARN("dns64_4to6: record holds less data than its length field\n");
      return IP64_DNS64_DROP;
    }

    if(ipv4data[in + DNS_ANSWER_TYPE0] == 0 &&
       ipv4data[in + DNS_ANSWER_TYPE1] == DNS_TYPE_A &&
       len == sizeof(uip_ip4addr_t)) {
      uip_ip4addr_t addr4;
      uip_ip6addr_t addr6;

      if(DNS_ANSWER_SIZE + (int)sizeof(addr6) > ipv6capacity - out) {
        LOG_WARN("dns64_4to6: no room for a translated record\n");
        return IP64_DNS64_DROP;
      }

      /* The record keeps its class and its TTL, but becomes a AAAA record
         holding an address of four times the length. */
      memcpy(&ipv6data[out], &ipv4data[in], DNS_ANSWER_SIZE);
      ipv6data[out + DNS_ANSWER_TYPE1] = DNS_TYPE_AAAA;
      ipv6data[out + DNS_ANSWER_LEN0] = 0;
      ipv6data[out + DNS_ANSWER_LEN1] = sizeof(addr6);
      in += DNS_ANSWER_SIZE;
      out += DNS_ANSWER_SIZE;

      /* A name of odd length leaves the record data at an odd offset,
         whereas uip_ip6addr_t is written through as 16-bit words, so the
         address is synthesized into a local one and copied into place. */
      memcpy(&addr4, &ipv4data[in], sizeof(addr4));
      ip64_addr_4to6(&addr4, &addr6);
      memcpy(&ipv6data[out], &addr6, sizeof(addr6));
      in += len;
      out += sizeof(addr6);
    } else {
      /* Every other record is copied as it stands. */
      if(DNS_ANSWER_SIZE + len > ipv6capacity - out) {
        LOG_WARN("dns64_4to6: no room for a record\n");
        return IP64_DNS64_DROP;
      }
      memcpy(&ipv6data[out], &ipv4data[in], DNS_ANSWER_SIZE + len);
      in += DNS_ANSWER_SIZE + len;
      out += DNS_ANSWER_SIZE + len;
    }
  }

  /*
   * The authority and the additional sections are copied as they stand, but
   * they still have to move, since the answers before them grew. A name in
   * them that points back into an answer keeps the offset it had, which the
   * growth invalidated; correcting those would mean rewriting the names in
   * the whole message.
   */
  tail = ipv4datalen - in;
  if(tail > ipv6capacity - out) {
    LOG_WARN("dns64_4to6: no room for the rest of the message\n");
    return IP64_DNS64_DROP;
  }
  memcpy(&ipv6data[out], &ipv4data[in], tail);
  out += tail;

  return out;
}
/*---------------------------------------------------------------------------*/
