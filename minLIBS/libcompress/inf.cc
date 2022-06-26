#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <min/inflate.h>

typedef enum INFERR
{
    INF_OK = 0,
    INF_RE,
    INF_WE,
} InfErr_t;

typedef struct huffman
{
    short *count;
    short *symbol;
} huffman_t;

static void inf_error(inflate_t *inf, char *msg)
{
    fprintf(stderr, "inflate error %d :: %s", inf->error, msg);
    assert(0);
}

static uint8_t read_u8(inflate_t *inf)
{
    int e = inf->get(inf->inparam);
    if (e < 0)
    {
        inf->error = INF_RE;
        inf_error(inf, "stream read error");
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
    if (inf->bitsLeft == 0)
    {
        inf->bitsBuffer = read_u8(inf);
        inf->bitsLeft = 8;
    }

    int b = inf->bitsBuffer & 0x01;
    inf->bitsBuffer >>= 1;
    inf->bitsLeft--;
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

static void clear_bitsBuffer(inflate_t *inf)
{
    inf->bitsLeft = 0;
    inf->bitsBuffer = 0;
}

static void write_u8(inflate_t *inf, uint8_t x)
{
    int e = inf->put(inf->outparam, x);
    if (e < 0)
    {
        inf->error = INF_WE;
        inf_error(inf, "write u8");
        return;
    }

    //32768
    if (inf->winsize == inf->winpos)
        inf->winpos = 0;

    inf->window[inf->winpos++] = x;
    // inf->outSize++;
}

static void inf_uncompressed(inflate_t *inf)
{
    clear_bitsBuffer(inf);
    int len = read_u16(inf);
    int nlen = read_u16(inf);

    if ((nlen & 0xffff) != (~len & 0xffff))
    {
        inf->error = 0;
        inf_error(inf, "uncompressed block check");
        return;
    }

    for (int i = 0; i < len; i++)
    {
        write_u8(inf, read_u8(inf));
    }
}

int huffman_build_tree(huffman_t *tree, short *lengths, int len)
{
    for (int i = 0; i < 16; i++)
    {
        tree->count[i] = 0;
    }

    for (int i = 0; i < len; i++)
    {
        tree->count[lengths[i]]++;
    }

    if (tree->count[0] == len)
    {
        fprintf(stderr, "error : complete ?");
    }

    short offs[16];

    int c = 0;
    offs[0] = 0;
    for (int i = 1; i < 16; i++)
    {
        offs[i] = c;
        c += tree->count[i];
    }

    //sorting
    for (int i = 0; i < len; i++)
    {
        if (lengths[i] != 0)
        {
            tree->symbol[offs[lengths[i]]] = i;
            offs[lengths[i]]++;
        }
    }

    return 0;
}

static int huffman_decodeSym(inflate_t *inf, huffman_t *tree, short *symbol)
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
            *symbol = tree->symbol[index + (code - first)];
            return 0;
        }

        first += count;
        index += count;
        first <<= 1;
        code <<= 1;
    }

    inf_error(inf, "error out of codes");
    return -1;
}

