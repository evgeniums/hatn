/****************************************************************************/
/*
    
*/
/** @file base64.cpp
 *     Encoding/decoding data into and from a base64-encoded format
 */
/****************************************************************************/

#include <cstring>
#include "base64.h"

HATN_COMMON_NAMESPACE_BEGIN

static const unsigned char base64_table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static unsigned char dtable[256];
void initDtable()
{
    size_t i=0;
    std::memset(dtable, 0x80, 256);
    for (i = 0; i < sizeof(base64_table) - 1; i++)
    {
        dtable[(size_t)base64_table[i]] = static_cast<unsigned char>(i);
    }
    dtable[(size_t)'='] = 0;
}
Base64Init::Base64Init()
{
    initDtable();
}
Base64Init Base64::init;

size_t Base64::decodeReserverLen(size_t size)
{
    if( size > 0 && (size % 4) == 0 )
    {
        return (size / 4) * 3;
    }
    return 0;
}
size_t Base64::decode(const char *src, size_t len, char* out)
{
    char *pos, block[4], tmp;
    size_t i, count;
    int pad = 0;

    count = 0;
    for (i = 0; i < len; i++)
    {
        if (dtable[(size_t)src[i]] != 0x80)
            count++;
    }

    if (count == 0 || count % 4)
        return 0;

    pos = out;

    count = 0;
    for (i = 0; i < len; i++) {
        tmp = dtable[(size_t)src[i]];
        if ((unsigned char)tmp == 0x80)
            continue;

        if (src[i] == '=')
            pad++;
        block[count] = tmp;
        count++;
        if (count == 4) {
            *pos++ = (block[0] << 2) | (block[1] >> 4);
            *pos++ = (block[1] << 4) | (block[2] >> 2);
            *pos++ = (block[2] << 6) | block[3];
            count = 0;
            if (pad) {
                if (pad == 1)
                    pos--;
                else if (pad == 2)
                    pos -= 2;
                else {
                    return 0;
                }
                break;
            }
        }
    }

    return pos - out;
}

size_t Base64::encodeReserverLen(size_t size)
{
    if(size > 0)
    {
        return ((size + 2) / 3)*4;
    }
    return 0;
}

namespace {

typedef enum
{
    step_A, step_B, step_C
} base64_encodestep;

typedef struct
{
    base64_encodestep step;
    char result;
} base64_encodestate;

void base64_init_encodestate(base64_encodestate* state_in)
{
    state_in->step = step_A;
    state_in->result = 0;
}

char base64_encode_value(char value_in)
{
    static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (value_in > 63) return '=';
    return encoding[(int)value_in];
}

int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in)
{
    const char* plainchar = plaintext_in;
    const char* const plaintextend = plaintext_in + length_in;
    char* codechar = code_out;
    char result;
    char fragment;

    result = state_in->result;

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

    switch (state_in->step)
    {
        while (1)
        {
    case step_A:
            if (plainchar == plaintextend)
            {
                state_in->result = result;
                state_in->step = step_A;
                return (int)(codechar - code_out);
            }
            fragment = *plainchar++;
            result = (fragment & 0x0fc) >> 2;
            *codechar++ = base64_encode_value(result);
            result = (fragment & 0x003) << 4;
#if __cplusplus >= 201703L
            [[fallthrough]];
#endif
    case step_B:
            if (plainchar == plaintextend)
            {
                state_in->result = result;
                state_in->step = step_B;
                return (int)(codechar - code_out);
            }
            fragment = *plainchar++;
            result |= (fragment & 0x0f0) >> 4;
            *codechar++ = base64_encode_value(result);
            result = (fragment & 0x00f) << 2;
#if __cplusplus >= 201703L
            [[fallthrough]];
#endif
    case step_C:
            if (plainchar == plaintextend)
            {
                state_in->result = result;
                state_in->step = step_C;
                return (int)(codechar - code_out);
            }
            fragment = *plainchar++;
            result |= (fragment & 0x0c0) >> 6;
            *codechar++ = base64_encode_value(result);
            result  = (fragment & 0x03f) >> 0;
            *codechar++ = base64_encode_value(result);

        }
    }
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    /* control should not reach here */
    return (int)(codechar - code_out);
}

int base64_encode_blockend(char* code_out, base64_encodestate* state_in)
{
    char* codechar = code_out;

    switch (state_in->step)
    {
    case step_B:
        *codechar++ = base64_encode_value(state_in->result);
        *codechar++ = '=';
        *codechar++ = '=';
        break;
    case step_C:
        *codechar++ = base64_encode_value(state_in->result);
        *codechar++ = '=';
        break;
    case step_A:
        break;
    }

    return (int)(codechar - code_out);
}

}


size_t Base64::encode(const char *src, size_t len, char* out)
{
    base64_encodestate state;
    base64_init_encodestate(&state);
    int i = base64_encode_block(src, static_cast<int>(len), out, &state);
    base64_encode_blockend(out + i, &state);
    return encodeReserverLen(len);
}

}}
