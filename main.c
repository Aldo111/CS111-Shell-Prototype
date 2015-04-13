// UCLA CS 111 Lab 1 main program

// Copyright 2012-2014 Paul Eggert.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.




#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <wait.h>
#include <stdint.h>
#include <sys/resource.h>

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "command.h"

#define BILLION 1000000000L

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-p PROF-FILE | -t] SCRIPT-FILE", program_name);
}

void print_profile_main (int pid ,struct  timespec start,struct timespec end, char * prof_log);

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  int command_number = 1;
  bool print_tree = false;
  char const *profile_name = 0;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "p:t"))
      {
      case 'p': profile_name = optarg; break;
      case 't': print_tree = true; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);
  int profiling = -1;
  if (profile_name)
    {
      profiling = prepare_profiling (profile_name);
      if (profiling < 0)
	error (1, errno, "%s: cannot open", profile_name);
    }

  command_t last_command = NULL;
  command_t command;
  struct timespec start;
  struct timespec end;
  clock_gettime(CLOCK_REALTIME, &start);

  
  while ((command = read_command_stream (command_stream)))
    {
      if (print_tree)
	{
	  printf ("# %d\n", command_number++);
	  print_command (command);
	}
      else
	{
	  last_command = command;

	 
	  execute_command (command, profiling);
	  //profiling
	  
	}
    }
  //profiling
  clock_gettime(CLOCK_REALTIME, &end);
  
  if (profiling == 1 && profile_name)
    {
      print_profile_main(getpid(), start, end, profile_name);
    }

  
  return print_tree || !last_command ? 0 : command_status (last_command);
}





void print_profile_main (int pid ,struct  timespec start,struct timespec end, char * prof_log) {
  
  struct rusage cpu_proc;
  //USER TIME
  struct timeval sys;
  struct timeval usr;
  getrusage(RUSAGE_SELF, &cpu_proc);
  sys = cpu_proc.ru_stime;
  usr = cpu_proc.ru_utime;
	  
  //sys time
  uint64_t sysTime= 1000000 * (sys.tv_sec) + (sys.tv_usec);
  double sys_sec=sysTime/1000000.0;
	  
  //user time
  uint64_t usrTime= 1000000 * (usr.tv_sec) + (usr.tv_usec);
  double usr_sec=usrTime/1000000.0;
	  

  //printf("Usr at: %llu.%.3ld\n", usr_sec, usr_deci);


  //REAL TIME
  uint64_t diff;
  clock_gettime(CLOCK_REALTIME, &end);
  diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;

  //since-EPOCH time
  struct timeval tv;

  gettimeofday(&tv, NULL);


  uint64_t sEpoch=1000000*(tv.tv_sec) + (tv.tv_usec);
  long int epoch_sec=sEpoch/1000000;
  long int epoch_deci=(sEpoch%1000000)/10000;
	  
	  
  //Time Difference between start and end
  double sec=diff/1000000000.00;
  //long int nsec=(diff%1000000000)/1000000;

	  
  //open the file we write to
  FILE *fp;
  fp = fopen(prof_log, "a");
  fprintf(fp, "%llu.%.2ld %.3f %.3f %.3f [%d]",  epoch_sec, epoch_deci, sec, usr_sec, sys_sec, pid);
  fprintf(fp,"\n");
	  
  fclose(fp);//close file
	 

}
