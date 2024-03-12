#include <raylib.h>
#include "constants.h"

int main() {
    InitWindow(800, 600, "Rounded Rectangle Example");

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(WHITE);

        // Draw a rounded rectangle with a specified radius
        DrawRectangleRounded((Rectangle){BTTN_X, WINDOW_SIZE * 0.750, float((WINDOW_SIZE * 0.833) - BTTN_X), BTTN_HEIGHT * 2.250}, 20, 10, BLACK);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
