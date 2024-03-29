#include "coap.h"
#include "udp.h"
#include "dbgserial.h"
#include <string.h>

void dump_coap_buffer(coap_buffer *buffer) {
    DBGprintf("\t");
    for (uint16_t i = 0; i < buffer->len; i++)
    {
        if (i % 30 == 0) {
            DBGprintf("\n\t");
        }
        DBGprintf("%02x ", buffer->p[i]);
    }
    
    DBGprintf("\n");
}

void dump_coap(coap_pkt *pkt) {
    DBGprintf("COAP packet: Version %u, Type %u, code class %u, detail %u, msg ID %04x\n", pkt->hdr->ver, pkt->hdr->type, pkt->hdr->code_class, pkt->hdr->code_detail);
    if (pkt->token.len > 0) {
        DBGprintf("token: \n");
        dump_coap_buffer(&pkt->token);
    } else {
        DBGprintf("no token\n");
    }
    for (uint8_t i = 0; i < MAX_COAP_OPTIONS; i++)
    {
        if (pkt->options[i].option_number != 0) {
            DBGprintf("COAP option %u \n", pkt->options[i].option_number);
            dump_coap_buffer(&pkt->options[i].option_value);
        }
    }

    if (pkt->data.len == 0) {
        DBGprintf("no body\n");
    } else {
        DBGprintf("body: \n");
        dump_coap_buffer(&pkt->data);
    }
    
}

uint8_t coap_parse(coap_pkt *pkt, sk_buff*buf) {
    uint16_t realLen = buf->len - UDP_PKT_START;
    if (realLen < 4) {
        return COAP_ERR_HEADER_SHORT;
    }
    pkt->hdr = (coap_pkt_hdr*)(buf->buff + UDP_PKT_START);

    if (pkt->hdr->ver != 1) {
        return COAP_ERR_VERSION;
    }

    // parse token
    if (pkt->hdr->tkl > 9 || (pkt->hdr->tkl + 4) > realLen) {
        return COAP_ERR_TOKEN_LENGTH;
    }
    pkt->token.p = (buf->buff + UDP_PKT_START + 4);
    pkt->token.len = pkt->hdr->tkl;

    // parse options, if any.
    for (uint8_t i = 0; i < MAX_COAP_OPTIONS; i++)
    {
        pkt->options[i].option_number = COAP_OPTION_NO_OPTION;
    }
    pkt->data.len = 0;
    
    uint16_t buflen = buf->len;
    uint16_t curOptionNum = 0;
    uint8_t optionIdx = 0;
    for (uint16_t i = UDP_PKT_START + 4 + pkt->hdr->tkl; i < buflen; i++) {
        uint8_t delta_length = buf->buff[i];
        if((delta_length & 0xF0) == 0xF0) {
            if (delta_length == 0xFF) {
                // found the start of the payload!
                pkt->data.p = buf->buff + i + 1;
                pkt->data.len = buf->len - (i+1);
                break;
            } else {
                return COAP_ERR_OPTION_DELTA_INVALID;
            }
        }

        if (optionIdx == MAX_COAP_OPTIONS) {
            return COAP_ERR_TOO_MANY_OPTIONS;
        }

        uint8_t opDelta = (delta_length & 0xF0)>>4;
        uint8_t opLength = delta_length & 0x0F;

        curOptionNum += opDelta;
        
        if (opDelta == 13) {
            if (i+1 < buflen) {
                i++;
                curOptionNum += buf->buff[i];
            } else {
                return COAP_ERR_OPTION_OVERFLOW;
            }
        } else if (opDelta == 14) {
            if (i+2 < buflen) {
                i++;
                uint16_t optionDeltaTemp = (((uint16_t)buf->buff[i]) <<8)|((uint16_t)buf->buff[i+1]);
                i++;
                curOptionNum += optionDeltaTemp + 255;
            } else {
                return COAP_ERR_OPTION_OVERFLOW;
            }
        }
        pkt->options[optionIdx].option_number = curOptionNum;

        uint16_t optionLen = opLength;
        if (opLength == 13) {
            if (i+1 < buflen) {
                i++;
                optionLen += buf->buff[i];
            } else {
                return COAP_ERR_OPTION_OVERFLOW;
            }
        } else if (opLength == 14) {
            if (i+2 < buflen) {
                i++;
                uint16_t optionLenTemp = (((uint16_t)buf->buff[i]) <<8)|((uint16_t)buf->buff[i+1]);
                i++;
                optionLen += optionLenTemp+255;
            } else {
                return COAP_ERR_OPTION_OVERFLOW;
            }
        }

        if (i+opLength > buflen) {
            return COAP_ERR_OPTION_PL_OVERFLOW;
        }
        pkt->options[optionIdx].option_value.p = buf->buff + i + 1;
        pkt->options[optionIdx].option_value.len = opLength;
        i += opLength;
        
        optionIdx++;
    }

    return 0;
};

