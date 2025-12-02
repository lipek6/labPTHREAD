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

typedef struct {
    unsigned long thread_id;
    unsigned long account_src;
    unsigned long account_dst;
    double amount;
    double old_amountSrc;
    double old_amountDst;
    double new_amountSrc;
    double new_amountDst;
    char timeStr[100];
} LogContext;



// Functions declarations
void orderAccounts(unsigned long *firstAccountLock, unsigned long *secondAccountLock);
void get_exact_time(char *buffer);
void log_deposit_succesfull(FILE* LogFile, const LogContext *ctx);
void log_withdraw_succesfull(FILE* LogFile, const LogContext *ctx);
void log_withdraw_failed(FILE* LogFile, const LogContext *ctx);
void log_check_succesfull(FILE* LogFile, const LogContext *ctx);
void log_transfer_succesfull(FILE* LogFile, const LogContext *ctx);
void log_transfer_failed(FILE* LogFile, const LogContext *ctx);



// add amount to specific Account Balance
void deposit(double amount, unsigned long account, unsigned long thread_id)
{
  LogContext ctx;
  ctx.account_src = account;
  ctx.thread_id   = thread_id;
  ctx.amount      = amount;

  pthread_mutex_lock(&Locks[account]);
    
    // OPERATION:
    ctx.old_amountSrc = Balances[account];  
    ctx.new_amountSrc = ctx.old_amountSrc + amount;
    Balances[account] = ctx.new_amountSrc;
    get_exact_time(ctx.timeStr);
    
    // ACCOUNT LOG:
    log_deposit_succesfull(AccountsLogs[account], &ctx);

  pthread_mutex_unlock(&Locks[account]);


  pthread_mutex_lock(&MainLogsLocks[depositIDX]);
    
    // MAIN LOG:
    log_deposit_succesfull(DepositLog, &ctx);
  
  pthread_mutex_unlock(&MainLogsLocks[depositIDX]);

  // THREAD LOG:
  log_deposit_succesfull(ThreadsLogs[thread_id], &ctx);

  return;
}



// subtract amount from specific Account Balance
void withdraw(double amount, unsigned long account, unsigned long thread_id)
{
  LogContext ctx;
  ctx.account_src = account;
  ctx.thread_id   = thread_id;
  ctx.amount      = amount;
  short errorOcurrence = 0;  

  pthread_mutex_lock(&Locks[account]);

    ctx.old_amountSrc = Balances[account];
    ctx.new_amountSrc = ctx.old_amountSrc - amount;
    get_exact_time(ctx.timeStr);

    if(ctx.new_amountSrc < (double) 0.00)
    {
      errorOcurrence = 1;
      // ACCOUNT LOG:
      log_withdraw_failed(AccountsLogs[account], &ctx);
    }
    else
    {
      // OPERATION:
      Balances[account] = ctx.new_amountSrc;
      // ACCOUNT LOG:
      log_withdraw_succesfull(AccountsLogs[account], &ctx);
    }
  
  pthread_mutex_unlock(&Locks[account]);

  if(errorOcurrence)
  {
    pthread_mutex_lock(&MainLogsLocks[errorIDX]);
    
      // ERROR LOG:
      log_withdraw_failed(ErrorsLog, &ctx);

    pthread_mutex_unlock(&MainLogsLocks[errorIDX]);
  
    // THREAD LOG:
    log_withdraw_failed(ThreadsLogs[thread_id], &ctx);
  }
  else
  {
    pthread_mutex_lock(&MainLogsLocks[withdrawIDX]);

      // MAIN LOG:
      log_withdraw_succesfull(WithdrawLog, &ctx);

    pthread_mutex_unlock(&MainLogsLocks[withdrawIDX]);

    // THREAD LOG:
    log_withdraw_succesfull(ThreadsLogs[thread_id], &ctx);
  }
  return;
}



