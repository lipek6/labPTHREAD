#define TRANSACTIONS 10
#define MONEY_AMOUNT 100.00
#define INITIAL_MONEY_AMOUNT 1000.00
#define AVAILABLE_OPS 4
#define MAINLOGS 6
typedef enum
{
  checkIDX    = 0,
  completeIDX = 1,
  depositIDX  = 2,
  errorIDX    = 3,
  transferIDX = 4,
  withdrawIDX = 5
} fileIDX;

// Shared dynamic arrays
pthread_mutex_t * Locks;                                                    // dynamic array of mutexes
pthread_mutex_t MainLogsLocks[MAINLOGS];                                    // common  array of mutexes
pthread_t * Children;                                                       // dynamic array of child threads
double * Balances;                                                          // dynamic array of accounts balances
FILE* * ThreadsLogs;                                                        // dynamic array of log files for threads
FILE* * AccountsLogs;                                                       // dynamic array of log files for accounts

// Shared variables     
unsigned long numThreads  = 0;                                              // desired # of threads
unsigned long numAccounts = 0;                                              // desired # of bank accounts
FILE* CheckLog;
FILE* CompleteLog;
FILE* DepositLog;
FILE* ErrorsLog;
FILE* TransferenceLog;
FILE* WithdrawLog;





// Functions declarations
void orderAccounts(unsigned long *firstAccountLock, unsigned long *secondAccountLock);
void get_exact_time(char *buffer);
void log_deposit_succesfull();
void log_withdraw_succesfull();
void log_withdraw_failed();
void log_check_succesfull();
void log_transfer_succesfull();
void log_transfer_failed();



// add amount to specific Account Balance
void deposit(double amount, unsigned long account, unsigned long thread_id)
{
  time_t currTime;
  double new_amount;
  double old_amount;

  pthread_mutex_lock(&Locks[account]);
    
    // OPERATION:
    old_amount = Balances[account];  
    new_amount = old_amount + amount;
    Balances[account] = new_amount;
    char timeStr[100]; get_exact_time(timeStr);
    
    // ACCOUNT LOG:
    log_deposit_succesfull(AccountsLogs[account], thread_id, account, amount, timeStr, old_amount, new_amount)

  pthread_mutex_unlock(&Locks[account]);


  pthread_mutex_lock(&MainLogsLocks[depositIDX]);
    
    // MAIN LOG:
    log_deposit_succesfull(DepositLog, thread_id, account, amount, timeStr, old_amount, new_amount)
  
  pthread_mutex_unlock(&MainLogsLocks[depositIDX]);

  // THREAD LOG:
  log_deposit_succesfull(ThreadsLogs[thread_id], thread_id, account, amount, timeStr, old_amount, new_amount)

  return;
}



// subtract amount from specific Account Balance
void withdraw(double amount, unsigned long account, unsigned long thread_id)
{
  time_t currTime;
  double new_amount;
  double old_amount;
  short errorOcurrence = 0;

  pthread_mutex_lock(&Locks[account]);

    old_amount = Balances[account];
    new_amount = old_amount - amount;
    char timeStr[100]; get_exact_time(timeStr);

    if(new_amount < (double) 0.00)
    {
      errorOcurrence = 1;
      // ACCOUNT LOG:
      log_withdraw_failed(AccountsLogs[account], thread_id, account, amount, timeStr, old_amount, new_amount);
    }
    else
    {
      // OPERATION:
      Balances[account] = new_amount;
      // ACCOUNT LOG:
      log_withdraw_succesfull(AccountsLogs[account], thread_id, account, amount, timeStr, old_amount, new_amount);
    }
  
  pthread_mutex_unlock(&Locks[account]);

  if(errorOcurrence)
  {
    pthread_mutex_lock(&MainLogsLocks[errorIDX]);
    
      // ERROR LOG:
      log_withdraw_failed(ErrorsLog, thread_id, account, amount, timeStr, old_amount, new_amount);

    pthread_mutex_unlock(&MainLogsLocks[errorIDX]);
  
    // THREAD LOG:
    log_withdraw_failed(ThreadsLogs[thread_id], thread_id, account, amount, timeStr, old_amount, new_amount);
  }
  else
  {
    pthread_mutex_lock(&MainLogsLocks[withdrawIDX]);

      // MAIN LOG:
      log_withdraw_succesfull(WithdrawLog, thread_id, account, amount, timeStr, old_amount, new_amount);

    pthread_mutex_unlock(&MainLogsLocks[withdrawIDX]);

    // THREAD LOG:
    log_withdraw_succesfull(ThreadsLogs[thread_id], thread_id, account, amount, timeStr, old_amount, new_amount);
  }
  return;
}



