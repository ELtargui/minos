#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <min/gfx.h>
#include <min/truetype.h>

static uint32_t utf8_decode(int *statep, int in, uint32_t *outp)
{
    if (*statep)
    {
        if ((in & 0xc0) != 0x80)
        {
            printf("statep=%d\n", *statep);
            assert(0 && "error in multi byte cp");
        }
        // printf("multi %d\n", in);
        *outp = (*outp << 6) | (in & 0x3f);
        *statep = *statep - 1;
        return *statep;
    }
    else if (in & 0x80)
    {
        if ((in & 0xe0) == 0xc0)
        {
            *outp = in & 0x1f;
            *statep = 1;
            return *statep;
        }
        else if ((in & 0xf0) == 0xe0)
        {
            *outp = in & 0x0f;
            *statep = 2;
            return *statep;
        }
        else if ((in & 0xf8) == 0xf0)
        {
            *outp = in & 0x07;
            *statep = 3;
            return *statep;
        }
        else
        {
            printf("cp : %x (%c)\n", in, in);
            assert(0 && "inavalide cp");
        }
    }
    else
    {
        *outp = in;
    }
    return 0;
}

#define FLAG_OnCurve (1 << 0)
#define FLAG_xShort (1 << 1)
#define FLAG_yShort (1 << 2)
#define FLAG_Repeat (1 << 3)
#define FLAG_X (1 << 4)
#define FLAG_Y (1 << 5)
#define FLAG_EOC (1 << 6)

typedef struct pointf
{
    float x;
    float y;
} pointf_t;

typedef struct line
{
    pointf_t a;
    pointf_t b;
    int direction;
} line_t;

typedef struct shape
{
    line_t *lines;
    int lines_cap;
    int lines_count;
} shape_t;

typedef struct x_intersection
{
    float x;
    int flag;
} x_intersection_t;

typedef struct ttf_coord
{
    int16_t x;
    int16_t y;
    int flags;
} ttf_coord_t;

static void seek_to(ttf_t *ttf, uint32_t offset)
{
    if (offset > ttf->size)
        assert(0);

    ttf->p = ttf->buffer + offset;
}

static void move_to(ttf_t *ttf, uint8_t *p)
{
    if (p > ttf->bend)
        assert(0);
    ttf->p = p;
}

static uint32_t read_u32(ttf_t *ttf)
{
    if (ttf->p + 2 > ttf->bend)
        assert(0);

    uint32_t r = 0;
    r = (uint32_t)*ttf->p++ << 24;
    r |= (uint32_t)*ttf->p++ << 16;
    r |= (uint32_t)*ttf->p++ << 8;
    r |= (uint32_t)*ttf->p++;

    return r;
}

static uint16_t read_u16(ttf_t *ttf)
{
    uint16_t r = 0;
    if (ttf->p + 1 > ttf->bend)
    {
        assert(0);
    }
    r = (uint8_t)*ttf->p++ << 8;
    r |= (uint8_t)*ttf->p++;

    return r;
}

static int16_t read_i16(ttf_t *ttf)
{
    return (int16_t)read_u16(ttf);
}

static uint8_t read_u8(ttf_t *ttf)
{
    if (ttf->p > ttf->bend)
        assert(0);
    return *ttf->p++;
}

static float read_fword(ttf_t *ttf)
{
    return (float)read_i16(ttf) / (float)(1 << 14);
}

static int compare_intersection(const void *l, const void *r)
{

    const x_intersection_t *a = l;
    const x_intersection_t *b = r;

    if (a->x < b->x)
        return -1;
    if (a->x > b->x)
        return 1;
    return 0;
}

static int process_scanline(line_t *lines, int count, float y, x_intersection_t *intersections)
{
    int cnt = 0;
    for (int i = 0; i < count; i++)
    {
        assert(lines[i].a.y <= lines[i].b.y);

        if (y <= lines[i].a.y || y > lines[i].b.y)
            continue;
        float u = (y - lines[i].a.y) / (lines[i].b.y - lines[i].a.y);

        float x = lines[i].a.x + u * (lines[i].b.x - lines[i].a.x);
        intersections[cnt].x = x;
        intersections[cnt].flag = lines[i].direction;
        cnt++;
    }

    if (cnt)
        qsort(intersections, cnt, sizeof(x_intersection_t), compare_intersection);
    return cnt;
}

// #define ABS(a) ((a >= 0) ? (a) : -(a))

