# Language & OS
C & Linux (Ubuntu 16.04)

# How to launch
Same instruction for all four projects :
```bash
$gcc main.c header.h -Wall -Wextra -Werror -O3 -lpthread -lm -DDEBUG -DTEST -o train.out
$./train.out
```
Add ```-lrt``` only for project 4

Remove ```-DDEBUG``` if you don't want to see the trains' movement messages

Remove ```-DTEST``` if you don't want to see the timing measurements

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
