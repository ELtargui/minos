#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <min/inflate.h>
#include <min/gfx.h>

typedef struct png
{
    FILE *file;
    int error;

    uint8_t filter_method;
    uint8_t color_type;
    uint8_t compression;
    uint8_t interlace;

    int idat_size;
    int streamPos;
    uint8_t *idat_data;
    uint32_t idat_offset;

    int scanlineFilter;
    int linePos;
    uint8_t *scanline;
    uint8_t *prev_scanline;

    int pix;
    int px;
    int py;

    int width;
    int hieght;
    int depth;
    uint32_t *pixels;

    inflate_t inflate;
} png_t;

#define VAL_BE(a, b, c, d) ((a << 24) | (b << 16) | (c << 8) | d)

#define IHDR VAL_BE(73, 72, 68, 82)
#define IDAT VAL_BE(73, 68, 65, 84)
#define PLTE VAL_BE(80, 76, 84, 69)
#define IEND VAL_BE(73, 69, 78, 68)

static void unfilter_scanline(png_t *png);

static uint8_t read_8(png_t *png)
{
    if (png->error)
        return -1;
    int c = fgetc(png->file);
    if (c < 0)
    {
        fprintf(stderr, "error : read file, %d\n", c);
        assert(0);
        return 0;
    }

    return c;
}

/*
static uint16_t read_16be(png_t *png)
{
    uint8_t a = read_8(png);
    uint8_t b = read_8(png);

    return (a << 8) | b;
}
*/
static uint32_t read_32be(png_t *png)
{
    uint8_t a = read_8(png);
    uint8_t b = read_8(png);
    uint8_t c = read_8(png);
    uint8_t d = read_8(png);

    return (a << 24) | (b << 16) | (c << 8) | d;
}

static int png_read(void *p)
{
    png_t *png = p;
    if (png->streamPos > png->idat_size)
    {
        png->error = 1;
        assert(0 && "out of compressed data");
        return -1;
    }
    assert(png->idat_data);
    return png->idat_data[png->streamPos++];
}

static int png_write(void *ctx, int x)
{
    png_t *png = ctx;

    if ((png->px == 0) && (png->scanlineFilter == -1))
    {
        png->scanlineFilter = x;
        return 0;
    }

    png->pix++;
    png->scanline[png->linePos++] = x;

    if (png->pix == 4)
    {
        png->pix = 0;
        png->px++;

        if (png->px == png->width)
        {
            unfilter_scanline(png);
            png->linePos = 0;
            png->px = 0;
            png->py++;
        }
    }

    return 0;
}

static void png_ihdr(png_t *png, int length)
{
    png->width = read_32be(png);
    png->hieght = read_32be(png);
    png->depth = read_8(png);
    png->color_type = read_8(png);
    png->compression = read_8(png);
    png->filter_method = read_8(png);
    png->interlace = read_8(png);
    read_32be(png);
}

static void png_idat(png_t *png, int length)
{
    if (png->idat_data == NULL)
    {
        png->idat_data = malloc(png->idat_size);
        assert(png->idat_data);
    }

    uint8_t *data = png->idat_data + png->idat_offset;
    png->idat_offset += length;

    for (int i = 0; i < length; i++)
    {
        data[i] = read_8(png);
    }
    read_32be(png);
}

static void png_iend(png_t *png, int length)
{
    assert(length == 0);
    read_32be(png);
}

static void png_skipChunk(png_t *png, uint32_t type, int length)
{
    char tag[5];
    tag[3] = type & 0xff;
    tag[2] = type >> 8;
    tag[1] = type >> 16;
    tag[0] = type >> 24;
    tag[4] = 0;

    (void)tag;
    // fprintf(stderr, "skip (%s) :: %d\n", tag, length);

    for (int i = 0; i < length; i++)
        read_8(png);

    read_32be(png);
}

static void png_decode(png_t *png)
{
    //fprintf(stderr, "decoding img\n");
    png->pixels = calloc(1, png->width * png->hieght * 4);
    png->scanline = calloc(1, png->width * 4);
    png->prev_scanline = calloc(1, png->width * 4);

    png->inflate.inparam = png;
    png->inflate.outparam = png;
    png->inflate.put = png_write;
    png->inflate.get = png_read;
    png->scanlineFilter = -1;

    uint8_t cmf = png_read(png);
    uint8_t flg = png_read(png);
    if ((cmf & 0x0f) != 8)
    {
        fprintf(stderr, "inavalid compression method %i\n", cmf & 0xf);
        png->error = 1;
        goto clean;
    }
    if (flg & (1 << 5))
    {
        fprintf(stderr, "present of preset dictionary\n");
        png->error = 1;
        goto clean;
    }

    png->error = inflate(&png->inflate);

    //Zlib Adler
    png_read(png);
    png_read(png);
    png_read(png);
    png_read(png);
clean:
    free(png->scanline);
    free(png->prev_scanline);
    free(png->idat_data);
}

