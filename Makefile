all: main
main: main.o io.o index.o list.o tool_funs.o find.o
	gcc -std=gnu99 -Wall -o main main.o io.o index.o list.o tool_funs.o find.o -lpthread -lm

main.o: main.c lib/common.h lib/list.h lib/io.h lib/index.h lib/tool_funs.h lib/find.h lib/structs.h
	gcc -std=gnu99 -Wall -c main.c -lpthread -lm

io.o: io.c lib/common.h lib/io.h
	gcc -std=gnu99 -Wall -c io.c

list.o: list.c lib/common.h lib/list.h lib/io.h
	gcc -std=gnu99 -Wall -c list.c

index.o: index.c lib/index.h lib/common.h lib/tool_funs.h lib/io.h lib/structs.h lib/find.h
	gcc -std=gnu99 -Wall -c index.c

tool_funs.o: tool_funs.c lib/common.h lib/tool_funs.h
	gcc -std=gnu99 -Wall -c tool_funs.c

find.o: find.c lib/common.h lib/find.h lib/tool_funs.h lib/io.h lib/structs.h
	gcc -std=gnu99 -Wall -c find.c

.PHONY: all clean
clean:
	rm main *.o