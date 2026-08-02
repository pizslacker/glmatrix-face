glmatrix-face: glmatrix-face.c
	gcc -Wall -Wextra -O3 -o glmatrix-face glmatrix-face.c -lX11 -lGL -lm
	strip glmatrix-face

clean:
	rm -f glmatrix-face
