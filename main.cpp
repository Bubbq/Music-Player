#include <fstream>
#include <raylib.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// used for traversing the entries in a single directory.
namespace fs = std::filesystem;

// list of filepaths of mp3s to be played
std::vector<std::string> songs;
std::vector<std::string> songNames;
// position of current song
int sn = 0;
   
// list of filepaths of directories in 'Music' folder of user's system
std::vector<std::string> playlists;
std::vector<std::string> playlistName;
// the position of current playlist 
int pn = 0;

// loads music from a given mp3 path
Sound sound;

// jpg filePath and texture obj used to render cover art
Image img;
Texture2D cover;

// replaces subtrings within strings in c++
std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace) {
   size_t pos = 0;
   while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
   }
   return subject;
}

// tranlastes any string to linux file format
std::string linuxFormat(std::string input){

    // change special characters
    input = ReplaceString(input, " ", "\\ ");
    input = ReplaceString(input, "(", "\\(");
    input = ReplaceString(input, ")", "\\)");

    return input;
}

// converts file paths to engish
std::string englishFormat(std::string input){
    size_t srt = input.find_last_of('/') + 1;
    size_t end = input.find_last_of('.');

    return input.substr(srt, end - srt);
}

// to load all playlists from a user (ONLY LOOKING FOR DIRECTORIES IN MUSIC FOLDER)
void loadPlaylists(){

    // TODO IMPLEMENT FOR ALTERNATE OS 
    std::string username = std::getenv("USER");

    // get the filepath of every directory and load into playlists vector
    for(const auto& entry : fs::directory_iterator("/home/" + username + "/Music")){
        if(entry.is_directory()){
        
            playlistName.push_back(englishFormat(entry.path().string()));
            playlists.push_back(entry.path());
            // std::cout << "PLAYLIST ADDED: " << entry.path() << std::endl;
        }
    }
}

// to load all mp3 filepaths in a given playlist
void loadSongs(std::string filePath){
    // iterate through all files in 'filePath'
    for(const auto& entry : fs::directory_iterator(filePath)){
        if(entry.path().extension() == ".mp3"){

            songNames.push_back(englishFormat(entry.path().string()));
            songs.push_back(entry.path());
            // std::cout << "SONG ADDED: " << entry.path() << std::endl;
        }
    }
}

// load the cover of the current song for viewing
void loadCover(){

    // check if img exists already
    std::ifstream inFile("./covers/" + songNames[sn] + ".jpg");

    if(!inFile){
        std::cerr << "NO COVER TO LOAD \n";
        return;
    }

    // if we have the img, load it and the approproate texture
    img = LoadImage(("./covers/" + songNames[sn] + ".jpg").c_str());
    cover = LoadTextureFromImage(img);
}

// saves the cover of teh current song for veiwing
void saveCover(){

    // TODO: FIX FOR MULTIPLE OS
    std::string fs = linuxFormat(songs[sn]);
    std::string jp = englishFormat(fs);

    // check if the img has been saved already
    std::ifstream inFile("./covers/" + songNames[sn] + ".jpg");

    if(!inFile){
        std::string command = "ffmpeg -i " + fs + " covers/" + jp + ".jpg";  
        std::system(command.c_str());
    }

    else 
        std::cerr << "FILE HAS ALREADY BEEN SAVED \n";
}

// saves the last music user played by writing to some file
void saveMusic(std::string playlistPath){

    std::ofstream outFile("savedMusic.txt");
    if(!outFile){
        std::cerr << "ERROR SAVING MUSIC" << std::endl;
        return;
    }

    outFile << playlistPath << std::endl;
    outFile << pn << std::endl;
    outFile << sn << std::endl;

    outFile.close();
}


// loads the last song and playlist a user was listening to
bool loadMusic(){

    std::ifstream inFile("./savedMusic.txt");
    if(!inFile){
        std::cerr << "NO MUSIC SAVED" << std::endl;
        return false;
    }

    std::string info;

    // get the last playlist path and load all mp3s 
    getline(inFile, info);
    loadSongs(info);

    getline(inFile, info);
    pn = std::stoi(info);

    getline(inFile, info);
    sn = std::stoi(info);
    
    // load the last sound to play
    sound = LoadSound(songs[sn].c_str());

    // load the song cover
    loadCover();
    
    return true;
    // std::cout << "LOADED MUSIC" << std::endl;
}

