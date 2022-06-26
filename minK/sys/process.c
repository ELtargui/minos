#include <mink/types.h>
#include <mink/i386.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/string.h>
#include <mink/process.h>
#include <mink/region.h>
#include <mink/fs.h>
#include <errno.h>

extern void init(void *args);
extern thread_t *finalizer;

list_t *process_table;
list_t *exited_threads_queue;
list_t *exited_process_queue;
process_t *init_process;

void process_set_name(process_t *process, const char *name)
{
    if (process->name)
    {
        free(process->name);
    }

    if (name)
        process->name = strdup(name);
    else
        process->name = NULL;
}

void thread_set_name(thread_t *thread, const char *name)
{
    if (thread->name)
    {
        free(thread->name);
    }

    if (name)
        thread->name = strdup(name);
    else
        thread->name = NULL;
}

static int next_id()
{
    static int __id = 1;
    return __id++;
}

process_t *new_process(process_t *parent)
{
    process_t *p = calloc(1, sizeof(process_t));

    p->pid = next_id();

    if (parent)
    {
        process_set_name(p, parent->name);
        p->argv = list_clone(parent->argv, (void *(*)(void *))strdup);
        p->envp = list_clone(parent->envp, (void *(*)(void *))strdup);

        list_append(parent->childs, p);
    }
    else
    {
        p->argv = new_list();
        p->envp = new_list();
    }

    p->childs = new_list();
    p->threads = new_list();
    p->regions = new_list();
    p->wait_queue = new_list();

    region_t *region = calloc(1, sizeof(region_t));
    region->base = 0x40000000;
    region->size = 0x80000000;
    region->free = True;
    list_append(p->regions, region);

    p->vmdir = vm_new_vmdir();

    p->files.cap = 0;
    p->files.count = 0;
    p->parent = parent;
    list_append(process_table, p);
    return p;
}

void free_process(process_t *process)
{
    dbgln("free process [%d:%s]", process->pid, process->name);
    process_set_name(process, NULL);
    list_remove(process->parent->childs, process);
    list_delete_all(process->argv, free);
    list_delete_all(process->envp, free);
    free(process->bin.path);
    free(process->argv);
    free(process->envp);
    free(process->wait_queue);

    assert(process->childs->size == 0);
    free(process->childs);
    assert(process->threads->size == 0);
    free(process->threads);

    assert(process->regions->size == 1);
    region_t *region = list_take_first(process->regions);
    assert(region);
    region_set_name(region, NULL);
    free(region);
    free(process->regions);

    free(process->vmdir);
    free(process->files.fds);
    free(process->files.flags);
    list_remove(process_table, process);
    free(process);
}

thread_t *new_thread(process_t *process, int id, void (*start)(void *), void *arg)
{
    thread_t *thread = calloc(1, sizeof(thread_t));
    assert(thread);
    thread->id = id;
    dbgln("thread:%p id:%d", thread, id);
    thread->sched_node.value = thread;

    uintptr_t sp = (uintptr_t)valloc(STACK_SIZE);
    thread->cpu.stack = (void *)sp;
    sp += STACK_SIZE;
    thread->cpu.ebp = sp;
    sp -= sizeof(void *);
    *(uintptr_t *)sp = 0;
    sp -= sizeof(void *);
    *(uintptr_t *)sp = (uintptr_t)arg;
    sp -= sizeof(void *);
    *(uintptr_t *)sp = 0;
    thread->cpu.esp = sp;
    thread->cpu.eip = (uintptr_t)start;

    if (process)
    {
        thread->cpu.vmdir = process->vmdir;
        thread->process = process;
        list_append(process->threads, thread);
        thread->joinable = True;
    }
    else
    {
        thread->cpu.vmdir = kernel_vmdir;
    }

    thread->priority = 30;
    thread->state = THREAD_READY;

    return thread;
}

void thread_free(thread_t *thread)
{
    dbgln("free thread [%d:%s]", thread->id, thread->name);
    assert(thread != current_thread());
    if (thread->process)
    {
        assert(list_remove(thread->process->threads, thread) == 0);
    }
    free(thread->cpu.stack);
    thread_set_name(thread, NULL);
    free(thread);
}

