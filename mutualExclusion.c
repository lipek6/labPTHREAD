// COMPILATION  : gcc mutualExclusion.c -lpthread -o mutualExclusion
// PROGRAM USAGE: ./mutualExclusion <DESIRED AMOUNT OF THREADS> <DESIRED AMOUNT OF ACCOUNTS>

#include "bankAccount.h"                                                // bank account info


// Macros of control
#define AVAILABLE_OPS 4                                                 // Number of operations (depoisite, withdraw, transfer and check)
#define INITIAL_MONEY_AMOUNT 100000.00                                    // Initial money for all accounts
#define MONEY_AMOUNT 1000.00                                             // Amount of money used on all transactions
#define TRANSACTIONS 10                                                 // Amount of transactions that each thread does


// Function declarations used specifically inside mutualEclusion.c

// Utility function to check if integer like values were passed as arguments to the program (as intended)
void checkStringConversion(char *endptr);
// Utility function to check if a file opened properly
void checkFilePtr(FILE * fileptr);
// Function that the childs(threads) will execute
void* child(void * buf);
// Function to randomly do TRANSACTIONS with a random AVAILABLE_OPS
void doRandTransactions(unsigned long id, unsigned int *threadSeed);
// Function to process the arguments passed at the program command line execution
void processCommandLine(unsigned long *numThreads, unsigned long *numAccounts, int argc, char** argv);



int main(int argc, char** argv)
{
   system("rm -f AccountLogs/*.txt ThreadLogs/*.txt");               // Clean all files used on the last execution
   system("mkdir -p GenericLogs AccountLogs ThreadLogs");            // If the directoried aren't here, create them

   double initialBalances = (double)INITIAL_MONEY_AMOUNT;            // initial balance on all accounts
   

   processCommandLine(&numThreads, &numAccounts, argc, argv);        // get desired # of all that things :)
   
   Balances      = malloc(numAccounts * sizeof(double));             // allocate all that dynamic arrays :)
   Children      = malloc(numThreads  * sizeof(pthread_t));   
   Locks         = malloc(numAccounts * sizeof(pthread_mutex_t));
   ThreadsLogs   = malloc(numThreads  * sizeof(FILE*));
   AccountsLogs  = malloc(numAccounts * sizeof(FILE*));


   printf("NUMBER OF THREADS : %lu\n", numThreads);
   printf("NUMBER OF ACCOUNTS: %lu\n", numAccounts);
   printf("INITIAL BALANCES  : %f\n\n", initialBalances);
   

   // Write some introduction in each file
   CheckLog        = fopen("GenericLogs/CheckLog.txt",       "w"); pthread_mutex_init(&MainLogsLocks[0], NULL); checkFilePtr(CheckLog); 
   CompleteLog     = fopen("GenericLogs/CompleteLog.txt",    "w"); pthread_mutex_init(&MainLogsLocks[1], NULL); checkFilePtr(CompleteLog);
   DepositLog      = fopen("GenericLogs/DepositLog.txt",     "w"); pthread_mutex_init(&MainLogsLocks[2], NULL); checkFilePtr(DepositLog);
   ErrorsLog       = fopen("GenericLogs/ErrorsLog.txt",       "w"); pthread_mutex_init(&MainLogsLocks[3], NULL); checkFilePtr(ErrorsLog);
   TransferenceLog = fopen("GenericLogs/TrasferenceLog.txt", "w"); pthread_mutex_init(&MainLogsLocks[4], NULL); checkFilePtr(TransferenceLog);
   WithdrawLog     = fopen("GenericLogs/WithdrawLog.txt",    "w"); pthread_mutex_init(&MainLogsLocks[5], NULL); checkFilePtr(WithdrawLog);


   // ACCOUNTS SETUP:
   char accFilename[100];     // Modify later
   for(unsigned long id = 0; id < numAccounts; id++)
   {
      sprintf(accFilename, "AccountLogs/Acc%luLog.txt", id);
      AccountsLogs[id] = fopen(accFilename, "w");
      checkFilePtr(AccountsLogs[id]);
      
      Balances[id] = initialBalances;
      pthread_mutex_init(&Locks[id], NULL);
   }

   
   // THREAD CREATION:
   int errorOcurrence;
   char threadFilename[100];     // Modify later
   for(unsigned long id = 0; id < numThreads; id++)
   {                   
      sprintf(threadFilename, "ThreadLogs/Thread%luLog.txt", id);
      ThreadsLogs[id] = fopen(threadFilename, "w");
      checkFilePtr(ThreadsLogs[id]);

      errorOcurrence = pthread_create(&(Children[id]), NULL, child, (void*) id);       // handle for the child, attributes of the child, attributes of the child, args to that function.
      if(errorOcurrence != 0)
      {
         fprintf(stderr, "\nFailed to create threads, you probably exagarated, stop being a jerk with the program :(\n");
         exit(4);
      }
   }


   // JOIN:
   for(unsigned long id = 0; id < numThreads; id++)
   {
      pthread_join(Children[id], NULL);
   }


   // PRINTING FINAL RESULTS
   for(unsigned long id = 0; id < numAccounts; id++)
   {
      printf("Account %lu final balance: $%.2f.\n", id, Balances[id]);
   }
      printf("\nCheck Log Files for more info about the execution\n");


   // Dealocating all the memory used
   for(int i = 0; i < numAccounts; i++) pthread_mutex_destroy(&Locks[i]);           // destroy mutexes
   for(int i = 0; i < MAINLOGS;    i++) pthread_mutex_destroy(&MainLogsLocks[i]);   // destroy mutexes
   for(unsigned long id = 0; id < numAccounts; id++) fclose(AccountsLogs[id]);      // close files
   for(unsigned long id = 0; id < numThreads; id++)  fclose(ThreadsLogs[id]);       // close files
   fclose(CheckLog);                                                                // close files
   fclose(CompleteLog);                                                             // close files
   fclose(DepositLog);                                                              // close files
   fclose(ErrorsLog);                                                               // close files
   fclose(TransferenceLog);                                                         // close files
   fclose(WithdrawLog);                                                             // close files
   free(Children);                                                                  // deallocate array
   free(Balances);                                                                  // deallocate array
   free(Locks);                                                                     // deallocate array
   free(AccountsLogs);                                                              // deallocate array
   free(ThreadsLogs);                                                               // deallocate array

   return 0;
}





