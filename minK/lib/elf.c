#include <mink/types.h>
#include <mink/stdio.h>
#include <mink/i386.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/fs.h>
#include <mink/elf.h>
#include <mink/string.h>
#include <errno.h>


int validate_elf_magic(void *head)
{
    Elf32_Ehdr_t *ehdr = (Elf32_Ehdr_t *)head;

    if (ehdr->e_ident[0] != ELFMAG0 ||
        ehdr->e_ident[1] != ELFMAG1 ||
        ehdr->e_ident[2] != ELFMAG2 ||
        ehdr->e_ident[3] != ELFMAG3)
    {
        return -ENOEXEC;
    }

    return 0;
}
