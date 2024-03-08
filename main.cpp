#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <raylib.h>

// current user playlist
std::vector<std::string> songQueue;
// containing all file paths of user playlists
std::vector<std::string> playlists;
// the ith song in the song queue 
// base sound and flag for songQueue to next one
Sound nujabase;
int SONG = 0;
int PLAYLIST = 0;
// flag determining if we should be playing music or not
bool PLAY = false;

struct BUTTON {
    std::string title;
    // times pressed
    int pc;
    // button position
    Rectangle pos;
    Color color;
};

// storing all playlsits buttons
std::vector<BUTTON> buttons;

// storing all the filepaths of users playlists
void getPlaylists(){

    std::ifstream inFile("playlistFiles.txt");

    if(!inFile){
        std::cout << "Error, playlistFiles.txt could not be opened!" << std::endl;
        return;
    }
    std::string filePath;
    // each line contains a playlist file path from the user, read line by line to store all those paths in the vector
    while(std::getline(inFile, filePath)){
        playlists.push_back(filePath);
        std::cout << "Playlist File: \t" << filePath << std::endl;
    }

    // prevent leak
    inFile.close();
}

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
            songQueue.push_back(line.substr(start, end - start));
            std::cout << line.substr(start, end - start) << std::endl;
        }
    }

    // prevent leak
    file.close();
}

void saveMusic(){
    // save the new current song
    std::ofstream outFile("savedMusic.txt");
    if(!outFile){
        std::cout << "Error saving file to savedMusic.txt" << std::endl;
        return;
    }

    // write the playlist path and the index of the song you were listening to
    outFile << playlists[PLAYLIST] << std::endl;
    outFile << SONG << std::endl;
    outFile.close();
}

// loads the previous songQueue a user was listening to
void loadMusic(){
    std::ifstream inFile("savedMusic.txt");
    if(!inFile){
        std::cout << "HAVE NOT SAVED MUSIC" << std::endl;
        return;
    }
    std::string info;
    getline(inFile, info);
    getSongPath(info);
    getline(inFile, info);
    SONG = std::stoi(info);
    nujabase = LoadSound(songQueue[SONG].c_str());
    std::cout << "LOADED MUSIC" << std::endl;
}

int main() {

    SetTargetFPS(60);
    InitWindow(512, 512, "Music Player");
    InitAudioDevice();
    Vector2 mc = { 0 };
    system("find ~/Music -name '*.xspf' > playlistFiles.txt");
  
    // find the playslists that a user has
    getPlaylists();
    // find all mp3s in chosen playlist
    loadMusic();

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

    while (!WindowShouldClose()) {
        // to move to next song if nothing is playing but the flag says so
        if(!IsSoundPlaying(nujabase) && PLAY){
            std::cout << "MOVED TO NEXT SONG" << std::endl;
            SONG++;
            // loop to beginning if we're at the last SONG
            if(SONG > (int)songQueue.size() - 1)
                SONG = 0;
            // play that next SONG
            nujabase = LoadSound(songQueue[SONG].c_str());
            PlaySound(nujabase);
            saveMusic();
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            mc = GetMousePosition();
            if(!songQueue.empty()){
                // toggling play/pause feature
                if (CheckCollisionPointRec(mc, play.pos) && !songQueue.empty()) {
                    std::cout << "PLAY/PAUSED PRESSED" << std::endl;
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
                    std::cout << "SKIPPED PRESSED" << std::endl;
                    // stop the current sound
                    StopSound(nujabase);
                    // move on to next song
                    SONG++;
                    // loop to beginning if we're at the last SONG
                    if(SONG > (int)songQueue.size() - 1)
                        SONG = 0;
                    // play that next song
                    nujabase = LoadSound(songQueue[SONG].c_str());
                    PlaySound(nujabase);
                    play.title = "PAUSE";
                    PLAY = true;
                    saveMusic();
                }

                // rewinding to last sound
                else if(CheckCollisionPointRec(mc, rewind.pos)){
                    std::cout << "REWIND PRESSED" << std::endl;
                    // stop the current sound
                    StopSound(nujabase);
                    // move on to next song
                    SONG--;
                    // loop to beginning if we're at the last SONG
                    if(SONG < 0)
                        SONG = (int)songQueue.size() - 1;
                    // play that next song
                    nujabase = LoadSound(songQueue[SONG].c_str());
                    PlaySound(nujabase);
                    play.title = "PAUSE";
                    PLAY = true;
                    saveMusic();
                }

            }
            else{
                std::cout << "CHOOSE A PLAYLIST" << std::endl; 
            }
                    
            // if the user pressed a playlist button, then update the song queue vector to have those songs
            for (int i = 0; i < (int)(buttons.size()); i++) {
                if (CheckCollisionPointRec(mc, buttons[i].pos) && !IsSoundPlaying(nujabase)) {
                    songQueue.clear();
                    getSongPath(playlists[i]);
                    SONG = 0;
                    nujabase = LoadSound(songQueue[SONG].c_str());
                    PlaySound(nujabase);
                    play.title = "PAUSE";
                    PLAY = true;
                    // TODO: DONT ALLOW USER TO CHOOSE PLAYLIST THAT THEY ARE ALREADY IN BC IT RESETS THEIR PROGRESS
                    PLAYLIST = i;
                    play.pc++;
                }
            }
        }

        BeginDrawing();

        // iterativley draw every playlist as a button to the side
        for(int i = 0; i < (int)playlists.size(); i++){
            BUTTON p;
            p.pos.height = 50;
            p.pos.width = 120;
            p.pos.x = play.pos.x - (play.pos.width * 1.500);
            p.pos.y = i * GetScreenHeight() * 0.200 + 10;
            p.color = GREEN;
            p.title = playlists[i].substr(playlists[i].find_last_of('/') + 1, playlists[i].find_last_of('.') - playlists[i].find_last_of('/') - 1);
            buttons.push_back(p);

            // draw the button for each playlist
            DrawRectangle(p.pos.x, p.pos.y, p.pos.width, p.pos.height, p.color);
            DrawText(p.title.c_str(), p.pos.x + (p.pos.width * 0.250) - 20, p.pos.y + (p.pos.height * 0.300), 20, BLACK);
        }

       // pause/play button
        DrawRectangle(play.pos.x, play.pos.y, play.pos.width, play.pos.height, play.color);
        DrawText(play.title.c_str(), play.pos.x + (play.pos.width * 0.250), play.pos.y + (play.pos.height * 0.300), 20, BLACK);
        
        // skip button
        DrawRectangle(skip.pos.x, skip.pos.y, skip.pos.width, skip.pos.height, skip.color);
        DrawText(skip.title.c_str(), skip.pos.x + (skip.pos.width * 0.250), skip.pos.y + (skip.pos.height * 0.300), 20, BLACK);

        // rewind button
        DrawRectangle(rewind.pos.x, rewind.pos.y, rewind.pos.width, rewind.pos.height, rewind.color);
        DrawText(rewind.title.c_str(), rewind.pos.x + (rewind.pos.width * 0.250), rewind.pos.y + (rewind.pos.height * 0.300), 20, BLACK);

        // benchmark testing
        DrawFPS(GetScreenWidth() * 0.780, GetScreenHeight() * 0.100);
        ClearBackground(BLACK);
        EndDrawing();
    }

    // prevent leak
    UnloadSound(nujabase);
    CloseWindow();
    return 0;
}

