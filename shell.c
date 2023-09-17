#include <sys/wait.h>

#include <stdio.h>

#include <string.h>

#include <stdlib.h>

#include <stdbool.h>

#include <sys/types.h>

#include <unistd.h>

#include <fcntl.h>

#define MAX_CMD_LINE_ARGS 128
#define MAX_PIPE_COMMANDS 10

// Function to return the minimum of two integers
int min(int a, int b) {
  return a < b ? a : b;
}

// Function to redirect the standard input to a file
void redirect_input(const char * filename) {
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("open filename in redirect_input failed");
    exit(EXIT_FAILURE);
  }
  dup2(fd, STDIN_FILENO);
  close(fd);
}

// Function to redirect the standard output to a file, with an optional append mode
void redirect_output(const char * filename, bool append) {
  int flags = O_WRONLY | O_CREAT;
  flags |= append ? O_APPEND : O_TRUNC;
  int fd = open(filename, flags, 0644);
  if (fd == -1) {
    perror("open failure in redirect_output");
    exit(EXIT_FAILURE);
  }
  dup2(fd, STDOUT_FILENO);
  close(fd);
}

// Function to execute a pipeline of two commands
void execute_pipeline(char * args[MAX_PIPE_COMMANDS][MAX_CMD_LINE_ARGS], int n) {
  int i, pipefds[2 * n];

  // Create n pipes
  for (i = 0; i < n; i++) {
    if (pipe(pipefds + i * 2) < 0) {
      perror("Couldn't pipe");
      exit(EXIT_FAILURE);
    }
  }

  int pid;
  int status;

  for (i = 0; i < n + 1; i++) {
    pid = fork();
    if (pid == 0) {
      // Not the first command
      if (i != 0) {
        if (dup2(pipefds[(i - 1) * 2], 0) < 0) {
          exit(EXIT_FAILURE);
        }
      }

      // Not the last command
      if (i != n) {
        if (dup2(pipefds[i * 2 + 1], 1) < 0) {
          exit(EXIT_FAILURE);
        }
      }

      // Close all pipe file descriptors
      for (int j = 0; j < 2 * n; j++) {
        close(pipefds[j]);
      }

      // Execute the command
      if (execvp(args[i][0], args[i]) < 0) {
        perror("exec failed");
        exit(EXIT_FAILURE);
      }
    } else if (pid < 0) {
      perror("error forking");
      exit(EXIT_FAILURE);
    }
  }

  // Close all pipe file descriptors in parent
  for (i = 0; i < 2 * n; i++) {
    close(pipefds[i]);
  }

  // Wait for all child processes to finish
  for (i = 0; i < n + 1; i++) {
    wait( & status);
  }
}

// Function to parse the command line input and handle redirections and pipe
int parse(char * s, char * argv[MAX_PIPE_COMMANDS][MAX_CMD_LINE_ARGS]) {
  char * token = strtok(s, " \t\n\v\f\r");
  char * input_file = NULL;
  char * output_file = NULL;
  bool append = false;
  int cmd_count = 0;
  int arg_count = 0;

  while (token != NULL) {
    if (strcmp(token, "<") == 0) {
      token = strtok(NULL, " \t\n\v\f\r");
      input_file = token;
    } else if (strcmp(token, ">") == 0) {
      token = strtok(NULL, " \t\n\v\f\r");
      output_file = token;
    } else if (strcmp(token, ">>") == 0) {
      token = strtok(NULL, " \t\n\v\f\r");
      output_file = token;
      append = true;
    } else if (strcmp(token, "|") == 0) {
      argv[cmd_count][arg_count] = NULL; // Null terminate the arguments list for the current command
      cmd_count++;
      arg_count = 0;
    } else {
      argv[cmd_count][arg_count] = token;
      arg_count++;
    }
    token = strtok(NULL, " \t\n\v\f\r");
  }

  argv[cmd_count][arg_count] = NULL; // Null terminate the arguments list for the last command

  if (input_file != NULL) {
    redirect_input(input_file);
  }

  if (output_file != NULL) {
    redirect_output(output_file, append);
  }

  return cmd_count;
}

