/*
 * Copyright (c) 2019, Inria.
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

#include <stdio.h>

#if TEST_LC_SWITCH
#include <sys/lc-switch.h>
#endif /* TEST_LC_SWITCH */

#if TEST_LC_ADDRLABELS
#include <sys/lc-addrlabels.h>
#endif /* TEST_LC_ADDRLABELS */

#define MAX_NUM_CALLS 10
lc_t lc;

int
return_lc_set_call_count(void)
{
  static int call_count = 0;

  printf("- LC_RESUME()\n");
  LC_RESUME(lc);

  /*
   * The following three lines should be called only for the first
   * call of this function after LC_INIT().
   */
  printf("- LC_SET()\n");
  call_count++;
  LC_SET(lc);
  /*
   * We should resume this function here for the second call and
   * further
   */

  printf("- LC_END()\n\n");
  LC_END();

  return call_count;
}

int
main(void)
{
  int ret = 0;
  LC_INIT(lc);

  /* We're going to call return_lc_set_call_count() several times */
  for(int i = 0; i < MAX_NUM_CALLS; i++) {
    if(return_lc_set_call_count() != 1) {
      /* return_lc_set_call_count() should always return 1 */
      ret = -1;
      break;
    }
  }

  if(ret == 0) {
    /* LC_INIT() clears lc */
    LC_INIT(lc);

    /*
     * After calling LC_INIT(), return_lc_set_call_count() should
     * return 2, which means LC_SET() should be called this time since
     * lc is cleared.
     */
    if(return_lc_set_call_count() != 2) {
      ret = -1;
    }
  }

  if(ret == 0) {
    printf("test passed\n");
  } else {
    printf("test failed\n");
  }

  return ret;
}