// check amount on specific Account Balance. Only case of output on the terminal
void checkAmount(unsigned long account, unsigned long thread_id)
{
  LogContext ctx;
  ctx.account_src = account;
  ctx.thread_id   = thread_id;

  pthread_mutex_lock(&Locks[account]);

    get_exact_time(ctx.timeStr);
    ctx.new_amountSrc = Balances[account];

    // TERMINAL LOG:
    printf("_________________________________________________________________________________________________\n");
    printf("| [CHILD %lu] at %s\n|\n", ctx.thread_id, ctx.timeStr);
    printf("| ACCOUNT       :  %lu\n", ctx.account_src);
    printf("| CURRENT AMOUNT:  $%.02f\n", ctx.new_amountSrc);
    printf("|________________________________________________________________________________________________\n\n");

    // ACCOUNT LOG:
    log_check_succesfull(AccountsLogs[account], &ctx);

  pthread_mutex_unlock(&Locks[account]);


  pthread_mutex_lock(&MainLogsLocks[checkIDX]);
    
    // MAIN LOG:
    log_check_succesfull(CheckLog, &ctx);
  
  pthread_mutex_unlock(&MainLogsLocks[checkIDX]);
  
  // THREAD LOG:
  log_check_succesfull(ThreadsLogs[thread_id], &ctx);
  
  return;
}



