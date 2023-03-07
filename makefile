CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = my_server
OBJS = ./log/*.cpp  \
       ./http/*.cpp ./server/*.cpp \
       ./buffer/*.cpp ./main.cpp

all: $(OBJS) ./ssl/SSL_ctx.h  ./pool/threadpool.h
	$(CXX) $(CFLAGS) $(OBJS) -o  $(TARGET)  -pthread -lmysqlclient -lssl -lcrypto

test:test.cpp ./ssl/SSL_ctx.h
	g++ test.cpp -o test -lssl -lcrypto