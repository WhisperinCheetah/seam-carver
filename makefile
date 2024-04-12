
seam:
	gcc -Wall -Wextra -ggdb -O3 -o main main.c stb_image.o stb_image_write.o -lm

clean:
	mv main main.old
	rm -f main
