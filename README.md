# glmatrix-face

An example C program that demonstrates _Text Mosaic Mapping_ with `OpenGL` for ASCII terminal art and animated _matrix_-like scrolling cipher code.

This version uses a Grid State, insted of drawing arbitrary floating coordinates, and manages a rigid 2D array of character cells (like a real terminal).
- Pixel mapping for every character cell
- Cipher blend (for every character overlapping face image)
- Shifts active text color from green to image color

The name says it all :P

![Cmatrix-Face in action...](https://github.com/pizslacker/glmatrix-face/blob/main/images/glmatrix-face.png)