int main() {

    // disables the info upon loading 
    SetTraceLogLevel(LOG_ERROR);
    
    // init
    SetTargetFPS(60);
    InitWindow(512, 512, "Music Player");
    InitAudioDevice();

    // flag representing if we're supposed to be playing music or not
    bool play = false;

    // number of times user pressed play/pause, needed for either playing or resuming sound
    int ppc = 0;
        
    // find all playlists a user has
    loadPlaylists();

    // load music, if any
    loadMusic();

    // game loop
    while (!WindowShouldClose()) {

        // moving to the next song once current is finished
        if(!IsSoundPlaying(sound) && play){
            
            sn = (sn + 1 <= (int)songs.size() - 1) ? sn + 1 : 0;

            // load the new sound and play
            sound = LoadSound(songs[sn].c_str());
            PlaySound(sound);

            saveCover();
            loadCover();
        }
        
        // start rendering
        BeginDrawing();

        // draw song cover
        if(cover.id != 0)
            DrawTexture(cover, 0, 0, WHITE);
        
        // play/pause button clicked
        if(GuiButton((Rectangle){float(GetScreenWidth() / 6.000) + 125,float(GetScreenHeight() * 0.850),100,50}, play ? "PAUSE" : "PLAY") && !songs.empty()){
            if(ppc == 0)
                PlaySound(sound);
                    
            else if(!IsSoundPlaying(sound))
                ResumeSound(sound);
                    
            else 
                PauseSound(sound);

            ppc++;
            play = !play;
            // std::cout << "PLAY/PAUSE PRESSED" << std::endl;
        }

        // skip button pressed
        else if(GuiButton((Rectangle){float(GetScreenWidth() / 6.000) + 250,float(GetScreenHeight() * 0.850),100,50}, "SKIP") && !songs.empty()){
                    
            StopSound(sound);
            sn = (sn + 1 <= (int)songs.size() - 1) ? sn + 1 : 0;
            saveCover();
            sound = LoadSound(songs[sn].c_str());
            PlaySound(sound);
            play = true;
            loadCover();
            // std::cout << "SKIPPED" << std::endl;
        }

        // rewind
        else if(GuiButton((Rectangle){float(GetScreenWidth() / 6.000),float(GetScreenHeight() * 0.850),100,50}, "REWIND") && !songs.empty()){

            StopSound(sound);
            sn = (sn - 1 > 0) ? sn - 1 : songs.size() - 1;
            saveCover();
            sound = LoadSound(songs[sn].c_str());
            PlaySound(sound);
            play = true;
            loadCover();
            // std::cout << "REWIND" << std::endl;
        }
        
        // display every possible playlist
        for(int i = 0; i < (int)playlists.size(); i++){

            if(GuiButton((Rectangle){float(GetScreenWidth() / 20.000) , float(i * GetScreenHeight() * 0.2000) + 10,100, 50}, playlistName[i].c_str()) && pn != i){
    
                play = true;
                sn = 0;
                pn = i;
                ppc++;

                // load new songs and thier names in selected playlist
                songs.clear();
                songNames.clear();

                loadSongs(playlists[i]);
                StopSound(sound);

                // start at the first song of that playlist
                sound = LoadSound(songs[sn].c_str());
                PlaySound(sound);

                saveCover();
                loadCover();
                // std::cout << "NOW ON PLAYLIST: " << playlistName[pn] << std::endl;
            }
        }

        // display song name
        if(!songNames.empty())
            DrawText(songNames[sn].c_str(), GetScreenWidth() / 2.000 - (songNames[sn].length() * 6.250 / 2.000), GetScreenHeight() * 0.750, 15, WHITE);
        
        // Get current time
        std::time_t t = std::time(nullptr);
        std::tm* currentTime = std::localtime(&t);

        // Format time 
        std::stringstream timeString;
        timeString << std::put_time(currentTime, "%I:%M %p");

        // current time
        DrawText(timeString.str().c_str(), GetScreenWidth() * 0.500, 20, 15, RAYWHITE);

        // benchmark testing
        DrawFPS(GetScreenWidth() * 0.780, GetScreenHeight() * 0.100);
        ClearBackground(BLACK);

        // end rendering
        EndDrawing();
    }

    // prevent leak
    if(IsSoundPlaying(sound))
        UnloadSound(sound);
    UnloadImage(img);
    UnloadTexture(cover);
    CloseWindow();

    // only save to file if user was playing any music
    if(!songs.empty())
        saveMusic(playlists[pn]);

    return 0;
}
