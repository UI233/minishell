OBJ=builtin.o command.o interpreter.o myshell.o
HEADERS=include/builtin.h include/command.h include/interpreter.h
PROJECT=myshell
CC=gcc

myshell: $(OBJ) $(HEADERS)
	$(CC) -o $(PROJECT) $(OBJ)

myshell.o: myshell.c myshell.h $(HEADERS)
	gcc -c myshell.c
builtin.o: src/builtin.c $(HEADERS)
	gcc -c src/builtin.c
interpreter.o: src/interpreter.c $(HEADERS)
	gcc -c src/interpreter.c
command.o: src/command.c $(HEADERS)
	gcc -c src/command.c

.PHOXY: clean
clean:
	rm $(OBJ) $(PROJECT)
