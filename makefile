
seam:
	gcc -Wall -Wextra -ggdb -O3 -I./raylib/ -o main main.c -L./raylib/ -lraylib -lm

clean:
	mv main main.old
	rm -f main
