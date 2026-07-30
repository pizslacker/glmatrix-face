glmatrix-face: glmatrix-face.c
	gcc -Wall -Wextra -O2 -o glmatrix-face glmatrix-face.c -lncurses
	strip glmatrix-face

clean:
	rm -f glmatrix-face