static int inflateCodes(inflate_t *inf, huffman_t *len_tree, huffman_t *dist_tree)
{
    static const short litlen[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51,
                                   59, 67, 83, 99, 115, 131, 163, 195, 227, 258};

    static const short litlenExtra[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4,
                                        4, 4, 4, 5, 5, 5, 5, 0};

    static const short dists[] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385,
                                  513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

    static const short distsExtra[] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
                                       10, 11, 11, 12, 12, 13, 13};

    short symbol;
    int len;
    int dist;

    while (inf->error == 0)
    {
        //decode literal/length value from input stream

        if (huffman_decodeSym(inf, len_tree, &symbol))
        {
            inf_error(inf, "error inavalide symbole");
            return -1;
        }
        if (symbol < 256)
        {
            //copy value (literal byte) to output stream
            write_u8(inf, symbol);
        }
        else
        {
            if (symbol == 256)
            {
                //EOB
                fprintf(stderr, "end of block");
                return 0;
            }
            else if (symbol >= 257 && symbol <= 285)
            {
                //decode distance from input stream
                symbol -= 257;
                if (symbol >= 29)
                {
                    inf->error = symbol;
                    inf_error(inf, "inavalid code(symbol)");
                    return -5;
                }

                len = litlen[symbol] + read_bits(inf, litlenExtra[symbol]);

                symbol = 0;
                if (huffman_decodeSym(inf, dist_tree, &symbol))
                {
                    inf_error(inf, "tree decode error");
                    return -1;
                }
                dist = dists[symbol] + read_bits(inf, distsExtra[symbol]);

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
    return 1;
}

static void inf_fixed(inflate_t *inf)
{
    static huffman_t lenCode;
    static huffman_t distCode;

    static short len_count[16];
    static short len_sym[288];
    static short dist_count[16];
    static short dist_sym[30];

    static int build = 1;
    if (build)
    {
        lenCode.count = len_count;
        lenCode.symbol = len_sym;
        distCode.count = dist_count;
        distCode.symbol = dist_sym;

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

        huffman_build_tree(&lenCode, lengths, 288);
        for (i = 0; i < 30; i++)
        {
            lengths[i] = 5;
        }
        huffman_build_tree(&distCode, lengths, 30);

        build = 0;
    }

    inflateCodes(inf, &lenCode, &distCode);
}

static void inf_dynamic(inflate_t *inf)
{
    huffman_t lenCodes;
    huffman_t distCodes;

    static short len_count[16];
    static short len_sym[286];
    static short dist_count[16];
    static short dist_sym[30];

    lenCodes.count = len_count;
    lenCodes.symbol = len_sym;

    distCodes.count = dist_count;
    distCodes.symbol = dist_sym;

    short nlen = read_bits(inf, 5) + 257;
    short ndist = read_bits(inf, 5) + 1;
    short ncode = read_bits(inf, 4) + 4;

    if (nlen > 286 || ndist > 30 || ncode > 19)
    {
        inf->error = 5;
        fprintf(stderr, "error nlen:%i, ndist:%i, ncode:%i", nlen, ndist, ncode);
        inf_error(inf, "inavalide vals {nlen ndist ncode}");
        return;
    }

    static const short order[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    short lengths[316] = {0};

    for (int i = 0; i < ncode; i++)
    {
        lengths[order[i]] = read_bits(inf, 3);
    }

    huffman_build_tree(&lenCodes, lengths, 19);

    int i = 0;
    while (i < nlen + ndist)
    {
        short sym = 0;

        if (huffman_decodeSym(inf, &lenCodes, &sym))
        {
            inf->error = sym;
            inf_error(inf, "error decoding dyn symbol");
            return;
        }

        if (sym < 16)
        {
            lengths[i++] = sym;
        }
        else
        {
            int len = 0;
            if (sym == 16)
            {
                if (i == 0)
                {
                    fprintf(stderr, "error '0'");
                    inf_error(inf, "error dyn start");
                    return;
                }
                len = lengths[i - 1];
                sym = 3 + read_bits(inf, 2);
            }
            else if (sym == 17)
            {
                sym = 3 + read_bits(inf, 3);
            }
            else
            {
                sym = 11 + read_bits(inf, 7);
            }

            if (i + sym > nlen + ndist)
            {
                inf->error = i + sym;
                inf_error(inf, "too many symbols");
                return;
            }

            while (sym--)
            {
                lengths[i++] = len;
            }
        }
    }

    if (lengths[256] == 0)
    {
        inf->error = -1;
        inf_error(inf, "no end of block");
        return;
    }

    huffman_build_tree(&lenCodes, lengths, nlen);
    if (nlen - lenCodes.count[0] == 1)
    {
        fprintf(stderr, "error: nlen:%i lenCodes.count[0]:%i", nlen, lenCodes.count[0]);
        inf->error = 15;
        return;
    }

    huffman_build_tree(&distCodes, lengths + nlen, ndist);
    if (ndist - distCodes.count[0] == 1)
    {
        fprintf(stderr, "error: ndist:%i distCodes.count[0]:%i", ndist, distCodes.count[0]);
        inf->error = 16;
        return;
    }

    inf->error = inflateCodes(inf, &lenCodes, &distCodes);
}

int inflate(inflate_t *inf)
{
    if(!inf->window)
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
            inf_uncompressed(inf);
            break;
        case 1:
            inf_fixed(inf);
            break;
        case 2:
            inf_dynamic(inf);
            break;
        default:
            inf->error = type;
            inf_error(inf, "inavalide method");
            break;
        }

        if (inf->error)
            break;
    } while (done == 0);
    fprintf(stderr, "complet :: %d", inf->error);
    return inf->error;
}
