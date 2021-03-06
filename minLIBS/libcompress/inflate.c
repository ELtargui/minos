#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <min/inflate.h>

// typedef struct inflate
// {
//     void *inparam;
//     void *outparam;
//     int (*get)(void *);
//     int (*put)(void *, int);

//     uint8_t *window;
//     int winsize;
//     int winpos;

//     uint8_t bits;
//     int bits_cnt;
// } inflate_t;

static uint8_t read_u8(inflate_t *inf)
{
    int e = inf->get(inf->inparam);
    if (e < 0)
    {
        assert(0);
        return 0;
    }

    // inf->inSize++;
    return (uint8_t)e;
}

static uint16_t read_u16(inflate_t *inf)
{
    uint16_t r = read_u8(inf);
    r |= read_u8(inf) << 8;
    return r;
}

static int read_bit(inflate_t *inf)
{
    if (inf->bits_cnt == 0)
    {
        inf->bits = read_u8(inf);
        inf->bits_cnt = 8;
    }

    int b = inf->bits & 0x01;
    inf->bits >>= 1;
    inf->bits_cnt--;
    return b;
}

static uint32_t read_bits(inflate_t *inf, int n)
{
    assert(n < 32 && "too many bits to read");
    uint32_t r = 0;
    for (int i = 0; i < n; i++)
    {
        r |= (read_bit(inf) << i);
    }

    return r;
}

static void clear_bits(inflate_t *inf)
{
    inf->bits_cnt = 0;
    inf->bits = 0;
}

static void write_u8(inflate_t *inf, uint8_t x)
{
    int e = inf->put(inf->outparam, x);
    if (e < 0)
    {
        assert(0);
        return;
    }

    //32768
    if (inf->winsize == inf->winpos)
        inf->winpos = 0;

    inf->window[inf->winpos++] = x;
}

static int uncompressed_block(inflate_t *inf)
{
    clear_bits(inf);
    int len = read_u16(inf);
    int nlen = read_u16(inf);

    if ((nlen & 0xffff) != (~len & 0xffff))
    {
        return -1;
    }

    for (int i = 0; i < len; i++)
    {
        write_u8(inf, read_u8(inf));
    }
    return 0;
}

typedef struct huffman_tree
{
    short *count;
    short *symbols;
} huffman_tree_t;

static int huffman_decode_symbole(inflate_t *inf, huffman_tree_t *tree)
{
    int code = 0;
    int count = 0;
    int first = 0;
    int index = 0;

    for (int i = 1; i < 16; i++)
    {
        code |= read_bit(inf);
        count = tree->count[i];

        if (code - count < first)
        {
            return tree->symbols[index + (code - first)];
        }

        first += count;
        index += count;
        first <<= 1;
        code <<= 1;
    }

    return -1;
}

static int inflate_codes(inflate_t *inf, huffman_tree_t *len_tree, huffman_tree_t *dist_tree)
{
    static const short litlen[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51,
                                   59, 67, 83, 99, 115, 131, 163, 195, 227, 258};

    static const short litlenExtra[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4,
                                        4, 4, 4, 5, 5, 5, 5, 0};

    static const short dists[] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385,
                                  513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

    static const short distsExtra[] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
                                       10, 11, 11, 12, 12, 13, 13};

    int len;
    int dist;

    while (1)
    {
        //decode literal/length value from input stream
        int symbole = huffman_decode_symbole(inf, len_tree);

        if (symbole < 0)
        {
            return -1;
        }
        if (symbole < 256)
        {
            //copy value (literal byte) to output stream
            write_u8(inf, symbole);
        }
        else
        {
            if (symbole == 256)
            {
                //EOB
                fprintf(stderr, "end of block\n");
                return 0;
            }
            else if (symbole >= 257 && symbole <= 285)
            {
                //decode distance from input stream
                symbole -= 257;
                if (symbole >= 29)
                {
                    return -5;
                }

                len = litlen[symbole] + read_bits(inf, litlenExtra[symbole]);

                symbole = huffman_decode_symbole(inf, dist_tree);
                if (symbole < 0)
                {
                    return -1;
                }
                dist = dists[symbole] + read_bits(inf, distsExtra[symbole]);

                int pos = 0;
                if (dist > inf->winpos)
                {
                    pos = inf->winsize - (dist - inf->winpos);
                }
                else
                {
                    pos = inf->winpos - dist;
                }

                for (int i = 0; i < len; i++)
                {
                    write_u8(inf, inf->window[pos++]);
                    if (pos == inf->winsize)
                    {
                        pos = 0;
                    }
                }
            }
        }
    }
    return 0;
}

