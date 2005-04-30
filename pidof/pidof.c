/*-
 * Copyright (c) 2005 Roman Bogorodskiy
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: pidof.c,v 1.2 2005/04/30 16:48:44 bogorodskiy Exp $
 */

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <kvm.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <fcntl.h>

static void get_pid_of_process(char *process_name);

static void
get_pid_of_process(char *process_name)
{
	static kvm_t *kd = NULL;
	struct kinfo_proc *p;
	int n_processes;
	int i;

	if ((kd = kvm_open("/dev/null", "/dev/null", "/dev/null", O_RDONLY, "kvm_open")) == NULL) 
			 (void)errx(1, "%s", kvm_geterr(kd));
	else {
		p = kvm_getprocs(kd, KERN_PROC_PROC, 0, &n_processes);
		for (i = 0; i<n_processes; i++)
			if (strncmp(process_name, p[i].ki_comm, COMMLEN+1) == 0)
				(void)printf("%d ", (int)p[i].ki_pid);
	}	
}

int
main(int argc, char **argv)
{
	int i;
	
	for (i = 1; i<argc; get_pid_of_process(argv[i++]));

	(void)printf("\n");
	
	return 0;
}
