/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "session.h"
#include "log.h"

char *pidfile = NULL;
CcnetSession  *session;


static void
remove_pidfile (const char *pidfile)
{
    if (pidfile) {
        g_unlink (pidfile);
    }
}

static int
write_pidfile (const char *pidfile_path)
{
    if (!pidfile_path)
        return -1;

    pid_t pid = getpid();

    FILE *pidfile = fopen(pidfile_path, "w");
    if (!pidfile) {
        ccnet_warning ("Failed to fopen() pidfile %s: %s\n",
                       pidfile_path, strerror(errno));
        return -1;
    }

    char buf[32];
    snprintf (buf, sizeof(buf), "%d\n", pid);
    if (fputs(buf, pidfile) < 0) {
        ccnet_warning ("Failed to write pidfile %s: %s\n",
                       pidfile_path, strerror(errno));
        return -1;
    }

    fflush (pidfile);
    fclose (pidfile);
    return 0;
}

static void
on_ccnet_exit(void)
{
    if (pidfile)
        remove_pidfile (pidfile);
}


static const char *short_options = "hvdc:D:f:P:";
static struct option long_options[] = {
    { "help", no_argument, NULL, 'h', }, 
    { "version", no_argument, NULL, 'v', }, 
    { "config-dir", required_argument, NULL, 'c' },
    { "logfile", required_argument, NULL, 'f' },
    { "debug", required_argument, NULL, 'D' },
    { "daemon", no_argument, NULL, 'd' },
    { "pidfile", required_argument, NULL, 'P' },
    { NULL, 0, NULL, 0, },
};


static void usage()
{
    fputs( 
"usage: ccnet-server [OPTIONS]\n\n"
"Supported OPTIONS are:\n"
"    -c CONFDIR\n"
"        Specify the ccnet configuration directory. Default is ~/.ccnet\n"
"    -d\n"
"        Run ccnet as a daemon\n"
"    -D FLAGS\n"
"        Specify debug flags for logging, for example\n"
"             Peer,Group,Processor\n"
"        supported flags are\n"
"             Peer,Group,Processor,Requirement,Routing,Netio,\n"
"             Message,Connection,File,Other\n"
"        or ALL to enable all debug flags\n"
"    -f LOG_FILE\n"
"        Log file path\n"
"    -P PIDFILE\n"
"        Specify the file to store pid\n",
        stdout);
}

int
main (int argc, char **argv)
{
    int c;
    char *config_dir;
    char *log_file = 0;
    const char *debug_str = 0;
    int daemon_mode = 0;
    const char *log_level_str = "debug";

    config_dir = DEFAULT_CONFIG_DIR;

    
    while ((c = getopt_long (argc, argv, short_options, 
                             long_options, NULL)) != EOF) {
        switch (c) {
        case 'h':
            usage();
            exit(0);
            break;
        case 'v':
            exit (1);
            break;
        case 'c':
            config_dir = optarg;
            break;
        case 'f':
            log_file = optarg;
            break;
        case 'D':
            debug_str = optarg;
            break;
        case 'd':
            daemon_mode = 1;
            break;
        case 'P':
            pidfile = optarg;
            break;
        default:
            fprintf (stderr, "unknown option \"-%c\"\n", (char)c);
            usage();
            exit (1);
        }
    }

        
    argc -= optind;
    argv += optind;

    if (config_dir == NULL) {
        fprintf (stderr, "Missing config dir\n");
        exit (1);
    }

    if (daemon_mode)
        daemon (1, 0);

    g_type_init ();

    /* log */
    if (!debug_str)
        debug_str = g_getenv("CCNET_DEBUG");
    ccnet_debug_set_flags_string (debug_str);

    config_dir = ccnet_expand_path (config_dir);

    if (!log_file) {
        char *logdir = g_build_filename (config_dir, "logs", NULL);
        checkdir_with_mkdir (logdir);
        g_free (logdir);
        log_file = g_build_filename (config_dir, "logs", "ccnet.log", NULL);
    }
    if (ccnet_log_init (log_file, log_level_str) < 0) {
        fprintf (stderr, "ccnet_log_init error: %s, %s\n", strerror(errno),
                 log_file);
        exit (1);
    }

    srand (time(NULL));

    session = ccnet_session_new (config_dir);
    if (!session) {
        fputs ("Error: failed to start ccnet session, "
               "see log file for the detail.\n", stderr);
        return -1;
    }
        
    if (ccnet_session_prepare(session) < 0) {
        fputs ("Error: failed to start ccnet session, "
               "see log file for the detail.\n", stderr);
        return -1;
    }
    
    /* write pidfile after session_prepare success, if there is
     * another instance of ccnet session_prepare will failed.
     */
    if (pidfile) {
        if (write_pidfile (pidfile) < 0) {
            ccnet_message ("Failed to write pidfile\n");
            return -1;
        }
    }
    atexit (on_ccnet_exit);

    ccnet_session_start (session);

    return 0;
}

