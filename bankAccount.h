// Shared dynamic arrays
pthread_mutex_t * Locks;                                          // dynamic array of mutexes
pthread_t * Children;                                             // dynamic array of child threads
double * Balances;                                                // dynamic array of accounts balances

// Shared variables
unsigned long numThreads  = 0;                                       // desired # of threads
unsigned long numAccounts = 0;                                       // desired # of bank accounts

// add amount to specific Account Balance
void deposit(double amount, unsigned long account, unsigned long thread_id)
{
  pthread_mutex_lock(&Locks[account]);

    Balances[account] += amount;
    
  
  pthread_mutex_unlock(&Locks[account]);
}

// subtract amount from specific Account Balance
void withdraw(double amount, unsigned long account, unsigned long thread_id)
{
  pthread_mutex_lock(&Locks[account]);

    if(Balances[account] - amount < (double) 0.00)
    {
      printf("Failed\n");
    }
    else
    {
      Balances[account] -= amount;
    }
  
  pthread_mutex_unlock(&Locks[account]);
}

void checkAmount(double amount, unsigned long account, unsigned long thread_id)
{
  pthread_mutex_lock(&Locks[account]);


    printf("%f\n", Balances[account]);


  pthread_mutex_unlock(&Locks[account]);
}

void transfer(double amount, unsigned long account_src, unsigned long account_dst, unsigned long id)
{
  if(account_dst == account_src) return;

  while(1)
  {
    pthread_mutex_lock(&Locks[account_src]);
    if(pthread_mutex_trylock(&Locks[account_dst]) == 0)
    {
      if(Balances[account_src] - amount < 0) break;

      Balances[account_src] -= amount;
      Balances[account_dst] += amount;
      break;
    }
    else
    {
      pthread_mutex_unlock(&Locks[account_src]);
    }
  }
  pthread_mutex_unlock(&Locks[account_dst]);
  pthread_mutex_unlock(&Locks[account_src]);
}