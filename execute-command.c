// UCLA CS 111 Lab 1 command execution
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

#include "command.h"
#include "command-internals.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <error.h>
#include <wait.h>
#include <time.h>
#include <stdint.h>
#include <sys/resource.h>
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
void print_profile (int pid2 , struct timespec start2, struct timespec end2, struct rusage cpu_proc2);

#define BILLION 1000000000L

char const *prof_log;

int
prepare_profiling (char const *name)
{
  prof_log=name; //store profile log

  //empty the log file for this session if it exists
  FILE * fp;
  fp=fopen(name,"w");
  fclose(fp);
  return 1;
}

int
command_status (command_t c)
{
  return c->status;
}


//int file_in;
//int file_out;

void
execute_command (command_t c, int profiling)
{
  //printf("Execute Command");
  //  printf("Profiling is:%d\n", profiling);
  if (c==NULL) return;

  pid_t pid;
  int status;
  int pipefd[2];
  int r=0;
  int w=1;
  if (c->type == SIMPLE_COMMAND)
    {
      //timing stuff
      struct timespec start;
      struct timespec end;
      clock_gettime(CLOCK_REALTIME, &start);
      struct rusage cpu_proc;
      //==

      pid=fork();

      if (pid==-1) return;

      if (pid==0)
	{
	  int file_in;
	  int file_out;
	  int stdin=dup(0);
	  int stdout=dup(1);
	  int exit_status = 0;
	  int i = 0;
	  int bs;
	  close(0);
	  close(1);

	  if (c->input!=NULL) file_in = open(c->input, O_RDONLY);
	  else
	    {
	      dup2(stdin,0);
	      close(stdin);
	    }
	  if (c->output!=NULL) { FILE * fp; fp=fopen(c->output,"w"); fclose(fp);
	    //after clearing out the file we output to, we now output to it
	    file_out = open(c->output, O_WRONLY); }
	  else
	    {
	      dup2(stdout,1);
	      close(stdout);

	    }

	  if (strcmp(*c->u.word, "exec") == 0)
	  {
	    c->u.word++;
	  }
	  exit_status = execvp(*c->u.word,c->u.word);
	  if (exit_status)
	    error(1,0,"execvp returned non-zero.");
	  if(c->input!=NULL) {  close(file_in); dup2(stdin,0); close(stdin); }
	  if(c->output!=NULL) { close(file_out); dup2(stdout,1); close(stdout);}
	  _exit(1); //if code reaches this far, it has failed
	}
      else
	{

	  waitpid(pid, &status, 0);

	  //USER TIME
	  struct timeval sys;
	  struct timeval usr;
	  getrusage(RUSAGE_CHILDREN, &cpu_proc);
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

	  if (profiling==1) {
	    //open the file we write to
	    FILE *fp;
	    fp = fopen(prof_log, "a");
	    fprintf(fp, "%llu.%.2ld %.3f %.3f %.3f ",  epoch_sec, epoch_deci, sec, usr_sec, sys_sec);
	    int i=0;
	    for (i=0; c->u.word[i]!=NULL; i++) {

	      fprintf(fp,"%s ", c->u.word[i]);

	    }

	    fprintf(fp,"\n");

	    fclose(fp);//close file
	  }


	  c->status = status;
	  //printf("STATUS (simple): %d\n", status);
	}
    }

  else if (c->type == SEQUENCE_COMMAND)
    {
      //use a do while or just a while loop, keep forking and waiting? - if status is false (!=0) execute then command,
      //if status is true (==0) don't execute then command
      execute_command(c->u.command[0],profiling);
      execute_command(c->u.command[1],profiling);
      c->status=c->u.command[1]->status;
    }

  else if (c->type == WHILE_COMMAND)
    {
      //uses same code as our IF_COMMAND, except modified for an until-loop and only 2 ommands

      int file_in;
      int file_out;
      int stdin=dup(0); //set up input output for the subshell
      int stdout=dup(1);
      close(0);
      close(1);
      if (c->input!=NULL) file_in = open(c->input, O_RDONLY);
      else {
	dup2(stdin,0);
	close(stdin);
      }
      if (c->output!=NULL) {
	FILE * fp; fp=fopen(c->output,"w"); fclose(fp);
	//after clearing out the file we output to, we now output to it
	file_out = open(c->output, O_WRONLY);
      }
      else {
	dup2(stdout,1);
	close(stdout);
      }
      // finished setting up IO

      //So for an UNTIL_COMMAND, this is our flow:
      /*
	UNTIL a DO b DONE
	-So, until 'a' becomes true, keep executing b.
	-so we keep executing b after we find that a evaluates as false
	-the moment a evaluates to true, we break out
 
 
      */
      int s1=-1;
      do {
	execute_command(c->u.command[0],profiling);
	s1=c->u.command[0]->status;
	//printf("STATUS1 (if) : %d\n",s1);
	if (s1 == 0)
	  { // if command[0] evaluated to true, then
	    execute_command(c->u.command[1],profiling);
	    //  int s2=WEXITSTATUS(status);
	    //printf("STATUS2: %d\n",status);
	  }
	else
	  break; //command[0] evaluated to true, so we exit
      } while (s1==0); //keep executing A while it's true

      //re apply stdin and stdout
      if(c->input!=NULL) {  close(file_in); dup2(stdin,0); close(stdin); }
      if(c->output!=NULL) { close(file_out); dup2(stdout,1); close(stdout); }


    }


  else if (c->type == UNTIL_COMMAND)
    {
      //uses same code as our IF_COMMAND, except modified for an until-loop and only 2 ommands

      int file_in;
      int file_out;
      int stdin=dup(0); //set up input output for the subshell
      int stdout=dup(1);
      close(0);
      close(1);
      if (c->input!=NULL) file_in = open(c->input, O_RDONLY);
      else {
	dup2(stdin,0);
	close(stdin);
      }
      if (c->output!=NULL) {
	FILE * fp; fp=fopen(c->output,"w"); fclose(fp);
	//after clearing out the file we output to, we now output to it
	file_out = open(c->output, O_WRONLY);
      }
      else {
	dup2(stdout,1);
	close(stdout);
      }
      // finished setting up IO

      //So for an UNTIL_COMMAND, this is our flow:
      /*
	UNTIL a DO b DONE
	-So, until 'a' becomes true, keep executing b.
	-so we keep executing b after we find that a evaluates as false
	-the moment a evaluates to true, we break out
 
 
      */
      int s1=-1;
      do {
	execute_command(c->u.command[0],profiling);
	s1=c->u.command[0]->status;
	//printf("STATUS1 (if) : %d\n",s1);
	if (s1 != 0)
	  { // if command[0] evaluated to false, then
	    execute_command(c->u.command[1],profiling);
	    //  int s2=WEXITSTATUS(status);
	    //printf("STATUS2: %d\n",status);
	  }
	else
	  break; //command[0] evaluated to true, so we exit
      } while (s1!=0); //keep executing A while it's false

      //re apply stdin and stdout
      if(c->input!=NULL) {  close(file_in); dup2(stdin,0); close(stdin); }
      if(c->output!=NULL) { close(file_out); dup2(stdout,1); close(stdout); }


    }

  else if (c->type == SUBSHELL_COMMAND)
    {
      /*
	added input/output
	copied from the simple command section
	except the subshell parent does NOT fork()
      */
      
      int file_in;
      int file_out;
      int stdin=dup(0); //set up input output for the subshell
      int stdout=dup(1);
      close(0);
      close(1);
      if (c->input!=NULL) file_in = open(c->input, O_RDONLY);
      else {
	dup2(stdin,0);
	close(stdin);
      }
      if (c->output!=NULL) {
	FILE * fp; fp=fopen(c->output,"w"); fclose(fp);
	//after clearing out the file we output to, we now output to it
	file_out = open(c->output, O_WRONLY);
      }
      else {
	dup2(stdout,1);
	close(stdout);
      }
      execute_command(c->u.command[0],profiling);
      c->status=c->u.command[0]->status;

      if(c->input!=NULL) {  close(file_in); dup2(stdin,0); close(stdin); }
      if(c->output!=NULL) { close(file_out); dup2(stdout,1); close(stdout); }
      //printf("checking if stdout works after subshell command...");
    }
  else if (c->type == IF_COMMAND)
    {
      /*
	changed so that the "then" statement executes only if the initial condition is true ( == 0)
	if the initial condition is false ( != 0) then it will check if there is a 3rd command
	if a 3rd command exists, then execute the else statement
	input/output should be working
      */

      int file_in;
      int file_out;
      int stdin=dup(0); //set up input output for the subshell
      int stdout=dup(1);
      close(0);
      close(1);
      if (c->input!=NULL) file_in = open(c->input, O_RDONLY);
      else {
	dup2(stdin,0);
	close(stdin);
      }
      if (c->output!=NULL) {
	FILE * fp; fp=fopen(c->output,"w"); fclose(fp);
	//after clearing out the file we output to, we now output to it
	file_out = open(c->output, O_WRONLY);
      }
      else {
	dup2(stdout,1);
	close(stdout);
      }
      // finished setting up IO

      execute_command(c->u.command[0],profiling);
      int s1=c->u.command[0]->status;
      //printf("STATUS1 (if) : %d\n",status);
      if (s1 == 0)
	{ // if success, then
	  execute_command(c->u.command[1],profiling);
	  int s2=c->u.command[1]->status;
	  //printf("STATUS2: %d\n",status);
	}
      else if ( (s1 != 0) && (c->u.command[2] != NULL) )
	{ // if first condition failed and there is a 3rd command (else)

	  execute_command(c->u.command[2],profiling);
	  //printf("STATUS: %d", c->u.command[2]->status);
	}

      //re apply stdin and stdout
      if(c->input!=NULL) {  close(file_in); dup2(stdin,0); close(stdin); }
      if(c->output!=NULL) { close(file_out); dup2(stdout,1); close(stdout); }

    }
  else if (c->type == PIPE_COMMAND)
    {
      /*
	fixed bug where the process would exit prematurely
      */

      int pid2;
      if (pipe(pipefd) < 0)
	error(1, 0, "failed to create pipe.");


      //timing stuff
      struct timespec start;
      struct timespec end;
      clock_gettime(CLOCK_REALTIME, &start);
      struct rusage cpu_proc;
      //==


      pid = fork();
      if (pid == 0) // first child
	{
	  close(pipefd[r]);
	  if (dup2(pipefd[w], w) < 0) // dup stdout to pipe[w]
	    error(1, 0, "Failed to write to pipe.");
	  execute_command(c->u.command[0], profiling); // first command
	  c->status = c->u.command[0]->status; // Check the status of the first command!
	  close(pipefd[w]); // close pipe
	  close(pipefd[r]);
	  close(r);
	  if (c->status == 0)
	    _exit(0);
	  else
	    _exit(1);
	}
      else if (pid > 0) // back in parent process, make second child
	{
	  //timing stuff for process 2
	  struct timespec start2;
	  struct timespec end2;
	  clock_gettime(CLOCK_REALTIME, &start2);
	  struct rusage cpu_proc2;
	  //==


	  pid2 = fork();
	  if (pid2 == 0) { // second child
	    close (pipefd[w]);
	    if (dup2(pipefd[r], r) < 0) // dup stdin to pipe[r]
	      error(1, 0, "Failed to read from pipe.");
	    execute_command(c->u.command[1], profiling); // second command
	    c->status = c->u.command[1]->status;
	    close(pipefd[r]); // close pipe
	    close(pipefd[w]);
	    close(w);
	    if (c->status == 0)
	      _exit(0);
	    else
	      _exit(1);
	  }
	  else if (pid2 > 0) { // finish parent process
	    close(pipefd[r]); // close pipes, parent doesn't need them
	    close(pipefd[w]);
	    int status;
	    waitpid(pid, &status, 0); // wait for and get exit status of both children
	    //printf("STATUS (pipe child 1): %d\n", status);
	    if (profiling == 1) {
	      print_profile( pid, start, end, cpu_proc );
	    }
	    //====WAIT FOR SECOND PROCESS======\			\
	    waitpid(pid2, &status, 0); // we only care about the second child's return status
	    if (profiling == 1) {
	      print_profile( pid2, start2, end2, cpu_proc2 );
	    }
	    
	    

	    //printf("STATUS (pipe child 2): %d\n", status);
	    c->status = status;
	    //printf("command status %d\n", c->status);
	  }
	  else {
	    error(1, 0, "Failed to create child process 2.");
	  }
	}
      else
	error(1, 0, "Failed to create child process 1.");
    }
}

void print_profile (int pid ,struct  timespec start,struct timespec end, struct rusage cpu_proc) {
   
	  //USER TIME
	  struct timeval sys;
	  struct timeval usr;
	  getrusage(RUSAGE_CHILDREN, &cpu_proc);
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