void fill_glyph(shape_t *shape, float xmin, float xmax, float ymin, float ymax, surface_t *img, uint32_t color)
{
    // color = color & 0x00ffffff;
    color_t c_rgba = color_u32(color);

    ymin = ymin < 0 ? 0 : ymin;
    ymax = ymax <= img->h ? ymax : img->h;

    xmin = xmin < 0 ? 0 : xmin;
    xmax = xmax <= img->w ? xmax : img->w;
    x_intersection_t intersections[shape->lines_count]; // = calloc(shape->lines_count, sizeof(x_intersection_t));

    // uint8_t *simples = malloc(((int)xmax - (int)xmin) * sizeof(uint8_t));

    for (int y = ymin; y < ymax; y++)
    {
        // memset(simples, 0, ((int)xmax - (int)xmin) * sizeof(uint8_t));

        float suby = y;
        for (int ys = 0; ys < 5; ys++)
        {
            int cnt = process_scanline(shape->lines, shape->lines_count, suby, intersections);
            if (cnt)
            {
                int i = 0;
                int winding = 0;
                for (int x = xmin; x < xmax; x++)
                {
                    while (i < cnt && x > intersections[i].x)
                    {
                        winding += intersections[i].flag;
                        i++;
                    }

                    if (winding != 0)
                    {
                        img->buffer[y * img->w + x] = c_rgba.raw;
                        // simples[x - (int)xmin] += 10;
                    }
                }
            }
            suby += 1.0f / 5.0f;
        }

        // for (int x = xmin; x < xmax; x++)
        // {
        //
        // }
    }

    // free(simples);
}

static void add_line(shape_t *shape, pointf_t a, pointf_t b)
{
    if (shape->lines_count >= shape->lines_cap)
    {
        shape->lines_cap += 100;
        shape->lines = realloc(shape->lines, shape->lines_cap * sizeof(line_t));
    }
    if (a.y < b.y)
    {
        shape->lines[shape->lines_count++] = (line_t){a, b, -1};
    }
    else
    {
        shape->lines[shape->lines_count++] = (line_t){b, a, 1};
    }
}

static pointf_t bezier(pointf_t p0, pointf_t ctrl, pointf_t p1, float t)
{
    //p(t) = (1-t)^2 * p0 + 2*t*(1-t)*ctrl + t^2 *p2

    float _1_m_t = 1.0 - t;
    float _1_m_t2 = _1_m_t * _1_m_t;
    float t2 = t * t;
    pointf_t p;
    p.x = _1_m_t2 * p0.x + 2.0f * t * _1_m_t * ctrl.x + t2 * p1.x;
    p.y = _1_m_t2 * p0.y + 2.0f * t * _1_m_t * ctrl.y + t2 * p1.y;
    return p;
}

static pointf_t midpoint(pointf_t a, pointf_t b)
{
    pointf_t m;
    m.x = (a.x + b.x) / 2.0f;
    m.y = (a.y + b.y) / 2.0f;

    return m;
}

static void add_curve(shape_t *shape, pointf_t a, pointf_t ctrl, pointf_t b)
{
    pointf_t m = a;
    for (int i = 1; i < 10; i++)
    {
        pointf_t n = bezier(a, ctrl, b, (float)i / 10.0f);
        add_line(shape, m, n);
        m = n;
    }
    add_line(shape, m, b);
}

static void glyph_make_shape(shape_t *shape, float scale, ttf_coord_t *coords, int npoints, int xoff, int yoff)
{
    pointf_t a, ctrl, start;
    int begin = 1;
    int in_curve = 0;

    for (int i = 0; i < npoints; i++)
    {
        pointf_t p;
        p.x = xoff + scale * coords[i].x;
        p.y = yoff + scale * -coords[i].y;

        if (begin)
        {
            if (!(coords[i].flags & FLAG_OnCurve))
            {
                printf("error\n");
                assert(0 && "start off curve");
            }
            start = p;
            a = p;
            begin = 0;
        }
        else
        {
            if (coords[i].flags & FLAG_OnCurve)
            {
                if (in_curve)
                {
                    add_curve(shape, a, ctrl, p);
                    a = p;
                    in_curve = 0;
                }
                else
                {
                    add_line(shape, a, p);
                    a = p;
                }
            }
            else
            {
                if (in_curve)
                {
                    pointf_t b = midpoint(ctrl, p);
                    add_curve(shape, a, ctrl, b);
                    a = b;
                    ctrl = p;
                }
                else
                {
                    ctrl = p;
                    in_curve = 1;
                }
            }

            if (coords[i].flags & FLAG_EOC)
            {
                if (in_curve)
                {
                    add_curve(shape, a, ctrl, start);
                }
                else
                {
                    add_line(shape, p, start);
                }
                in_curve = 0;
                begin = 1;
            }
        }
    }
}

