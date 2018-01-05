# Description
This program gives different solutions using threads to have three trains doing trips on a rail network, without collision or overtaking between trains.

Each time a train goes from one gare to another, a random time between 1 and 3 second(s) is taken. A whole trip is repeated several times for each train with a lot of different starting random seeds.

# Language & OS
C & Linux (Ubuntu 16.04)

# How to launch
Same instruction for all four projects :
```bash
$gcc main.c header.h -Wall -Wextra -Werror -O3 -lpthread -lm -DDEBUG -DTEST -o train.out
$./train.out
```
Add ```-lrt``` only for project 4

Remove ```-DDEBUG``` if you don't want to see the trains' movement messages (they are written to stdout)

Remove ```-DTEST``` if you don't want to see the timing measurements (they are written to stderr)

In header.h, adjust N_TRAJETS and MAX_SEED to change the duration of the program. N_TRAJETS is the number of trip done for each seed and MAX_SEED is the number of the last seed that will be tested for rand values, starting from 1.

# Network
- A <-> B
- B <-> C
- B <-> D
- D -> C
- C <-> E
- E <-> A

# Routes
- train 1 : A->B->C->B->Back to beginning
- train 2 : A->B->D->C->B->Back to beginning
- train 3 : A->B->D->C->E->Back to beginning

# Different solutions
- projet_train1 : using only mutex
- projet_train2 : using only semaphore
- projet_train3 : using only rwlock
- projet_train4 : using only message queue
