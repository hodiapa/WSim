
/**
 *  \file   options.c
 *  \brief  WSim command line options
 *  \author Antoine Fraboulet
 *  \date   2005
 **/

#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "arch/common/debug.h"
#include "config.h"
#include "options.h"
#include "mgetopt.h"

#define DEFAULT_VERBOSE            0
#define DEFAULT_PROGNAME           "prog.elf"
#define DEFAULT_SIM_MODE           SIM_MODE_RUN
#define DEFAULT_GDB_PORT           2159
#define DEFAULT_RUN_INSN           0
#define DEFAULT_RUN_TIME           0
#define DEFAULT_DO_DUMP            0
#define DEFAULT_DO_TRACE           0
#define DEFAULT_DO_ETRACE          0
#define DEFAULT_DO_ETRACE_AT_BEGIN 0

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

/* liblogger cannot be used during option parse */

#define __DEBUG_OPTIONS
#ifdef DEBUG_OPTIONS
#  define OPT_DMSG(x...)  fprintf(stderr,x)
#else
#  define OPT_DMSG(x...)  do { } while(0)
#endif

#define OPT_PRINT(x...)   fprintf(stdout,x)
#define OPT_ERROR(x...)   fprintf(stderr,x)
#define OPT_WARNING(x...) fprintf(stderr,x)

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

/* option list that is used during parsing */
static struct moption_list_t wsim_options;

/* base options */
static struct moption_t help_opt = {
  .longname    = "help",
  .type        = no_argument,
  .helpstring  = "print this message",
  .value       = NULL
};

/* run mode */
static struct moption_t mode_opt = {
  .longname    = "mode",
  .type        = required_argument,
  .helpstring  = "run mode [run,gdb,insn,time,console]",
  .value       = NULL
};

static struct moption_t modearg_opt = {
  .longname    = "modearg",
  .type        = required_argument,
  .helpstring  = "mode optional argument",
  .value       = NULL
};

/* traces & logs */
static struct moption_t logfile_opt = {
  .longname    = "logfile",
  .type        = required_argument,
  .helpstring  = "logfile (default wsim.log)",
  .value       = NULL
};

static struct moption_t version_opt = {
  .longname    = "version",
  .type        = no_argument,
  .helpstring  = "print simulator version",
  .value       = NULL
};

static struct moption_t verbose_opt = {
  .longname    = "verbose",
  .type        = required_argument,
  .helpstring  = "verbosity level (default 0)",
  .value       = NULL
};

static struct moption_t trace_opt = {
  .longname    = "trace",
  .type        = optional_argument,
  .helpstring  = "enable simulation trace",
  .value       = NULL
};

static struct moption_t etrace_opt = {
  .longname    = "esimu",
  .type        = optional_argument,
  .helpstring  = "enable energy trace",
  .value       = NULL
};

static struct moption_t etrace_start_opt = {
  .longname    = "esimu-start",
  .type        = no_argument,
  .helpstring  = "enable energy trace from simulation start",
  .value       = NULL
};

/* dump */
static struct moption_t dump_opt = {
  .longname    = "dump",
  .type        = optional_argument,
  .helpstring  = "dump state to file after exec",
  .value       = NULL
};

