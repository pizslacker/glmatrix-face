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

// Image data for animation frames
unsigned char *img_normal = NULL;
unsigned char *img_wink = NULL;
int img_w, img_h, img_channels;

// Fade and Wink state
float face_alpha = 0.0f;
int face_timer = 0;

int is_winking = 0;
int wink_duration = 0;

typedef enum {
    STATE_WAITING,
    STATE_FADING_IN,
    STATE_HOLDING,
    STATE_FADING_OUT
} FadeState;

FadeState fade_state = STATE_WAITING;

void keyboard_input(unsigned char key, int x, int y) {
    if (key == 27) { 
        if (img_normal) free(img_normal);
        if (img_wink) free(img_wink);
        exit(0); 
    }
}

void load_images(const char *file_normal, const char *file_wink) {
    // Load normal face
    img_normal = stbi_load(file_normal, &img_w, &img_h, &img_channels, 4);
    if (!img_normal) {
        printf("Failed to load image: %s\n", file_normal);
        exit(1);
    }

    // Load winking face
    int w_w, w_h, w_c;
    img_wink = stbi_load(file_wink, &w_w, &w_h, &w_c, 4);
    if (!img_wink) {
        printf("Failed to load image: %s\n", file_wink);
        printf("Make sure you have created the second frame for the animation!\n");
        exit(1);
    }

    // Safety check to prevent memory access violations
    if (img_w != w_w || img_h != w_h) {
        printf("Error: %s and %s must have the exact same dimensions!\n", file_normal, file_wink);
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
        drop_speed[x] = 0.2f + ((rand() % 10) / 20.0f);
        
        for (int y = 0; y < num_rows; y++) {
            grid_chars[x][y] = 33 + (rand() % 94);
            grid_intensity[x][y] = 0.0f;
        }
    }
}

void draw_char(float x, float y, char c, float r, float g, float b) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y + char_h);
    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, c);
}

void display() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    float scale = 0.65f;
    float draw_w = window_w * scale;
    float draw_h = draw_w * ((float)img_h / img_w);
    float start_x = (window_w - draw_w) / 2.0f;
    float start_y = (window_h - draw_h) / 2.0f;

    // Determine which image buffer to read from this frame
    unsigned char *current_img = is_winking ? img_wink : img_normal;

    for (int x = 0; x < num_cols; x++) {
        for (int y = 0; y < num_rows; y++) {
            float intensity = grid_intensity[x][y];
            float screen_x = x * char_w;
            float screen_y = y * char_h;

            float r = 0.0f, g = intensity, b = 0.0f;

            if (intensity > 0.9f) {
                r = intensity;
                b = intensity;
            }

            if (screen_x >= start_x && screen_x < start_x + draw_w &&
                screen_y >= start_y && screen_y < start_y + draw_h) {
                
                int px = (int)(((screen_x - start_x) / draw_w) * img_w);
                int py = (int)(((screen_y - start_y) / draw_h) * img_h);
                
                int pixel_index = (py * img_w + px) * 4;
                float img_r = current_img[pixel_index] / 255.0f;
                float img_g = current_img[pixel_index + 1] / 255.0f;
                float img_b = current_img[pixel_index + 2] / 255.0f;

                float luminance = (img_r + img_g + img_b) / 3.0f;
                
                float face_baseline = face_alpha * luminance;
                if (face_baseline > intensity) {
                    intensity = face_baseline;
                }

                r = (r * (1.0f - face_alpha)) + (img_r * intensity * face_alpha);
                g = (g * (1.0f - face_alpha)) + (img_g * intensity * face_alpha);
                b = (b * (1.0f - face_alpha)) + (img_b * intensity * face_alpha);
            }

            if (intensity > 0.05f) {
                draw_char(screen_x, screen_y, grid_chars[x][y], r, g, b);
            }
        }
    }

    glutSwapBuffers();
}

void update_logic(int value) {
    for (int x = 0; x < num_cols; x++) {
        for (int y = 0; y < num_rows; y++) {
            if (grid_intensity[x][y] > 0.0f) {
                grid_intensity[x][y] -= 0.04f;
            }
            if (rand() % 100 < 2) {
                grid_chars[x][y] = 33 + (rand() % 94);
            }
        }
    }

    for (int x = 0; x < num_cols; x++) {
        drop_y[x] += drop_speed[x];
        int head_row = (int)drop_y[x];
        
        if (head_row < num_rows) {
            grid_intensity[x][head_row] = 1.0f; 
            grid_chars[x][head_row] = 33 + (rand() % 94);
        } else if (drop_y[x] > num_rows + (rand() % 20)) {
            drop_y[x] = 0;
            drop_speed[x] = 0.2f + ((rand() % 10) / 20.0f);
        }
    }

    // Fade Logic
    face_timer++;
    switch (fade_state) {
        case STATE_WAITING:
            if (face_timer > 300) { 
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
            if (face_timer > 600) { // Hold for ~10 seconds
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

    // --- WINK LOGIC ---
    // Only attempt to wink if the face is currently visible
    if (fade_state == STATE_HOLDING && face_alpha > 0.8f) {
        if (!is_winking) {
            // ~1% chance to wink every frame (~1 wink every few seconds)
            if (rand() % 100 == 0) {
                is_winking = 1;
                // Wink duration between 8 and 15 frames (120ms - 240ms)
                wink_duration = 8 + (rand() % 8); 
            }
        } else {
            // Count down the wink duration
            wink_duration--;
            if (wink_duration <= 0) {
                is_winking = 0; // Open eyes again
            }
        }
    } else {
        is_winking = 0; // Ensure eyes are open if fading out
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
    glutCreateWindow("GLMatrix Face Winking");

    // Load both animation frames
    load_images("images/face.jpg", "images/facewink.jpg");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard_input);
    glutTimerFunc(16, update_logic, 0);

    glutMainLoop();
    return 0;
}
