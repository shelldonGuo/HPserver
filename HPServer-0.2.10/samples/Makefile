
# output path
BIN = ./bin

# itermidiate objects
OBJ_ECHO = $(addprefix $(BIN)/, \
	  echo-server.o \
	  )
	  
OBJ_SIGNAL = $(addprefix $(BIN)/, \
	  signal-handler.o \
	  )
	  
# source file path
INC = -I. -I../include -I../include/linux
SRC = ../src

# static lib path
COMMON_LIB_PATH = -L. -L../bin
COMMON_LIBS = $(COMMON_LIB_PATH) -lHPServer

LIB_CPP   = -L. -lstdc++ -lrt -lpthread

all : clean $(BIN)/echo-server $(BIN)/signal-handler
#all : clean $(BIN)/echo-server
	echo "build all samples"
	
# build echo server
$(BIN)/echo-server : prepare $(OBJ_ECHO)
	g++ -O -o $@ $(OBJ_ECHO) -g $(LIB_CPP) $(LIB_EVENT) $(COMMON_LIBS) 

# build signal handler test
$(BIN)/signal-handler : prepare $(OBJ_SIGNAL)
	g++ -O -o $@ $(OBJ_SIGNAL) -g $(LIB_CPP) $(LIB_EVENT) $(COMMON_LIBS) 

# src/*.cpp
$(BIN)/%.o : %.cpp
	g++ $(INC) -g -c $< -o $@	

# prepare the bin dir	
.PHONY : prepare	
prepare : 
		-mkdir $(BIN)
	  
.PHONY : clean
clean  :
		-rm -rf $(BIN)
		
