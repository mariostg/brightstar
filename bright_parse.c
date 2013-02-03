#include "bright_parse.h"

config_s *config=NULL;
config_s *init_config(void)
{
    config_s *config=calloc(1, sizeof(config_s));
    if(!config) return NULL;
    config->op=OP_MAIN; //This will be to ensure we have only one operation provided 
                        //on the command line.
    return config;
}

int parsearg_op(int opt)
{
    switch(opt)
    {
        case 'D':config->op = (config->op != OP_MAIN ? 0 : OP_DISPLAY); break;
        case 'S':config->op = (config->op != OP_MAIN ? 0 : OP_SYSTEM); break;
        case 'h':config->help = 1; break;
        default: return 1;
    }
    return 0;
}

int parsearg_system(int opt)
{
    switch(opt)
    {
        case 'd':config->op_s_download = 1; break;
        case 'h':config->op_s_help = 1; break;
        case 'i':config->op_s_install = 1; break;
        case 's':config->op_s_sync = 1; break;
        case 'u':config->op_s_uninstall = 1; break;
        default: return 1;
    }
    return 0;
}

int parsearg_display(int opt)
{
    switch(opt)
    {
        case 'a':config->op_d_all_pkgname = 1; break; 
        case 'd':config->op_d_descpkg = 1; break; 
        case 'h':config->op_d_help = 1; break; 
        case 'r':config->op_d_readme = 1; break; 
        case 'c':config->op_d_changelog = 1; break; 
        case 'm':config->op_d_match_name = 1; break; 
        default: return 1;
    }
    return 0;
}

int parse_args(int argc, char *argv[])
{
    int opt;
    int option_index = 0;
    const char *optstring = ":DSacdhimrspu";
    struct option long_options[] =
    {
        {"display",no_argument, 0, 'D'},
        {"system",no_argument, 0, 'S'},
        {"all",no_argument, 0, 'a'},
        {"changelog",no_argument, 0, 'c'},
        {"download",no_argument, 0, 'd'},
        {"describe",no_argument, 0, 'd'},
        {"help",no_argument, 0, 'h'},
        {"install",no_argument, 0, 'i'},
        {"match",no_argument, 0, 'm'},
        {"readme",no_argument, 0, 'r'},
        {"package",no_argument, 0, 'p'},
        {"sync",no_argument, 0, 's'},
        {"uninstall",no_argument, 0, 'u'},
        {0, 0, 0, 0}
    };

    while((opt = getopt_long(argc, argv, optstring, long_options, &option_index))) {
        if(opt < 0) {
            break;
        } else if(opt == 0) {
            continue;
        } else if(opt == '?') {
            printf("%s: option '-%c' is invalid: ignored\n", argv[0], optopt);
        }
        parsearg_op(opt);
    }

    if(config->op==0)
    {
        printf("%s", "Only one operation at a time allowed");
        return 1;
    }

    optind=1;
    //puts("2nd while");
    while((opt = getopt_long(argc, argv, optstring, long_options, &option_index))) 
    {
        if(opt < 0) {
            break;
        } else if(opt == 0) {
            continue;
        } else if(opt == '?') {
            return 1;
        }
        switch(config->op)
        {
            case OP_SYSTEM:
                parsearg_system(opt);
                break;
            case OP_DISPLAY:
                parsearg_display(opt);
                break;
            default:
                return 1;
        }
    }
    return 0;
}

