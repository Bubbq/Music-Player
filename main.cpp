#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <raylib.h>

// current user playlist
std::vector<std::string> playlist;
int SONG = 0;
bool PLAY = false;

// the play/pause button
struct BUTTON {
    std::string title;
    // times pressed
    int pc;
    // button position
    Rectangle pos;
    Color color;
};

// loading all the mp3 file locations in a given playlist
void getSongPath(std::string filePath) {

    // loading standard input for the playlist file
    std::ifstream file(filePath);
    if (!file) {
        std::cout << "Error opening " << filePath << std::endl;
        return;
    }

    // reading all lines of the file
    std::string line;
    while (std::getline(file, line)) {
        // getting the position of location tag of xml file, holds the file path of mp3s 
        size_t start = line.find("<location>");
        size_t end = line.find("</location>");
        if (start != std::string::npos && end != std::string::npos) {
            // removing the '<location>file://' part of the tag
            start += 17;
            // add mp3 filepath to current playlist
            playlist.push_back(line.substr(start, end - start));
        }
    }

    // prevent leak
    file.close();
}

int main() {

    SetTargetFPS(60);
    InitWindow(512, 512, "Music Player");
    InitAudioDevice();
    Vector2 mc = { 0 };
    system("find ~/Music -name '*.xspf' > playlistFiles.txt");

    // TODO: need to pass the file not manually!
    getSongPath("/home/bubbq/Music/nujabase.xspf");

    BUTTON play;
    play.pos.height = 50;
    play.pos.width = 120;
    play.pos.x = (GetScreenWidth() * 0.500) - (play.pos.width * 0.500);
    play.pos.y = GetScreenHeight() * 0.800;
    play.color = GREEN;
    play.pc = 0;
    play.title = "PLAY";

    BUTTON skip;
    skip.pos.height = 50;
    skip.pos.width = 120;
    skip.pos.x = play.pos.x + (play.pos.width * 1.500);
    skip.pos.y = play.pos.y;
    skip.color = GREEN;
    skip.title = "SKIP";

    BUTTON rewind;
    rewind.pos.height = 50;
    rewind.pos.width = 120;
    rewind.pos.x = play.pos.x - (play.pos.width * 1.500);
    rewind.pos.y = play.pos.y;
    rewind.color = GREEN;
    rewind.title = "REWIND";

    // base sound and flag for moving to next one
    Sound nujabase = LoadSound(playlist[SONG].c_str());

    while (!WindowShouldClose()) {

        // to move to next song if nothing is playing but the flag says so
        if(!IsSoundPlaying(nujabase) && PLAY){
            SONG++;
            // loop to beginning if we're at the last SONG
            if(SONG > (int)playlist.size() - 1)
                SONG = 0;
            // play that next SONG
            nujabase = LoadSound(playlist[SONG].c_str());
            PlaySound(nujabase);
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            mc = GetMousePosition();
            
            // toggling play/pause feature
            if (CheckCollisionPointRec(mc, play.pos)) {
                // playing song (first time pressing button)
                if (play.pc == 0) {
                    PlaySound(nujabase);
                    play.title = "PAUSE";
                }
                // resume 
                else if (play.title == "PLAY") {
                    ResumeSound(nujabase);
                    play.title = "PAUSE";
                    PLAY = true;
                }
                // pausing
                else {
                    PauseSound(nujabase);
                    play.title = "PLAY";
                    PLAY = false;
                }
                play.pc++;
            }

            // skipping to next sound
            else if(CheckCollisionPointRec(mc, skip.pos)){
                // stop the current sound
                StopSound(nujabase);
                // move on to next song
                SONG++;
                // loop to beginning if we're at the last SONG
                if(SONG > (int)playlist.size() - 1)
                    SONG = 0;
                // play that next song
                nujabase = LoadSound(playlist[SONG].c_str());
                PlaySound(nujabase);
                play.title = "PAUSE";
                PLAY = true;
            }

            // rewinding to last sound
            else if(CheckCollisionPointRec(mc, rewind.pos)){
                // stop the current sound
                StopSound(nujabase);
                // move on to next song
                SONG--;
                // loop to beginning if we're at the last SONG
                if(SONG < 0)
                    SONG = (int)playlist.size() - 1;
                // play that next song
                nujabase = LoadSound(playlist[SONG].c_str());
                PlaySound(nujabase);
                play.title = "PAUSE";
                PLAY = true;
            }
        }

        BeginDrawing();
       
        DrawRectangle(play.pos.x, play.pos.y, play.pos.width, play.pos.height, play.color);
        DrawText(play.title.c_str(), play.pos.x + (play.pos.width * 0.250), play.pos.y + (play.pos.height * 0.300), 20, BLACK);
        
        DrawRectangle(skip.pos.x, skip.pos.y, skip.pos.width, skip.pos.height, skip.color);
        DrawText(skip.title.c_str(), skip.pos.x + (skip.pos.width * 0.250), skip.pos.y + (skip.pos.height * 0.300), 20, BLACK);

        DrawRectangle(rewind.pos.x, rewind.pos.y, rewind.pos.width, rewind.pos.height, rewind.color);
        DrawText(rewind.title.c_str(), rewind.pos.x + (rewind.pos.width * 0.250), rewind.pos.y + (rewind.pos.height * 0.300), 20, BLACK);

        DrawFPS(GetScreenWidth() * 0.780, GetScreenHeight() * 0.100);
        ClearBackground(BLACK);
        EndDrawing();
    }
    UnloadSound(nujabase);
    CloseWindow();
    return 0;
}

