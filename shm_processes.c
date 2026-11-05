/*
 * Project #2 - Part 1: Poor Student / Dear Old Dad
 * Authors: Danny Toledo-Navarro, [Partner name, if any]
 * Outside assistance: Claude (Anthropic AI assistant) helped draft this code.
 *
 * Usage:  ./shm_proc          (1 Dear Old Dad, 1 Poor Student)
 *         ./shm_proc 1 3      (Dear Old Dad, 3 Poor Students)
 *         ./shm_proc 2 10     (Lovable Mom + Dear Old Dad, 10 Poor Students)
 */
#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

void  DadProcess(int [], sem_t *);
void  MomProcess(int [], sem_t *);
void  ClientProcess(int [], sem_t *);
void  StopAll(int);

int    ShmID;           /* global so StopAll() can remove the shared memory */
sem_t  *mutex;          /* protects the BankAccount */

int  main(int  argc, char *argv[])
{
     int    *ShmPTR;
     pid_t  pid;
     int    status;
     int    numParents = 1, numStudents = 1, i;

     if (argc == 3) {
          numParents  = atoi(argv[1]);
          numStudents = atoi(argv[2]);
     }
     if (numParents < 1)  numParents = 1;
     if (numParents > 2)  numParents = 2;
     if (numStudents < 1) numStudents = 1;

     setbuf(stdout, NULL);   /* print right away, in order */

     ShmID = shmget(IPC_PRIVATE, 1*sizeof(int), IPC_CREAT | 0666);
     if (ShmID < 0) {
          printf("*** shmget error (server) ***\n");
          exit(1);
     }

     ShmPTR = (int *) shmat(ShmID, NULL, 0);
     if (*ShmPTR == -1) {
          printf("*** shmat error (server) ***\n");
          exit(1);
     }

     ShmPTR[0] = 0;          /* ShmPTR[0] is the BankAccount */

     /* create, initialize semaphore (starts at 1 = unlocked) */
     sem_unlink("bankmutex");
     if ((mutex = sem_open("bankmutex", O_CREAT, 0644, 1)) == SEM_FAILED) {
          perror("semaphore initilization");
          exit(1);
     }

     /* Lovable Mom (extra credit) */
     if (numParents == 2) {
          pid = fork();
          if (pid < 0) {
               printf("*** fork error (server) ***\n");
               exit(1);
          }
          else if (pid == 0) {
               MomProcess(ShmPTR, mutex);
               exit(0);
          }
     }

     /* Poor Student(s) */
     for (i = 0; i < numStudents; i++) {
          pid = fork();
          if (pid < 0) {
               printf("*** fork error (server) ***\n");
               exit(1);
          }
          else if (pid == 0) {
               ClientProcess(ShmPTR, mutex);
               exit(0);
          }
     }

     /* original process is Dear Old Dad; killing it stops everyone */
     signal(SIGINT,  StopAll);
     signal(SIGTERM, StopAll);
     DadProcess(ShmPTR, mutex);

     wait(&status);
     shmdt((void *) ShmPTR);
     shmctl(ShmID, IPC_RMID, NULL);
     exit(0);
}

void  DadProcess(int  SharedMem[], sem_t *mutex)
{
     int  localBalance, amount;

     srand(time(NULL) ^ getpid());
     while (1) {
          sleep(rand() % 6);
          printf("Dear Old Dad: Attempting to Check Balance\n");

          sem_wait(mutex);
          localBalance = SharedMem[0];
          if (rand() % 2 == 0) {
               if (localBalance < 100) {
                    /* Deposit Money */
                    localBalance = SharedMem[0];
                    amount = rand() % 101;
                    if (amount % 2 == 0) {
                         localBalance += amount;
                         printf("Dear old Dad: Deposits $%d / Balance = $%d\n", amount, localBalance);
                         SharedMem[0] = localBalance;
                    }
                    else {
                         printf("Dear old Dad: Doesn't have any money to give\n");
                    }
               }
               else {
                    printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
               }
          }
          else {
               printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
          }
          sem_post(mutex);
     }
}

void  MomProcess(int  SharedMem[], sem_t *mutex)
{
     int  localBalance, amount;

     srand(time(NULL) ^ getpid());
     while (1) {
          sleep(rand() % 11);
          printf("Loveable Mom: Attempting to Check Balance\n");

          sem_wait(mutex);
          localBalance = SharedMem[0];
          if (localBalance <= 100) {
               /* Deposit Money */
               localBalance = SharedMem[0];
               amount = rand() % 126;
               localBalance += amount;
               printf("Lovable Mom: Deposits $%d / Balance = $%d\n", amount, localBalance);
               SharedMem[0] = localBalance;
          }
          sem_post(mutex);
     }
}

void  ClientProcess(int  SharedMem[], sem_t *mutex)
{
     int  localBalance, need;

     srand(time(NULL) ^ getpid());
     while (1) {
          sleep(rand() % 6);
          printf("Poor Student: Attempting to Check Balance\n");

          sem_wait(mutex);
          localBalance = SharedMem[0];
          if (rand() % 2 == 0) {
               /* Withdraw Money */
               localBalance = SharedMem[0];
               need = rand() % 51;
               printf("Poor Student needs $%d\n", need);
               if (need <= localBalance) {
                    localBalance -= need;
                    printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, localBalance);
               }
               else {
                    printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
               }
               SharedMem[0] = localBalance;
          }
          else {
               printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
          }
          sem_post(mutex);
     }
}

/* Runs when Dad is sent a signal: stop the children and clean up */
void  StopAll(int sig)
{
     signal(SIGTERM, SIG_IGN);
     kill(0, SIGTERM);
     while (wait(NULL) > 0) ;
     shmctl(ShmID, IPC_RMID, NULL);
     sem_unlink("bankmutex");
     exit(0);
}