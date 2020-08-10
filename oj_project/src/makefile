LDFLAGS=-L /home/jl/ThirdPartLibForCpp/ThirdPartLibForCpp/el7.x86_64/third_part/lib
DEBUGFLAGS=-g
BIN=oj_server
CC=g++

.PHONY:$(BIN)
$(BIN):oj_server.cpp
	$(CC) $^ -o $@  $(LDFLAGS) -lctemplate -std=c++11 -ljsoncpp -lpthread

.PHONY:clean
clean:
	rm -f $(BIN)
