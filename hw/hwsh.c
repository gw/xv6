#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Simplifed xv6 shell.

#define MAXARGS 10

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
  int type; //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
  int type;            // ' '
  char *argv[MAXARGS]; // arguments to the command to be exec-ed
};

struct redircmd {
  int type;        // < or >
  struct cmd *cmd; // the command to be run (e.g., an execcmd)
  char *file;      // the input/output file
  int mode;        // the mode to open the file with
  int fd;          // the file descriptor number to use for the file
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // left side of pipe
  struct cmd *right; // right side of pipe
};

int fork1(void); // Fork but exits on failure.
struct cmd *parsecmd(char *);
char *get_path(char *);
int search_dir(char *, char *);

// Execute cmd.  Never returns.
void runcmd(struct cmd *cmd) {
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if (cmd == 0)
    exit(0);

  switch (cmd->type) {
  default:
    fprintf(stderr, "unknown runcmd\n");
    exit(-1);

  case ' ':
    ecmd = (struct execcmd *)cmd;
    if (ecmd->argv[0] == 0)
      exit(0);

    // If program exists in current workdir,
    // use given relative path. Otherwise,
    // search all PATH dirs.
    char *fullpath, *exe, *cwd, *tofree;

    cwd = tofree = malloc(PATH_MAX);
    getcwd(cwd, PATH_MAX);

    exe = ecmd->argv[0];
    fullpath = search_dir(cwd, exe) ? exe : get_path(exe);

    free(tofree);
    execv(fullpath, ecmd->argv);
    break;

  case '>':
  case '<':
    rcmd = (struct redircmd *)cmd;
    fprintf(stderr, "redir not implemented\n");
    // Your code here ...
    runcmd(rcmd->cmd);
    break;

  case '|':
    pcmd = (struct pipecmd *)cmd;
    fprintf(stderr, "pipe not implemented\n");
    // Your code here ...
    break;
  }
  exit(0);
}

// Search all folders in PATH for a file named `name`.
// Returns absolute path to file if found, else `name`.
char *get_path(char *name) {
  char *path, *tok, *result;

  path = getenv("PATH");
  while ((tok = strsep(&path, ":")) != NULL) { // PATH directories
    if (search_dir(tok, name)) {
      result = malloc(PATH_MAX);
      strcat(result, tok);
      strcat(result, "/");
      strcat(result, name);
      return result;
    }
  }
  return name;
}

// Search a directory `dirname` for an entry with d_name `trgt`.
// Returns 1 if found else 0.
int search_dir(char *dirname, char *trgt) {
  int found;
  size_t len;
  DIR *dirp;
  struct dirent *direntp;

  assert((dirp = opendir(dirname)) != NULL); // Get directory stream DIR *

  found = 0;
  len = strlen(trgt);
  while ((direntp = readdir(dirp)) != NULL) { // `dirent`s in stream
    if (len == direntp->d_namlen && strcmp(trgt, direntp->d_name) == 0) // Match
      found = 1;
  }
  closedir(dirp);
  return found;
}

// Get a line from STDIN using fgets, copyng at most `nbuf` bytes into `buf`.
// Returns -1 if fgets doesn't read any bytes for w/e reason, else 0.
int getcmd(char *buf, int nbuf) {
  if (isatty(fileno(stdin)))
    fprintf(stdout, "6.828$ ");

  // Zero-out the buffer and
  // try to read a line into it.
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin); // Blocks until something appears on STDIN

  if (buf[0] == 0) // EOF
    return -1;

  return 0;
}

int main(void) {
  static char buf[100]; // 1 line of STDIN
  int fd, r;

  // Read and run input commands.
  while (getcmd(buf, sizeof(buf)) >= 0) {
    if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ') {
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      buf[strlen(buf) - 1] = 0; // chop \n
      if (chdir(buf + 3) < 0)
        fprintf(stderr, "cannot cd %s\n", buf + 3);
      continue;
    }
    if (fork1() == 0)
      runcmd(parsecmd(buf));
    wait(&r);
  }
  // `fgets` in `getcmd` has received EOF or errored
  exit(0);
}

int fork1(void) {
  int pid;

  pid = fork();
  if (pid == -1)
    perror("fork");
  return pid;
}

// Generate a standard exec command.
// Returns a pointer to a newly-malloc'd execcmd struct, cast back to a
// `struct cmd`.
struct cmd *execcmd(void) {
  struct execcmd *ret;

  ret = malloc(sizeof(*ret));
  memset(ret, 0, sizeof(*ret));

  ret->type = ' ';

  return (struct cmd *)ret;
}

