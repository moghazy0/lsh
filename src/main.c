#define _GNU_SOURCE

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_HISTORY 100

// ================= HISTORY =================
char *history[MAX_HISTORY];
int history_count = 0;

// ================= BUILTIN DECLARATIONS =================
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_pwd(char **args);
int lsh_echo(char **args);
int lsh_history(char **args);
int lsh_env(char **args);

// ================= BUILTIN TABLE =================
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

// ================= BUILTINS =================

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

int lsh_help(char **args)
{
    printf("LSH Shell\n");
    printf("Built-in commands:\n");

    for (int i = 0; i < lsh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    return 1;
}

int lsh_exit(char **args)
{
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }
    return 0;
}

int lsh_pwd(char **args)
{
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("lsh");
    }

    return 1;
}

int lsh_echo(char **args)
{
    if (args[1] == NULL) {
        printf("\n");
        return 1;
    }

    for (int i = 1; args[i] != NULL; i++) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL)
            printf(" ");
    }

    printf("\n");
    return 1;
}

int lsh_history(char **args)
{
    for (int i = 0; i < history_count; i++) {
        printf("%d | %s\n", i + 1, history[i]);
    }
    return 1;
}

int lsh_env(char **args)
{
    extern char **environ;

    if (environ == NULL) return 1;

    for (char **env = environ; *env != NULL; env++) {
        printf("%s\n", *env);
    }

    return 1;
}

// ================= EXECUTION =================

int lsh_launch(char **args)
{
    pid_t pid = fork();
    int status;

    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } 
    else if (pid < 0) {
        perror("lsh");
    } 
    else {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int lsh_execute(char **args)
{
    if (args[0] == NULL)
        return 1;

    for (int i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return lsh_launch(args);
}

// ================= INPUT =================

char *lsh_read_line(void)
{
#define LSH_RL_BUFSIZE 1024

    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(bufsize);

    if (!buffer) {
        fprintf(stderr, "allocation error\n");
        exit(EXIT_FAILURE);
    }

    int c;

    while (1) {
        c = getchar();

        if (c == EOF || c == '\n') {
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
                fprintf(stderr, "allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

char **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE;
    int position = 0;

    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    token = strtok(line, LSH_TOK_DELIM);

    while (token != NULL) {
        tokens[position++] = token;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }

    tokens[position] = NULL;
    return tokens;
}

// ================= LOOP =================

void lsh_loop(void)
{
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = lsh_read_line();

        // remove newline
        line[strcspn(line, "\n")] = 0;

        if (strlen(line) > 0 && history_count < MAX_HISTORY) {
            history[history_count++] = strdup(line);
        }

        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);

    } while (status);
}

// ================= MAIN =================

int main(int argc, char **argv)
{
    lsh_loop();
    return EXIT_SUCCESS;
}