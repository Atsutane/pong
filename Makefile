all:
	mkdir dbg
	$(CC) src/pong.c -o dbg/pong -Wall -lncurses