void finalizer_add_thread(thread_t *thread)
{
    if (thread != current_thread())
        assert(thread->state == THREAD_EXITED);

    list_append(exited_threads_queue, thread);
    thread->state = THREAD_EXITED;
    if (finalizer->state == THREAD_BLOCKED)
        sched_run(finalizer);
    if (current_thread() == thread)
        sched_yield();
}

void finalizer_add_process(process_t *process)
{
    list_append(exited_process_queue, process);
    if (finalizer->state == THREAD_BLOCKED)
        sched_run(finalizer);
}

void finalizer_main()
{
    while (1)
    {
        while (exited_threads_queue->head)
        {
            thread_t *thread = list_take_last(exited_threads_queue);
            while (thread->state != THREAD_EXITED)
                sched_yield();
            thread_free(thread);
        }

        while (exited_process_queue->head)
        {
            process_t *process = list_take_last(exited_process_queue);
            free_process(process);
        }

        sched_block_on(NULL, THREAD_BLOCKED);
    }
}

void process_install()
{
    process_table = new_list();
    init_process = new_process(NULL);
    exited_threads_queue = new_list();
    exited_process_queue = new_list();
    init_process->mainthread = new_thread(init_process, init_process->pid, init, (void *)0xC001C0DE);
    sched_init();
}

process_t *process_from_pid(int pid)
{
    foreach (process_table, n)
    {
        process_t *p = n->value;
        if (p->pid == pid)
            return p;
    }

    return NULL;
}

thread_t *thread_from_id(process_t *process, int id)
{
    foreach (process->threads, n)
    {
        thread_t *th = n->value;
        if (th->id == id)
            return th;
    }
    return NULL;
}

void process_reset_address_space(process_t *process)
{
    dbgln("process:%d:%s", process->pid, process->name);

    foreach (process->regions, n)
    {
        region_t *region = n->value;

        if (!region->free)
        {
            region_free_impl(process, region);
        }
    }

    merge_free_regions(process->regions);
    inavalidate_all();
    dbgln("ok");
}

int process_mmap(process_t *process, uintptr_t addr, size_t len,
                 int prot, int flags,
                 int fd, off_t offset, const char *name)
{

    if (flags & ~(MAP_SHARED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_LAZY))
    {
        dbgln("inavalide flags:%d", flags);
        return -EINVAL;
    }

    if (flags & MAP_FIXED && (addr % PAGESIZE))
    {
        dbgln("inavalide addr align:%p", addr);
        return -EINVAL;
    }

    if (addr % PAGESIZE)
    {
        addr &= PAGEMASK;
        addr += PAGESIZE;
    }

    if (len == 0)
    {
        dbgln("inavalide len:%d", len);
        return -EINVAL;
    }

    if (len % PAGESIZE)
    {
        len &= PAGEMASK;
        len += PAGESIZE;
    }

    if (prot != PROT_NONE &&
        !(prot & PROT_READ) &&
        !(prot & PROT_WRITE) &&
        !(prot & PROT_EXEC))
    {
        dbgln("inavalide prot:%d", prot);
        return -EINVAL;
    }

    filedescriptor_t *file = NULL;
    if (fd != -1)
    {
        file = fd_get(process, fd);
        if (!file)
            return -EBADF;
    }

    if (flags & MAP_ANONYMOUS)
    {
        assert(fd == -1);
        if (flags & MAP_SHARED)
        {
            TODO(shared);
        }
    }

    region_t *region = alloc_region(process, addr, len, flags & MAP_FIXED ? 1 : 0);

    if (!region)
    {
        return -ENOMEM;
    }

    if (prot & PROT_NONE)
    {
        //XXX: is that right
        // TODO(PROT_NONE);
        region->user = 0;
        region->readable = 0;
        region->writable = 0;
        region->executable = 0;
        inavalidate_region(region->base, region->size);

        return region->base;
    }

    region->user = 1;
    region->readable = 1;
    region->writable = prot & PROT_WRITE ? 1 : 0;
    region->executable = prot & PROT_EXEC ? 1 : 0;

    region_set_name(region, name);
    if (file)
    {
        int e = file_map(fd, region, offset, len);
        if (e)
        {
            dbgln("failed to map file %d(%s)", fd, file->node->name);
            region_free_impl(process, region);
            return e;
        }

        inavalidate_region(region->base, region->size);
        return region->base;
    }
    assert(flags & MAP_ANONYMOUS);
    region_map(process, region, 1, prot & PROT_READ, prot & PROT_WRITE, prot & PROT_EXEC);
    // dbgln("mmap [%p:%x]", region->base, region->size);
    return region->base;
}

