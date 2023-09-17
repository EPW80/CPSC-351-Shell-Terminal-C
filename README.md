# Unix_shell project

This project implements a simple shell command pipeline executor, which allows the execution of multiple commands chained by pipes and handles input/output redirections. The primary components include functions for parsing the command-line inputs, executing single commands, and managing pipelines of commands.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

## Prerequisites

- GCC Compiler (or any compatible C compiler)
- Linux/Unix Environment (as it uses fork, pipe and exec which are specific to UNIX-like systems)

## Installation

- Clone the repository or download the source files.
- Navigate to the project directory.
- Compile the source files using a C compiler:

```
gcc -o shell shell.c
```

- Run the executable:

```
./shell
```

## Usage

Once you run the executable, you can enter commands in the following format:

```
command1 [arg1 arg2 ...] [| command2 [arg1 arg2 ...]] [< input_file] [> output_file] [>> append_output_file] [&]
```

| - Pipe symbol to chain multiple commands. The output of the preceding command will be the input to the following command.
< - Redirect input from a file.

> - Redirect output to a file (overwrite if file exists).
>> - Append output to a file.
>>& - Run command in the background (not wait for it to finish).

- For example:

```
cat file.txt | grep "keyword" > output.txt
```

## Code Structure

The project consists of three primary functions:

- parse - Parses a command string to extract individual commands and their arguments, as well as input/output redirection specifications.
- execute - Forks a new process to execute the parsed command or chain of commands.
- execute_pipeline - Manages the execution of multiple commands connected by pipes.

## Functions Documentation

int parse(char* s, char* argv[MAX_PIPE_COMMANDS][MAX_CMD_LINE_ARGS])

- s: A string containing the entire command input.
- argv: A 2D array to store the commands and their arguments.
- Return: The number of commands parsed.
- int execute(char\* input)

- input: A string containing the entire command input.
- Return: 0 on success, -1 on failure.
- void execute_pipeline(char\* args[MAX_PIPE_COMMANDS][MAX_CMD_LINE_ARGS], int n)

- args: A 2D array containing the commands and their arguments.
- n: The number of commands to execute.

## Constants

- MAX_PIPE_COMMANDS: The maximum number of commands in a pipeline.
- MAX_CMD_LINE_ARGS: The maximum number of arguments per command.

## Demo:

![](./demo.gif)

## Contributor

Erik Williams
