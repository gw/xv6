## Interrupts in `ide.c`
> Let's see what happens if we turn on interrupts while holding the ide lock. In iderw in ide.c, add a call to sti() after the acquire(), and a call to cli() just before the release(). Why does this sometimes cause a kernel panic on boot?

Booted QEMU a few times and intermittently observed both `panic: acquire` and `panic: sched locks`.

### `panic: acquire`
Stack trace (as given by `getcallerpcs`, which outputs caller return addresses instead of ebp addresses for some reason):
```
80104fa1 spinlock.c:acquire():41  // cpu tries to acquire lock it already holds from partially executing iderw()
801027f5 ide.c:ideintr():111  // ide interrupt handler
801069d0 trap.c:trap():88
801066ab trapasm.S:alltraps():24
8010296e ide.c:iderw():164  // sti() allows ide interrupt in crit-sect
801001e7 bio.c:bread():105
8010137e fs.c:readsb():37
80101677 fs.c:iinit():174  // forkret() calls this for the first process
80104b93 proc.c:forkret():362  // This and below is set up by proc.c:allocproc()
801066ae trapasm.S:trapret():31
```

### `panic: sched locks`
```
80104abd proc.c:sched():328  // Expects only 1 lock to be held (ncli == 1), but the CPU has two, the ptable and ide locks. Panics.
80104b54 proc.c:yield():344  // Acquires ptable.lock and calls sched()
80106bf8 trap.c:trap():149  // Calls yield on timer interrupts
801066ab trapasm.S:alltraps():24
80102957 ide.c:iderw():164  // sti() allows timer interrupt in crit-sect
801001e7 bio.c:bread():105
80101ee1 fs.c:readi():451
80100b9a exec.c:exec():31
801062f6 sysfile.c:sys_exec():418
80105630 syscall.c:syscall():165 // SYS_exec called by initcode.S
```

## Interrupts in `file.c`
> Now let's see what happens if we turn on interrupts while holding the file_table_lock. This lock protects the table of file descriptors, which the kernel modifies when an application opens or closes a file. In filealloc() in file.c, add a call to sti() after the call to acquire(), and a cli() just before each of the release()es. Explain in a few sentences why the kernel didn't panic. Why do file_table_lock and ide_lock have different behavior in this respect?

We'll never see `panic: acquire` as a result of enabling interrupts for the critical section of `filealloc()` b/c `ftable.lock` is only acquired by functions that aren't invoked by ISRs. Thus, an interrupt will never result in an attempt to acquire the already-held `ftable.lock`.

However, it's still possible for the CPU to receive a timer interrupt while executing the `filealloc()` critical section--it's just extremely rare because it's critical section is likely orders of magnitude shorter than that of `iderw()` (the latter involves literally sleeping to wait for the disk to handle a request).

## xv6's spinlock implementation
> Why does release() clear lk->pcs[0] and lk->cpu before clearing lk->locked? Why not wait until after?

Because as soon as the lock is released, another process could potentially grab it. New acquirers overwrite `pcs` and `cpu` quickly but in the rare event that a panic occurs before a new acquirer can do so then the debug information they provide will incorrectly correspond to the previous holder.