// transfer amount from source Account Balance to destiny Account Balance
void transfer(double amount, unsigned long account_src, unsigned long account_dst, unsigned long thread_id)
{
  if(account_dst == account_src) return;                                    // Trasnsferences to same account must be ignored.

  LogContext ctx;
  ctx.account_src = account_src;
  ctx.account_dst = account_dst;
  ctx.thread_id   = thread_id;
  ctx.amount      = amount;

  unsigned long firstAccountLock  = account_src;                            
  unsigned long secondAccountLock = account_dst;                            
  orderAccounts(&firstAccountLock, &secondAccountLock);                     // Deadlock avoidance

  short errorOcurrence = 0;
  pthread_mutex_lock(&Locks[firstAccountLock]);
    pthread_mutex_lock(&Locks[secondAccountLock]);
      
      get_exact_time(ctx.timeStr);
      ctx.old_amountSrc = Balances[account_src];
      ctx.new_amountSrc = ctx.old_amountSrc - amount;
      ctx.old_amountDst = Balances[account_dst];
      ctx.new_amountDst = ctx.old_amountDst + amount;

      if(ctx.new_amountSrc < 0)
      {
        log_transfer_failed(AccountsLogs[account_src], &ctx);
        errorOcurrence = 1;
        //log_transfer_failed(AccountsLogs[account_dst], &ctx);     // We don't need to notify who is not receiving money
      }
      else
      {
        // OPERATION:
        Balances[account_src] -= amount;
        Balances[account_dst] += amount;

        // ACCOUNTS LOGS:
        log_transfer_succesfull(AccountsLogs[account_src], &ctx);
        log_transfer_succesfull(AccountsLogs[account_dst], &ctx);
      }

    pthread_mutex_unlock(&Locks[secondAccountLock]);
  pthread_mutex_unlock(&Locks[firstAccountLock]);

  if(errorOcurrence)
  {
    pthread_mutex_lock(&MainLogsLocks[errorIDX]);

      // MAIN LOG:
      log_transfer_failed(ErrorsLog, &ctx);

    pthread_mutex_unlock(&MainLogsLocks[errorIDX]);
    
    // THREAD LOG:
    log_transfer_failed(ThreadsLogs[thread_id], &ctx);
  }
  else
  {
    pthread_mutex_lock(&MainLogsLocks[transferIDX]);

      // MAIN LOG:
      log_transfer_succesfull(TransferenceLog, &ctx);

    pthread_mutex_unlock(&MainLogsLocks[transferIDX]);
    
    // THREAD LOG:
    log_transfer_succesfull(ThreadsLogs[thread_id], &ctx);
  }

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



void log_deposit_succesfull(FILE* LogFile, const LogContext *ctx)
{
  fprintf(LogFile, "_________________________________________________________________________________________________\n");
  fprintf(LogFile, "| [CHILD %lu] at %s\n|\n", ctx->thread_id, ctx->timeStr);
  fprintf(LogFile, "| ACCOUNT   :  %lu\n", ctx->account_src);
  fprintf(LogFile, "| OLD AMOUNT:  $%.02f\n", ctx->old_amountSrc);
  fprintf(LogFile, "| DEPOSITED :  $%.02f\n", ctx->amount);
  fprintf(LogFile, "| NEW AMOUNT:  $%.02f\n", ctx->new_amountSrc);
  fprintf(LogFile, "|________________________________________________________________________________________________\n\n");
}

void log_withdraw_succesfull(FILE* LogFile, const LogContext *ctx)
{
  fprintf(LogFile, "_________________________________________________________________________________________________\n");
  fprintf(LogFile, "| [CHILD %lu] at %s\n|\n", ctx->thread_id, ctx->timeStr);
  fprintf(LogFile, "| ACCOUNT   :  %lu\n", ctx->account_src);
  fprintf(LogFile, "| OLD AMOUNT:  $%.02f\n", ctx->old_amountSrc);
  fprintf(LogFile, "| WITHDRAW  :  $%.02f\n", ctx->amount);
  fprintf(LogFile, "| NEW AMOUNT:  $%.02f\n", ctx->new_amountSrc);
  fprintf(LogFile, "|________________________________________________________________________________________________\n\n");
}

void log_withdraw_failed(FILE* LogFile, const LogContext *ctx)
{
  fprintf(LogFile, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
  fprintf(LogFile, "| [CHILD %lu] at %s\n|\n", ctx->thread_id, ctx->timeStr);
  fprintf(LogFile, "| FAILED WIRHDRAW! \n");
  fprintf(LogFile, "| ACCOUNT   :  %lu\n", ctx->account_src);
  fprintf(LogFile, "| Tried to withdraw $%.02f, but had only $%.02f\n", ctx->amount, ctx->old_amountSrc);
  fprintf(LogFile, "| Withdraw cancelled because would cause debt of $%.02f\n", ctx->new_amountSrc);
  fprintf(LogFile, "|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");
}

void log_check_succesfull(FILE* LogFile, const LogContext *ctx)
{
  fprintf(LogFile, "_________________________________________________________________________________________________\n");
  fprintf(LogFile, "| [CHILD %lu] at %s\n|\n", ctx->thread_id, ctx->timeStr);
  fprintf(LogFile, "| ACCOUNT       :  %lu\n", ctx->account_src);
  fprintf(LogFile, "| CURRENT AMOUNT:  $%.02f\n", ctx->new_amountSrc);
  fprintf(LogFile, "|________________________________________________________________________________________________\n\n");

}

void log_transfer_succesfull(FILE* LogFile, const LogContext *ctx)
{
  fprintf(LogFile, "_________________________________________________________________________________________________\n");
  fprintf(LogFile, "| [CHILD %lu] at %s\n|\n", ctx->thread_id, ctx->timeStr);
  fprintf(LogFile, "| SOURCE ACCOUNT    :  %lu\n", ctx->account_src);
  fprintf(LogFile, "| DESTINY ACCOUNT   :  %lu\n", ctx->account_dst);
  fprintf(LogFile, "| SOURCE OLD AMOUNT :  $%.02f\n", ctx->old_amountSrc);
  fprintf(LogFile, "| DESTINY OLD AMOUNT:  $%.02f\n", ctx->old_amountDst);
  fprintf(LogFile, "| TRANSFERED        :  $%.02f\n", ctx->amount);
  fprintf(LogFile, "| SOURCE NEW AMOUNT :  $%.02f\n", ctx->new_amountSrc);
  fprintf(LogFile, "| DESTINY NEW AMOUNT:  $%.02f\n", ctx->new_amountDst);
  fprintf(LogFile, "|________________________________________________________________________________________________\n\n");
}

void log_transfer_failed(FILE* LogFile, const LogContext *ctx)
{
  fprintf(LogFile, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
  fprintf(LogFile, "| [CHILD %lu] at %s\n|\n", ctx->thread_id, ctx->timeStr);
  fprintf(LogFile, "| FAILED TRANSFERENCE! \n");
  fprintf(LogFile, "| SOURCE ACCOUNT    :  %lu\n", ctx->account_src);
  fprintf(LogFile, "| DESTINY ACCOUNT   :  %lu\n", ctx->account_dst);
  fprintf(LogFile, "| Account %lu Tried to transfer $%.02f, to Account %lu but had only $%.02f\n", ctx->account_src, ctx->amount, ctx->account_dst, ctx->old_amountSrc);
  fprintf(LogFile, "| Transference aborted because would cause debt of $%.02f\n to Account %lu", ctx->new_amountSrc, ctx->account_src);
  fprintf(LogFile, "|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");
}
