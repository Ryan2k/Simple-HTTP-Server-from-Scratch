

RETRIEVER_SOURCES = Retriever.cpp
RETRIEVER_EXE = Retriever
SERVER_SOURCES = Server.cpp
SERVER_EXE = Server

CC = g++
CXXFLAGS = -std=c++14 -g -Wall -Wextra

all:
	$(CC) -pthread -o $(SERVER_EXE) $(SERVER_SOURCES)
	$(CC) -o $(RETRIEVER_EXE) $(RETRIEVER_SOURCES)

clean:
	rm	$(SERVER_EXE) $(RETRIEVER_EXE)