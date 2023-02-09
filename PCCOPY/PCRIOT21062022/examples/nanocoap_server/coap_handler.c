/*
 * Copyright (C) 2016 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fmt.h"
#include "net/nanocoap.h"
#include "hashes/sha256.h"
#include "kernel_defines.h"

#include "net/ipv6/addr.h"
//#include "_nib-internal.h"

#include "net/gnrc/ipv6/nib.h"
#include "net/gnrc/ipv6/nib/ft.h"

#include "net/gnrc/netif.h"
#include "net/ipv6/addr.h"
// #include "tests-gnrc_ipv6_nib.h"
extern  gnrc_ipv6_nib_ft_t entry;
/* internal value that can be read/written via CoAP */
static uint8_t internal_value = 0;

static const uint8_t block2_intro[] = "This is RIOT (Version: ";
static const uint8_t block2_board[] = "running on the ";
static const uint8_t block2_mcu[] = " board with a ";
static const uint8_t block2_default[] = "routing default";
// static const uint8_t block2_via[] = " |___v_i_a___| ";
static const uint8_t block2_destination[] = "the destination";
static const uint8_t block2_nexthop[] = " the next hop: ";

static ssize_t _echo_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
    (void)context;
    char uri[CONFIG_NANOCOAP_URI_MAX];

    if (coap_get_uri_path(pkt, (uint8_t *)uri) <= 0) {
        return coap_reply_simple(pkt, COAP_CODE_INTERNAL_SERVER_ERROR, buf,
                                 len, COAP_FORMAT_TEXT, NULL, 0);
    }
    char *sub_uri = uri + strlen("/echo/");
    size_t sub_uri_len = strlen(sub_uri);
    return coap_reply_simple(pkt, COAP_CODE_CONTENT, buf, len, COAP_FORMAT_TEXT,
                             (uint8_t *)sub_uri, sub_uri_len);
}

static ssize_t _riot_board_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
    (void)context;
    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
            COAP_FORMAT_TEXT, (uint8_t*)RIOT_BOARD, strlen(RIOT_BOARD));
}

static ssize_t _riot_block2_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
    (void)context;
    coap_block_slicer_t slicer;
    gnrc_ipv6_nib_ft_t entry;      
    coap_block2_init(pkt, &slicer);
    uint8_t *payload = buf + coap_get_total_hdr_len(pkt);
    // int node_number = 10;
    // char rpltable[node_number][3]={0};
    //memset(rpltable, 0, node_number);
    uint8_t *bufpos = payload;
    void *state = NULL;
    uint8_t dst_address[] = {0};
    uint8_t nexthop_address[] = {0};
    //int i = 0;
    bufpos += coap_put_option_ct(bufpos, 0, COAP_FORMAT_TEXT);
    bufpos += coap_opt_put_block2(bufpos, COAP_OPT_CONTENT_FORMAT, &slicer, 1);
    *bufpos++ = 0xff;
    unsigned iface = 0U;
    while (gnrc_ipv6_nib_ft_iter(NULL, iface, &state, &entry)) {
        char addr_str[IPV6_ADDR_MAX_STR_LEN];
        if ((entry.dst_len == 0) || ipv6_addr_is_unspecified(&entry.dst)) {
            // printf("default%s ", (fte->primary ? "*" : ""));
            bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_default, sizeof(block2_default)-1);
            // if (fte->primary){
            //     bufpos += coap_blockwise_put_char(&slicer, bufpos, '*');
            // }
            // else{
            //     bufpos += coap_blockwise_put_char(&slicer, bufpos, ' ');
            // }
            // fmt_byte_hex(char a[i][0], entry->primary);
        }
        else {
            bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_destination, sizeof(block2_destination)-1);
            ipv6_addr_to_str(addr_str, &entry.dst, sizeof(addr_str));
            fmt_hex_bytes(dst_address, addr_str);
            bufpos += coap_blockwise_put_bytes(&slicer, bufpos, dst_address, sizeof(dst_address)-1);
            // printf("%s/%u ", ipv6_addr_to_str(addr_str, &fte->dst, sizeof(addr_str)),fte->dst_len);
            bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_destination, sizeof(block2_destination)-1);
            // char a[i][1] = ipv6_addr_to_str(addr_str, &entry->dst, sizeof(addr_str)));
            // fmt_byte_hex(char a[i][2], entry->dst_len);

        }
        if (!ipv6_addr_is_unspecified(&entry.next_hop)) {
            ipv6_addr_to_str(addr_str, &entry.next_hop, sizeof(addr_str));
            fmt_hex_bytes(nexthop_address, addr_str);
            bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_nexthop, sizeof(block2_nexthop)-1);
            // printf("via %s ", ipv6_addr_to_str(addr_str, &fte->next_hop, sizeof(addr_str)));
            //char a[i][3] = ipv6_addr_to_str(addr_str, &entry->next_hop, sizeof(addr_str))
        }
        // printf("dev #%u\n", fte->iface);
        // char a[i][4] = entry->iface;
       // i++;
    }
    /* Add actual content */
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_intro, sizeof(block2_intro)-1);
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, (uint8_t*)RIOT_VERSION, strlen(RIOT_VERSION)-1);
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, (uint8_t*)RIOT_VERSION, strlen(RIOT_VERSION)-1);
    bufpos += coap_blockwise_put_char(&slicer, bufpos, ')');
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_board, sizeof(block2_board)-1);
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, (uint8_t*)RIOT_BOARD, strlen(RIOT_BOARD));
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, block2_mcu, sizeof(block2_mcu)-1);
    bufpos += coap_blockwise_put_bytes(&slicer, bufpos, (uint8_t*)RIOT_MCU, strlen(RIOT_MCU));
    /* To demonstrate individual chars */
    bufpos += coap_blockwise_put_char(&slicer, bufpos, ' ');
    bufpos += coap_blockwise_put_char(&slicer, bufpos, 'M');
    bufpos += coap_blockwise_put_char(&slicer, bufpos, 'C');
    bufpos += coap_blockwise_put_char(&slicer, bufpos, 'U');
    bufpos += coap_blockwise_put_char(&slicer, bufpos, '.');
  

    unsigned payload_len = bufpos - payload;
    return coap_block2_build_reply(pkt, COAP_CODE_205,
                                   buf, len, payload_len, &slicer);
}