int process_unmap(uintptr_t addr, size_t size)
{
    region_t *region = get_region(current_process(), addr);
    if (!region)
        return -EINVAL;
    if (region->free)
        return -EINVAL;
    if (region->user == 0)
        return -EINVAL;

    // dbgln("free %p:%x from region [%p:%x]", addr, size, region->base, region->size);
    region_free_impl(current_process(), region);
    merge_free_regions(current_process()->regions);
    return 0;
}

void return_to_userspace()
{
    // dbgln("xxx");
    interrupt_enable();
}

int fork()
{
    interrupt_disable();
    process_t *parent = current_process();
    assert(parent);

    process_t *child = new_process(parent);

    foreach (parent->regions, n)
    {
        region_t *r = n->value;
        if (!r->free)
        {
            if (region_child_cow(parent, child, r))
            {
                ERROR(region_child_cow);
            }
        }
    }

    child->bin = parent->bin;

    child->files.cap = parent->files.cap;
    child->files.count = parent->files.count;
    child->files.fds = calloc(child->files.cap, sizeof(filedescriptor_t));
    child->files.flags = calloc(child->files.cap, sizeof(int));

    for (int i = 0; i < parent->files.cap; i++)
    {
        if (parent->files.fds[i])
        {
            child->files.fds[i] = file_clone(parent, i);
            child->files.flags[i] = parent->files.flags[i];
        }
    }

    thread_t *thread = new_thread(child, child->pid, NULL, NULL);
    child->mainthread = thread;

    thread->cpu.eip = (uint32_t)return_to_userspace;
    uintptr_t sp = (uintptr_t)thread->cpu.stack + STACK_SIZE;

    sp -= sizeof(regs_t);
    thread->cpu.ebp = sp;
    regs_t *r = (regs_t *)sp;
    sp -= sizeof(void *);
    *(uintptr_t *)sp = (uintptr_t)return_to_user;
    thread->cpu.esp = sp;
    thread->cpu.r = r; //(regs_t *)thread->cpu.esp;

    memcpy(thread->cpu.r, current_thread()->cpu.r, sizeof(regs_t));
    thread->cpu.r->eax = 0; //return 0 to child

    sched_run(thread);
    interrupt_resume();
    return child->pid;
}

int waitpid(int pid, int *wstatus, int options)
{
    dbgln("waitpid(%d, %p, %x)", pid, wstatus, options);
    process_t *process = current_process();
    assert(process);
    if (process->childs->size == 0)
    {
        dbgln("no childs");
        return -ECHILD;
    }

    process_t *child = NULL;
    while (child == NULL)
    {
        foreach (process->childs, n)
        {
            process_t *p = n->value;

            if (pid < -1 && p->gid != -pid)
                continue;
            if (pid == 0 && p->gid != process->gid)
                continue;
            if (pid > 0 && p->pid != process->pid)
                continue;
            // if(pid == -1)

            if (!p->state.have_state)
            {
                continue;
            }

            if (options & WCONTINUED && !(p->state.continued))
            {
                continue;
            }

            if ((options & WUNTRACED) && p->state.traced && !p->state.stoped)
            {
                continue;
            }

            if (!p->state.exited)
                continue;

            child = p;
            break;
        }

        if (!child)
        {
            if (options & WNOHANG)
            {
                return 0;
            }

            process->waitingthread = current_thread();
            int e = sched_block_on(NULL, THREAD_BLOCKED);
            process->waitingthread = NULL;
            if (e)
            {
                return -EINTR;
            }
        }
    }
    assert(child);

    int ret = child->pid;

    if (wstatus)
    {
        *wstatus = child->exit_status;
    }

    if (child->state.exited)
    {
        free_process(child);
    }
    else
    {
        child->state.have_state = 0;
        child->state.exited = 0;
        child->state.continued = 0;
        child->state.stoped = 0;
        child->state.traced = 0;
    }

    return ret;
}

