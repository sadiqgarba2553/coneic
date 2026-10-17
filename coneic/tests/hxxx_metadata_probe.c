/* Annex-B conversion regression: payload and timing must survive allocation.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_block.h>
#include <stdio.h>
#include <string.h>
#include "hxxx_nal.h"

static bool check(const char *name, const uint8_t *input, size_t input_size,
                  uint8_t prefix, const uint8_t *expected, size_t expected_size,
                  bool comma)
{
    block_t *b = block_Alloc(input_size);
    if (!b) return false;
    memcpy(b->p_buffer, input, input_size);
    b->i_pts = INT64_C(1200000);
    b->i_dts = INT64_C(1100000);
    b->i_length = INT64_C(100000);
    b->i_nb_samples = 1024;
    b->i_flags = BLOCK_FLAG_TYPE_I | BLOCK_FLAG_DISCONTINUITY;
    b = hxxx_AnnexB_to_xVC(b, prefix);
    bool payload = b && b->i_buffer == expected_size &&
                   !memcmp(b->p_buffer, expected, expected_size);
    bool metadata = b && b->i_pts == INT64_C(1200000) &&
                    b->i_dts == INT64_C(1100000) && b->i_length == INT64_C(100000) &&
                    b->i_nb_samples == 1024 &&
                    b->i_flags == (BLOCK_FLAG_TYPE_I | BLOCK_FLAG_DISCONTINUITY);
    if (b) block_Release(b);
    printf("%s{\"name\":\"%s\",\"payload\":%s,\"metadata\":%s,\"passed\":%s}",
           comma ? "," : "", name, payload ? "true" : "false",
           metadata ? "true" : "false", payload && metadata ? "true" : "false");
    return payload && metadata;
}

int main(void)
{
    const uint8_t short_prefix[] = {0,0,1,0x65,0xaa,0,0,1,0x41,0xbb,0xcc};
    const uint8_t long_prefix[] = {0,0,0,1,0x65,0xaa,0,0,0,1,0x41,0xbb,0xcc};
    const uint8_t out4[] = {0,0,0,2,0x65,0xaa,0,0,0,3,0x41,0xbb,0xcc};
    const uint8_t out2[] = {0,2,0x65,0xaa,0,3,0x41,0xbb,0xcc};
    const uint8_t out1[] = {2,0x65,0xaa,3,0x41,0xbb,0xcc};
    const uint8_t single[] = {0,0,1,0x65,0xaa};
    const uint8_t single_out[] = {0,0,0,2,0x65,0xaa};
    bool ok = true;
    fputs("{\"checks\":[", stdout);
    ok &= check("Multiple three-byte start codes grow the buffer",short_prefix,sizeof(short_prefix),4,out4,sizeof(out4),false);
    ok &= check("Four-byte start codes convert in place",long_prefix,sizeof(long_prefix),4,out4,sizeof(out4),true);
    ok &= check("Two-byte output lengths shrink the buffer",long_prefix,sizeof(long_prefix),2,out2,sizeof(out2),true);
    ok &= check("One-byte output lengths shrink the buffer",long_prefix,sizeof(long_prefix),1,out1,sizeof(out1),true);
    ok &= check("Single NAL conversion preserves properties",single,sizeof(single),4,single_out,sizeof(single_out),true);
    printf("],\"passed\":%s}\n",ok?"true":"false");
    return ok ? 0 : 1;
}
