/*
    NAME:      Mikal Brown
    CLASS:     WPI CS 5008
    TITLE:     Operating Systems – Process + Shell Project
    DEADLINE:  April 9, 2024

    Program Overview:
    ------------------------------------------------------------
    Built a lightweight shell that works on both Windows and Linux.
    Nothing fancy, just the essentials — run commands, switch dirs,
    change your prompt, and toss tasks in the background.

    It tracks system stats like CPU time and memory usage so I can
    actually *see* what’s going on under the hood when a command runs.

    On Windows, it wraps everything in "cmd.exe /c" so I can run built-ins
    like 'dir' and 'echo' without breaking. On Linux, it’s all fork and exec,
    like a proper Unix system should be.

    Point of this project was to stop treating the shell like magic
    and actually understand how it works. No hand-holding, no system(),
    just raw syscalls and control.
    
     Commands This Shell Can Handle:
    cd ..                   : move up a folder
    set prompt = mysh:      : update your prompt to "mysh:"
    echo Hello World        : print something to the screen
    dir / ls                : list files
    pwd                     : print current directory
    sleep 5 &               : run sleep in background
    jobs                    : show background processes
    exit                    : close the shell when you're done
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
typedef unsigned long long ULONGLONG;
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#define MAX_INPUT 128
#define MAX_ARGS 32
#define MAX_JOBS 10

typedef struct {
    int pid;
    char cmd[MAX_INPUT];
} Job;

Job background_jobs[MAX_JOBS];
int job_count = 0;

#ifdef _WIN32
ULONGLONG filetime_to_ull(FILETIME ft) {
    return (((ULONGLONG)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}
#endif

void execute_command(char *args[], int background) {
#ifdef _WIN32
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    char cmdline[MAX_INPUT] = "";
    for (int i = 0; args[i] != NULL; i++) {
        strcat(cmdline, args[i]);
        strcat(cmdline, " ");
    }

    // Wrap command in cmd.exe /c
    char full_command[MAX_INPUT + 20];
    snprintf(full_command, sizeof(full_command), "cmd.exe /c %s", cmdline);

    DWORD start_time = GetTickCount();

    if (!CreateProcess(NULL, full_command, NULL, NULL, FALSE,
        CREATE_NEW_PROCESS_GROUP, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "CreateProcess failed (%lu)\n", GetLastError());
        return;
    }

    if (!background) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD end_time = GetTickCount();

        FILETIME creation, exit, kernel, user;
        if (GetProcessTimes(pi.hProcess, &creation, &exit, &kernel, &user)) {
            ULONGLONG userTime = filetime_to_ull(user);
            ULONGLONG kernelTime = filetime_to_ull(kernel);

            printf("\nStatistics:\n");
            printf("Elapsed time: %lu ms\n", end_time - start_time);
            printf("User CPU time: %llu ms\n", userTime / 10000);
            printf("Kernel CPU time: %llu ms\n", kernelTime / 10000);
        }
    } else {
        if (job_count < MAX_JOBS) {
            background_jobs[job_count].pid = pi.dwProcessId;
            strncpy(background_jobs[job_count].cmd, args[0], MAX_INPUT - 1);
            printf("[%d] %lu\n", job_count + 1, pi.dwProcessId);
            job_count++;
        }
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

#else
    struct timeval start, end;
    struct rusage usage;
    pid_t pid;
    int status;

    gettimeofday(&start, NULL);

    if ((pid = fork()) < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        if (!background) {
            waitpid(pid, &status, 0);

            gettimeofday(&end, NULL);
            getrusage(RUSAGE_CHILDREN, &usage);

            long elapsed = (end.tv_sec - start.tv_sec) * 1000 +
                           (end.tv_usec - start.tv_usec) / 1000;
            long user = usage.ru_utime.tv_sec * 1000 + usage.ru_utime.tv_usec / 1000;
            long sys = usage.ru_stime.tv_sec * 1000 + usage.ru_stime.tv_usec / 1000;

            printf("\nStatistics:\n");
            printf("User CPU time: %ld ms\n", user);
            printf("System CPU time: %ld ms\n", sys);
            printf("Elapsed time: %ld ms\n", elapsed);
            printf("Page faults: %ld minor, %ld major\n",
                   usage.ru_minflt, usage.ru_majflt);
        } else {
            if (job_count < MAX_JOBS) {
                background_jobs[job_count].pid = pid;
                strncpy(background_jobs[job_count].cmd, args[0], MAX_INPUT - 1);
                printf("[%d] %d\n", job_count + 1, pid);
                job_count++;
            }
        }
    }
#endif
}

void check_background_jobs() {
    for (int i = 0; i < job_count; i++) {
#ifdef _WIN32
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, background_jobs[i].pid);
        if (hProcess) {
            DWORD exitCode;
            if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                printf("[%d] %d Completed\n", i + 1, background_jobs[i].pid);
                CloseHandle(hProcess);
                for (int j = i; j < job_count - 1; j++) {
                    background_jobs[j] = background_jobs[j + 1];
                }
                job_count--;
                i--;
            }
        }
#else
        int status;
        pid_t result = waitpid(background_jobs[i].pid, &status, WNOHANG);
        if (result > 0) {
            printf("[%d] %d Completed\n", i + 1, background_jobs[i].pid);
            for (int j = i; j < job_count - 1; j++) {
                background_jobs[j] = background_jobs[j + 1];
            }
            job_count--;
            i--;
        }
#endif
    }
}

void interactive_shell() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    char prompt[32] = "==>";

    while (1) {
        check_background_jobs();

        printf("%s ", prompt);
        fflush(stdout);

        if (!fgets(input, MAX_INPUT, stdin)) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';
        if (strlen(input) == 0) continue;

        int arg_count = 0;
        char *token = strtok(input, " ");
        while (token && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        if (arg_count == 0) continue;

        if (strcmp(args[0], "exit") == 0) {
            while (job_count > 0) {
                check_background_jobs();
#ifdef _WIN32
                Sleep(1000);
#else
                sleep(1);
#endif
            }
            break;
        } else if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                fprintf(stderr, "cd: missing argument\n");
            }
#ifdef _WIN32
            else if (!SetCurrentDirectory(args[1])) {
                fprintf(stderr, "cd: %lu\n", GetLastError());
            }
#else
            else if (chdir(args[1]) != 0) {
                perror("cd");
            }
#endif
        } else if (strcmp(args[0], "set") == 0 &&
                   arg_count >= 4 &&
                   strcmp(args[1], "prompt") == 0 &&
                   strcmp(args[2], "=") == 0) {
            strncpy(prompt, args[3], sizeof(prompt) - 1);
        } else if (strcmp(args[0], "jobs") == 0) {
            for (int i = 0; i < job_count; i++) {
                printf("[%d] %d %s\n", i + 1,
                       background_jobs[i].pid, background_jobs[i].cmd);
            }
        } else {
            int background = (arg_count > 1 &&
                              strcmp(args[arg_count - 1], "&") == 0);
            if (background) {
                args[arg_count - 1] = NULL;
            }
            execute_command(args, background);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        interactive_shell();
    } else {
        execute_command(&argv[1], 0);
    }
    return 0;
}
