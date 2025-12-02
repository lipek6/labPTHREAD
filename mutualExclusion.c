#include <stdio.h>                                                      // printf()
#include <stdlib.h>                                                     // atoi(), exit(), ...
#include <pthread.h>                                                    // pthread types and functions
#include <unistd.h>                                                     // sleep() to avoid busy-waiting
#include "bankAccount.h"                                                // bank account info
#define TRANSACTIONS 10
#define MONEY_AMOUNT 100.00
#define AVAILABLE_OPS 4

void processCommandLine(unsigned long *numThreads, unsigned long *numAccounts, int argc, char** argv);
void doRandTransactions(unsigned long id);
void checkStringConversion(char *endptr);
void* child(void * buf);





int main(int argc, char** argv)
{
   time_t currTime; time(&currTime);
   srand(currTime);

   



   double initialBalances = 1000.00;                                    // initial balance on all accounts
   unsigned long id = 0;                                                // loop control variable
   
   processCommandLine(&numThreads, &numAccounts, argc, argv);// get desired # of all that shit :)
   
   Balances = malloc(numAccounts * sizeof(double));          // allocate all that shit :)
   Children = malloc(numThreads  * sizeof(pthread_t));   
   Locks    = malloc(numAccounts * sizeof(pthread_mutex_t));
   
   printf("NUMBER OF THREADS : %lu\n", numThreads);
   printf("NUMBER OF ACCOUNTS: %lu\n", numAccounts);
   printf("INITIAL BALANCES  : %f\n\n", initialBalances);

   
   // SETUP
   for(int i = 0; i < numAccounts; i++)
   {
      Balances[i] = initialBalances;
      pthread_mutex_init(&Locks[i], NULL);
   }

   
   // FORK:
   for(id = 1; id < numThreads; id++)
   {                   
      pthread_create(&(Children[id-1]), NULL, child, (void*) id);       // handle for the child, attributes of the child, attributes of the child, args to that function
   }


   // JOIN:
   for(id = 1; id < numThreads; id++)
   {
      pthread_join(Children[id-1], NULL);
   }

   // PRINTING FINAL RESULTS
   printf("\nThe final balance of the account using %lu threads is $%.2f.\n\n", numThreads, Balances[0]);
   printf("\nThe final balance of the account using %lu threads is $%.2f.\n\n", numThreads, Balances[1]);

   for(int i = 0; i < numAccounts; i++) pthread_mutex_destroy(&Locks[i]);// deallocate mutex
   free(Children);                                                       // deallocate array
   free(Balances);                                                       // deallocate array
   free(Locks);                                                          // deallocate array
  
   return 0;
}





// simulate id performing random transactions 
void doRandTransactions(unsigned long id)
{
   double amount = (double) MONEY_AMOUNT;
   unsigned long account_src, account_dst;
   int operation;
   
   for(int i = 0; i < TRANSACTIONS; i++)
   {
      operation = abs(rand()) % AVAILABLE_OPS;
      account_src = abs(rand()) % numAccounts;

      switch(operation)
      {
         case(0):
            deposit(amount, account_src, id);
            break;
         case(1):
            withdraw(amount, account_src, id);
            break;
         case(2):
            checkAmount(amount, account_src, id);
            break;
         case(3):
            account_dst = abs(rand()) % numAccounts;
            transfer(amount, account_src, account_dst, id);
            break;
      }
   }
   return;
}



void* child(void * buf)
{ 
   unsigned long childID = (unsigned long) buf;  
   doRandTransactions(childID);  
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