static ssize_t _riot_value_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
    (void) context;

    ssize_t p = 0;
    char rsp[16];
    unsigned code = COAP_CODE_EMPTY;

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pkt));

    switch(method_flag) {
    case COAP_GET:
        /* write the response buffer with the internal value */
        p += fmt_u32_dec(rsp, internal_value);
        code = COAP_CODE_205;
        break;
    case COAP_PUT:
    case COAP_POST:
        if (pkt->payload_len < 16) {
            /* convert the payload to an integer and update the internal value */
            char payload[16] = { 0 };
            memcpy(payload, (char*)pkt->payload, pkt->payload_len);
            internal_value = strtol(payload, NULL, 10);
            code = COAP_CODE_CHANGED;
        }
        else {
            code = COAP_CODE_REQUEST_ENTITY_TOO_LARGE;
        }
    }

    return coap_reply_simple(pkt, code, buf, len,
            COAP_FORMAT_TEXT, (uint8_t*)rsp, p);
}

ssize_t _sha256_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len, void *context)
{
    (void)context;

    /* using a shared sha256 context *will* break if two requests are handled
     * at the same time.  doing it anyways, as this is meant to showcase block1
     * support, not proper synchronisation. */
    static sha256_context_t sha256;

    uint8_t digest[SHA256_DIGEST_LENGTH];

    uint32_t result = COAP_CODE_204;

    coap_block1_t block1;
    int blockwise = coap_get_block1(pkt, &block1);

    printf("_sha256_handler(): received data: offset=%u len=%u blockwise=%i more=%i\n", \
            (unsigned)block1.offset, pkt->payload_len, blockwise, block1.more);

    if (block1.offset == 0) {
        puts("_sha256_handler(): init");
        sha256_init(&sha256);
    }

    sha256_update(&sha256, pkt->payload, pkt->payload_len);

    if (block1.more == 1) {
        result = COAP_CODE_CONTINUE;
    }

    size_t result_len = 0;
    if (!blockwise || !block1.more) {
        puts("_sha256_handler(): finish");
        sha256_final(&sha256, digest);
        result_len = SHA256_DIGEST_LENGTH * 2;
    }

    ssize_t reply_len = coap_build_reply(pkt, result, buf, len, 0);
    uint8_t *pkt_pos = (uint8_t*)pkt->hdr + reply_len;
    if (blockwise) {
        pkt_pos += coap_opt_put_block1_control(pkt_pos, 0, &block1);
    }
    if (result_len) {
        *pkt_pos++ = 0xFF;
        pkt_pos += fmt_bytes_hex((char *)pkt_pos, digest, sizeof(digest));
    }

    return pkt_pos - (uint8_t*)pkt->hdr;
}

/* must be sorted by path (ASCII order) */
const coap_resource_t coap_resources[] = {
    COAP_WELL_KNOWN_CORE_DEFAULT_HANDLER,
    { "/echo/", COAP_GET | COAP_MATCH_SUBTREE, _echo_handler, NULL },
    { "/riot/board", COAP_GET, _riot_board_handler, NULL },
    { "/riot/value", COAP_GET | COAP_PUT | COAP_POST, _riot_value_handler, NULL },
    { "/riot/ver", COAP_GET, _riot_block2_handler, NULL },
    { "/sha256", COAP_POST, _sha256_handler, NULL },
};

const unsigned coap_resources_numof = ARRAY_SIZE(coap_resources);
