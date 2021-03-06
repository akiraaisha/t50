/*
 *  T50 - Experimental Mixed Packet Injector
 *
 *  Copyright (C) 2010 - 2014 - T50 developers
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <common.h>
#include <sys/wait.h> /* POSIX.1 compliant */

static pid_t pid = -1;      /* -1 is a trick used when __HAVE_TURBO__ isn't defined. */

static void initialize(void);
static const char *getOrdinalSuffix(unsigned);
static const char *getMonth(unsigned);

/* Main function launches all T50 modules */
int main(int argc, char *argv[])
{
  struct config_options *co;  /* Pointer to options. */
  struct cidr *cidr_ptr;      /* Pointer to cidr host id and 1st ip address. */
  modules_table_t *ptbl;      /* Pointer to modules table */
  uint8_t proto;              /* Used on main loop. */

  initialize();

  /* Configuring command line interface options. */
  if ((co = getConfigOptions(argc, argv)) == NULL)
    return EXIT_FAILURE;

  /* This is a requirement of t50. User must be root to use it. 
     Previously on checkConfigOptions(). */
  if (getuid())
  {
    ERROR("User must have root priviledge to run.");
    return EXIT_FAILURE;
  }

  /* Validating command line interface options. */
  if (!checkConfigOptions(co))
    return EXIT_FAILURE;

  /* Setting socket file descriptor. */
  /* NOTE: createSocket() handles its own errors before returning. */
  if (!createSocket())
    return EXIT_FAILURE;

  /* Setup random seed using current date/time timestamp. */
  /* NOTE: Random seed don't need to be so precise! */
  SRANDOM(time(NULL));

#ifdef  __HAVE_TURBO__
  /* Entering in TURBO. */
  if (co->turbo)
  {
    /* Decides if it's necessary to fork a new process. */
    if ((co->ip.protocol == IPPROTO_T50 && co->threshold > (threshold_t)getNumberOfRegisteredModules()) || 
        (co->ip.protocol != IPPROTO_T50 && co->threshold > 1))
    {
      threshold_t new_threshold;

      if ((pid = fork()) == -1)
      {
        perror("Error creating child process. Exiting...");
        return EXIT_FAILURE;
      }

      /* Setting the priority to both parent and child process to highly favorable scheduling value. */
      /* FIXME: Why not setup this value when t50 runs as a single process? */
      if (setpriority(PRIO_PROCESS, PRIO_PROCESS, -15)  == -1)
      {
        perror("Error setting process priority. Exiting...");
        return EXIT_FAILURE;
      }

      /* Divide the process iterations in main loop between processes. */
      new_threshold = co->threshold / 2; 

      /* FIX: Ooops! Parent process get the extra packet, if given threshold is odd. */
      if ((co->threshold % 2) && !IS_CHILD_PID(pid))
        new_threshold++;

      co->threshold = new_threshold;
    }
  }
#endif  /* __HAVE_TURBO__ */

  /* Calculates CIDR for destination address. */
  if ((cidr_ptr = config_cidr(co->bits, co->ip.daddr)) == NULL)
    return EXIT_FAILURE;

  /* Show launch info only for parent process. */
  if (!IS_CHILD_PID(pid))
  {
    time_t lt;
    struct tm *tm;

    /* Getting the local time. */
    lt = time(NULL); 
    tm = localtime(&lt);

    printf("\b\n%s %s successfully launched at %s %2d%s %d %.02d:%.02d:%.02d\n",
      PACKAGE,  
      VERSION, 
      getMonth(tm->tm_mon), 
      tm->tm_mday, 
      getOrdinalSuffix(tm->tm_mday),
      (tm->tm_year + 1900), 
      tm->tm_hour, 
      tm->tm_min, 
      tm->tm_sec);
  }

  /* Selects the initial protocol to use. */
  proto = co->ip.protocol;
  ptbl = mod_table;
  if (proto != IPPROTO_T50)
    ptbl += co->ip.protoname;

  /* Preallocate packet buffer. */
  alloc_packet(INITIAL_PACKET_SIZE);

  /* Execute if flood or while threshold greater than 0. */
  while (co->flood || (co->threshold-- > 0))
  {
    /* Holds the actual packet size after module function call. */
    size_t size;

    /* Set the destination IP address to RANDOM IP address. */
    /* NOTE: The previous code did not account for 'hostid == 0'! */
    co->ip.daddr = cidr_ptr->__1st_addr;
    if (cidr_ptr->hostid)
      co->ip.daddr += RANDOM() % cidr_ptr->hostid;
    co->ip.daddr = htonl(co->ip.daddr);

    /* Calls the 'module' function and sends the packet. */
    co->ip.protocol = ptbl->protocol_id;
    ptbl->func(co, &size);

    if (!sendPacket(packet, size, co))
      return EXIT_FAILURE;
  
    /* If protocol if 'T50', then get the next true protocol. */
    if (proto == IPPROTO_T50)
      if ((++ptbl)->func == NULL)
        ptbl = mod_table;
  }

  /* Show termination message only for parent process. */
  if (!IS_CHILD_PID(pid))
  {
    time_t lt;
    struct tm *tm;

#ifdef  __HAVE_TURBO__
    int status;

    /* Wait 5 seconds for child process, then ungracefully closes the program. */
    alarm(5);
    wait(&status);
#endif

    /* FIX: To graciously end the program, only the parent process can close the socket. 
       NOTE: I realize that closing descriptors are reference counted.
             Kept the logic just in case! */
    closeSocket();

    /* Getting the local time. */
    lt = time(NULL); 
    tm = localtime(&lt);

    printf("\b\n%s %s successfully finished at %s %2d%s %d %.02d:%.02d:%.02d\n",
      PACKAGE,
      VERSION,
      getMonth(tm->tm_mon),
      tm->tm_mday,
      getOrdinalSuffix(tm->tm_mday),
      (tm->tm_year + 1900),
      tm->tm_hour,
      tm->tm_min,
      tm->tm_sec);
  }

  return 0;
}

