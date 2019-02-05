#include "utils/base64.h"
#include <malloc.h>
#include <stddef.h>

char * zl_base64_encode(int * ret_size, const char * src, int n)
{
    static const char __base64_byte_alpha[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '+', '/'

    };

    char *ret;
    int byte_idx = 0;
    int bit_off = 0;
    int ret_idx = 0;
    char p_byte;
    char np_byte;
    char b;
    int val_size = (n + (3 - n % 3) % 3) / 3 * 4 - (3 - n % 3) % 3;
    *ret_size = val_size + (3 - n % 3) % 3;

    ret = calloc(*ret_size + 1, 1);
    if (ret == NULL) {
        *ret_size = 0;
        return NULL;
    }

    while (ret_idx != val_size) {
        p_byte = byte_idx >= n ? 0 : src[byte_idx];
        np_byte = byte_idx + 1 >= n ? 0 : src[byte_idx + 1];

        b = 0;
        switch (bit_off) {
        case 0:
            b = (0xFC & p_byte) >> 2;
            bit_off = 6;
            break;
        case 1:
            b = (0x7E & p_byte) >> 1;
            bit_off = 7;
            break;
        case 2:
            b = (0x3F & p_byte);
            bit_off = 0;
            byte_idx++;
            break;
        case 3:
            b = ((0x1F & p_byte) << 1) | ((0x80 & np_byte) >> 7);
            bit_off = 1;
            byte_idx++;
            break;
        case 4:
            b = ((0x0F & p_byte) << 2) | ((0xC0 & np_byte) >> 6);
            bit_off = 2;
            byte_idx++;
            break;
        case 5:
            b = ((0x07 & p_byte) << 3) | ((0xE0 & np_byte) >> 5);
            bit_off = 3;
            byte_idx++;
            break;
        case 6:
            b = ((0x03 & p_byte) << 4) | ((0xF0 & np_byte) >> 4);
            bit_off = 4;
            byte_idx++;
            break;
        case 7:
            b = ((0x01 & p_byte) << 5) | ((0xF8 & np_byte) >> 3);
            bit_off = 5;
            byte_idx++;
            break;
        }

        ret[ret_idx++] = __base64_byte_alpha[(int) b];
    }

    for (; val_size < *ret_size; ret[val_size++] = '=');
    ret[val_size] = 0;
    return ret;
}
