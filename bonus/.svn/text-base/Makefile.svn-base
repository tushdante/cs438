

NAME_OF_EXECUTABLE_FILE = bonus


COMPILER = g++
COMPILER_OPTS = -c -g


LINKER = g++
LINKER_OPTS = -o $(NAME_OF_EXECUTABLE_FILE)

OBJS = bonus.o


$(NAME_OF_EXECUTABLE_FILE) : $(OBJS)
	$(LINKER) $(LINKER_OPTS) $(OBJS) 

bonus.o : bonus.cpp
	$(COMPILER) $(COMPILER_OPTS) bonus.cpp
	
	
clean:
	-rm *.o $(NAME_OF_EXECUTABLE_FILE)