static int build_huffman_tree(huffman_tree_t *tree, short *lengths, int cnt)
{
    for (int i = 0; i < 16; i++)
    {
        tree->count[i] = 0;
    }

    for (int i = 0; i < cnt; i++)
    {
        tree->count[lengths[i]]++;
    }

    if (tree->count[0] == cnt)
    {
        fprintf(stderr, "error : complete ?");
        assert(0);
        return -1;
    }

    short offsets[16];
    int c = 0;

    offsets[0] = 0;
    for (int i = 1; i < 16; i++)
    {
        offsets[i] = c;
        c += tree->count[i];
    }

    //sorting
    for (int i = 0; i < cnt; i++)
    {
        if (lengths[i] != 0)
        {
            tree->symbols[offsets[lengths[i]]] = i;
            offsets[lengths[i]]++;
        }
    }

    return 0;
}

static int fixed_hufman_tree_builded = 0;

static int fixed_block(inflate_t *inf)
{
    static huffman_tree_t len_tree;
    static huffman_tree_t dist_tree;

    if (!fixed_hufman_tree_builded)
    {
        static short len_count[16];
        static short dist_count[16];

        static short len_symbols[288];
        static short dist_symbols[30];

        len_tree.count = len_count;
        len_tree.symbols = len_symbols;

        dist_tree.count = dist_count;
        dist_tree.symbols = dist_symbols;

        short lengths[288];
        int i = 0;

        for (i = 0; i < 144; i++)
        {
            lengths[i] = 8;
        }
        for (; i < 256; i++)
        {
            lengths[i] = 9;
        }
        for (; i < 280; i++)
        {
            lengths[i] = 7;
        }
        for (; i < 288; i++)
        {
            lengths[i] = 8;
        }

        build_huffman_tree(&len_tree, lengths, 288);
        for (i = 0; i < 30; i++)
        {
            lengths[i] = 5;
        }
        build_huffman_tree(&dist_tree, lengths, 30);

        fixed_hufman_tree_builded = 1;
    }

    return inflate_codes(inf, &len_tree, &dist_tree);
}

static int dynamic_block(inflate_t *inf)
{
    static short len_count[16];
    static short len_sym[286];
    static short dist_count[16];
    static short dist_sym[30];

    huffman_tree_t len_tree = {.count = len_count, .symbols = len_sym};
    huffman_tree_t dist_tree = {.count = dist_count, .symbols = dist_sym};

    short nlen = read_bits(inf, 5) + 257;
    short ndist = read_bits(inf, 5) + 1;
    short ncode = read_bits(inf, 4) + 4;

    if (nlen > 286 || ndist > 30 || ncode > 19)
    {
        fprintf(stderr, "error nlen:%i, ndist:%i, ncode:%i", nlen, ndist, ncode);
        return -1;
    }

    static const short order[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    short lengths[316] = {0};

    for (int i = 0; i < ncode; i++)
    {
        lengths[order[i]] = read_bits(inf, 3);
    }

    build_huffman_tree(&len_tree, lengths, 19);

    int i = 0;
    while (i < nlen + ndist)
    {
        int symbole = huffman_decode_symbole(inf, &len_tree);
        if (symbole < 0)
        {
            fprintf(stderr, "error decoding dyn symbol [%d]\n", symbole);
            return -1;
        }

        if (symbole < 16)
        {
            lengths[i++] = symbole;
        }
        else
        {
            int len = 0;
            if (symbole == 16)
            {
                if (i == 0)
                {
                    fprintf(stderr, "error '0'");
                    return -1;
                }
                len = lengths[i - 1];
                symbole = 3 + read_bits(inf, 2);
            }
            else if (symbole == 17)
            {
                symbole = 3 + read_bits(inf, 3);
            }
            else
            {
                symbole = 11 + read_bits(inf, 7);
            }

            if (i + symbole > nlen + ndist)
            {
                fprintf(stderr, "too many symbols %d\n", i + symbole);
                return -1;
            }

            while (symbole--)
            {
                lengths[i++] = len;
            }
        }
    }

    if (lengths[256] == 0)
    {
        fprintf(stderr, "no end of block\n");
        return -1;
    }

    build_huffman_tree(&len_tree, lengths, nlen);
    if (nlen - len_tree.count[0] == 1)
    {
        fprintf(stderr, "error: nlen:%i len_tree.count[0]:%i\n", nlen, len_tree.count[0]);
        return -1;
    }

    build_huffman_tree(&dist_tree, lengths + nlen, ndist);
    if (ndist - dist_tree.count[0] == 1)
    {
        fprintf(stderr, "error: ndist:%i dist_tree.count[0]:%i\n", ndist, dist_tree.count[0]);
        return -1;
    }

    return inflate_codes(inf, &len_tree, &dist_tree);
}

int inflate(inflate_t *inf)
{
    if (!inf->window)
    {
        inf->winsize = 32768;
        inf->window = calloc(inf->winsize, sizeof(uint8_t));
    }
    int done = 0;
    do
    {
        if (read_bit(inf))
        {
            done = 1;
        }
        int type = read_bits(inf, 2);

        switch (type)
        {
        case 0:
            uncompressed_block(inf);
            break;
        case 1:
            fixed_block(inf);
            break;
        case 2:
            dynamic_block(inf);
            break;
        default:
            assert(0);
            break;
        }

    } while (done == 0);
    return 0;
}
