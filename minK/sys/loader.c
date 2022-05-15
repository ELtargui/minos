#include <mink/types.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/fs.h>
#include <mink/process.h>
#include <mink/region.h>
#include <mink/string.h>
#include <mink/elf.h>
#include <errno.h>

static uintptr_t push_u32(uintptr_t esp, uintptr_t ui)
{
    esp -= 4;
    *(uintptr_t *)esp = ui;
    return esp;
}

static uintptr_t push_string(uintptr_t esp, char *str)
{
    int len = strlen(str) + 1;
    esp -= len;
    if (esp % 16)
    {
        esp -= 32;
        esp &= 0xfffffff0;
    }

    memcpy((void *)esp, str, len);
    return esp;
}

const char *basename(const char *path)
{
    const char *p = path;
    const char *name = p + 1;
    while (*p)
    {
        if (*p == '/' && p[1])
            name = p + 1;
        p++;
    }
    return name;
}

int execve(const char *filename)
{
    fsnode_t *file = fs_open(filename, 0, 0);
    if (!file)
    {
        return -ENOENT;
    }
    int e = -1;
    Elf32_Ehdr_t ehdr;
    if ((e = fsnode_read(file, 0, sizeof(Elf32_Ehdr_t), &ehdr)) != sizeof(Elf32_Ehdr_t))
    {
        return e;
    }

    e = validate_elf_magic(&ehdr);
    if (e)
    {
        return e;
    }

    if (!ehdr.e_phnum)
    {
        dbgln("ehdr.e_phnum:%d type:%d @:%p", ehdr.e_phnum, ehdr.e_type, ehdr.e_entry);
        assert(0 && "no program header tables");
    }

    Elf32_Phdr_t Phdr[ehdr.e_phnum];
    e = fsnode_read(file, ehdr.e_phoff, sizeof(Elf32_Phdr_t) * ehdr.e_phnum, &Phdr);
    if (e != (int)sizeof(Elf32_Phdr_t) * ehdr.e_phnum)
    {
        return e;
    }

    process_t *process = current_process();
    process->bin.entry = ehdr.e_entry;
    process->bin.path = strdup(filename);

    process_reset_address_space(process);

    for (int i = 0; i < ehdr.e_phnum; i++)
    {
        if (Phdr[i].p_type != PT_LOAD)
        {
            assert(0 && "not a PT_LOAD");
        }

        uint32_t size = Phdr[i].p_memsz;
        if (size % PAGESIZE)
        {
            size &= PAGEMASK;
            size += PAGESIZE;
        }

        region_t *region = alloc_region(current_process(), Phdr[i].p_vaddr & PAGEMASK, size, True);
        assert(region);
        region_set_name(region, "exec");
        region_map(current_process(), region, 1, 1, 1, 1);

        e = fsnode_read(file, Phdr[i].p_offset, Phdr[i].p_filesz, (void *)Phdr[i].p_vaddr);
        if (e != (int)Phdr[i].p_filesz)
            assert(0);

        size_t r = Phdr[i].p_filesz;
        while (r < Phdr[i].p_memsz)
        {
            *(char *)(Phdr[i].p_vaddr + r) = 0;
            r++;
        }
    }

    region_t *rstack = alloc_region(current_process(), USER_STACK_BASE, USER_STACK_SIZE, True);
    assert(rstack);
    region_map(current_process(), rstack, 1, 1, 1, 0);
    memset((void*)rstack->base, 0, rstack->size);

    uintptr_t esp = USER_STACK_BASE + USER_STACK_SIZE;

    if (process->envp->size)
    {
        TODO(environ);
    }

    for (int i = 0; i < process->argv->size; i++)
    {
        esp = push_u32(esp, 0);
    }

    char **_args = (char **)esp;
    int i = 0;
    foreach (process->argv, n)
    {
        esp = push_string(esp, n->value);
        _args[i++] = (char *)esp;
    }

    // esp = push_u32(esp, 0xffffbeef);
    // esp = push_u32(esp, 0);
    esp = push_u32(esp, (uint32_t)0); //envp
    esp = push_u32(esp, (uint32_t)_args);//argv
    esp = push_u32(esp, (uint32_t)i);//argc

    process_set_name(process, strdup(basename(process->bin.path)));

    cpu_set_kernel_stack((uintptr_t)current_thread()->cpu.stack + STACK_SIZE);
    enter_user_space(process->bin.entry, esp);
    assert(0);

    return -1;
}
