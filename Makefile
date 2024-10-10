CFLAGS = -Wall -Wextra -std=c99

build: sfl

sfl:	
	gcc $(CFLAGS) main.c -o sfl

run_sfl: sfl
	./sfl

clean:
	rm -f sfl
