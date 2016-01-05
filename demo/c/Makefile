all: fooltest1 fooltest2_publictransit fooltest2_driving_and_publictransit
fooltest1: fooltest1.c
	gcc -o fooltest1 fooltest1.c -lmmspa4pg -DDEBUG -g3 -O0
	#gcc -o fooltest1 fooltest1.c -lmmspa4pg
fooltest2_publictransit: fooltest2_publictransit.c
	gcc -o fooltest2_publictransit fooltest2_publictransit.c -lmmspa4pg -DDEBUG -g3 -O0
	#gcc -o fooltest-2 fooltest-2.x.c -lmmspa4pg
fooltest2_driving_and_publictransit: fooltest2_driving_and_publictransit.c
	gcc -o fooltest2_driving_and_publictransit fooltest2_driving_and_publictransit.c -lmmspa4pg -DDEBUG -g3 -O0
	#gcc -o fooltest2_driving_and_publictransit fooltest2_driving_and_publictransit.c -lmmspa4pg
clean:
	rm -rf fooltest1 fooltest2 fooltest2_publictransit fooltest2_driving_and_publictransit
