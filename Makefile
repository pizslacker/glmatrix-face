glmatrix-face: glmatrix-face.c
	gcc -Wall -Wextra -O3 -o glmatrix-face glmatrix-face.c -lGL -lGLU -lglut -lm
	strip glmatrix-face

clean:
	rm -f glmatrix-face
