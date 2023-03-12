CXX = g++
CFLAGS = -std=c++17 -O2 -Wall -g 

TARGET = my_server
OBJS = ./log/*.cpp  \
       ./http/*.cpp ./server/*.cpp \
       ./buffer/*.cpp ./timer/heaptimer.cpp  \
	   ./main.cpp 

all: $(OBJS) ./ssl/SSL_ctx.h  ./pool/threadpool.h ./server/IPAddress.h ./timer/heaptimer.h
	$(CXX) $(CFLAGS) $(OBJS) -o  $(TARGET)  -pthread -lmysqlclient -lssl -lcrypto

test:test.cpp ./ssl/SSL_ctx.h
	g++ test.cpp -o test -lssl -lcrypto