## What's on the stack at 0x10000c?

Commented output of `x/24x $esp` at right before executing 0x10000c, which corresponds to entry.S:47:

```
(gdb) x/24x $esp
0x7bcc:	0x00007db7 <- Ret address of next instruction after elf->entry call, bootmain.c:47
                   0x00000000	0x00000000	0x00000000
0x7bdc:	0x00000000 0x00000000	0x00000000	0x00000000
0x7bec:	0x00000000 0x00000000 0x00000000  0x00000000
0x7bfc:	0x00007c4d <- Return address of next instruction after bootmain call, bootasm.S:66 	       

# Everything after is junk
```