// check amount on specific Account Balance. Only case of output on the terminal
void checkAmount(unsigned long account, unsigned long thread_id)
{
  pthread_mutex_lock(&Locks[account]);
    char timeStr[100]; get_exact_time(timeStr);
    double currAmount = Balances[account];

    // TERMINAL LOG:
    printf("_________________________________________________________________________________________________\n");
    printf("| [CHILD %lu] at %s\n|\n", thread_id, timeStr);
    printf("| ACCOUNT       :  %lu\n", account);
    printf("| CURRENT AMOUNT:  $%.02f\n", currAmount);
    printf("|________________________________________________________________________________________________\n\n");

    // ACCOUNT LOG:
    log_check_succesfull(AccountsLogs[account], thread_id, account, timeStr, currAmount);

  pthread_mutex_unlock(&Locks[account]);


  pthread_mutex_lock(&MainLogsLocks[checkIDX]);
    
    // MAIN LOG:
    log_check_succesfull(CheckLog, thread_id, account, timeStr, currAmount);
  
  pthread_mutex_unlock(&MainLogsLocks[checkIDX]);
  
  // THREAD LOG:
  log_check_succesfull(ThreadsLogs[thread_id], thread_id, account, timeStr, currAmount);
  
  return;
}



// transfer amount from source Account Balance to destiny Account Balance
void transfer(double amount, unsigned long account_src, unsigned long account_dst, unsigned long thread_id)
{
  if(account_dst == account_src) return;                                    // Trasnsferences to same account must be ignored.

  unsigned long firstAccountLock  = account_src;                            
  unsigned long secondAccountLock = account_dst;                            
  orderAccounts(&firstAccountLock, &secondAccountLock);                     // Deadlock avoidance

  double old_amountSrc;
  double new_amountSrc;
  double old_amountDst;
  double new_amountDst;

  pthread_mutex_lock(&Locks[firstAccountLock]);
    pthread_mutex_lock(&Locks[secondAccountLock]);
      
      old_amountSrc = Balances[account_src];
      new_amountSrc = old_amountSrc - amount;
      old_amountDst = Balances[account_dst];
      new_amountDst = old_amountDst + amount;

      if(new_amountSrc < 0)
      {
        printf("Failed\n");
      }
      else
      {
        // OPERATION:
        Balances[account_src] -= amount;
        Balances[account_dst] += amount;

        // ACCOUNTS LOGS:
        log_transfer_succesfull(AccountsLogs[account_src], thread_id, account_src, account_dst, amount, timeStr, old_amountSrc, old_amountDst, new_amountSrc, new_amountDst);
        log_transfer_succesfull(AccountsLogs[account_dst], thread_id, account_src, account_dst, amount, timeStr, old_amountSrc, old_amountDst, new_amountSrc, new_amountDst);
      }

    pthread_mutex_unlock(&Locks[secondAccountLock]);
  pthread_mutex_unlock(&Locks[firstAccountLock]);

  pthread_mutex_lock(&MainLogsLocks[transferIDX]);

    // MAIN LOG:
    log_transfer_succesfull(TransferenceLog, thread_id, account_src, account_dst, amount, timeStr, old_amountSrc, old_amountDst, new_amountSrc, new_amountDst);

  pthread_mutex_unlock(&MainLogsLocks[transferIDX]);
  
  // THREAD LOG:
  log_transfer_succesfull(ThreadsLogs[thread_id], thread_id, account_src, account_dst, amount, timeStr, old_amountSrc, old_amountDst, new_amountSrc, new_amountDst);

  return;
}



// Deadlock avoidance utility function to transferences
void orderAccounts(unsigned long *firstAccountLock, unsigned long *secondAccountLock)
{
  if(*firstAccountLock < *secondAccountLock) return;                  // Already ordered

  unsigned long holder = *firstAccountLock;
  *firstAccountLock = *secondAccountLock;
  *secondAccountLock = holder;
  return;
}



void get_exact_time(char *buffer) 
{
  struct timespec ts;
  struct tm tm_info;
  
  clock_gettime(CLOCK_REALTIME, &ts);
  localtime_r(&ts.tv_sec, &tm_info);
  
  char timePart[64];
  strftime(timePart, sizeof(timePart), "%a %b %d %H:%M:%S", &tm_info);
  sprintf(buffer, "%s.%09ld %d", timePart, ts.tv_nsec, tm_info.tm_year + 1900);
  
  return;
} 