/* This function handles interruptions. */
static void signal_handler(int signal)
{
  /* Make sure the socket descriptor is closed. 
     FIX: But only if this is the parent process. Closing the cloned descriptor on the
          child process can be catastrophic to the parent. 
     NOTE: I realize that the act of closing descriptors are reference counted.
           Keept the logic just in case! */
#ifdef __HAVE_TURBO__
  if (!IS_CHILD_PID(pid))
  {
    /* Kills the child process! */
    kill(pid, SIGKILL);
#endif

      closeSocket();

#ifdef __HAVE_TURBO__
  }
#endif

  /* FIX: The shell documentation (bash) specifies that a process
          when exits because a signal, must return 128+signal#. */
  exit(128 + signal);
}

static void initialize(void)
{
  /* NOTE: See 'man 2 signal' */
  struct sigaction sa;

  /* --- Initialize signal handlers --- */

  /* Using sig*() functions for compability. */
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART; /* same signal() semantics?! */

  /* Trap all "interrupt" signals, except SIGKILL, SIGSTOP and SIGSEGV (uncatchable, accordingly to 'man 7 signal'). */
  sa.sa_handler = signal_handler;
  sigaction(SIGHUP,  &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL);
  sigaction(SIGINT,  &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGABRT, &sa, NULL);
  sigaction(SIGTRAP, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGTSTP, &sa, NULL);
#ifdef  __HAVE_TURBO__
  /* FIX: Is it wise to simply terminate the main process
          if the child terminates? 

          Maybe it is wiser if we implement some kind of
          timeout when waiting for the child to terminate. */
  sigaction(SIGALRM, &sa, NULL);

  sa.sa_handler = SIG_IGN;
  sigaction(SIGCHLD, &sa, NULL);
#endif

  /* --- Make sure stdout is unbuffered (otherwise, it's line buffered). --- */
  fflush(stdout);
  setvbuf(stdout, NULL, _IONBF, 0); 
}

/* Auxiliary function to return the [constant] ordinary suffix string for a number. */
static const char *getOrdinalSuffix(unsigned n)
{
  static const char *suffixes[] = { "st", "nd", "rd", "th" };

  /* FIX: 11, 12 & 13 have 'th' suffix, not 'st, nd or rd'. */
  if ((n < 11) || (n > 13))
    switch (n % 10) {
      case 1: return suffixes[0];
      case 2: return suffixes[1];
      case 3: return suffixes[2];
    }

  return suffixes[3];
}

static const char *getMonth(unsigned n)
{
  /* Months */
  static const char * const months[] =
    { "Jan", "Feb", "Mar", "Apr", "May",  "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov",  "Dec" };

  if (n > 11)
    return "";

  return months[n];
}
