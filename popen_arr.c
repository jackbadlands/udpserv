#define _GNU_SOURCE // execvpe 
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "popen_arr.h"


// Implemented by Vitaly _Vi Shukela in 2013, License=MIT

static int popen2_impl(FILE** in, FILE** out,  const char* program, const char* const argv[], const char* const envp[], int lookup_path) {
    
	int child_stdout = -1;
	int child_stdin = -1;
	
	int to_be_written = -1;
	int to_be_read = -1;
	
	if(in) {
	    int p[2]={-1,-1}; 
	    int ret = pipe(p);
	    if(ret!=0) { return -1; }
	    to_be_written=p[1];
	    child_stdin =p[0];
	    *in = fdopen(to_be_written, "w");
	    if (*in == NULL) {
	        close(to_be_written);
	        close(child_stdin);
	        return -1;
	    }
	}
	if(out) {
	    int p[2]={-1,-1};
	    int ret = pipe(p);
	    if(ret!=0) {
	        if (in) {
	           close(child_stdin);
	           fclose(*in);
	           *in = NULL;
	        }
	        return -1;
	    }
	    to_be_read   =p[0];
	    child_stdout=p[1];
	    *out = fdopen(to_be_read, "r");
	}
	
	
	int childpid = fork();
	if(!childpid) {
	   if(child_stdout!=-1) {
	       close(to_be_read);
	       dup2(child_stdout, 1);
	       close(child_stdout);
	   }
	   if(child_stdin!=-1) {
	       close(to_be_written);
	       dup2(child_stdin, 0);
	       close(child_stdin);
	   }
	   if (lookup_path) {
	       if (envp) {
	           execvpe(program, (char**)argv, (char**)envp);
	       } else {
	           execvp (program, (char**)argv);
	       }
	   } else {
	       if (envp) {
	           execve(program, (char**)argv, (char**)envp);
	       } else {
	           execv (program, (char**)argv);
	       }
	   }
	   _exit(ENOSYS);
	}
	
    if(child_stdout!=-1) {
        close(child_stdout);
    }
    if(child_stdin!=-1) {
        close(child_stdin);
    }
    
    return childpid;
}


int popen2_arr  (FILE** in, FILE** out,  const char* program, const char* const argv[], const char* const envp[])
{
    signal(SIGPIPE, SIG_IGN);
    return popen2_impl(in, out, program, argv, envp, 0);
}
int popen2_arr_p(FILE** in, FILE** out,  const char* program, const char* const argv[], const char* const envp[])
{
    signal(SIGPIPE, SIG_IGN);
    return popen2_impl(in, out, program, argv, envp, 1);
}
FILE* popen_arr(const char* program, const char* const argv[], int pipe_into_program) {
    FILE* f = NULL;
    if (pipe_into_program) {
        popen2_arr_p(&f, NULL, program, argv, NULL);
    } else {
        popen2_arr_p(NULL, &f, program, argv, NULL);
    }
    return f;
}
