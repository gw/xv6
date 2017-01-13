## Part 2: Explanation of Date system call implementation

1. Modify Makefile to link a binary called `date`, whose source is a new user prog `date.c`
2. `date.c` gets exec'd when user types `date` in shell
3. `date.c` allocates an rtcdate struct on the stack and then calls `date`, which is defined as a global name in `usys.S`. The assembly instructions there invoke the `SYS_date` macro defined in `syscall.h`, which expands to the integer 22, which gets loaded into %eax as the argument to `int $T_SYSCALL` which expands to `int 64`.
4. This eventually traps into the code at `trap.c`, which inspects `trapno` and calls `syscall`, in `syscall.c`. We use the syscall argument (22) to index into the `syscalls` lookup array, which results in a call to `sys_date`, defined in `sysdatetime.c`.
5. `sys_date` gets the address of the previously-allocated rtcdate struct and writes into it with the `cmostime` helper function.
6. `syscall` stores `sys_date`s return value in `%eax`, per convention, which is then tested on `date.c:10` (see the disassembly in `date.asm`) when control returns to the user.
