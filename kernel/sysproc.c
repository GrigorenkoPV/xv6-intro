#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_dump(void)
{
  return dump();
}

uint64
sys_dump2(void)
{
  int pid;
  int register_num;
  uint64 return_value_addr;
  argint(0, &pid);
  argint(1, &register_num);
  argaddr(2, &return_value_addr);

  if (register_num < 2 || register_num > 11) {
    return -3;
  }

  struct proc* caller = myproc();
  struct proc* subject = get_proc(pid);

  if (!subject) {
    return -2;
  }

  if (caller->pid != subject->pid && caller->pid != subject->parent->pid) {
    return -1;
  }

  uint64 result = get_register(subject->trapframe, register_num);

  if (copyout(caller->pagetable, return_value_addr, (void*)(&result),
              sizeof(uint64))) {
    return -4;
  }
  return 0;
}
