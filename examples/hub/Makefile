MUDUO_DIRECTORY ?= /home/ubuntu/OtherGithub/build/debug-install-cpp11
MUDUO_INCLUDE = $(MUDUO_DIRECTORY)/include
MUDUO_LIBRARY = $(MUDUO_DIRECTORY)/lib

CXXFLAGS = -g -O0 -Wall -Wextra -Werror \
	   -Wconversion -Wno-unused-parameter \
	   -Wold-style-cast -Woverloaded-virtual \
	   -Wpointer-arith -Wshadow -Wwrite-strings \
	   -march=native -rdynamic \
	   -I$(MUDUO_INCLUDE)

LDFLAGS = -L$(MUDUO_LIBRARY) -lmuduo_net -lmuduo_base -lpthread -lrt

PUBSUBOBJS = pubsub.o codec.o ${MUDUO_LIBRARY}/libmuduo_net.a ${MUDUO_LIBRARY}/libmuduo_base.a 

# 制作静态库
lib: libpubsub.a

libpubsub.a: ${PUBSUBOBJS}
	ar rcs $@ $^
	mv libpubsub.a ${MUDUO_LIBRARY}
	rm pubsub.o codec.o

pubsub.o: pubsub.cc
	g++ -static ${CXXFLAGS} -c $< -o $@

codec.o: codec.cc
	g++ -static ${CXXFLAGS} -c $< -o $@


# 编译可执行文件
bin: hub pub sub
clean:
	rm -f pub sub hub

hub: hub.cc codec.cc
	g++ $(CXXFLAGS) -o $@ $^ -L$(MUDUO_LIBRARY) -lmuduo_inspect -lmuduo_net -lmuduo_base -lpthread -lrt

pub: pub.cc
	g++ $(CXXFLAGS) -o $@ $^ -L$(MUDUO_LIBRARY)  -lpubsub -lmuduo_net  -lmuduo_base -lpthread -lrt

sub: sub.cc
	g++ $(CXXFLAGS) -o $@ $^ -L$(MUDUO_LIBRARY) -lpubsub -lmuduo_net -lmuduo_base -lpthread -lrt

.PHONY: lib bin clean