#pragma once

#include <mink/types.h>
#include <mink/i386.h>
#include <mink/list.h>
#include <mink/fs.h>
#include <mink/lock.h>

#define STACK_SIZE 0x4000
#define USER_STACK_SIZE 0x8000
#define USER_STACK_BASE (0xC0000000 - USER_STACK_SIZE)

#define WCONTINUED (1 << 0) //Report status of continued child process.
#define WNOHANG (1 << 1)    //Do not hang if no status is available; return immediately.
#define WUNTRACED (1 << 3)  //Report status of stopped child process.

struct process;
struct thread;

typedef enum thread_state
{
    THREAD_READY = 0,
    THREAD_RUNNABLE,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_SLEEPING,
    THREAD_EXITED,
} thread_state_t;

typedef struct thread
{
    int id;
    char *name;
    struct cpu
    {
        void *stack;
        vmdir_t *vmdir;
        uintptr_t eip;
        uintptr_t ebp;
        uintptr_t esp;
        regs_t *r;
        uint8_t fpu[512] __attribute__((aligned(16)));
    } cpu;
    struct process *process;
    int priority;
    int sched_priority;

    thread_state_t state;
    uint64_t weakup_sec;
    uint32_t weakup_tick;

    list_node_t sched_node;
    void *exit_value;
    struct thread *joiner;
    struct thread *joinee; //going to join
    void *joinee_retval;   //going to join
    int joinable;
    int interrupted;
} thread_t;

typedef struct filedescriptor
{
    fsnode_t *node;
    off_t offset;
    int ref;
} filedescriptor_t;

typedef struct process
{
    int pid;
    int gid;
    char *name;
    list_t *argv;
    list_t *envp;

    struct bin
    {
        uintptr_t entry;
        char *path;
    } bin;

    struct files
    {
        int cap;
        int count;
        filedescriptor_t **fds;
        int *flags;
    } files;

    vmdir_t *vmdir;
    thread_t *mainthread;
    list_t *threads;
    list_t *childs;
    list_t *regions;
    list_t *wait_queue;

    struct process *parent;

    thread_t *waitingthread;
    struct state
    {
        int have_state : 1;
        int exited : 1;
        int continued : 1;
        int traced : 1;
        int stoped : 1;

        int waitchild : 1;
        int __u : 26;
    } state;

    int exit_status;
} process_t;

extern list_t *process_table;
extern process_t *init_process;

void process_set_name(process_t *process, const char *name);
void thread_set_name(thread_t *thread, const char *name);

process_t *new_process(process_t *parent);
thread_t *new_thread(process_t *process, int id, void (*start)(void *), void *arg);

void process_install();

process_t *process_from_pid(int pid);
thread_t *thread_from_id(process_t *process, int id);

void process_reset_address_space(process_t *process);

int execve(const char *filename);
int process_mmap(process_t *process, uintptr_t addr, size_t len, int prot, int flags, int fd, off_t offset, const char *name);
int process_unmap(uintptr_t addr, size_t size);
int fork();
int waitpid(int pid, int *statp, int options);
void process_exit(process_t *process, int status);
int spawn_thread(uintptr_t funp, uintptr_t argp, uintptr_t stack, size_t stacksize);
int thread_join(thread_t *thread);
int thread_detach(thread_t *thread);
void thread_exit(thread_t *thread);

void thread_exit_self();
int nanosleep(const struct timespec *req, struct timespec *rem);

int fd_alloc(process_t *process, fsnode_t *node, int flags);
filedescriptor_t *fd_get(process_t *process, int fd);

int file_open(const char *filename, int flags, int mode);
int file_close(process_t *process, int fd);
void file_close_all(process_t *process, int exec);
int file_read(process_t *process, int fd, void *buf, size_t size);
int file_write(process_t *process, int fd, void *buf, size_t size);
int file_ioctl(int fd, int cmd, void *a, void *b);
int file_truncate(const char *name, off_t length);
int file_fdtruncate(int fd, off_t length);
int file_map(int fd, struct region *region, off_t offset, size_t len);
int file_dup(int old);
int file_dup3(int old, int new, int flags);
filedescriptor_t *file_clone(process_t *process, int fd);

thread_t *current_thread();
process_t *current_process();
void sched_init();
list_t *sched_sleep_queue();
void schedule();
void sched_yield();
void sched_switch_to(thread_t *thread);
void sched_ontick(uint64_t sec, uint32_t tick);
void sched_time(uint64_t *sec, uint32_t *tick);
void to_sched_time(const struct timespec *t, uint64_t *sec, uint32_t *tick);
void sched_run(thread_t *thread);
int sched_block_on(list_t *queue, thread_state_t state);
int sched_block_until(uint64_t sec, uint32_t tick);
void sched_run_queue(list_t *queue, int sig);
void sched_exit(thread_t *thread);
void sched_debug_threads();
