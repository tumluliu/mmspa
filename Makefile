OBJECTS = mmspa.o multimodal-bellmanford.o multimodal-dijkstra-target.o multimodal-dijkstra.o multimodal-twoq.o parser.o bf.o dikf.o f_heap.o

all: libmmspa.so fooltest

libmmspa.so: mmspa.o multimodal-bellmanford.o multimodal-dijkstra-target.o multimodal-dijkstra.o multimodal-twoq.o parser.o bf.o dikf.o f_heap.o
	cc -o ./lib/libmmspa.so $(OBJECTS) -shared

fooltest: libmmspa.so
	cc -o ./bin/fooltest -lmmspa -L/Users/user/Projects/mmspa/lib -Wall fooltest.c

.PHONY: clean
clean: 
	-rm *.o ./lib/libmmspa.so ./bin/fooltest 