static float glyph_htmx(ttf_t *ttf, int index)
{
    if (ttf->numOfLongHorMetrics == 1)
    {
        printf("monospaced font\n");
        assert(0 && "todo");
    }
    if (index < ttf->numOfLongHorMetrics)
    {
        move_to(ttf, ttf->hmtx + index * 4);
        return read_u16(ttf) * ttf->scale;
    }

    move_to(ttf, ttf->hmtx + 4 * (ttf->numOfLongHorMetrics - 1));
    return read_u16(ttf) * ttf->scale;
}

static int glyph_index(ttf_t *ttf, int cp)
{
    if (!ttf->camp_subtable)
        return 0; //missing
    move_to(ttf, ttf->cmap + ttf->camp_subtable);
    int format = read_u16(ttf);

    switch (format)
    {
    case 4:
    {
        /*length*/ read_u16(ttf);
        /*language*/ read_u16(ttf);
        int segCount = read_u16(ttf) / 2;

        read_u16(ttf);
        read_u16(ttf);
        read_u16(ttf);

        uint8_t *endCode_base = ttf->p;
        uint8_t *startCode_base = endCode_base + segCount * 2 + 2;
        for (int i = 0; i < segCount; i++)
        {
            move_to(ttf, endCode_base + i * 2);
            uint16_t endCode = read_u16(ttf);
            uint16_t startCode;
            if (endCode >= cp)
            {
                move_to(ttf, startCode_base + i * 2);
                startCode = read_u16(ttf);
                if (startCode <= cp)
                {
                    move_to(ttf, startCode_base + segCount * 2 + i * 2);
                    int16_t idDelta = read_u16(ttf);

                    move_to(ttf, startCode_base + segCount * 4 + i * 2);
                    int idRangeOffset = read_u16(ttf);

                    if (idRangeOffset)
                    {
                        move_to(ttf, (startCode_base + segCount * 4 + i * 2) + idRangeOffset + (cp - startCode) * 2);
                        return read_u16(ttf);
                    }

                    return idDelta + cp;
                }
            }
        }
    }
    break;
    default:
        fprintf(stderr, "unsuported camp format:%d\n", format);
        return 0;
    }
    fprintf(stderr, "missing cp:%d(%c)\n", cp, cp);
    return 0;
}

static uint32_t ttf_glyph_location(ttf_t *ttf, uint32_t glyf)
{
    uint32_t offset = 0;
    if (ttf->indexToLocFormat)
    {
        move_to(ttf, ttf->loca + 4 * glyf);
        offset = read_u32(ttf);
    }
    else
    {
        move_to(ttf, ttf->loca + 2 * glyf);
        offset = read_u16(ttf) * 2;
    }

    return offset;
}

