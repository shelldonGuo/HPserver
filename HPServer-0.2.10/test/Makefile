
# output path
BIN = ./bin

# itermidiate objects
OBJ_TEST_INIT = $(addprefix $(BIN)/, \
	  test-init.o \
	  )
OBJ_TEST_TIMER = $(addprefix $(BIN)/, \
	  test-timer.o \
	  )
OBJ_TEST_SIM = $(addprefix $(BIN)/, \
	  test-sim-events.o \
	  )
OBJ_TEST_MUL = $(addprefix $(BIN)/, \
	  test-mul-events.o \
	  )
OBJ_TEST_THREADS = $(addprefix $(BIN)/, \
	  test-threads.o \
	  )

# source file path
INC = -I. -I../include -I../include/linux
SRC = ../src
 
# static lib path
COMMON_LIB_PATH = -L. -L../bin
COMMON_LIBS = $(COMMON_LIB_PATH) -lHPServer

LIB_CPP   = -L. -lstdc++ -lrt -lpthread

all : clean $(BIN)/test-init $(BIN)/test-timer $(BIN)/test-sim-events  $(BIN)/test-mul-events  $(BIN)/test-threads
#all : clean $(BIN)/echo-server

test : all
		@echo "--------------test start--------------"
		@echo "======================================"
		@echo "-"
		@echo "use 'ulimit -n 10240' to enlarge the maximum fd size, before your testing."
		@echo "-"
		@echo "======================================"
		@ $(BIN)/test-init
		@ $(BIN)/test-timer
		@ $(BIN)/test-sim-events
		@ $(BIN)/test-mul-events
		@ $(BIN)/test-threads 33665
		
# build tests
$(BIN)/test-init : prepare $(OBJ_TEST_INIT)
	g++ -O -o $@ $(OBJ_TEST_INIT) -g $(LIB_CPP) $(LIB_EVENT) $(COMMON_LIBS) 

$(BIN)/test-timer: prepare $(OBJ_TEST_TIMER)
	g++ -O -o $@ $(OBJ_TEST_TIMER) -g $(LIB_CPP) $(LIB_EVENT) $(COMMON_LIBS) 

$(BIN)/test-sim-events : prepare $(OBJ_TEST_SIM)
	g++ -O -o $@ $(OBJ_TEST_SIM) -g $(LIB_CPP) $(LIB_EVENT) $(COMMON_LIBS) 
	
$(BIN)/test-mul-events : prepare $(OBJ_TEST_MUL)
	g++ -O -o $@ $(OBJ_TEST_MUL) -g $(LIB_CPP) $(LIB_EVENT) $(COMMON_LIBS)

$(BIN)/test-threads : prepare $(OBJ_TEST_THREADS)
	g++ -O -o $@ $(OBJ_TEST_THREADS) -g $(LIB_CPP) $(LIB_EVENT) $(COMMON_LIBS)
	
	
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
		
