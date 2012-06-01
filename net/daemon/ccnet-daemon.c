/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "session.h"
#include "log.h"


CcnetSession  *session;

static const char *short_options = "hvdp:c:D:f:M";
static struct option long_options[] = {
    { "help", no_argument, NULL, 'h', }, 
    { "version", no_argument, NULL, 'v', }, 
    { "port", required_argument, NULL, 'p', }, 
    { "config-dir", required_argument, NULL, 'c' },
    { "logfile", required_argument, NULL, 'f' },
    { "debug", required_argument, NULL, 'D' },
    { "daemon", no_argument, NULL, 'd' },
    { "no-multicast", no_argument, NULL, 'M' },
    { NULL, 0, NULL, 0, },
};


static void usage()
{
    fputs( 
"usage: ccnet [OPTIONS]\n\n"
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
"    -M\n"
"        disable multicasting for peer discovery in local network\n"
"    -f LOG_FILE\n"
"        Log file path\n",
        stdout);
}

int
main (int argc, char **argv)
{
    int c;
    int disable_multicast = 0;
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
        case 'M':
            disable_multicast = 1;
            break;
        default:
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

#ifndef WIN32
    if (daemon_mode)
        daemon (1, 0);
#else
    WSADATA     wsadata;
    WSAStartup(0x0101, &wsadata);
#endif

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

    session->disable_multicast = disable_multicast;
   
    ccnet_session_start (session);

    return 0;
}

