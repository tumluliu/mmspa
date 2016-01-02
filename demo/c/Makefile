all: fooltest-1 fooltest-2
fooltest-1: fooltest-1.x.c
	#gcc -o fooltest-1 fooltest-1.x.c -lmmspa4pg -g3 -O0
	gcc -o fooltest-1 fooltest-1.x.c -lmmspa4pg
fooltest-2: fooltest-2.x.c
	#gcc -o fooltest-2 fooltest-2.x.c -lmmspa4pg -g3 -O0
	gcc -o fooltest-2 fooltest-2.x.c -lmmspa4pg
clean:
	rm -rf fooltest-1 fooltest-2
