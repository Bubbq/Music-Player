#include <stdio.h>
#include <raylib.h>
#include <string.h>

// reuseable button struct
struct PLAY_BUTTON{
    // title of the button
    char title[32];
    // number of times pressed
    int pc;
    // button position
    Rectangle pos;
    Color color;
};

int main(){
    // benchmark
    SetTargetFPS(60);

    // init
    InitWindow(1024, 512, "Music Player");
    InitAudioDevice();
    Vector2 mc = {0};

    // play/pause button
    struct PLAY_BUTTON play;
    play.pos.height = 50;
    play.pos.width = 120;
    play.pos.x = (GetScreenWidth() / 2.00) -  (play.pos.width / 2.00);
    play.pos.y = 3.00 * GetScreenHeight() / 4.00;
    play.color = GREEN;
    play.pc = 0;
    strcpy(play.title, "PLAY");
         
    // basic sound
    Sound nujabase = LoadSound("/home/bubbq/Documents/CS_Projects/Music-Player/ttest.mp3");
    
    // game loop
    while(!WindowShouldClose()){
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // Check if the mouse press was on the play/pause button
            mc = GetMousePosition();
            printf("(%f, %f)\n", mc.x, mc.y);
            // first time click starts the audio
            if (CheckCollisionPointRec(mc, play.pos)) {
                if(play.pc == 0){
                    PlaySound(nujabase);
                    strcpy(play.title, "PAUSE");
                }

                else if(strcmp(play.title, "PLAY") == 0) {
                    ResumeSound(nujabase);
                    strcpy(play.title, "PAUSE");
                }
                else{
                    // If the title is "PAUSE", pause the sound
                    PauseSound(nujabase);
                    strcpy(play.title, "PLAY");
                }
            }
            play.pc++;
        }

        // rendering phase
        BeginDrawing();

        // draw play music button and its title
        DrawRectangle(play.pos.x, play.pos.y, play.pos.width, play.pos.height, play.color);
        DrawText(play.title, play.pos.x + (play.pos.width * 0.250), play.pos.y + (play.pos.height * 0.300), 20, BLACK);
       
        // benchmark testing
        DrawFPS(GetScreenWidth() * 0.900, GetScreenHeight() * 0.100);
        
        // setting background color
        ClearBackground(WHITE);

        // end rendering phase
        EndDrawing();
    }

    // prevent leak
    UnloadSound(nujabase);
    CloseWindow();
    return 0;
}
