CFLAGS = -g -Wall -std=c++11 -I /home/ubuntu/OtherGithub/build/debug-install-cpp11/include
LIBS = -L /home/ubuntu/OtherGithub/build/debug-install-cpp11/lib -lmuduo_base -lmuduo_net -lpthread

SRCS = main.cc echo.cc
EXECUTABLE = echo
CC = g++

all:
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(SRCS) $(LIBS)

.PHONY:clean
clean:
	rm $(EXECUTABLE)