static int load_glyph(ttf_t *ttf, uint32_t gid, ttf_coord_t **pcoords, float *xmin, float *xmax, float *ymin, float *ymax)
{
    uint32_t glyf = ttf_glyph_location(ttf, gid);
    if (ttf_glyph_location(ttf, gid + 1) == glyf)
        return 0;
    move_to(ttf, ttf->glyf + glyf);
    int16_t numberOfContours = read_i16(ttf);

    *xmin = (float)read_i16(ttf);
    *ymin = -(float)read_i16(ttf);
    *xmax = (float)read_i16(ttf);
    *ymax = -(float)read_i16(ttf);
    if (*ymin > *ymax)
    {
        float t = *ymax;
        *ymax = *ymin;
        *ymin = t;
    }

    if (numberOfContours > 0)
    {
        uint16_t endPtsOfContours[numberOfContours + 1];
        for (int16_t i = 0; i < numberOfContours; i++)
        {
            endPtsOfContours[i] = read_u16(ttf);
        }
        uint16_t instructionLength = read_u16(ttf);
        move_to(ttf, ttf->p + instructionLength * sizeof(uint8_t));
        int npoints = endPtsOfContours[numberOfContours - 1] + 1;
        if (npoints == 0)
            return 0;
        ttf_coord_t *coords = calloc(npoints, sizeof(ttf_coord_t));

        int id = 0;
        while (id < npoints)
        {
            uint8_t f = read_u8(ttf);
            coords[id++].flags = f;

            if (f & FLAG_Repeat)
            {
                uint8_t rep = read_u8(ttf);
                while (rep)
                {
                    coords[id++].flags = f;
                    rep--;
                }
            }
        }

        int prev = 0;
        id = 0;
        while (id < npoints)
        {
            uint8_t flag = coords[id].flags;
            int x;
            if (flag & FLAG_xShort)
            {
                if (flag & FLAG_X)
                {
                    x = prev + read_u8(ttf);
                }
                else
                {
                    x = prev - read_u8(ttf);
                }
            }
            else
            {
                if (flag & FLAG_X)
                {
                    x = prev;
                }
                else
                {
                    int16_t dx = read_u16(ttf);
                    x = prev + dx;
                }
            }
            coords[id++].x = x;
            prev = x;
        }

        prev = 0;
        id = 0;
        while (id < npoints)
        {
            uint8_t flag = coords[id].flags;
            int y;
            if (flag & FLAG_yShort)
            {
                if (flag & FLAG_Y)
                {
                    y = prev + read_u8(ttf);
                }
                else
                {
                    y = prev - read_u8(ttf);
                }
            }
            else
            {
                if (flag & FLAG_Y)
                {
                    y = prev;
                }
                else
                {
                    int16_t dy = read_u16(ttf);
                    y = prev + dy;
                }
            }
            coords[id++].y = y;
            prev = y;
        }
        for (int i = 0; i < numberOfContours; i++)
        {
            coords[endPtsOfContours[i]].flags |= FLAG_EOC;
        }
        *pcoords = coords;
        return npoints;
    }

    if (numberOfContours < 0)
    {
        *pcoords = NULL;
        printf("todo compound glyph\n");
        assert(0);
    }

    return 0;
}

surface_t *ttf_init_ctx(ttf_t *ttf, void *buffer, int w, int h)
{
    ttf->surface = surface_from_buffer(buffer, w, h);
    return ttf->surface;
}

void ttf_set_size(ttf_t *ttf, float size)
{
    ttf->scale = (size * 96) / (72.0f * ttf->unitsPerEm);
}

int ttf_text_width(ttf_t *ttf, const char *s)
{
    const uint8_t *c = (const uint8_t *)s;
    int ut8_state = 0;
    int w = 0;
    while (*c)
    {
        uint32_t cp = 0;
        if (!utf8_decode(&ut8_state, *c, &cp))
        {
            w += glyph_htmx(ttf, glyph_index(ttf, cp));
        }
        c++;
    }
    return w + 1;
}

int ttf_draw_text(ttf_t *ttf, const char *s, int x, int y, uint32_t color)
{
    const uint8_t *c = (const uint8_t *)s;
    int ut8_state = 0;
    int xi = x;
    while (*c)
    {
        uint32_t cp = 0;
        if (!utf8_decode(&ut8_state, *c, &cp))
        {
            int index = glyph_index(ttf, cp);
            ttf_draw_glyph_index(ttf, index, x, y, color);
            x += glyph_htmx(ttf, index);
        }
        c++;
    }
    return x - xi;
}

int ttf_draw_glyph(ttf_t *ttf, uint32_t cp, int x, int y, uint32_t color)
{
    int index = glyph_index(ttf, cp);
    if (index < 0)
    {
        printf("error\n");
        return 0;
    }
    return ttf_draw_glyph_index(ttf, index, x, y, color);
}

int ttf_draw_glyph_index(ttf_t *ttf, uint32_t index, int x, int y, uint32_t color)
{
    ttf_coord_t *coords = NULL;
    float xmin, xmax, ymin, ymax;
    int np = load_glyph(ttf, index, &coords, &xmin, &xmax, &ymin, &ymax);
    if (!np)
    {
        // printf("no countours\n");
        return 0;
    }
    assert(coords);

    xmin = x + ttf->scale * xmin - 2;
    ymin = y + ttf->scale * ymin - 2;
    xmax = x + ttf->scale * xmax + 2;
    ymax = y + ttf->scale * ymax + 2;

    shape_t shape;
    shape.lines = NULL;
    shape.lines_cap = 0;
    shape.lines_count = 0;

    glyph_make_shape(&shape, ttf->scale, coords, np, x, y);
    fill_glyph(&shape, xmin, xmax, ymin, ymax, ttf->surface, color);
    if (shape.lines)
        free(shape.lines);
    free(coords);
    return 0;
}

