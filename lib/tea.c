//
//  main.c
//  tea_test
//
//  Created by clever on 2024/5/21.
//

#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include <emscripten.h>

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

void build_decoding_table()
{

    decoding_table = malloc(256);

    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char)encoding_table[i]] = i;
}

void base64_cleanup()
{
    free(decoding_table);
}

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length)
{

    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(*output_length);
    if (encoded_data == NULL)
        return NULL;

    for (int i = 0, j = 0; i < input_length;)
    {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}

unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length)
{

    if (decoding_table == NULL)
        build_decoding_table();

    if (input_length % 4 != 0)
        return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=')
        (*output_length)--;
    if (data[input_length - 2] == '=')
        (*output_length)--;

    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL)
        return NULL;

    for (int i = 0, j = 0; i < input_length;)
    {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) + (sextet_c << 1 * 6) + (sextet_d << 0 * 6);

        if (j < *output_length)
            decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length)
            decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length)
            decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}

static uint32_t DELTA = 0x9e3779b9;

static size_t TEA_BLOCK_LEN = 8;
static size_t TEA_KEY_LEN = 16;

static void tea_encrypt(uint32_t *v, uint32_t *k, size_t iter)
{
    uint32_t v0 = v[0], v1 = v[1], sum = 0, i;           /* set up */
    uint32_t k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3]; /* cache key */
    for (i = 0; i < iter; i++)
    { /* basic cycle start */
        sum += DELTA;
        v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        v1 += ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
    } /* end cycle */
    v[0] = v0;
    v[1] = v1;
}

static void tea_decrypt(uint32_t *v, uint32_t *k, size_t iter)
{
    uint32_t v0 = v[0], v1 = v[1], sum = DELTA * iter, i; /* set up */
    uint32_t k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3];  /* cache key */
    for (i = 0; i < iter; i++)
    { /* basic cycle start */
        v1 -= ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
        v0 -= ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        sum -= DELTA;
    } /* end cycle */
    v[0] = v0;
    v[1] = v1;
}

EMSCRIPTEN_KEEPALIVE
int teaEncode(char *out_str, const char *data, const char *key_base64, size_t iter)
{
    size_t key_base64_len = strlen(key_base64);
    size_t key_len = 0;
    unsigned char *result = base64_decode(key_base64, key_base64_len, &key_len);

    if (key_len != TEA_KEY_LEN)
    {
        printf("Invalid Key Length \n");
        return -1;
    }

    uint32_t key[4];
    memcpy((uint32_t *)key, result, TEA_KEY_LEN);

    for (size_t i = 0; i < strlen(data) / TEA_BLOCK_LEN; i++)
    {
        tea_encrypt((uint32_t *)&data[i * TEA_BLOCK_LEN], key, iter);
        //        tea_encrypt((uint32_t*)(data + i * TEA_BLOCK_LEN), key, 16);
    }

    // to base64
    size_t data_len = strlen(data);
    size_t result_len = 0;
    char *result_base64 = base64_encode((const unsigned char *)data, data_len, &result_len);
    if (out_str == NULL)
    {
        return result_len;
    }
    return snprintf(out_str, result_len + 1, result_base64);
}

EMSCRIPTEN_KEEPALIVE
int teaDecrypt(char *out_str, const char *data_base64, const char *key_base64, size_t iter)
{

    size_t data_base64_len = strlen(data_base64);
    size_t data_len = 0;
    unsigned char *data = base64_decode(data_base64, data_base64_len, &data_len);

    size_t key_base64_len = strlen(key_base64);
    size_t key_len = 0;
    unsigned char *result = base64_decode(key_base64, key_base64_len, &key_len);

    if (key_len != TEA_KEY_LEN)
    {
        printf("Invalid Key Length \n");
        return -1;
    }

    uint32_t key[4];
    memcpy((uint32_t *)key, result, TEA_KEY_LEN);

    for (size_t i = 0; i < strlen(data) / TEA_BLOCK_LEN; i++)
    {
        tea_decrypt((uint32_t *)&data[i * TEA_BLOCK_LEN], key, iter);
        //        tea_encrypt((uint32_t*)(data + i * TEA_BLOCK_LEN), key, 16);
    }
    if (out_str == NULL)
    {
        return strlen(data);
    }
    return snprintf(out_str, strlen(data) + 1, data);
}