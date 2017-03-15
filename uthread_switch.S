	.text

/* Switch from current_thread to next_thread. Make next_thread
 * the current_thread, and set next_thread to 0.
 * Use eax as a temporary register; it is caller saved.
 */
	.globl thread_switch
thread_switch:
	/* Save registers to current_thread's stack */
	pushal

	/* Save current_thread's stack pointer */
	movl current_thread, %eax
	movl %esp, (%eax)

	/* Switch to next_thread's stack */
	movl next_thread, %eax
	movl (%eax), %esp

	/* Set current_thread to next_thread */
	movl %eax, current_thread

	/* Restore next_thread's register state */
	popal

	/* Set next_thread to 0 */
	movl $0, next_thread

	/* After `popal`, %esp now points to return address. Pop it and jump. */
	ret