static struct moption_t preload_opt = {
  .longname    = "preload",
  .type        = required_argument,
  .helpstring  = "preload flash with hex file",
  .value       = NULL
};

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void options_add_base(struct moption_t *o)
{
  mgetopt_options_add(&wsim_options,o,mgetopt_base);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void options_add(struct moption_t *o)
{
  mgetopt_options_add(&wsim_options,o,mgetopt_optional);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void options_start()
{
  memset(&wsim_options,0,sizeof(struct moption_list_t));

  options_add_base(& help_opt           );
  options_add_base(& version_opt        );
  options_add_base(& mode_opt           );
  options_add_base(& modearg_opt        );
  options_add_base(& preload_opt        );
  /*  options_add_base(& dump_opt      ); */
  options_add_base(& logfile_opt        );
  options_add_base(& trace_opt          );
#if defined(ETRACE)
  options_add_base(& etrace_opt         );
  options_add_base(& etrace_start_opt   );
#endif
  options_add_base(& verbose_opt        );
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

static void options_usage(const char* progname)
{
  char msg[]="\
command line : %s [options] elf_file\n\n\
options:\n";
  /*
  char *cmd;
  cmd = strrchr(progname,'/');
  cmd = (cmd == NULL) ? progname : cmd;
  OPT_PRINT(msg,cmd);
  */
  OPT_PRINT(msg,basename((char*)progname));
  mgetopt_options_print(stdout,&wsim_options);
  OPT_PRINT("\n");
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void options_print_version(struct options_t UNUSED *s)
{
  OPT_PRINT("%s: version %s, build %d, date %s\n",PACKAGE,PACKAGE_VERSION,
	    WSIM_BUILD_ID, WSIM_BUILD_DATE);
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void options_print_params(struct options_t* s)
{
  struct stat fs;

  char msg[] = "\
  verbose level : %d\n\
  logfile name  : %s\n\
  trace file    : %s\n\
  elf file      : %s\n";

  OPT_PRINT("== Command line summary == \n");
  OPT_PRINT(msg,
	    s->verbose,
	    s->logfilename,
	    s->tracefile,
	    s->progname);

  switch (s->sim_mode)
    {
    case SIM_MODE_RUN:
      OPT_PRINT("  mode          : free run\n");
      break;
    case SIM_MODE_GDB:
      OPT_PRINT("  mode          : gdb on port %d\n",s->gdb_port);
      break;
    case SIM_MODE_INSN:
      OPT_PRINT("  mode          : insn %" PRId64 "\n",s->sim_insn);
      break;
    case SIM_MODE_TIME:
      OPT_PRINT("  mode          : time %" PRId64 " ns\n",s->sim_time);
      break;
    case SIM_MODE_CONS:
      OPT_PRINT("  mode          : console\n");
      break;
    default:
      OPT_PRINT("  mode          : unknown ?\n");
      break;
    }

  if (stat(s->progname, &fs) == -1)
    {
      fprintf(stdout," ** Cannot stat elf file\n");
    }
  OPT_PRINT("== Command line summary == \n");
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

void options_read_cmdline(struct options_t *s, int *argc, char *argv[])
{
  int   parseindex = 1;
  int   optindex;

  char STR_MISSING[] = "missing option argument for option %s\n";
  char STR_UNKNOWN[] = "unknown option %s\n";

  s->verbose     = DEFAULT_VERBOSE;

  strcpy (s->progname,  DEFAULT_PROGNAME);
#if defined(PID_IN_FILENAMES)
  sprintf(s->logfilename,"wsim-%d.log",getpid());
  sprintf(s->dumpfile,   "wsim-%d.dmp",getpid());
  sprintf(s->tracefile,  "wsim-%d.trc",getpid());
  sprintf(s->etracefile, "wsim-%d.etr",getpid());
#else
  sprintf(s->logfilename,"wsim.log");
  sprintf(s->dumpfile,   "wsim.dmp");
  sprintf(s->tracefile,  "wsim.trc");
  sprintf(s->etracefile, "wsim.etr");
#endif
  sprintf(s->preload,    "none");

  s->sim_mode           = DEFAULT_SIM_MODE;
  s->gdb_port           = DEFAULT_GDB_PORT;
  s->sim_insn           = DEFAULT_RUN_INSN;
  s->sim_time           = DEFAULT_RUN_TIME;
  s->do_dump            = DEFAULT_DO_DUMP;
  s->do_trace           = DEFAULT_DO_TRACE;
  s->do_etrace          = DEFAULT_DO_ETRACE;
  s->do_preload         = 0;
  s->do_etrace_at_begin = DEFAULT_DO_ETRACE_AT_BEGIN;

  /* parse all options */
  switch (mgetopt_long(&parseindex,*argc,argv, &wsim_options, &optindex))
    {
    case OPT_MISSING:
      fprintf(stderr,STR_MISSING,wsim_options.list[optindex]->longname);
      exit(1);
    case OPT_UNKNOWN:
      fprintf(stderr,STR_UNKNOWN,argv[parseindex]);
      options_usage(argv[0]);
      exit(1);
    case OPT_EOOPT: /* ok, do nothing */
      break;
    }


  /* mod values */
  if (help_opt.isset)
    {
      options_usage(argv[0]);
      exit(0);
    }

  if (dump_opt.isset)
    {
      s->do_dump = 1;
      if (dump_opt.value)
	strncpy(s->dumpfile,dump_opt.value,MAX_FILENAME);
    }

  if (logfile_opt.isset)
    {
      strncpy(s->logfilename,logfile_opt.value,MAX_FILENAME);
    }
  

  /* mode */
  if (mode_opt.isset)
    {
      char *optarg = modearg_opt.value;
      OPT_DMSG("mode set : ");

      if (strcmp(mode_opt.value,"run") == 0)
	{
	  OPT_DMSG(" run\n");
	  s->sim_mode = SIM_MODE_RUN;
	}
      else if (strcmp(mode_opt.value,"gdb") == 0)
	{
	  OPT_DMSG(" gdb\n");
	  s->sim_mode = SIM_MODE_GDB;
	  if (optarg)
	    s->gdb_port = atoi(optarg);
	}
      else if (strcmp(mode_opt.value,"insn") == 0)
	{
	  OPT_DMSG(" insn\n");
	  s->sim_mode  = SIM_MODE_INSN;
	  if (optarg)
	    {
	      uint32_t mul = 1;
	      switch (optarg[strlen(optarg) - 1])
		{
		case 'k':
		  mul = 1000;
		  break;
		case 'M':
		  mul = 1000 * 1000;
		  break;
		default:
		  mul = 1;
		  break;
		}	    
	      s->sim_insn = atoll(optarg) * mul;
	    }
	}
      else if (strcmp(mode_opt.value,"time") == 0)
	{
	  OPT_DMSG(" time\n");
	  s->sim_mode  = SIM_MODE_TIME;
	  if (optarg)
	    {
	      uint32_t mul = 1;
	      switch (optarg[strlen(optarg) - 1])
		{
		case 's':
		  mul = 1000 * 1000 * 1000;
		  break;
		case 'u':
		  mul = 1000 * 1000;
		  break;
		case 'm':
		  mul = 1000;
		  break;
		default:
		  mul = 1;
		  break;
		}
	      s->sim_time  = atoll(optarg) * mul;
	    }
	}
      else if (strcmp(mode_opt.value,"console") == 0)
	{
	  OPT_DMSG(" cons\n");
	  s->sim_mode  = SIM_MODE_CONS;
	}
      else
	{
	  OPT_ERROR("mode must be set to gdb|console|run|insn|time\n");
	  exit(1);
	}
    }
  else /* run mode not set */
    {
      s->sim_mode = SIM_MODE_RUN;
      if (modearg_opt.isset)
	{
	  OPT_ERROR("modearg must be used according to mode=[gdb|insn|time]\n");
	  exit(1);
	}
    }


  if (verbose_opt.isset)
    {
      OPT_DMSG(" verbose set\n");
      if (verbose_opt.value)
	s->verbose = atoi(verbose_opt.value);
    }


  if (trace_opt.isset)
    {
      s->do_trace = 1;
      if (trace_opt.value)
	strncpy(s->tracefile,trace_opt.value,MAX_FILENAME);
    }


  if (etrace_opt.isset)
    {
      s->do_etrace = 1;
      if (etrace_opt.value)
	strncpy(s->etracefile,etrace_opt.value,MAX_FILENAME);
    }

  if (etrace_start_opt.isset)
    {
      s->do_etrace = 1; /* --esimu-start implies --esimu */
      s->do_etrace_at_begin = 1;
    }

  if (version_opt.isset)
    {
      options_print_version(s);
    }

  if (preload_opt.isset)
    {
      s->do_preload = 1;
      if (preload_opt.value)
	strncpy(s->preload, preload_opt.value, MAX_FILENAME);
    }

  OPT_DMSG("parseindex = %d, argc = %d\n",parseindex,*argc);
  if (parseindex < *argc)
    {
      strncpy(s->progname,argv[parseindex],MAX_FILENAME);
    }
  else
    {
      if (s->sim_mode == SIM_MODE_GDB)
	{
	  OPT_WARNING("\n ** starting in GDB mode without program loaded ** \n\n");
	}
      else
	{
	  OPT_ERROR("\n ** missing exec name, no program loaded ** \n\n");
	  options_usage(argv[0]);
	  exit(1);
	}
    }
}

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */
