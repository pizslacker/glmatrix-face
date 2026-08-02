glmatrix-face-xscreensaver: glmatrix-face.c
	gcc -Wall -Wextra -O3 -o glmatrix-face-xscreensaver glmatrix-face.c -lX11 -lGL -lm
	strip glmatrix-face-xscreensaver

clean:
	rm -f glmatrix-face-xscreensaver
