# Language
C11

# How to launch
Same instruction for all four projects :
```bash
$cmake
$make
$projet_train{number_of_project}
```

## CMakeLists.txt tweekings :
Remove commentary around ```"-lpthread"``` if needed by your OS

Remove ```"-DDEBUG"``` if you don't want to see the trains' movement messages

# Network
- A <-> B
- B <-> C
- B <-> D
- D -> C
- C <-> E
- E <-> A

# Routes
- train 1 : ABCBA
- train 2 : ABDCBA
- train 3 : ABDCEA

# Different solutions
- projet_train1 : using only mutex
- projet_train2 : using only semaphore
- projet_train3 : using only rwlock
- projet_train4 : using only message queue
