#include <cstddef>
#include <cstdio>
#include <fstream>
#include <raylib.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "constants.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// used for traversing the entries in a single directory.
namespace fs = std::filesystem;

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
    input = ReplaceString(input, "&", "\\&");
    input = ReplaceString(input, "'", "\\'\\");

    return input;
}

// converts file paths to engish
std::string englishFormat(std::string input){
    size_t srt = input.find_last_of('/') + 1;
    size_t end = input.find_last_of('.');

    return input.substr(srt, end - srt);
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

// to load all mp3 filepaths in a given playlist
void loadSongs(std::string filePath, std::vector<std::string>& songs){
    // iterate through all files in 'filePath'
    for(const auto& entry : fs::directory_iterator(filePath)){
        if(entry.path().extension() == ".mp3")
            songs.push_back(entry.path());
            // std::cout << "SONG ADDED: " << entry.path() << std::endl;
    }
}

// load the cover of the current song for viewing
void loadCover(Image& img, Texture2D& cover, Texture2D& background,  std::vector<std::string>& songs, int& sn){
    
    UnloadTexture(cover);
    UnloadTexture(background);
    
    // check if img exists already
    std::ifstream inFile("./covers/" + englishFormat(songs[sn]) + ".jpg");

    if(!inFile){
        std::cerr << "NO COVER TO LOAD \n";
        return;
    }

    // if we have the img, load it and the approproate texture
    img = LoadImage(("./covers/" + englishFormat(songs[sn]) + ".jpg").c_str());
    ImageResize(&img, IMG_WIDTH, IMG_HEIGHT);

    Image cp = ImageCopy(img);
    ImageResize(&cp, WINDOW_SIZE, WINDOW_SIZE);
    ImageBlurGaussian(&cp, 75);

    cover = LoadTextureFromImage(img);
    background = LoadTextureFromImage(cp);
    UnloadImage(cp);
}

// saves the cover of teh current song for veiwing
void saveCover(std::vector<std::string>& songs, int& sn){
    
    // check if 'covers' folder exists, if not, then create
    std::ifstream covers("./covers");

    if(!covers)
        system("mkdir covers");

    // TODO: FIX FOR MULTIPLE OS
    std::string fs = linuxFormat(songs[sn]);

    // check if the img has been saved already
    std::ifstream inFile("./covers/" + englishFormat(songs[sn]) + ".jpg");

    if(!inFile){
        std::string command = "ffmpeg -i " + fs + " -vf scale=250:-1" + " covers/" + englishFormat(fs) + ".jpg -loglevel error";  
        std::system(command.c_str());
    }

    else 
        std::cerr << "FILE HAS ALREADY BEEN SAVED \n";
}

// saves the last music user played by writing to some file
void saveMusic(std::string playlistPath, int& sn, int& pn, int& cst){

    std::ofstream outFile("savedMusic.txt");
    if(!outFile){
        std::cerr << "ERROR SAVING MUSIC" << std::endl;
        return;
    }

    outFile << playlistPath << std::endl;
    outFile << pn << std::endl;
    outFile << sn << std::endl;
    outFile << cst << std::endl;

    outFile.close();
}

// updates the current song by the position update 'pu'
void updateCurrentSong(Music& music, Image& img, Texture2D& cover, Texture2D& background, std::vector<std::string>& songs, bool& play, int& sn, int pu){
    
    // corrected value incase the current songs positioning is invalid
    int cv = pu > 0 ? 0 : int(songs.size() - 1);

    // update current song index
    sn = (sn + pu > int(songs.size() - 1)) || (sn + pu < 0) ? cv : sn + pu;

    // play the new sound
    if(IsMusicReady(music))
        UnloadMusicStream(music);

    music = LoadMusicStream(songs[sn].c_str());
    music.looping = false;

    PlayMusicStream(music);

    // save and load the new cover
    saveCover(songs, sn);
    loadCover( img, cover, background, songs, sn);

    // set play flag to true to override pause, if any
    play = true;
}

// shows bar with current and remaining time displayed
void drawProgressBar(int& cst, int& cml, double& dur){
        // Convert the times to minutes and seconds

        // current min and sec
        int cm = cst / 60;
        int cs = cst % 60;

        // remaining min and sec
        int rm = (cml - cst) / 60;
        int rs = (cml - cst) % 60;

        std::string currTime = std::to_string(cm) + ":" + (cs < 10 ? "0" : "") + std::to_string(cs);
        std::string remainTime = std::to_string(rm) + ":" + (rs < 10 ? "0" : "") + std::to_string(rs);

        // get percentage complete of the songs
        dur = double(cst) / cml;
        
        DrawText(currTime.c_str(), ((WINDOW_SIZE * 0.500) - (IMG_WIDTH * 0.500)) - 35, (WINDOW_SIZE * 0.775 + (BTTN_HEIGHT * 2.250) * 0.250), 15, WHITE);
        
        DrawText(remainTime.c_str(), ((WINDOW_SIZE * 0.500) + (IMG_WIDTH * 0.500)) + 10, (WINDOW_SIZE * 0.775 + (BTTN_HEIGHT * 2.250) * 0.250), 15, WHITE);

        // progress bar
        DrawRectangleRounded((Rectangle){((WINDOW_SIZE * 0.500) - (IMG_WIDTH * 0.500)), (WINDOW_SIZE * 0.775 + (BTTN_HEIGHT * 2.250) * 0.250), IMG_WIDTH, 12}, 20, 10, WHITE);
        
        // bar of music completed
        DrawRectangleRounded((Rectangle){(WINDOW_SIZE * 0.500) - (IMG_WIDTH * 0.500), (WINDOW_SIZE * 0.775 + (BTTN_HEIGHT * 2.250) * 0.250), float(IMG_WIDTH * dur), 12}, 20, 10, (Color){160, 160, 160, 160});
}

// loads the last song and playlist a user was listening to
bool loadMusic(Music& music, Image& img, Texture2D& cover, Texture2D& background, std::vector<std::string>& songs, int& sn, int& pn, int& cst){

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

    getline(inFile, info);
    cst = std::stoi(info);
    
    // load the last sound to play
    music = LoadMusicStream(songs[sn].c_str());

    // move to last position 
    SeekMusicStream(music, cst);

    // prevent looping
    music.looping = false;

    // load the song cover
    loadCover(img, cover, background, songs, sn);

    return true;
    // std::cout << "LOADED MUSIC" << std::endl;
}

int main() {

    // disables the info upon execution 
    SetTraceLogLevel(LOG_ERROR);
    
    // init
    SetTargetFPS(120);
    InitWindow(WINDOW_SIZE, WINDOW_SIZE, "Music Player");
    InitAudioDevice();
    
    // list of filepaths of mp3s to be played
    std::vector<std::string> songs;

    // position of current song
    int sn = 0;

    // list of filepaths of directories in 'Music' folder of user's system
    std::vector<std::string> playlists;

    // the position of current playlist 
    int pn = -1;

    // flag representing if we're supposed to be playing music or not
    bool play = false;

    // number of times user pressed play/pause, needed for either playing or resuming sound
    int ppc = 0;

    // current song time
    int cst;

    // current music length
    int cml;

    // the percentage of completed music
    double dur;

    // loads music from a given mp3 path
    Music music;

    // jpg filePath and texture objs used to render cover and background art
    Image img;
    Texture2D cover;
    Texture2D background;

    // find all playlists a user has in their MUSIC folder
    loadPlaylists(playlists);

    // load music, if any
    loadMusic(music, img, cover, background, songs, sn, pn, cst);

    // game loop
    while (!WindowShouldClose()) {
        
        // start rendering
        BeginDrawing();

        // making cover and background image
        if(cover.id != 0 && background.id != 0){
            DrawTexture(background, 0, 0, RAYWHITE);
            DrawTexture(cover, (WINDOW_SIZE * 0.500)  - (IMG_WIDTH * 0.500), (WINDOW_SIZE * 0.465) - (IMG_HEIGHT * 0.500), RAYWHITE);
        }
        
        // REWIND BTTN
        if(GuiButton((Rectangle){BTTN_X,BTTN_Y,BTTN_WIDTH,BTTN_HEIGHT}, "REWIND") && !songs.empty()){

            updateCurrentSong(music, img, cover, background, songs, play, sn, -1);
            // std::cout << "REWIND" << std::endl;
        }
        
        // PLAY PAUSE BTTN
        if(GuiButton((Rectangle){(WINDOW_SIZE * 0.500) - (BTTN_WIDTH * 0.500),BTTN_Y,BTTN_WIDTH,BTTN_HEIGHT}, play ? "PAUSE" : "PLAY") && !songs.empty()){
            play = !play;
            if(ppc == 0)
                PlayMusicStream(music);
            if(!play)
                PauseMusicStream(music);
            else
                ResumeMusicStream(music);
        }

        // SKIP BTTN
        if(GuiButton((Rectangle){WINDOW_SIZE - (BTTN_X + BTTN_WIDTH), BTTN_Y, BTTN_WIDTH,BTTN_HEIGHT}, "SKIP") && !songs.empty()){

            updateCurrentSong(music, img, cover, background, songs, play, sn, 1);
            // std::cout << "SKIPPED" << std::endl;
        }
        
        // display every possible playlist
        for(int i = 0; i < (int)playlists.size(); i++){

            if(GuiButton((Rectangle){10 , float(i * WINDOW_SIZE * 0.075) + 5,BTTN_WIDTH * 0.750, BTTN_HEIGHT * 0.750}, englishFormat(playlists[i]).c_str()) && pn != i){
    
                // update the current playlist position
                pn = i;
                
                // increment so that pressing play wont reset the current song to beginning
                ppc++;

                // load new songs in selected playlist
                songs.clear();
                loadSongs(playlists[i], songs);
   
                // update the current song & cover
                updateCurrentSong(music, img, cover, background, songs, play, sn, -(sn));
            }
        }
        
        if(IsMusicReady(music)){

            if(!IsMusicStreamPlaying(music) && play)
                updateCurrentSong(music, img, cover, background, songs, play, sn, 1);
            
            if(play)
                UpdateMusicStream(music);

            // init the cml & cst
            cst = GetMusicTimePlayed(music);
            cml = GetMusicTimeLength(music);
            
            // show bar with current time
            drawProgressBar(cst, cml, dur);
        }

         // the current time
        std::time_t t = std::time(nullptr);
        std::tm* ct = std::localtime(&t);

        // time string
        std::stringstream ts;
        
        // day & month string
        std::stringstream dm;
        
        ts << std::put_time(ct, "%I:%M");
        dm << std::put_time(ct, "%A, %B %d");
        
        // Hour:Minute
        DrawText(ts.str().c_str(), WINDOW_SIZE * 0.500 - (MeasureText(ts.str().c_str(), 70) * 0.500), WINDOW_SIZE * 0.080, 70, RAYWHITE);
        
        // Day, Month #date
        DrawText(dm.str().c_str(), (WINDOW_SIZE * 0.500) - (MeasureText(dm.str().c_str(), 25) * 0.500), WINDOW_SIZE * 0.020, 25, WHITE);
        
        // display song name
        if(!songs.empty())
            DrawText(englishFormat(songs[sn]).c_str(), (WINDOW_SIZE * 0.500) - (MeasureText(englishFormat(songs[sn]).c_str(), 20) * 0.500), WINDOW_SIZE * 0.750, 20, WHITE);

        else
            DrawText("---", ((WINDOW_SIZE * 0.500) - (MeasureText("---", 20) * 0.500)), WINDOW_SIZE * 0.750, 20, WHITE);

        DrawFPS(WINDOW_SIZE * 0.825, WINDOW_SIZE * 0.050);

        ClearBackground(BLANK);

        // end rendering
        EndDrawing();
    }

    // prevent leak
    if(IsMusicReady(music)){
        UnloadMusicStream(music);
        UnloadImage(img);
    }
    if(cover.id!=0)
        UnloadTexture(cover);
    if(background.id != 0)
        UnloadTexture(background);
    
    CloseWindow();
    CloseAudioDevice();

    // only save to file if user played any music at all
    if(!songs.empty())
        saveMusic(playlists[pn], sn, pn, cst);

    return 0;
}
