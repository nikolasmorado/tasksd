#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

/*
 * NAME
 *      tasksd - hierarchical task manager
 *
 * SYNOPSIS
 *      tasksd [ <task> [ <description> ] ]
 *
 * DESCRIPTION
 *      Stores tasks in file hierarchy in tasksd dir
 *
 *      Example: 
 *
 *     ~/.local/state/tasksd
 *       └── project
 *           ├── api
 *           │   └── investigation
 *           │       └── .taskd
 *           ├── infra
 *           │   └── .taskd
 *           └── ui
 *               ├── canvas
 *               │   └── generation
 *               │       └── .taskd
 *               ├── chat
 *               │   ├── credits
 *               │   │   └── .taskd
 *               │   └── models
 *               │       └── .taskd
 *               └── navbar
 *                   └── .taskd
 *
 *      Each file is just the flat task details.
 *
 *
 * ARGUMENTS
 *      path         slash delimited task (ex: project/thing/task)
 *      description  quote wrapped text
 *
 * BEHAVIOUR
 *      (none)            open fzf browser for all tasks
 *      path              print description of task at path, fzf if no .taskd
 *      path description  create or overwrite task at path
 *
 * EXAMPLES
 *      tasksd
 *      tasksd work/saas/github
 *      tasksd winder/electrical/power "build perfboard of power circuit"
 *
 * DEPENDENCIES
 *      fzf(1)  required for no arg mode
 *
 * FILES
 *      ~/.local/state/tasksd/
 */

// garbage one shotter

int
main(int argc, char *argv[]) {
        struct stat st;
        if (stat(".local/state/tasksd", &st) != 0 || !S_ISDIR(st.st_mode))
                mkdir(".local/state/tasksd", 0755);

        chdir(getenv("HOME"));
        chdir(".local/state/tasksd");

        char *fzfcmd =
                "find . -name '.taskd' | sed 's|^./||; s|/.taskd$||' | sort | "
                "fzf "
                "--reverse "
                "--style minimal "
                "--height 12 "
                "--cycle "
                "--no-unicode "
                "--preview 'cat {1}/.taskd'";

        if (argc <= 1) {
                system(fzfcmd);
                printf("no path\n");
                return 0;
        }

        if (argc == 2) {

                if (stat(argv[1], &st) == 0 && S_ISDIR(st.st_mode)) {
                                chdir(argv[1]);
                                system(fzfcmd);
                                return 0;
                }

                char fp[strlen(argv[1]) + 8];
                sprintf(fp, "%s/.taskd", argv[1]);
                FILE *f = fopen(fp, "r");

                if (!f) {
                        printf("no file\n");
                        return -1;
                }

                char buf[512];
                char *p = buf;
                int c;
                while ( (c = fgetc(f)) != EOF ) {
                        *p = c;
                        p++;
                }

                printf("%s", buf);

                return 0;
        }

        if (argc < 3)
                return -1;
        

        char *p = argv[1];
        while (*p != '\0') {
                if (*p == '/') {
                        *p = '\0';
                        mkdir(argv[1], 0755);
                        *p = '/';
                }
                p++;
        }

        mkdir(argv[1], 0755);

        char fp[strlen(argv[1]) + 8];
        sprintf(fp, "%s/.taskd", argv[1]);
        FILE *f = fopen(fp, "w+");

        if (!f) {
                printf("no file\n");
                return -1;
        }

        fprintf(f, "%s", argv[2]);

        return 0;
}

