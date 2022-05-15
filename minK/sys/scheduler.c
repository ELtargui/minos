#include <mink/types.h>
#include <mink/i386.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/string.h>
#include <mink/process.h>
#include <errno.h>

extern void finalizer_main();

thread_t *finalizer = NULL;
static thread_t *current = NULL;
static thread_t *idle_thread = NULL;

static uint64_t sched_sec;
static uint32_t sched_tick;
static list_t *sched_queue;
static list_t *sleep_queue;
static lock_t sleep_lock;

const char *thread_state_str(thread_t *thread);

thread_t *current_thread()
{
    return current;
}

process_t *current_process()
{
    return current ? current->process : NULL;
}

void idle(void *arg)
{
    (void)arg;

    while (1)
    {
        STI();
        HLT();
    }
}

void sched_init()
{
    idle_thread = new_thread(NULL, 0, idle, NULL);
    finalizer = new_thread(NULL, 0, finalizer_main, NULL);
    sched_queue = new_list();
    sched_queue->name = "sched_queue";
    sleep_queue = new_list();
    sleep_queue->name = "sleep_queue";
    init_lock(&sleep_lock, "sleep_queue");
}

list_t *sched_sleep_queue()
{
    return sleep_queue;
}

void add_thread(thread_t *thread)
{
    if (thread == idle_thread)
        return;
    if (thread->sched_node.list != NULL)
    {
        dbgln("error:%q:%s", thread->sched_node.list->name);
    }
    assert(thread->sched_node.list == NULL);
    thread->state = THREAD_RUNNABLE;
    list_append_node(sched_queue, &thread->sched_node);
}

thread_t *get_next()
{
    if (!sched_queue->head)
        return idle_thread;
    list_node_t *node = list_take_first_node(sched_queue);
    assert(node);
    thread_t *thread = node->value;
    assert(thread->state == THREAD_RUNNABLE);

    return thread;
}

int thread_save_ctx(thread_t *thread)
{
    uintptr_t esp, ebp, eip;
    asm volatile("mov %%ebp, %0"
                 : "=r"(ebp));
    asm volatile("mov %%esp, %0"
                 : "=r"(esp));
    eip = read_eip();

    if (eip == 0xf1234567)
    {
        return 1;
    }

    thread->cpu.esp = esp;
    thread->cpu.ebp = ebp;
    thread->cpu.eip = eip;
    return 0;
}

void schedule()
{
    if (!current)
        return;
    thread_t *next = get_next();
    if (thread_save_ctx(current))
    {
        return;
    }
    if (current->state == THREAD_RUNNING)
    {
        add_thread(current);
    }
    sched_switch_to(next);
}

void sched_yield()
{
    schedule();
}

void sched_ontick(uint64_t sec, uint32_t tick)
{
    sched_sec = sec;
    sched_tick = tick;

    if (!trylock(&sleep_lock))
    {
        list_node_t *node = sleep_queue->head;
        while (node)
        {
            list_node_t *next = node->next;
            thread_t *thread = node->value;
            // dbgln("[id:%d][%d:%d]-[%d:%d]", thread->id, (uint32_t)sec, tick, (uint32_t)thread->weakup_sec, thread->weakup_tick);
            if (sched_sec > thread->weakup_sec || (sched_sec == thread->weakup_sec && sched_tick > thread->weakup_tick))
            {
                sched_run(thread);
            }
            else
            {
                break;
            }
            node = next;
        }
        unlock(&sleep_lock);
    }
    schedule();
}

void sched_switch_to(thread_t *thread)
{
    assert(thread);
    if (!current || current->cpu.vmdir != thread->cpu.vmdir)
        vm_flush_vmdir(thread->cpu.vmdir);

    thread->state = THREAD_RUNNING;
    current = thread;
    cpu_set_kernel_stack((uintptr_t)thread->cpu.stack + STACK_SIZE);

    asm volatile(
        "mov %0, %%ebx\n"
        "mov %1, %%esp\n"
        "mov %2, %%ebp\n"
        "mov $0xf1234567, %%eax\n"
        "jmp *%%ebx" ::
            "r"(thread->cpu.eip),
        "r"(thread->cpu.esp),
        "r"(thread->cpu.ebp)
        : "%ebx", "%esp", "%eax");
}