void thread_exit(thread_t *thread)
{
    if (thread == current_thread())
        thread_exit_self();
    sched_exit(thread);
    thread->state = THREAD_EXITED;
    thread_free(thread);
}

void thread_exit_self()
{
    if (current_process())
    {
        assert(list_remove(current_process()->threads, current_thread()) == 0);
        current_thread()->process = NULL;
    }
    finalizer_add_thread(current_thread());
    sched_yield();
    assert(0);
}

void process_exit(process_t *process, int status)
{
    assert(process);
    process->exit_status = status;

    if (process == init_process)
        assert(0 && "exit init process");

    list_node_t *node = process->threads->head;
    while (node)
    {
        list_node_t *next = node->next;
        thread_t *thread = node->value;
        if (thread != current_thread())
        {
            thread_exit(thread);
        }
        node = next;
    }

    process_reset_address_space(process);

    while (process->childs->head)
    {
        node = list_take_first_node(process->childs);
        list_append_node(init_process->childs, node);
    }

    process->state.have_state = True;
    process->state.exited = 1;

    process_t *parent = process->parent;
    assert(parent);
    if (parent->waitingthread)
    {
        sched_run(parent->waitingthread);
    }

    if (current_thread()->process == process)
    {
        thread_exit_self();
        assert(0);
    }
}

static uintptr_t push_u32(uintptr_t esp, uintptr_t ui)
{
    esp -= 4;
    *(uintptr_t *)esp = ui;
    return esp;
}

int spawn_thread(uintptr_t funp, uintptr_t argp, uintptr_t stack, size_t stacksize)
{
    process_t *process = current_process();
    assert(process);
    assert(process->mainthread);

    thread_t *thread = new_thread(process, next_id(), NULL, NULL);

    thread->cpu.eip = (uint32_t)return_to_user;
    uintptr_t esp = (uintptr_t)thread->cpu.stack + STACK_SIZE;
    esp -= sizeof(uintptr_t);
    thread->cpu.ebp = esp;
    thread->cpu.esp = esp - sizeof(regs_t);
    thread->cpu.r = (regs_t *)thread->cpu.esp;

    *thread->cpu.r = *current_thread()->cpu.r;

    region_t *region = get_region(process, stack);
    assert(region);
    assert(region->free == 0);
    assert(region->size >= stacksize);
    assert(region->base == stack);
    region_set_name(region, "thread stack");

    esp = stack + stacksize;

    thread->cpu.r->eip = funp;
    thread->cpu.r->ebp = esp;

    esp = push_u32(esp, argp);
    esp = push_u32(esp, 0xfff45678);

    thread->cpu.r->uesp = esp;
    thread->cpu.r->esp = esp;

    thread_set_name(thread, "thread");
    sched_run(thread);

    return thread->id;
}

int thread_join(thread_t *thread)
{
    if (thread == current_thread())
        return -EDEADLK;

    if (!thread->joinable)
        return -EINVAL;

    if (thread->joinee == current_thread())
        return -EDEADLK;
    if (thread->joiner)
        return -EINVAL;

    thread->joiner = current_thread();
    current_thread()->joinee = thread;
    sched_block_on(NULL, THREAD_BLOCKED);
    current_thread()->joinee = NULL;
    return 0;
}

int thread_detach(thread_t *thread)
{
    if (!thread->joinable)
        return -EINVAL;
    thread->joinable = 0;
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    if (req->tv_nsec > 999999999)
        return -EINVAL;

    uint64_t start_sec;
    uint32_t start_tick;

    sched_time(&start_sec, &start_tick);
    to_sched_time(req, &current_thread()->weakup_sec, &current_thread()->weakup_tick);
    // dbgln("[%d:%d]to[%d:%d]", (uint32_t)start_sec, start_tick, (uint32_t)current_thread()->weakup_sec, current_thread()->weakup_tick);

    int interrupted = sched_block_on(sched_sleep_queue(), THREAD_SLEEPING);
    if (interrupted)
    {
        if (rem)
        {
            rem->tv_sec = current_thread()->weakup_sec - start_sec;
            rem->tv_nsec = current_thread()->weakup_tick - start_tick;
        }
    }

    return interrupted;
}
