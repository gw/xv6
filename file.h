// This is what a file descriptor refers to.
// These all live in the global open file table
// ftable--see file.c
struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe;
  struct inode *ip;
  uint off;
};


// in-memory copy of an inode
struct inode {
  uint dev;           // Device number
  uint inum;          // Inode number
  int ref;            // num open FDs / proc->cwd's referring to it
  struct sleeplock lock;
  int flags;          // I_VALID

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;        // num hard links
  uint size;
  uint addrs[NDIRECT+2];
};
#define I_VALID 0x2

// table mapping major device number to
// device functions
struct devsw {
  int (*read)(struct inode*, char*, int);
  int (*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];

#define CONSOLE 1

//PAGEBREAK!
// Blank page.