// Function to execute a parsed command
int execute(char * input) {
  char * shell_argv[MAX_PIPE_COMMANDS][MAX_CMD_LINE_ARGS];
  int arg_count[MAX_PIPE_COMMANDS] = {
    0
  };

  // Initialize shell_argv to NULL
  for (int i = 0; i < MAX_PIPE_COMMANDS; i++) {
    for (int j = 0; j < MAX_CMD_LINE_ARGS; j++) {
      shell_argv[i][j] = NULL;
    }
  }

  int shell_argc = parse(input, shell_argv);

  // Calculate the argument count for each command
  for (int i = 0; i <= shell_argc; i++) {
    for (int j = 0; j < MAX_CMD_LINE_ARGS; j++) {
      if (shell_argv[i][j] != NULL) {
        arg_count[i]++;
      } else {
        break;
      }
    }
  }

  int status = 0;
  bool run_in_background = false;

  // Check if the last argument of the last command is "&"
  if (arg_count[shell_argc] > 0 && strcmp(shell_argv[shell_argc][arg_count[shell_argc] - 1], "&") == 0) {
    shell_argv[shell_argc][arg_count[shell_argc] - 1] = NULL; // Remove "&" from arguments
    run_in_background = true;
  }

  if (shell_argc > 0) {
    execute_pipeline(shell_argv, shell_argc);
  } else {
    pid_t pid = fork();
    if (pid < 0) {
      fprintf(stderr, "Fork() failed\n");
      return -1;
    } else if (pid == 0) {
      // Child process
      if (execvp(shell_argv[0][0], (char *
          const * ) shell_argv[0]) < 0) {
        perror("execvp failed");
        exit(EXIT_FAILURE);
      }
    } else {
      // Parent process
      if (!run_in_background) {
        while (wait( & status) != pid) {}
      }
    }
  }
  return 0;
}

int main(int argc,
  const char * argv[]) {
  char input[BUFSIZ];
  char last_input[BUFSIZ];
  bool finished = false;
  memset(last_input, 0, BUFSIZ * sizeof(char));

  // Main loop to read and execute commands
  while (!finished) {
    memset(input, 0, BUFSIZ * sizeof(char));
    printf("osh > ");
    fflush(stdout);

    if (fgets(input, BUFSIZ, stdin) == NULL) {
      if (feof(stdin)) {
        // End of file reached, exit
        break;
      }
      fprintf(stderr, "no command entered\n");
      continue; // Skip the rest of the loop iteration
    }

    size_t len = strlen(input);
    if (input[strlen(input) - 1] == '\n') {
      input[strlen(input) - 1] = '\0';
    }

    // Handling exit and history (!!) commands
    if (strncmp(input, "exit", 4) == 0) {
      finished = true;
    } else if (strncmp(input, "!!", 2) == 0) {
      if (strlen(last_input) > 0) {
        execute(last_input);
      } else {
        printf("No commands in history.\n");
      }
    } else {
      // Store the last command and execute the current command
      strncpy(last_input, input, min(strlen(input), BUFSIZ - 1));
      execute(input);
    }
  }

  printf("\t\t...exiting\n");
  return 0;
}

/*
epw@EPWPC:~/shell_project$ gcc -o shell shell.c
epw@EPWPC:~/shell_project$ sudo ./shell
[sudo] password for epw: 
osh > ls -l
total 36
-rw-r--r-- 1 epw  epw    183 Sep 16 13:35 input.txt
-rw-r--r-- 1 root root   202 Sep 17 11:29 output.txt
-rwxr-xr-x 1 epw  epw  17192 Sep 17 14:10 shell
-rw-r--r-- 1 epw  epw   7069 Sep 17 12:55 shell.c
osh > cat < input.txt > output.txt
epw@EPWPC:~/shell_project$ sudo ./shell
osh > ls -l | grep output.txt
-rw-r--r-- 1 root root   202 Sep 17 14:10 output.txt
osh > !!
-rw-r--r-- 1 root root   202 Sep 17 14:10 output.txt
osh > ls -la | grep Sep | more
-rw-r--r--  1 epw  epw    183 Sep 16 13:35 input.txt
-rw-r--r--  1 root root   202 Sep 17 14:10 output.txt
-rwxr-xr-x  1 epw  epw  17192 Sep 17 14:10 shell
-rw-r--r--  1 epw  epw   7069 Sep 17 12:55 shell.c
osh > cat shell.c | head -10 | tail -5

#include <stdlib.h>

#include <stdbool.h>

osh > exit   
                ...exiting
*/