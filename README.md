planner: Tries all the combinations, very slow and nearly impossible to run for (X,Y,Z) > 3

planner-genAlgo: Use a genetic algorithm to improve a population initialized from random values. Much much faster but not very accurate for huge reactors.

How to build: make all

How to run: ```main <X size> <Y size> <Z size>```

then select [1] or [2] for either :
- 1, or combination : explore all the combination possibilities
- 2, or genetic algorithm generation : explore random reactor generation and try to improve them over multiple generations. In this mode, you can choose to generate an X, Y and/or Z symetric reactor (for easier build for example)