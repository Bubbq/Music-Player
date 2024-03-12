#include <iostream>
#include <raylib.h>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include "constants.h"

namespace fs = std::filesystem;
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"


// converts file paths to engish
std::string englishFormat(std::string input){
    size_t srt = input.find_last_of('/') + 1;
    size_t end = input.find_last_of('.');

    return input.substr(srt, end - srt);
}

// to load all mp3 filepaths in a given playlist
void loadSongs(std::string filePath, std::vector<std::string>& songs){
    // iterate through all files in 'filePath'
    for(const auto& entry : fs::directory_iterator(filePath)){
        if(entry.path().extension() == ".mp3")
            songs.push_back(entry.path());
            // std::cout << "SONG ADDED: " << entry.path() << std::endl;
    }
}

// loads the last song and playlist a user was listening to
bool loadMusic(Music& music, std::vector<std::string>& songs, int& sn, int& pn){

    std::ifstream inFile("./savedMusic.txt");
    if(!inFile){
        std::cerr << "NO MUSIC SAVED" << std::endl;
        return false;
    }

    std::string info;

    // get the last playlist path and load all mp3s 
    getline(inFile, info);
    loadSongs(info, songs);

    getline(inFile, info);
    pn = std::stoi(info);

    getline(inFile, info);
    sn = std::stoi(info);
    
    // load the last sound to play
    music = LoadMusicStream(songs[sn].c_str());

    // load the song cover
    // loadCover(sound, img, cover, background, songs, sn);
    
    return true;
    // std::cout << "LOADED MUSIC" << std::endl;
}


// to load all playlists from a user (ONLY LOOKING FOR DIRECTORIES IN MUSIC FOLDER)
void loadPlaylists(std::vector<std::string>& playlists){

    // TODO IMPLEMENT FOR ALTERNATE OS 
    std::string username = std::getenv("USER");

    // get the filepath of every directory and load into playlists vector
    for(const auto& entry : fs::directory_iterator("/home/" + username + "/Music")){
        if(entry.is_directory())
            playlists.push_back(entry.path());
            // std::cout << "PLAYLIST ADDED: " << entry.path() << std::endl;
    }
}



// saves the last music user played by writing to some file
void saveMusic(std::string playlistPath, int& sn, int& pn){

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

// updates the current song by the position update 'pu'
void updateCurrentSong(Music& music, std::vector<std::string>& songs, bool& play, int& sn, int pu){
    
    // corrected value incase the current songs positioning is invalid
    int cv = pu > 0 ? 0 : int(songs.size() - 1);

    // update current song index
    sn = (sn + pu > int(songs.size() - 1)) || (sn + pu < 0) ? cv : sn + pu;

    // play the new sound
    // StopMusicStream(music);

    // only unload music if there's music to be played
    if(IsMusicReady(music))
        UnloadMusicStream(music);
    music = LoadMusicStream(songs[sn].c_str());
    PlayMusicStream(music);

    // save and load the new cover
    // saveCover(songs, sn);
    // loadCover(sound, img, cover, background, songs, sn);

    // set play flag to true to override pause, if any
    play = true;
}


int main() {
    InitWindow(WINDOW_SIZE,WINDOW_SIZE, "MP3 Music Example");
    InitAudioDevice();
    SetTargetFPS(30);
    // Load music from an MP3 file
    Music music;
    
    bool play = false;
    std::vector<std::string> songs;
    std::vector<std::string> playlists;
    int sn = 0;
    int pn = 0;

    loadPlaylists(playlists);
    loadMusic(music, songs, sn, pn);


    // Start music playback
    PlayMusicStream(music);

    while (!WindowShouldClose()) {
        if(play)
            UpdateMusicStream(music);
        BeginDrawing();

        ClearBackground(RAYWHITE);

        // REWIND BTTN
        if(GuiButton((Rectangle){BTTN_X,BTTN_Y,BTTN_WIDTH,BTTN_HEIGHT}, "REWIND") && !songs.empty()){

            updateCurrentSong(music, songs, play, sn, -1);
            // std::cout << "REWIND" << std::endl;
        }

        // SKIP BTTN
        if(GuiButton((Rectangle){float(BTTN_X + (BTTN_WIDTH * 2.000)), BTTN_Y, BTTN_WIDTH,BTTN_HEIGHT}, "SKIP") && !songs.empty()){

            updateCurrentSong(music, songs, play, sn, 1);
            // std::cout << "SKIPPED" << std::endl;
        }

        // PLAY PAUSE BTTN
        if(GuiButton((Rectangle){BTTN_X + BTTN_WIDTH,BTTN_Y,BTTN_WIDTH,BTTN_HEIGHT}, play ? "PAUSE" : "PLAY") && !songs.empty()){
            play = !play;
        
            if(!play)
                PauseMusicStream(music);
            else
                ResumeMusicStream(music);
        }

        // display every possible playlist
        for(int i = 0; i < (int)playlists.size(); i++){

            if(GuiButton((Rectangle){10 , float(i * WINDOW_SIZE * 0.125) + 10,BTTN_WIDTH * 0.750, BTTN_HEIGHT * 0.750}, englishFormat(playlists[i]).c_str()) && pn != i){
    
                // update the current playlist position
                pn = i;
                
                // load new songs in selected playlist
                songs.clear();
                loadSongs(playlists[i], songs);
   
                // update the current song & cover
                updateCurrentSong(music,songs,play, sn, -(sn));
            }
        }

        // Draw your game objects here

        EndDrawing();
    }

    // Stop and unload the music stream when done
    StopMusicStream(music);
    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