ttf_t *ttf_load(const char *font)
{
    int fd = open(font, O_RDONLY);
    if (fd < 0)
    {
        perror("open font file");
        return NULL;
    }

    lseek(fd, 0, SEEK_END);
    size_t size = (size_t)lseek(fd, 0, SEEK_CUR);
    if (size == (size_t)-1)
    {
        perror("lseek");
        return NULL;
    }

    lseek(fd, 0, SEEK_SET);

    uint8_t *buffer = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (buffer == MAP_FAILED)
    {
        perror("mmap file");
        return NULL;
    }

    ttf_t *ttf = calloc(1, sizeof(ttf_t));
    ttf->buffer = buffer;
    ttf->size = size;
    ttf->bend = buffer + size;
    seek_to(ttf, 0);

    int type = read_u32(ttf);

    if (type != 0x00010000 && type != 0x74727565)
    {
        printf("not a turetype [%x]\n", type);
        assert(0);
    }

    int ntables = read_u16(ttf);
    /* uint16_t searchRange =  */ read_u16(ttf);
    /* uint16_t entrySelector = */ read_u16(ttf);
    /* uint16_t rangeShift =  */ read_u16(ttf);

    for (int i = 0; i < ntables; i++)
    {
        uint32_t tag = read_u32(ttf);
        uint32_t check = read_u32(ttf);
        uint32_t offset = read_u32(ttf);
        uint32_t length = read_u32(ttf);

        (void)check;

        switch (tag)
        {
        case 0x68656164: /* head */
            ttf->head = ttf->buffer + offset;
            break;
        case 0x636d6170: /* cmap */
            ttf->cmap = ttf->buffer + offset;
            ttf->cmap_len = length;
            break;
        case 0x676c7966: /* glyf */
            ttf->glyf = ttf->buffer + offset;
            ttf->glyf_len = length;
            break;
        case 0x6c6f6361: /* loca */
            ttf->loca = ttf->buffer + offset;
            ttf->loca_len = length;
            break;
        case 0x68686561: /* hhea */
            ttf->hhea = ttf->buffer + offset;
            break;
        case 0x686d7478: /* hmtx */
            ttf->hmtx = ttf->buffer + offset;
            break;
        case 0x6e616d65: /* name */
            ttf->name = ttf->buffer + offset;
            break;
        default:
            break;
        }
    }

    if (!ttf->head ||
        !ttf->cmap ||
        !ttf->glyf ||
        !ttf->loca ||
        !ttf->hhea ||
        !ttf->hmtx ||
        !ttf->name)
    {
        printf("error : missing a required table\n");
        assert(0);
    }
    move_to(ttf, ttf->head);

    /* uint32_t v =  */ read_u32(ttf);
    /* uint32_t fontRevision =  */ read_u32(ttf);
    /* uint32_t checkSumAdjustment = */ read_u32(ttf);

    uint32_t magicNumber = read_u32(ttf);
    assert(magicNumber == 0x5F0F3CF5);
    /* uint16_t flags = */ read_u16(ttf);

    ttf->unitsPerEm = 1.0f * read_u16(ttf);
    // uint32_t time;
    /* time = */ read_u32(ttf);
    /* time = */ read_u32(ttf);
    /* time = */ read_u32(ttf);
    /* time = */ read_u32(ttf);

    /* uint16_t xMin = */ read_fword(ttf);
    /* uint16_t yMin =  */ read_fword(ttf);
    /* uint16_t xMax =  */ read_fword(ttf);
    /*  uint16_t yMax =  */ read_fword(ttf);
    /* uint16_t macStyle = */ read_u16(ttf);
    /* uint16_t lowestRecPPEM = */ read_u16(ttf);
    /*  ttf->fontDirectionHint = */ read_i16(ttf);
    ttf->indexToLocFormat = read_i16(ttf);
    /* int16_t glyphDataFormat = */ read_i16(ttf);

    move_to(ttf, ttf->cmap);

    /*cmap version */ read_u16(ttf);
    int num_subtables = read_u16(ttf);
    for (int i = 0; i < num_subtables; i++)
    {
        uint16_t platformID = read_u16(ttf);
        uint16_t platformSpecificID = read_u16(ttf);
        uint32_t offset = read_u32(ttf);

        if (platformID == 3)
        {
            if (platformSpecificID == 10 || platformSpecificID == 1)
            {
                ttf->camp_subtable = offset;
                break;
            }
        }
    }

    move_to(ttf, ttf->hhea + 2 * 17);
    ttf->numOfLongHorMetrics = read_u16(ttf);

    return ttf;
}