// simulate id performing random transactions 
void doRandTransactions(unsigned long id, unsigned int *threadSeed)
{
   double amount = (double) MONEY_AMOUNT;
   unsigned long account_src, account_dst;
   int operation;
   
   for(int i = 0; i < TRANSACTIONS; i++)
   {
      operation = abs(rand_r(threadSeed)) % AVAILABLE_OPS;
      account_src = abs(rand_r(threadSeed)) % numAccounts;

      switch(operation)
      {
         case(0):
            deposit(amount, account_src, id);
            break;
         case(1):
            withdraw(amount, account_src, id);
            break;
         case(2):
            checkAmount(account_src, id);
            break;
         case(3):
            account_dst = abs(rand_r(threadSeed)) % numAccounts;
            transfer(amount, account_src, account_dst, id);
            break;
      }
   }
   return;
}



void* child(void * buf)
{ 
   unsigned long childID = (unsigned long) buf;  
   unsigned int threadSeed = time(NULL) + childID;

   doRandTransactions(childID, &threadSeed);  
   return NULL;
}



void processCommandLine(unsigned long *numThreads, unsigned long *numAccounts, int argc, char** argv)
{
   char *endptr;                                // This guy is here to check bad usage of the args

   // Complete usage
   if (argc == 3)
   {

      *numThreads  = abs(strtoul(argv[1], &endptr, 10)); checkStringConversion(endptr);
      *numAccounts = abs(strtoul(argv[2], &endptr, 10)); checkStringConversion(endptr);

      return;
   }
   // Incomplete usage, assuming values
   else if (argc == 2)
   {
      *numThreads  = strtoul(argv[1], &endptr, 10); checkStringConversion(endptr);
      *numAccounts = 1;

      return;
   }
   // Incomplete usage, assuming values
   else if (argc == 1)
   {
      *numThreads  = 1;
      *numAccounts = 1;

      return;
   }
   // Too much arguments, exiting
   else
   {      
      fprintf(stderr, "\nUsage: ./mutualExclusion [NumberOfThreads][NumberOfBankAccounts]\n");
      fprintf(stderr, "Use only 2 arguments dammit!\n");
      exit(1);
   }
}



void checkStringConversion(char *endptr)
{
   if(*endptr != '\0')
   {
      fprintf(stderr, "\nUsage: ./mutualExclusion [NumberOfThreads][NumberOfBankAccounts]\n");
      fprintf(stderr, "Did you use normal numbers on the input? I don't think so.\n");
      exit(2);
   }
   return;
}

void checkFilePtr(FILE * fileptr)
{
   if(fileptr == NULL)
   {
      fprintf(stderr, "Error opening/creating log file.\n");
      exit(3);
   }
}