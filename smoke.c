#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__)
#else
#define VERBOSE_PRINT(S, ...) ((void) 0) // do nothing
#endif

struct Agent {
  uthread_mutex_t mutex;
  uthread_cond_t  match;
  uthread_cond_t  paper;
  uthread_cond_t  tobacco;
  uthread_cond_t  smoke;
  uthread_cond_t  success;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  agent->mutex   = uthread_mutex_create();
  agent->paper   = uthread_cond_create(agent->mutex);
  agent->match   = uthread_cond_create(agent->mutex);
  agent->tobacco = uthread_cond_create(agent->mutex);
  agent->smoke   = uthread_cond_create(agent->mutex);
  agent->success = uthread_cond_create(agent->mutex);
  return agent;
}

//
// TODO
// You will probably need to add some procedures and struct etc.
//

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

// # of threads waiting for a signal. Used to ensure that the agent
// only signals once all other threads are ready.
int num_active_threads = 0;

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked
int agentisdone;
int unique_sum = 0;

/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can modify it if you like, but be sure that all it does
 * is choose 2 random resources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};

  srandom(time(NULL));
  
  uthread_mutex_lock (a->mutex);
  // Wait until all other threads are waiting for a signal
  while (num_active_threads < 3)
    uthread_cond_wait (a->smoke);

  for (int i = 0; i < NUM_ITERATIONS; i++) {
    int r = random() % 6;
    switch(r) {
    case 0:
      signal_count[TOBACCO]++;
      VERBOSE_PRINT ("match available\n");
      uthread_cond_signal (a->match);
      VERBOSE_PRINT ("paper available\n");
      uthread_cond_signal (a->paper);
      break;
    case 1:
      signal_count[PAPER]++;
      VERBOSE_PRINT ("match available\n");
      uthread_cond_signal (a->match);
      VERBOSE_PRINT ("tobacco available\n");
      uthread_cond_signal (a->tobacco);
      break;
    case 2:
      signal_count[MATCH]++;
      VERBOSE_PRINT ("paper available\n");
      uthread_cond_signal (a->paper);
      VERBOSE_PRINT ("tobacco available\n");
      uthread_cond_signal (a->tobacco);
      break;
    case 3:
      signal_count[TOBACCO]++;
      VERBOSE_PRINT ("paper available\n");
      uthread_cond_signal (a->paper);
      VERBOSE_PRINT ("match available\n");
      uthread_cond_signal (a->match);
      break;
    case 4:
      signal_count[PAPER]++;
      VERBOSE_PRINT ("tobacco available\n");
      uthread_cond_signal (a->tobacco);
      VERBOSE_PRINT ("match available\n");
      uthread_cond_signal (a->match);
      break;
    case 5:
      signal_count[MATCH]++;
      VERBOSE_PRINT ("tobacco available\n");
      uthread_cond_signal (a->tobacco);
      VERBOSE_PRINT ("paper available\n");
      uthread_cond_signal (a->paper);
      break;
    }
    VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
    uthread_cond_wait (a->smoke);
  }
  agentisdone = 1;
  uthread_mutex_unlock (a->mutex);
  return NULL;
}

//enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};



void go_smoke(struct Agent *a) {

    if (unique_sum == 6) {
        smoke_count[1]++; //match guy
        unique_sum = 0;
        uthread_cond_signal(a->success);
    }
    if (unique_sum == 3) {
        smoke_count[4]++; //tobacco guy
        unique_sum = 0;
        uthread_cond_signal(a->success);
    }
    if (unique_sum == 5) {
        smoke_count[2]++; //paper guy
        unique_sum = 0;
        uthread_cond_signal(a->success);
    }
}

void* paper_sensor(void* v) {
    struct Agent *a = v;
  while (!agentisdone){
      uthread_mutex_lock(a->mutex);
    num_active_threads++;
    if (num_active_threads==3){uthread_cond_signal(a->smoke);}
    uthread_cond_wait(a->paper);
    num_active_threads--;
    unique_sum += 2;
    go_smoke(a);
    uthread_cond_wait(a->success);
    uthread_cond_signal(a->success);
    uthread_mutex_unlock(a->mutex);


  }
}

void* tobacco_sensor(void* v) {
    struct Agent *a = v;
    while (!agentisdone) {
        uthread_mutex_lock(a->mutex);
        num_active_threads++;
        if (num_active_threads == 3) { uthread_cond_signal(a->smoke); }
        uthread_cond_wait(a->tobacco);
        num_active_threads--;
        unique_sum += 4;
        go_smoke(a);
        uthread_cond_wait(a->success);
        uthread_cond_signal(a->success);
        uthread_mutex_unlock(a->mutex);


    }
}

void* match_sensor(void* v) {
    struct Agent *a = v;
    while (!agentisdone) {
        uthread_mutex_lock(a->mutex);
        num_active_threads++;
        if (num_active_threads == 3) { uthread_cond_signal(a->smoke); }
        uthread_cond_wait(a->match);
        num_active_threads--;
        unique_sum += 1;
        go_smoke(a);
        uthread_cond_wait(a->success);
        uthread_cond_signal(a->success);
        uthread_mutex_unlock(a->mutex);


    }
}












int main (int argc, char** argv) {
  
  struct Agent* a = createAgent();
    uthread_t agent_thread,tobacco_thread,paper_thread,match_thread;

  uthread_init(5);

  // TODO

  agent_thread = uthread_create(agent, a);
    match_thread = uthread_create(match_sensor, a);
  tobacco_thread = uthread_create(tobacco_sensor, a);
  paper_thread = uthread_create(paper_sensor, a);

  uthread_join(agent_thread, NULL);

  printf ("Signal count match: %d, Smoke count match: %d\n",signal_count[1], smoke_count[1]);
    printf ("Signal count tobacco: %d, Smoke count tobacco: %d\n",signal_count[4], smoke_count[4]);
    printf ("Signal count paper: %d, Smoke count paper: %d\n",signal_count[2], smoke_count[2]);

  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);

  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);

  return 0;
}