void sched_time(uint64_t *sec, uint32_t *tick)
{
    *sec = sched_sec;
    *tick = sched_tick;
}

void to_sched_time(const struct timespec *t, uint64_t *sec, uint32_t *tick)
{
    uint64_t _sec = 0;
    uint32_t _tick = 0;

    sched_time(&_sec, &_tick);

    _sec += t->tv_sec;
    _tick += 1 + t->tv_nsec / (1000000000 / pit_frequency());

    if (_tick >= pit_frequency())
    {
        _tick -= pit_frequency();
        _sec++;
    }

    *sec = _sec;
    *tick = _tick;
}

void sched_run(thread_t *thread)
{
    switch (thread->state)
    {
    case THREAD_READY:
        break;
    case THREAD_RUNNABLE:
        return;
    case THREAD_RUNNING:
        // ERROR(running);
        return;
    case THREAD_BLOCKED:
        if (thread->sched_node.list)
        {
            list_remove_node(thread->sched_node.list, &thread->sched_node);
        }
        break;
    case THREAD_SLEEPING:
        list_remove_node(sleep_queue, &thread->sched_node);
        break;
    case THREAD_EXITED:
        ERROR(exited);
        break;
    default:
        ERROR(inavalide_state);
        break;
    }

    add_thread(thread);
}

void sched_unblock(){

}

void sched_exit(thread_t *thread)
{
    switch (thread->state)
    {
    case THREAD_READY:
        break;
    case THREAD_RUNNABLE:
        list_remove_node(sched_queue, &thread->sched_node);
        break;
    case THREAD_RUNNING:
        break;
    case THREAD_BLOCKED:
        if (thread->sched_node.list)
        {
            list_remove_node(thread->sched_node.list, &thread->sched_node);
        }
        break;
    case THREAD_SLEEPING:
        list_remove_node(sleep_queue, &thread->sched_node);
        break;
        break;
    case THREAD_EXITED:
        ERROR(exited);
        break;
    default:
        ERROR(inavalide_state);
        break;
    }
    thread->state = THREAD_READY;
}

int sched_block_on(list_t *queue, thread_state_t state)
{
    // interrupt_disable();
    if (state == THREAD_SLEEPING)
    {
        if (!queue)
            queue = sleep_queue;

        lock(&sleep_lock);
        assert(current_thread()->sched_node.list == NULL);
        list_node_t *node = queue->head;
        int added = False;
        while (node)
        {
            thread_t *thread = node->value;
            if (current->weakup_sec < thread->weakup_sec || (current->weakup_sec == thread->weakup_sec && current->weakup_tick < thread->weakup_tick))
            {
                list_node_add_before(queue, node, &current->sched_node);
                added = True;
                break;
            }
            node = node->next;
        }

        if (!added)
            list_append_node(queue, &current->sched_node);
        unlock(&sleep_lock);
    }
    else if (queue)
    {
        list_append_node(queue, &current_thread()->sched_node);
    }

    current_thread()->state = state;
    // interrupt_resume();
    sched_yield();

    return current_thread()->interrupted;
}

void sched_run_queue(list_t *queue, int sig)
{
    list_node_t *node = queue->head;
    while (node)
    {
        list_node_t *next = node->next;
        thread_t *thread = node->value;
        thread->interrupted = sig;
        sched_run(thread);
        node = next;
    }
}

const char *thread_state_str(thread_t *thread)
{
    switch (thread->state)
    {
    case THREAD_READY:
        return "READY";
    case THREAD_RUNNABLE:
        return "RUNNABLE";
    case THREAD_RUNNING:
        return "RUNNING";
    case THREAD_BLOCKED:
        return "BLOCKED";
    case THREAD_SLEEPING:
        return "SLEEPING";
    case THREAD_EXITED:
        return "EXITED";
    default:
        return "Inavalide state";
    }
}

void sched_debug_threads()
{
    interrupt_disable();
    foreach (process_table, n)
    {
        process_t *p = n->value;
        dbgln("PID :: %d (%s)", p->pid, p->name);
        foreach (p->threads, tn)
        {
            thread_t *t = tn->value;
            dbgln("    [%d] : %s (%d)", t->id, thread_state_str(t), t->state);
            dbgln("         queue:%s", t->sched_node.list ? t->sched_node.list->name : "--");
        }
    }
    interrupt_resume();
}
