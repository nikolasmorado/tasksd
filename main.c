#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>


/*
 * NAME
 *      tasksd - hierarchical task manager
 *
 * SYNOPSIS
 *      tasksd [ <task> [ <description> ] ]
 *      tasksd ls <task>
 *      tasksd rm <task>
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
 *      Each file is has some hugo style frontmatter at the top for some things
 *      I plan to add, and then has a list of subtasks as a checklist
 *
 *
 * ARGUMENTS
 *      operation    can specify some operations (ls, rm)
 *      path         slash delimited task (ex: project/thing/task)
 *      description  quote wrapped text
 *
 * BEHAVIOUR
 *      (none)            open fzf browser for all tasks
 *      path              print description of task at path, fzf if no .taskd
 *      path description  create or overwrite task at path
 *      rm path           delete task
 *      ls path           list out subtasks
 *
 * EXAMPLES
 *      tasksd
 *      tasksd work/saas/github
 *      tasksd winder/electrical/power "build perfboard of power circuit"
 *      tasksd ls winder
 *      tasksd rm winder/cad
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
        int ret;
        if (stat(".local/state/tasksd", &st) != 0 || !S_ISDIR(st.st_mode))
                mkdir(".local/state/tasksd", 0755);


        ret = chdir(getenv("HOME"));
        if (ret == -1) {
                printf("error finding home dir");
                return -1;
        }

        ret = chdir(".local/state/tasksd");
        if (ret == -1) {
                printf("error finding tasks dir");
                return -1;
        }

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
                ret = system(fzfcmd);

                if (ret == 1) {
                        printf ("no tasks\n");
                        return 0;
                }

                if (ret == 2 || ret == 126 || ret == 127) {
                        printf ("fzf error: %d", ret);
                        return -1;
                }

                printf("no path\n");
                return 0;
        }

        if (argc == 2) {
                if (stat(argv[1], &st) == 0 && S_ISDIR(st.st_mode)) {
                        ret = chdir(argv[1]);

                        if (ret == -1) {
                                return -1;
                        } 

                        ret = system(fzfcmd);

                        if (ret == 1) {
                                printf ("no tasks\n");
                                return 0;
                        }

                        if (ret == 2 || ret == 126 || ret == 127) {
                                printf ("fzf error: %d", ret);
                                return -1;
                        }

                        return 0;
                }

                char fp[strlen(argv[1] + 8)];
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

        if (strcmp(argv[1], "rm") == 0) {
                char fp[strlen(argv[2]) + 8];
                sprintf(fp, "%s/.taskd", argv[2]);
                unlink(fp);
                char *p = fp + strlen(fp);
                while (p > fp) {
                        rmdir(fp);
                        while (p > fp && *--p != '/');
                        *p = '\0';
                }
                return 0;
        }

        if (strcmp(argv[1], "ls") == 0) {
                if (stat(argv[2], &st) == 0 && S_ISDIR(st.st_mode)) {
                        DIR *d = opendir(argv[2]);
                        
                        if (!d) {
                                return -1;
                        }
                        
                        struct dirent *e;
                        
                        while ((e = readdir(d))) { 

                                if (
                                        strcmp(".", e->d_name) == 0 ||
                                        strcmp("..", e->d_name) == 0 ||
                                        strcmp(".taskd", e->d_name) == 0
                                ) {
                                        continue;
                                }

                                printf("%s \n", e->d_name);
                        }       

                        return 0;
                }
        }

        if (strcmp(argv[1], "done") == 0) {
                char fp[strlen(argv[2]) + 8];
                sprintf(fp, "%s/.taskd", argv[2]);
                FILE *f = fopen(fp, "r+");

                if (!f) {
                        printf("no file\n");
                        return -1;
                }

                fseek(f, 0, SEEK_END);
                long sz = ftell(f);

                rewind(f);

                char *buf = malloc(sz + 1);
                size_t fret = fread(buf, 1, sz, f);

                if (fret != (size_t)sz) {
                        free(buf);
                        printf("read err\n");
                        return -1;
                }

                buf[sz] = '\0';

                char *p = buf;
                int c = 0;

                while (*p) {
                        if(
                                strncmp(p, "- [", 3) == 0 &&
                                c++ == atoi(argv[3])
                        ) {
                                p[3] = 'x';
                                break;
                        }
                        while (*p && *p != '\n') p++;
                        if (*p) p++;
                }

                rewind(f);
                fwrite(buf, 1, sz, f);
                fflush(f);

                free(buf);
                fclose(f);

                return 0;
        }


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

        int e = stat(fp, &st);
        FILE *f = fopen(fp, "a+");

        if (!f) {
                printf("no file\n");
                return -1;
        }

        if (e != 0) {
                fprintf(f, "priority: \nblockers: \n---\n");
        }

        fprintf(f, "- [ ] %s\n", argv[2]);

        return 0;
}

