# Context switch
#
#   void swtch(struct context **old, struct context *new);
#
# Save current register context in old
# and then load register context from new.
#
# Caller must hold the only ptable.lock and interrupts
# must be disabled, as `swtch` must occur atomically.

.globl swtch
swtch:
  movl 4(%esp), %eax
  movl 8(%esp), %edx

  # Save old callee-save registers
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi

  # Switch stacks
  movl %esp, (%eax)  # Write stack pointer to *old
  movl %edx, %esp  # Switch %esp to `new`

  # Load new callee-save registers.
  # Format is exact same as saved above,
  # as these were saved by a previous
  # call to `swtch`. %eip was saved
  # implicitly as return address by the
  # `call` instruction that invoked that
  # previous `swtch` execution--`ret`
  # jumps to it. So this completely
  # encapsulates switching b/w threads
  # of execution. Combine with `switchuvm`
  # to switch b/w procs.
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp
  ret