// Generate a file I/O redirect command.
// Returns a pointer to a newly-malloc'd redircmd struct, cast back to a
// `struct cmd`.
struct cmd *redircmd(struct cmd *subcmd, char *file, int type) {
  struct redircmd *ret;

  ret = malloc(sizeof(*ret));
  memset(ret, 0, sizeof(*ret));

  ret->type = type;
  ret->cmd = subcmd;
  ret->file = file;
  ret->mode = (type == '<') ? O_RDONLY : O_WRONLY | O_CREAT | O_TRUNC;
  ret->fd = (type == '<') ? 0 : 1;

  return (struct cmd *)ret;
}

// Generate a pipe command from two subcommands.
// Returns a pointer to a newly-malloc'd pipecmd struct, cast back to a
// `struct cmd`.
struct cmd *pipecmd(struct cmd *left, struct cmd *right) {
  struct pipecmd *ret;

  ret = malloc(sizeof(*ret));
  memset(ret, 0, sizeof(*ret));

  ret->type = '|';
  ret->left = left;
  ret->right = right;

  return (struct cmd *)ret;
}

// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

// Parse the next shell language token in a string.
// Updates `ps` to point at the start of the next potential token in the string.
// Updates `q` and `eq` to point to the start and end of the parsed token.
// Returns '|', '<', '>', or 'a' to represent the token's type, and 0 if end of
// string is reached before a token is found.
int gettoken(char **ps, char *es, char **q, char **eq) {
  int ret;

  while (*ps < es && strchr(whitespace, **ps)) // Remove leading whitespace
    (*ps)++;

  if (q)
    *q = *ps; // Start of token

  ret = **ps;
  switch (**ps) {
  case 0: // Null-byte (end of string)
    break;
  case '|': // Symbols
  case '<':
  case '>':
    (*ps)++;
    break;
  default: // Named command
    ret = 'a';
    // Advance to next whitespace or symbol character
    while (*ps < es && !strchr(whitespace, **ps) && !strchr(symbols, **ps))
      (*ps)++;
    break;
  }

  if (eq)
    *eq = *ps; // End of token

  // Remove trailing whitespace
  while (*ps < es && strchr(whitespace, **ps))
    (*ps)++;

  printf("Tok: %c\n", ret);
  return ret;
}

// Advances a string pointer `ps` to the next non-whitespace character.
// Returns 1 if said character is not `\0` and is in `toks`, else 0.
int peek(char **ps, char *es, char *toks) {
  while (*ps < es && strchr(whitespace, **ps)) // Ignore whitespace
    (*ps)++;
  return **ps && strchr(toks, **ps);
}

struct cmd *parseline(char **, char *);
struct cmd *parsepipe(char **, char *);
struct cmd *parseexec(char **, char *);
struct cmd *parseredirs(struct cmd *, char **, char *);

// Copy the characters between the pointers `s` and `es` onto the heap.
// Adds a terminating null-byte.
// Returns a pointer to the new copy.
char *mkcopy(char *s, char *es) {
  int n = es - s;
  char *c = malloc(n + 1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd *parsecmd(char *s) {
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if (s != es) {
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd *parseline(char **ps, char *es) {
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
  return cmd;
}

// Parse a pipe construct from a string.
// First parses the leftmost exec command. If that's followed by a pipe, it
// builds a pipecommand containing the leftmost exec and the result of recursing
// on the token to the right of the pipe.
struct cmd *parsepipe(char **ps, char *es) {
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if (peek(ps, es, "|")) {
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

// Parse an exec construct from a string.
// execs can contain multiple redirs.
struct cmd *parseexec(char **ps, char *es) {
  char *q, *eq;
  int toktype, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  // Allocate empty execcmd struct
  ret = execcmd();
  cmd = (struct execcmd *)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  // Keep consuming args until we hit a pipe, checking for a redir at the end of
  // every iteration.
  while (!peek(ps, es, "|")) {
    if ((toktype = gettoken(ps, es, &q, &eq)) == 0)
      break; // End of string
    if (toktype != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc++] = mkcopy(q, eq);
    if (argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0; // Add final NULL arg. See `man 2 exec`
  return ret;
}

// Parse file I/O redirection construct from a string.
// Modifies the given `cmd` if it is part of a redirection, and returns a
// pointer to it.
struct cmd *parseredirs(struct cmd *cmd, char **ps, char *es) {
  printf("ENTER parseredirs\n");
  int toktype;
  char *q, *eq;

  while (peek(ps, es, "<>")) {
    printf("FOUND angle bracket\n");
    // Next token is angle bracket
    toktype = gettoken(ps, es, 0, 0); // Consume it

    if (gettoken(ps, es, &q, &eq) != 'a') {
      // Next token after angle bracket is not a filename
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }

    switch (toktype) {
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}
