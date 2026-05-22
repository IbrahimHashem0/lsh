/***************************************************************************//**
  @file        main.c
 @author      Ibrahim Hashem (Modified Structure)
  @brief       Customized LSH with new builtin commands for OS Project
*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// --- Custom Environment and Command History tracking ---
#define HISTORY_MAX_SIZE 150
char *session_history[HISTORY_MAX_SIZE];
int total_history_records = 0;

extern char **environ;

void log_command_history(char *input_line) {
    if (input_line == NULL || strlen(input_line) == 0) return;
    
    if (total_history_records < HISTORY_MAX_SIZE) {
        session_history[total_history_records] = strdup(input_line);
        total_history_records++;
    } else {
        free(session_history[0]);
        for (int idx = 1; idx < HISTORY_MAX_SIZE; idx++) {
            session_history[idx - 1] = session_history[idx];
        }
        session_history[HISTORY_MAX_SIZE - 1] = strdup(input_line);
    }
}

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

// --- My New Custom Builtin Prototypes ---
int lsh_pwd(char **args);
int lsh_echo(char **args);
int lsh_history(char **args);
int lsh_env(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "pwd",     
  "echo",    
  "history", 
  "env"      
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &lsh_pwd,     
  &lsh_echo,    
  &lsh_history, 
  &lsh_env      
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Builtin command: change directory.
 */
int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

/**
   @brief Builtin command: print help.
 */
int lsh_help(char **args)
{
  int i;
  printf("Custom LSH - Shell Project\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit.
 */
int lsh_exit(char **args)
{
  return 0;
}

/*
  Implementation of Builtin Command: pwd
  Prints the current working directory path.
*/
int lsh_pwd(char **args) {
    char current_path[2048];
    if (getcwd(current_path, sizeof(current_path)) != NULL) {
        printf("%s\n", current_path);
    } else {
        perror("lsh: error retrieving directory");
    }
    return 1;
}

/*
  Implementation of Builtin Command: echo
  Prints the arguments passed to the terminal screen.
*/
int lsh_echo(char **args) {
    int index = 1;
    for (; args[index] != NULL; index++) {
        printf("%s", args[index]);
        if (args[index + 1] != NULL) {
            printf(" ");
        }
    }
    printf("\n");
    return 1;
}

/*
  Implementation of Builtin Command: history
  Displays the list of previously entered commands.
*/
int lsh_history(char **args) {
    if (total_history_records == 0) {
        printf("History cache is empty.\n");
        return 1;
    }
    int counter = 0;
    while (counter < total_history_records) {
        printf(" [%d]  %s\n", counter + 1, session_history[counter]);
        counter++;
    }
    return 1;
}

/*
  Implementation of Builtin Command: env
  Prints all environment variables in KEY=VALUE format.
*/
int lsh_env(char **args) {
    char **current_env = environ;
    while (*current_env != NULL) {
        printf("%s\n", *current_env);
        current_env++;
    }
    return 1;
}

/**
  @brief Launch a program and wait for it to terminate.
 */
int lsh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief Execute shell built-in or launch program.
 */
int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

/**
   @brief Read a line of input from stdin.
 */
char *lsh_read_line(void)
{
#ifdef LSH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0;
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);
    } else  {
      perror("lsh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define LSH_RL_BUFSIZE 1024
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens.
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = lsh_read_line();
    
    // --- Call my custom log history function ---
    log_command_history(line);

    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

/**
   @brief Main entry point.
 */
int main(int argc, char **argv)
{
  lsh_loop();
  return EXIT_SUCCESS;
}
