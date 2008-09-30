#include "file/gif/lzw.h"

static int
read_code (FILE * file, unsigned char *buf, int *bit_pos, int bit_size)
{
    int i;
    int code = 0;
    int pos = 1;

    for (i = 0; i < bit_size; i++)
    {
        int byte_pos = (*bit_pos >> 3) & 255;

        if (byte_pos == 0)
        {
            int data_len = fgetc (file);

            if (data_len == 0)
            {
                //printf ("Fatal. Errorneous GIF stream.\n");
                //abort ();
                return -1;
            }
            fread (buf + 256 - data_len, 1, data_len, file);
            byte_pos = 256 - data_len;
            *bit_pos = byte_pos << 3;
        }
        if (buf[byte_pos] & (1 << (*bit_pos & 7)))
            code += pos;
        pos += pos;
        (*bit_pos)++;
    }
    return code;
}

static void
write_code (FILE * file, unsigned char *buf, int *bit_pos, int bit_size, int code)
{
    int i;
    int pos = 1;

    for (i = 0; i < bit_size; i++)
    {
        int byte_pos = *bit_pos >> 3;

        if (code & pos)
            buf[byte_pos] |= (1 << (*bit_pos & 7));
        else
            buf[byte_pos] &= ~(1 << (*bit_pos & 7));
        (*bit_pos)++;
        if (*bit_pos == 2040)
        {
            fputc (byte_pos + 1, file);
            fwrite (buf, 1, byte_pos + 1, file);
            *bit_pos = 0;
        }
        pos += pos;
    }
}

int
LZW_decode (FILE * file,
            void (*write_pixel)(int pos, int code, unsigned char *data),
            unsigned char *data)
{
    unsigned char buf[256];
    int orig_bit_size;
    int bit_size;
    int bit_pos;
    int clear_marker;
    int end_marker;
    struct
    {
        int prefix;
        int c;
        int len;
    }
    codes[4096];                /* Maximum bit size is 12. */
    int n;
    int i, prev, code, c;
    int out_pos = 0;

    orig_bit_size = fgetc (file);
    n = 2 + (1 << orig_bit_size);

    for (i = 0; i < n; i++)
    {
        codes[i].c = i;
        codes[i].len = 0;
    }

    clear_marker = n - 2;
    end_marker = n - 1;

    bit_size = orig_bit_size + 1;

    bit_pos = 0;

    /* Expect to read clear code as first code here. */
    prev = read_code (file, buf, &bit_pos, bit_size);
    if (prev == -1 || ferror (file))
        return -1;
    do
    {
        code = read_code (file, buf, &bit_pos, bit_size);
        if (code == -1 || ferror (file))
            return -1;
        if (code == clear_marker)
        {
            bit_size = orig_bit_size;
            n = 1 << bit_size;
            n += 2;
            bit_size++;
            prev = code;
            continue;
        }

        if (code == end_marker)
            break;

        /* Known code: ok. Else: must be doubled char. */
        if (code < n)
            c = code;
        else
            c = prev;

        /* Output the code. */
        out_pos += codes[c].len;

        i = 0;
        do
        {
            if (out_pos - i < 0)
               return -1;

            write_pixel (out_pos - i, codes[c].c, data);
            if (codes[c].len)
                c = codes[c].prefix;
            else
                break;
            i++;
        }
        while (1);

        out_pos++;

        /* Unknown code -> must be double char. */
        if (code >= n)
        {
            write_pixel (out_pos, codes[c].c, data);
            out_pos++;
        }

        /* Except after clear marker, build new code. */
        if (prev != clear_marker)
        {
            codes[n].prefix = prev;
            codes[n].len = codes[prev].len + 1;
            codes[n].c = codes[c].c;
            n++;
        }

        /* Out of bits? Increase. */
        if (n == (1 << bit_size))
        {
            if (bit_size < 12)
                bit_size++;
        }

        prev = code;
    }
    while (1);
    return 0;
}

static int
get_minimum_bitsize (int (*read_pixel)(int pos, unsigned char *data),
                     int size, unsigned char *data)
{
    int i, max = 0, b = 2;
    for (i = 0; i < size; i++)
    {
        int c = read_pixel (i, data);
        if (c > max)
            max = c;
    }
    while ((1 << b) <= max)
    {
        b++;
    }
    return b;
}

void
LZW_encode (FILE * file, int (*read_pixel)(int pos, unsigned char *data),
            int size, unsigned char *data)
{
    unsigned char buf[256];
    int orig_bit_size;
    int bit_size;
    int bit_pos;
    int clear_marker;
    int end_marker;
    struct
    {
        int prefix;
        int c;
        int len;
    }
    codes[4096];                /* Maximum bit size is 12. */
    int code, prev;
    int in_pos;
    int n, i;

    orig_bit_size = get_minimum_bitsize (read_pixel, size, data);

    n = 2 + (1 << orig_bit_size);

    for (i = 0; i < n; i++)
    {
        codes[i].c = i;
        codes[i].len = 0;
    }

    clear_marker = n - 2;
    end_marker = n - 1;

    fputc (orig_bit_size, file);

    bit_size = orig_bit_size + 1;

    bit_pos = 0;

    /* Play fair and put a clear marker at the start. */
    write_code (file, buf, &bit_pos, bit_size, clear_marker);

    prev = read_pixel (0, data);

    for (in_pos = 1; in_pos < size; in_pos++)
    {
        code = read_pixel (in_pos, data);

        if (prev != clear_marker)
        {
            /* Search for the code. */
            for (i = end_marker + 1; i < n; i++)
            {
                if (codes[i].prefix == prev && codes[i].c == code)
                {
                    code = i;
                    break;
                }
            }

            /* If not found, add it, and write previous code. */
            if (i == n)
            {
                codes[n].prefix = prev;
                codes[n].len = codes[prev].len + 1;
                codes[n].c = code;
                n++;

                write_code (file, buf, &bit_pos, bit_size, prev);
            }
        }

        /* Out of bits? Increase. */
        if (n == 1 + (1 << bit_size))
        {
            if (bit_size < 12)
                bit_size++;
        }

        /* Too big table? Clear and start over. */
        if (n == 4096)
        {
            write_code (file, buf, &bit_pos, bit_size, clear_marker);
            bit_size = orig_bit_size + 1;
            n = end_marker + 1;
        }

        prev = code;
    }
    write_code (file, buf, &bit_pos, bit_size, prev);
    write_code (file, buf, &bit_pos, bit_size, end_marker);
    if (bit_pos)
    {
        int byte_pos = (bit_pos + 7) / 8;

        fputc (byte_pos, file);
        fwrite (buf, 1, byte_pos, file);
    }
}
