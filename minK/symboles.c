#include <mink/types.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/string.h>
#include <mink/fs.h>

static list_t *symboles;
static uintptr_t lowest_address = 0xffffffff; //kernel_base
static uintptr_t highest_address = 0;         //kernel_end

void load_symboles()
{
    int e = 0;
    fsnode_t *file = fs_open("/ksym.nm", 0, 0);
    if (!file)
    {
        assert(0 && "no symbole table");
    }

    symboles = new_list();

    char *buf = malloc(file->length);
    e = fsnode_read(file, 0, file->length, buf);
    if (e != (int)file->length)
    {
        dbgln("error:e = %d", e);
        assert(0);
    }

    size_t i = 0;
    while (i < file->length)
    {
        /**
         * sysmbols
         *   00108038 T _start
         *   00108050 T main
         *   0010c0a8 B kernel_vmdir
         */
        uintptr_t address = 0;

        for (int j = 0; j < 8; ++j)
        {
            address = (address << 4) | to_xdigit(buf[i++]);
        }
        assert(buf[i++] == ' ');
        i++;
        assert(buf[i++] == ' ');

        char *name = &buf[i];
        size_t name_len = 0;

        while (buf[i] && buf[i] != '\n')
        {
            name_len++;
            i++;
        }

        if(buf[i] == '\n')i++;

        symbole_t *s = malloc(sizeof(symbole_t));
        s->address = address;
        s->name = strndup(name, name_len);

        list_append(symboles, s);

        if (address > highest_address)
            highest_address = address;
        if (address < lowest_address)
            lowest_address = address;
    }
}

symbole_t *symbole_from_name(const char *name)
{
    if (symboles)
    {
        foreach (symboles, n)
        {
            symbole_t *s = n->value;
            if (!strcmp(s->name, name))
                return s;
        }
    }
    return NULL;
}

symbole_t *symbole_from_address(uint32_t addr)
{
    if (!symboles)
        return NULL;
    if ((addr > highest_address) || (addr < lowest_address))
        return NULL;

    foreach (symboles, n)
    {
        symbole_t *s = n->value;
        if (n->next)
        {
            symbole_t *ns = n->next->value;
            if (addr >= s->address && addr < ns->address)
            {
                return s;
            }
        }
        else
        {
            assert(addr >= s->address);
            return s;
        }
    }

    return NULL;
}
