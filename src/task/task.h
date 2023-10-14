#ifndef __TASK_H__
#define __TASK_H__

#include "../config.h"
#include "../idt/idt.h"
#include "../memory/paging/paging.h"

struct process;

struct registers {
    //  general-purpose registers
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t ip;  // PC program counter register
    uint32_t cs;  //  code segment register
    uint32_t flags;  // flags register 
    uint32_t esp;  // stack pointer register
    uint32_t ss;  // stack segment register
};


struct task {
    // The page directory of the task
    struct paging_4gb_chunk* page_directory;

    // The registers of the task when the task is not running
    struct registers registers;

    // The next task in the linked list
    struct task* next;

    // Previous task in the linked list
    struct task* prev;

    // The process of the task
    struct process* process;
};


struct task* task_new(struct process* process);
struct task* task_current();
struct task* task_get_next();
int task_free(struct task* task);

int task_switch(struct task* task);
int task_page();
int task_page_task(struct task* task);

void task_run_first_ever_task();


void task_current_save_stat(struct interrupt_frame *frame);
int copy_string_from_task(struct task* task, void* virt, void* phys, int max);
void* task_get_stack_item(struct task* task, int index);
void* task_virtual_address_to_physical(struct task* task, void* virtual_address);
void task_next();


// task.asm

//  leave kernel land and execute in user land
void task_return(struct registers* regs);
void restore_general_purpose_registers(struct registers* regs);
void user_registers();

#endif