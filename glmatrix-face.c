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

// 3 Image Frames
unsigned char *img_frames[3] = {NULL, NULL, NULL};
int img_w, img_h, img_channels;

// Animation State
float face_alpha = 0.0f;
float smile_factor = 0.0f; // 0.0 = Neutral, 0.5 = Half, 1.0 = Full
int state_timer = 0;

typedef enum {
    STATE_WAITING,
    STATE_FADING_IN,
    STATE_SMILING,
    STATE_HOLDING,
    STATE_FADING_OUT
} FadeState;

FadeState fade_state = STATE_WAITING;

void keyboard_input(unsigned char key, int x, int y) {
    if (key == 27) { 
        for (int i = 0; i < 3; i++) {
            if (img_frames[i]) free(img_frames[i]);
        }
        exit(0); 
    }
}

void load_images(const char *file1, const char *file2, const char *file3) {
    const char *filenames[3] = {file1, file2, file3};
    int w[3], h[3], c[3];

    for (int i = 0; i < 3; i++) {
        img_frames[i] = stbi_load(filenames[i], &w[i], &h[i], &c[i], 4);
        if (!img_frames[i]) {
            printf("Failed to load image: %s\n", filenames[i]);
            exit(1);
        }
        
        if (i > 0 && (w[i] != w[0] || h[i] != h[0])) {
            printf("Error: All images must be the exact same size!\n");
            exit(1);
        }
    }
    
    img_w = w[0];
    img_h = h[0];
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

    for (int x = 0; x < num_cols; x++) {
        for (int y = 0; y < num_rows; y++) {
            float intensity = grid_intensity[x][y];
            float screen_x = x * char_w;
            float screen_y = y * char_h;

            float r = 0.0f, g = intensity, b = 0.0f;

            if (intensity > 0.9f) {
                r = intensity; b = intensity;
            }

            // If the cell is over the face and the face is visible
            if (face_alpha > 0.0f && 
                screen_x >= start_x && screen_x < start_x + draw_w &&
                screen_y >= start_y && screen_y < start_y + draw_h) {
                
                int px = (int)(((screen_x - start_x) / draw_w) * img_w);
                int py = (int)(((screen_y - start_y) / draw_h) * img_h);
                int pixel_index = (py * img_w + px) * 4;
                
                // --- CROSSFADE LOGIC ---
                unsigned char *imgA, *imgB;
                float blend_t;

                // Determine which two frames to blend based on the smile_factor
                if (smile_factor < 0.5f) {
                    imgA = img_frames[0]; // Neutral
                    imgB = img_frames[1]; // Half-smile
                    blend_t = smile_factor * 2.0f; // Scale 0.0-0.5 to 0.0-1.0
                } else {
                    imgA = img_frames[1]; // Half-smile
                    imgB = img_frames[2]; // Full-smile
                    blend_t = (smile_factor - 0.5f) * 2.0f; // Scale 0.5-1.0 to 0.0-1.0
                }

                // Fetch pixel from A
                float rA = imgA[pixel_index] / 255.0f;
                float gA = imgA[pixel_index + 1] / 255.0f;
                float bA = imgA[pixel_index + 2] / 255.0f;

                // Fetch pixel from B
                float rB = imgB[pixel_index] / 255.0f;
                float gB = imgB[pixel_index + 1] / 255.0f;
                float bB = imgB[pixel_index + 2] / 255.0f;

                // Calculate the final interpolated image pixel
                float img_r = (rA * (1.0f - blend_t)) + (rB * blend_t);
                float img_g = (gA * (1.0f - blend_t)) + (gB * blend_t);
                float img_b = (bA * (1.0f - blend_t)) + (bB * blend_t);

                float luminance = (img_r + img_g + img_b) / 3.0f;
                
                float face_baseline = face_alpha * luminance;
                if (face_baseline > intensity) {
                    intensity = face_baseline;
                }

                // Blend the matrix green with the image pixel
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
    // 1. Matrix logic
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

    // 2. State Machine Logic
    state_timer++;
    switch (fade_state) {
        case STATE_WAITING:
            if (state_timer > 300) { // Wait 5 seconds
                fade_state = STATE_FADING_IN;
                state_timer = 0;
            }
            break;
            
        case STATE_FADING_IN:
            face_alpha += 0.01f;
            if (face_alpha >= 1.0f) {
                face_alpha = 1.0f;
                fade_state = STATE_SMILING;
                state_timer = 0;
            }
            break;

        case STATE_SMILING:
            smile_factor += 0.01f; // Gradually transition the frames
            if (smile_factor >= 1.0f) {
                smile_factor = 1.0f;
                fade_state = STATE_HOLDING;
                state_timer = 0;
            }
            break;

        case STATE_HOLDING:
            // 180 frames at ~60 FPS is exactly 3 seconds
            if (state_timer > 180) { 
                fade_state = STATE_FADING_OUT;
                state_timer = 0;
            }
            break;

        case STATE_FADING_OUT:
            // Fade the face to black while running the smile backwards
            face_alpha -= 0.01f;
            smile_factor -= 0.01f;
            
            if (face_alpha <= 0.0f) face_alpha = 0.0f;
            if (smile_factor <= 0.0f) smile_factor = 0.0f;

            if (face_alpha == 0.0f && smile_factor == 0.0f) {
                fade_state = STATE_WAITING;
                state_timer = 0;
            }
            break;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update_logic, 0); // ~60 FPS
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
    glutCreateWindow("Cipher Face Matrix - Gradual Smile");

    load_images("face1.jpg", "face2.jpg", "face3.jpg");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard_input);
    glutTimerFunc(16, update_logic, 0);

    glutMainLoop();
    return 0;
}