void log_deposit_succesfull()
{
  fprintf(LogFile, "_________________________________________________________________________________________________\n");
  fprintf(LogFile, "| [CHILD %lu] at %s\n|\n", thread_id, timeStr);
  fprintf(LogFile, "| ACCOUNT   :  %lu\n", account);
  fprintf(LogFile, "| OLD AMOUNT:  $%.02f\n", old_amount);
  fprintf(LogFile, "| DEPOSITED :  $%.02f\n", amount);
  fprintf(LogFile, "| NEW AMOUNT:  $%.02f\n", new_amount);
  fprintf(LogFile, "|________________________________________________________________________________________________\n\n");
}

void log_withdraw_succesfull()
{
  fprintf(LogFile, "_________________________________________________________________________________________________\n");
  fprintf(LogFile, "| [CHILD %lu] at %s\n|\n", thread_id, timeStr);
  fprintf(LogFile, "| ACCOUNT   :  %lu\n", account);
  fprintf(LogFile, "| OLD AMOUNT:  $%.02f\n", old_amount);
  fprintf(LogFile, "| WITHDRAW  :  $%.02f\n", amount);
  fprintf(LogFile, "| NEW AMOUNT:  $%.02f\n", new_amount);
  fprintf(LogFile, "|________________________________________________________________________________________________\n\n");
}

void log_withdraw_failed()
{
  fprintf(LogFile, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
  fprintf(LogFile, "| [CHILD %lu] at %s\n|\n", thread_id, timeStr);
  fprintf(LogFile, "| FAILED WIRHDRAW! \n");
  fprintf(LogFile, "| ACCOUNT   :  %lu\n", ctx->account_src);
  fprintf(LogFile, "| Tried to withdraw $%.02f, but had only $%.02f\n", ctx->amount, ctx->old_amountSrc);
  fprintf(LogFile, "| Withdraw cancelled because would cause debt of $%.02f\n", ctx->new_amountSrc);
  fprintf(LogFile, "|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");
}

void log_check_succesfull(currAmount)
{
  fprintf(LogFile, "_________________________________________________________________________________________________\n");
  fprintf(LogFile, "| [CHILD %lu] at %s\n|\n", thread_id, timeStr);
  fprintf(LogFile, "| ACCOUNT       :  %lu\n", account);
  fprintf(LogFile, "| CURRENT AMOUNT:  $%.02f\n", currAmount);
  fprintf(LogFile, "|________________________________________________________________________________________________\n\n");

}

void log_transfer_succesfull()
{
  fprintf(LogFile, "_________________________________________________________________________________________________\n");
  fprintf(LogFile, "| [CHILD %lu] at %s\n|\n", thread_id, timeStr);
  fprintf(LogFile, "| SOURCE ACCOUNT    :  %lu\n", account_src);
  fprintf(LogFile, "| DESTINY ACCOUNT   :  %lu\n", account_dst);
  fprintf(LogFile, "| SOURCE OLD AMOUNT :  $%.02f\n", old_amountSrc);
  fprintf(LogFile, "| DESTINY OLD AMOUNT:  $%.02f\n", old_amountDst);
  fprintf(LogFile, "| TRANSFERED        :  $%.02f\n", amount);
  fprintf(LogFile, "| SOURCE NEW AMOUNT :  $%.02f\n", new_amountSrc);
  fprintf(LogFile, "| DESTINY NEW AMOUNT:  $%.02f\n", new_amountDst);
  fprintf(LogFile, "|________________________________________________________________________________________________\n\n");
}

void log_transfer_failed()
{
  fprintf(LogFile, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
  fprintf(LogFile, "| [CHILD %lu] at %s\n|\n", thread_id, timeStr);
  fprintf(LogFile, "| FAILED TRANSFERENCE! \n");
  fprintf(LogFile, "| SOURCE ACCOUNT    :  %lu\n", account_src);
  fprintf(LogFile, "| DESTINY ACCOUNT   :  %lu\n", account_dst);
  fprintf(LogFile, "| Account %lu Tried to transfer $%.02f, to Account %lu but had only $%.02f\n", account_src, amount, account_dst, old_amountSrc);
  fprintf(LogFile, "| Transference aborted because would cause debt of $%.02f\n to Account %lu", new_amountSrc, account_src);
  fprintf(LogFile, "|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");
}
