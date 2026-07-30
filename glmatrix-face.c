#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MAX_COLS 250
#define MAX_ROWS 200

// Matrix grid state
char grid_chars[MAX_COLS][MAX_ROWS];
float grid_intensity[MAX_COLS][MAX_ROWS];
float drop_y[MAX_COLS];
float drop_speed[MAX_COLS];

int num_cols = 0;
int num_rows = 0;
int char_w = 10;
int char_h = 14; 
int window_w = 800, window_h = 600;

// Image data kept in CPU memory
unsigned char *img_data = NULL;
int img_w, img_h, img_channels;

// Fade state
float face_alpha = 0.0f;
int face_timer = 0;

typedef enum {
    STATE_WAITING,
    STATE_FADING_IN,
    STATE_HOLDING,
    STATE_FADING_OUT
} FadeState;

FadeState fade_state = STATE_WAITING;

// Standard C function for keyboard input (fixes compile error)
void keyboard_input(unsigned char key, int x, int y) {
    if (key == 27) { // ESC key
        if (img_data) free(img_data);
        exit(0); 
    }
}

void load_image(const char *filename) {
    // Force loading 4 channels (RGBA) so we always know byte offsets
    img_data = stbi_load(filename, &img_w, &img_h, &img_channels, 4);
    if (!img_data) {
        printf("Failed to load image: %s\n", filename);
        exit(1);
    }
}

void init_rain() {
    num_cols = window_w / char_w;
    num_rows = window_h / char_h;
    if (num_cols > MAX_COLS) num_cols = MAX_COLS;
    if (num_rows > MAX_ROWS) num_rows = MAX_ROWS;
    
    for (int x = 0; x < num_cols; x++) {
        drop_y[x] = rand() % num_rows;
        drop_speed[x] = 0.2f + ((rand() % 10) / 20.0f); // Drops move slower on the grid
        
        for (int y = 0; y < num_rows; y++) {
            grid_chars[x][y] = 33 + (rand() % 94);
            grid_intensity[x][y] = 0.0f;
        }
    }
}

void draw_char(float x, float y, char c, float r, float g, float b) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y + char_h); // Offset baseline so it draws inside the cell
    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, c);
}

void display() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Calculate image placement bounds
    float scale = 0.65f; // Face takes up 65% of screen width
    float draw_w = window_w * scale;
    float draw_h = draw_w * ((float)img_h / img_w);
    float start_x = (window_w - draw_w) / 2.0f;
    float start_y = (window_h - draw_h) / 2.0f;

    for (int x = 0; x < num_cols; x++) {
        for (int y = 0; y < num_rows; y++) {
            float intensity = grid_intensity[x][y];
            float screen_x = x * char_w;
            float screen_y = y * char_h;

            float r = 0.0f, g = intensity, b = 0.0f; // Default Matrix Green

            // If it's the head of a drop, make it white
            if (intensity > 0.9f) {
                r = intensity;
                b = intensity;
            }

            // --- CHECK IF CELL IS OVER THE FACE IMAGE ---
            if (screen_x >= start_x && screen_x < start_x + draw_w &&
                screen_y >= start_y && screen_y < start_y + draw_h) {
                
                // Map the screen cell to the specific pixel in the image array
                int px = (int)(((screen_x - start_x) / draw_w) * img_w);
                int py = (int)(((screen_y - start_y) / draw_h) * img_h);
                
                // Fetch the pixel colors (4 bytes per pixel because we forced 4 channels)
                int pixel_index = (py * img_w + px) * 4;
                float img_r = img_data[pixel_index] / 255.0f;
                float img_g = img_data[pixel_index + 1] / 255.0f;
                float img_b = img_data[pixel_index + 2] / 255.0f;

                // Calculate brightness of this specific pixel
                float luminance = (img_r + img_g + img_b) / 3.0f;
                
                // Illuminate background characters based on the photo
                float face_baseline = face_alpha * luminance;
                if (face_baseline > intensity) {
                    intensity = face_baseline;
                }

                // Morph the matrix colors into the photo colors
                r = (r * (1.0f - face_alpha)) + (img_r * intensity * face_alpha);
                g = (g * (1.0f - face_alpha)) + (img_g * intensity * face_alpha);
                b = (b * (1.0f - face_alpha)) + (img_b * intensity * face_alpha);
            }

            // Only draw characters that are bright enough to be seen
            if (intensity > 0.05f) {
                draw_char(screen_x, screen_y, grid_chars[x][y], r, g, b);
            }
        }
    }

    glutSwapBuffers();
}

void update_logic(int value) {
    // 1. Decay the rain tails and mutate characters
    for (int x = 0; x < num_cols; x++) {
        for (int y = 0; y < num_rows; y++) {
            if (grid_intensity[x][y] > 0.0f) {
                grid_intensity[x][y] -= 0.04f;
            }
            // Add a "cipher" effect: randomly swap characters so the face feels alive
            if (rand() % 100 < 2) {
                grid_chars[x][y] = 33 + (rand() % 94);
            }
        }
    }

    // 2. Move drops down the grid
    for (int x = 0; x < num_cols; x++) {
        drop_y[x] += drop_speed[x];
        int head_row = (int)drop_y[x];
        
        if (head_row < num_rows) {
            grid_intensity[x][head_row] = 1.0f; // Light up the head
            grid_chars[x][head_row] = 33 + (rand() % 94);
        } else if (drop_y[x] > num_rows + (rand() % 20)) {
            drop_y[x] = 0;
            drop_speed[x] = 0.2f + ((rand() % 10) / 20.0f);
        }
    }

    // 3. Fade Logic
    face_timer++;
    switch (fade_state) {
        case STATE_WAITING:
            if (face_timer > 300) { // Wait 5 seconds
                fade_state = STATE_FADING_IN;
                face_timer = 0;
            }
            break;
        case STATE_FADING_IN:
            face_alpha += 0.005f;
            if (face_alpha >= 1.0f) {
                face_alpha = 1.0f;
                fade_state = STATE_HOLDING;
                face_timer = 0;
            }
            break;
        case STATE_HOLDING:
            if (face_timer > 400) { // Hold for ~6 seconds
                fade_state = STATE_FADING_OUT;
                face_timer = 0;
            }
            break;
        case STATE_FADING_OUT:
            face_alpha -= 0.005f;
            if (face_alpha <= 0.0f) {
                face_alpha = 0.0f;
                fade_state = STATE_WAITING;
                face_timer = 0;
            }
            break;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update_logic, 0);
}

void reshape(int w, int h) {
    window_w = w;
    window_h = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);
    glMatrixMode(GL_MODELVIEW);
    init_rain();
}

int main(int argc, char **argv) {
    srand(time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(window_w, window_h);
    glutCreateWindow("Cipher Face Matrix");

    load_image("face.jpg");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard_input);
    glutTimerFunc(16, update_logic, 0);

    glutMainLoop();
    return 0;
}
