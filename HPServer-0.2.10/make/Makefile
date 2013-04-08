
# output path
BIN = ../bin
INSTALL_DIR = ../HPServer
INSTALL_DIR_INC = ../HPServer/include
INSTALL_DIR_LIB = ../HPServer/lib
INSTALL_DIR_BIN = ../HPServer/bin

CFLAG = DEBUG
# debug stuff 
ifeq ($(CFLAG), DEBUG)
CFLAGS += -g
else
CFLAGS += -O -s
endif

# itermidiate objects
OBJ = $(addprefix $(BIN)/, \
	  SockUtil.o	\
	  TimeUtil.o	\
	  InetAddr.o  \
	  Log.o			\
	  SockAcceptor.o  \
	  DemuxTable.o \
	  SignalInfo.o	\
	  Reactor_Epoll.o	\
	  Reactor_Select.o	\
	  Reactor_Imp.o		\
	  EventScheduler.o	\
	  Reactor.o		\
	  )

# source file path
INC = -I. -I../include -I../include/linux
SRC = ../src

# static lib path
STATIC_LIB_PATH = -L. -L../lib

all : prepare $(BIN)/libHPServer.so
	@echo "build libHPServer..."

install: $(BIN)/libHPServer.so
	@echo "build package..."
	@echo "Install dir = $(INSTALL_DIR)"
	@rm $(INSTALL_DIR) -rf
	# copy include files
	@-mkdir $(INSTALL_DIR) $(INSTALL_DIR_INC) $(INSTALL_DIR_LIB) $(INSTALL_DIR_BIN)
	
	@-mkdir $(INSTALL_DIR_INC)/linux
	cp ../include/HP_Config.h ../include/defines.h          $(INSTALL_DIR_INC)/linux/ -rf
	cp ../include/linux/SockUtil.h ../include/TimeUtil.h    $(INSTALL_DIR_INC)/linux/ -rf
	cp ../include/InetAddr.h ../include/Log.h               $(INSTALL_DIR_INC)/linux/ -rf
	cp ../include/SockAcceptor.h ../include/Connector.h     $(INSTALL_DIR_INC)/linux/ -rf
	cp ../include/EventHandler.h ../include/Reactor.h       $(INSTALL_DIR_INC)/linux/ -rf
	cp ../include/Reactor_Imp.h ../include/EventScheduler.h $(INSTALL_DIR_INC)/linux/ -rf
	#copy libraries
	-mkdir $(INSTALL_DIR_LIB)/linux	
	cp ../bin/*.a   $(INSTALL_DIR_LIB)/linux/ -rf
	-mkdir $(INSTALL_DIR_BIN)/linux	
	cp ../bin/*.so* $(INSTALL_DIR_BIN)/linux/ -rf

# build as a dynamic lib

$(BIN)/libHPServer.so : $(OBJ)
		g++ $(OBJ) -fPIC -shared -o $@

# src/*.cpp
$(BIN)/%.o : $(SRC)/%.cpp
	g++ $(INC) $(CFLAGS) -c $< -o $@	

# src/linux/*.cpp	
$(BIN)/%.o : $(SRC)/linux/%.cpp
	g++ $(INC) $(CFLAGS) -c $< -o $@
	
# prepare the bin dir	
.PHONY : prepare	
prepare : 
		-mkdir $(BIN)
	  
.PHONY : clean
clean  :
		-rm -rf $(BIN)

