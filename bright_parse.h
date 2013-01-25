#ifndef BRIGHT_PARSE_H
#define BRIGHT_PARSE_H
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

typedef struct config_s{
    unsigned int op;
    unsigned int op_s_download;
    unsigned int op_s_help;
    unsigned int op_s_install;
    unsigned int op_s_sync;
    unsigned int op_s_uninstall;
    unsigned int op_d_all_pkgname;
    unsigned int op_d_changelog;
    unsigned int op_d_descpkg;
    unsigned int op_d_help;
    unsigned int op_d_match_name;
    unsigned int op_d_readme;
    unsigned int help;
} config_s;

extern config_s *config;

enum{OP_MAIN=1, OP_SYSTEM, OP_DISPLAY};


config_s *init_config(void);
int parse_args(int argc, char *argv[]);
int parsearg_op(int opt);
int parsearg_system(int opt);
int parsearg_display(int opt);
#endif /* BRIGHT_PARSE_H*/