// serialize a coap packet into the provided buffer.
// Assumes that options is sorted. Very bad things will happen if it's not.
// Also assumes token.length == hdr->tkl.
uint8_t coap_serialize(coap_pkt *pkt, uint8_t *buf, uint16_t len, uint16_t *realLen) {
    if(len < 4) {
        // Not enough header space...
        return COAP_ERR_HEADER_SHORT;
    }
    uint16_t bufIdx = 0;
    // Copy header in...
    memcpy(buf, (uint8_t*)pkt->hdr, 4);
    bufIdx += 4;

    if (pkt->token.len > 0) {
        if ((len-bufIdx) < pkt->token.len) {
            return COAP_ERR_TOKEN_LENGTH;
        }
        memcpy(buf+bufIdx, pkt->token.p, pkt->token.len);
        bufIdx += pkt->token.len;
    }

    uint16_t option_last = 0;
    for (uint8_t i = 0; i < MAX_COAP_OPTIONS; i++)
    {
        if (pkt->options[i].option_number == COAP_OPTION_NO_OPTION) {
            break;
        }
        // figure out how to encode the option number...

        uint16_t optionDelta = pkt->options[i].option_number - option_last;
        option_last = pkt->options[i].option_number;
        uint8_t* deltaLengthPtr = buf+bufIdx;
        bufIdx++;

        uint8_t opDelta = 0;
        if (optionDelta < 13) {
            opDelta = optionDelta;
        } else if ((optionDelta-13) < 255) {
            opDelta = 13;
            if ((len-bufIdx) > 1) {
                buf[bufIdx++] = (uint8_t)(optionDelta-13);
            } else {
                return COAP_ERR_OPTION_OVERFLOW;
            }
        } else {
            // two byte format.
            opDelta = 14;
            uint16_t deltaTemp = optionDelta-269;
            if ((len-bufIdx) > 2) {
                buf[bufIdx++] = ((deltaTemp & 0xFF00)>>8);
                buf[bufIdx++] = (deltaTemp & 0x00FF);
            } else {
                return COAP_ERR_OPTION_OVERFLOW;
            }
        }

        uint16_t optionLength = pkt->options[i].option_value.len;
        uint8_t opLen = 0;
        if (optionLength < 13) {
            opLen = optionLength;
        } else if ((optionLength-13) < 255) {
            opLen = 13;
            if ((len-bufIdx) > 1) {
                buf[bufIdx++] = (uint8_t)(optionLength-13);
            } else {
                return COAP_ERR_OPTION_OVERFLOW;
            }
        } else {
            // two byte format.
            opLen = 14;
            uint16_t deltaTemp = optionLength-269;
            if ((len-bufIdx) > 2) {
                buf[bufIdx++] = ((deltaTemp & 0xFF00)>>8);
                buf[bufIdx++] = (deltaTemp & 0x00FF);
            } else {
                return COAP_ERR_OPTION_OVERFLOW;
            }
        }
        *deltaLengthPtr = (opDelta<<4) | opLen;

        if ((len-bufIdx) < optionLength) {
            return COAP_ERR_OPTION_PL_OVERFLOW;
        }
        memcpy(buf+bufIdx, pkt->options[i].option_value.p, optionLength);
        bufIdx+= optionLength;
    }

    if (pkt->data.len > 0) {
        if ((len-bufIdx) < pkt->data.len + 1) {
            return COAP_ERR_PAYLOAD_OVERFLOW;
        }
        buf[bufIdx++] = 0xFF;
        memcpy(buf+bufIdx, pkt->data.p, pkt->data.len);
        bufIdx+=pkt->data.len;
    }
    
    *realLen = bufIdx;
    return COAP_ERR_NONE;
}
