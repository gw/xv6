# User-Level Threads

>Once your xv6 shell runs, type "uthread", and gdb will break at thread_switch. Now you can type commands like the following to inspect the state of uthread:

```shell
(gdb) p/x next_thread->sp
$4 = 0x4d48
(gdb) x/9x next_thread->sp
0x4ae8 :      0x00000000      0x00000000      0x00000000      0x00000000
0x4af8 :      0x00000000      0x00000000      0x00000000      0x00000000
0x4b08 :      0x0000016b
```

>What address is 0x0000016b, which sits on the bottom of the stack of next_thread?

It's the fake return address pushed by `thread_create`, which in this case is the address of the `mythread` function in `uthread.c`. I confirmed this at the command line:

```shell
$ objdump -t _uthread

SYMBOL TABLE:
00000000         *UND*		 00000000
00000000 l    d  .text		 00000000 .text
000009fc l    d  .rodata		 00000000 .rodata
00000a60 l    d  .eh_frame		 00000000 .eh_frame
00000d48 l    d  .data		 00000000 .data
00000d60 l    d  .bss		 00000000 .bss
00000000 l    d  .comment		 00000000 .comment
00000000 l    d  .debug_aranges		 00000000 .debug_aranges
00000000 l    d  .debug_info		 00000000 .debug_info
00000000 l    d  .debug_abbrev		 00000000 .debug_abbrev
00000000 l    d  .debug_line		 00000000 .debug_line
00000000 l    d  .debug_str		 00000000 .debug_str
00000000 l    d  .debug_loc		 00000000 .debug_loc
00000000 l    df *ABS*		 00000000 uthread.c
00000d60 l       .bss		 00008020 all_thread
0000001e l     F .text		 000000c1 thread_schedule
0000016b l     F .text		 00000079 mythread   <================
```
