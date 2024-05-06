# Tai-shell

A simple shell written in C for educational purposes in the Operating Systems discipline.

## Requirements:
- `$ prog`: Shell waits for the child process.
- `$ prog &`: Program runs in the background to avoid zombie processes.
- `$ prog parameters`: Use `strtok()` to handle parameters.
- `$ prog parameters &`: Program runs in the background.
- `$ prog [parameters] > file`: Redirect output to a file using `dup2()`.
- `$ prog1 / prog2`: Execute program with a pipe.

Created by [Eduardo Souza Malagutti](https://github.com/eduMalagutti) and [Vitor Taichi Taira](https://github.com/TaiFile) as an exercise in UFScar's Operating Systems course.
