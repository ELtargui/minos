#include <mink/types.h>
#include <mink/multiboot.h>
#include <mink/stdio.h>
#include <mink/i386.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/fs.h>
#include <mink/process.h>
#include <mink/string.h>

extern void install_ramdisk();
extern void install_ext2fs();
extern void syscall_install();
extern void sched_switch_to(thread_t *thread);
extern fsnode_t *dbg_fsnode();
extern void mouse_install();
extern void kbd_install();
extern void fb_install(vbe_info_t *info);
extern void ipc_install();
extern void null_install();

int init_putc(void *stream, int c)
{
    (void)stream;
    while (!(inportb(0x3F8 + 5) & 0x20))
    {
    }
    outportb(0x3F8, c);
    return 1;
}

void init(void *args)
{
    (void)args;
    interrupt_enable();
    interrupt_resume();
    assert(current_process() == init_process);
    dbgln("preparing init...");
    assert(init_process->argv->head);
    list_t *argv = init_process->argv;
    // list_t *envp = init_process->envp;
    execve(argv->head->value);
    assert(0 && "failed to run init");
}

void main(uint32_t magic, void *bootinfo, uint32_t stack)
{
    (void)magic;
    (void)stack;
    set_std_output(NULL, init_putc);
    interrupt_disable();

    printf("starting minOS\n");
    dbgln("magic:%x stack:%p\n", magic, stack);

    cpu_init();
    install_mem(bootinfo);

    dbgln("hello world!");

    fs_install();

    install_ramdisk();
    install_ext2fs();

    fs_mount("ext2fs", "/dev/ramdisk", "/", 0);

    load_symboles();

    process_install();
    shm_install();

    syscall_install();
    kbd_install();
    mouse_install();
    fb_install((vbe_info_t *)((multiboot_t *)bootinfo)->vbe_mode_info);
    ipc_install();
    null_install();

    vfs_bind("/dev/dbg", dbg_fsnode(), 0666);

    start_pit(1000);
    list_append(init_process->argv, strdup("/bin/init"));
    list_append(init_process->argv, strdup("world!"));
    sched_switch_to(init_process->mainthread);

    assert(0 && "not reached");
}