png_t *png_open(const char *file)
{
    //fprintf(stderr, "load :: %s\n", file);
    FILE *img = fopen(file, "rb");
    if (!img)
    {
        fprintf(stderr, "error: open\n");
        return NULL;
    }

    uint8_t pngsig[] = {137, 80, 78, 71, 13, 10, 26, 10};
    uint8_t sig[8];
    fread(sig, 8, 1, img);

    if (memcmp(pngsig, sig, 8))
    {

        fprintf(stderr, "inavalide png signature\n");
        for (int i = 0; i < 8; i++)
        {
            printf("%d|", sig[i]);
        }
        printf("\n");
        fclose(img);
        return NULL;
    }

    png_t *png = calloc(1, sizeof(png_t));
    png->file = img;

    int done = 0;
    do
    {
        if (done)
            break;
        if (feof(png->file))
            break;
        int length = read_32be(png);
        int type = read_32be(png);

        switch (type)
        {
        case IEND:
            png_skipChunk(png, type, length);
            done = 1;
            break;
        case IDAT:
            png->idat_size += length;
            png_skipChunk(png, type, length);
            break;
        default:
            png_skipChunk(png, type, length);
            break;
        }
    } while (png->error == 0);

    fseek(png->file, 8, SEEK_SET);
    done = 0;
    do
    {
        if (done)
            break;
        if (feof(png->file))
            break;
        int length = read_32be(png);
        int type = read_32be(png);

        switch (type)
        {
        case IHDR:
            png_ihdr(png, length);
            break;
        case PLTE:
            assert(0 && "implement plte");
            break;
        case IDAT:
            png_idat(png, length);
            break;
        case IEND:
            png_iend(png, length);
            done = 1;
            break;
        default:
            png_skipChunk(png, type, length);
            break;
        }
    } while (png->error == 0);

    if (png->error == 0)
    {
        png_decode(png);
    }

    fclose(png->file);

    return png;
}

void png_free(png_t *png)
{
    free(png);
}

surface_t *load_png(const char *file)
{
    png_t *png = png_open(file);
    if (!png)
        return NULL;
    surface_t *s = surface_from_buffer(png->pixels, png->width, png->hieght);
    png_free(png);
    return s;
}

#define ABS(a) ((a >= 0) ? (a) : -(a))

static uint8_t paeth(uint8_t a, uint8_t b, uint8_t c)
{
    int p = a + b - c;

    int pa = p > a ? p - a : a - p;
    int pb = p > b ? p - b : b - p;
    int pc = p > c ? p - c : c - p;

    if (pa <= pb && pa <= pc)
        return a;
    else if (pb <= pc)
        return b;
    else
        return c;
}

typedef union pixel4
{
    uint32_t raw;
    struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
} pixel4_t;

static uint32_t png_swap(pixel4_t color)
{
    uint8_t r = color.r;
    color.r = color.b;
    color.b = r;
    return color.raw;
}

static void unfilter_scanline(png_t *png)
{
    pixel4_t x, a, b, c;
    uint32_t *pixels = (uint32_t *)png->scanline;
    uint32_t *ppixels = (uint32_t *)png->prev_scanline;

    switch (png->scanlineFilter)
    {
    case 0:
        for (int i = 0; i < png->width; i++)
        {
            pixels[i] = png_swap((pixel4_t)pixels[i]);
        }
        break;
    case 1:
        pixels[0] = png_swap((pixel4_t)pixels[0]);
        for (int i = 1; i < png->width; i++)
        {
            a.raw = pixels[i - 1];
            x.raw = png_swap((pixel4_t)pixels[i]);

            x.r = x.r + a.r;
            x.g = x.g + a.g;
            x.b = x.b + a.b;
            x.a = x.a + a.a;

            pixels[i] = x.raw;
        }
        break;
    case 2:
        for (int i = 0; i < png->width; i++)
        {
            b.raw = ppixels[i];
            x.raw = png_swap((pixel4_t)pixels[i]);

            x.r = x.r + b.r;
            x.g = x.g + b.g;
            x.b = x.b + b.b;
            x.a = x.a + b.a;

            pixels[i] = x.raw;
        }
        break;
    case 3:
        for (int i = 0; i < png->width; i++)
        {
            a.raw = pixels[i - 1];
            b.raw = ppixels[i];
            x.raw = png_swap((pixel4_t)pixels[i]);

            x.r = x.r + ((a.r + b.r) / 2);
            x.g = x.g + ((a.g + b.g) / 2);
            x.b = x.b + ((a.b + b.b) / 2);
            x.a = x.a + ((a.a + b.a) / 2);

            pixels[i] = x.raw;
        }
        break;
    case 4:
    {
        a.raw = 0;
        b.raw = ppixels[0];
        c.raw = 0;
        x.raw = png_swap((pixel4_t)pixels[0]);

        x.r = x.r + paeth(a.r, b.r, c.r);
        x.g = x.g + paeth(a.g, b.g, c.g);
        x.b = x.b + paeth(a.b, b.b, c.b);
        x.a = x.a + paeth(a.a, b.a, c.a);

        pixels[0] = x.raw;

        for (int i = 1; i < png->width; i++)
        {
            a.raw = pixels[i - 1];
            b.raw = ppixels[i];
            c.raw = ppixels[i - 1];
            x.raw = png_swap((pixel4_t)pixels[i]);

            x.r = x.r + paeth(a.r, b.r, c.r);
            x.g = x.g + paeth(a.g, b.g, c.g);
            x.b = x.b + paeth(a.b, b.b, c.b);
            x.a = x.a + paeth(a.a, b.a, c.a);

            pixels[i] = x.raw;
        }
    }
    break;
    default:
        fprintf(stderr, "inavalide filter type :: %i\n", png->scanlineFilter);
        assert(0);
        break;
    }

    png->scanlineFilter = -1;
    // memcpy((void *)png->prev_scanline, (void *)png->scanline, png->width * 4);
    void *tmp = png->prev_scanline;
    png->prev_scanline =png->scanline;
    memcpy((void *)((char *)png->pixels + png->width * png->py * 4), (void *)png->scanline, png->width * 4);
    png->scanline = tmp;
}
