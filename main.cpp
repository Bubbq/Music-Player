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
void loadCover(Sound& sound, Image& img, Texture2D& cover, Texture2D& background,  std::vector<std::string>& songs, int& sn){
    
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
void updateCurrentSong(Sound& sound, Image& img, Texture2D& cover, Texture2D& background, std::vector<std::string>& songs, bool& play, int& sn, int pu){
    
    // corrected value incase the current songs positioning is invalid
    int cv = pu > 0 ? 0 : int(songs.size() - 1);

    // update current song index
    sn = (sn + pu > int(songs.size() - 1)) || (sn + pu < 0) ? cv : sn + pu;

    // play the new sound
    StopSound(sound);
    UnloadSound(sound);
    sound = LoadSound(songs[sn].c_str());
    PlaySound(sound);

    // save and load the new cover
    saveCover(songs, sn);
    loadCover(sound, img, cover, background, songs, sn);

    // set play flag to true to override pause, if any
    play = true;
}

// loads the last song and playlist a user was listening to
bool loadMusic(Sound& sound, Image& img, Texture2D& cover, Texture2D& background, std::vector<std::string>& songs, int& sn, int& pn){

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
    sound = LoadSound(songs[sn].c_str());

    // load the song cover
    loadCover(sound, img, cover, background, songs, sn);
    
    return true;
    // std::cout << "LOADED MUSIC" << std::endl;
}

int main() {

    // disables the info upon execution 
    SetTraceLogLevel(LOG_ERROR);
    
    // init
    SetTargetFPS(30);
    InitWindow(WINDOW_SIZE, WINDOW_SIZE, "Music Player");
    InitAudioDevice();
    
    // list of filepaths of mp3s to be played
    std::vector<std::string> songs;

    // position of current song
    int sn = 0;

    // list of filepaths of directories in 'Music' folder of user's system
    std::vector<std::string> playlists;

    // the position of current playlist 
    int pn = 0;

    // flag representing if we're supposed to be playing music or not
    bool play = false;

    // number of times user pressed play/pause, needed for either playing or resuming sound
    int ppc = 0;

    // loads music from a given mp3 path
    Sound sound;

    // jpg filePath and texture obj used to render cover art
    Image img;
    Texture2D cover;
    Texture2D background;

    // the current time
    std::time_t t = std::time(nullptr);
    std::tm* ct = std::localtime(&t);
        
    // find all playlists a user has
    loadPlaylists(playlists);

    // load music, if any
    loadMusic(sound, img, cover, background, songs, sn, pn);

    // game loop
    while (!WindowShouldClose()) {

        // start rendering
        BeginDrawing();

        // making cover and background image
        if(cover.id != 0){
            DrawTexture(background, 0, 0, RAYWHITE);
            DrawTexture(cover, (WINDOW_SIZE * 0.500)  - (IMG_WIDTH * 0.500), (WINDOW_SIZE * 0.450) - (IMG_HEIGHT * 0.500), RAYWHITE);
        }

        DrawRectangleRounded((Rectangle){(WINDOW_SIZE * 0.500) - (IMG_WIDTH * 0.500), (WINDOW_SIZE * 0.775 + (BTTN_HEIGHT * 2.250) * 0.250), IMG_WIDTH, 12}, 20, 10, WHITE);
        
        // moving to the next song once current is finished
        if(!IsSoundPlaying(sound) && play)
            updateCurrentSong(sound, img, cover, background, songs, play, sn, 1);

        // if (((Rectangle){10,10,10,10}, GuiIconText(ICON_ARROW_RIGHT_FILL, "Open Image"))) { /* ACTION */ }   
        
        // REWIND BTTN
        if(GuiButton((Rectangle){BTTN_X,BTTN_Y,BTTN_WIDTH,BTTN_HEIGHT}, "REWIND") && !songs.empty()){

            updateCurrentSong(sound, img, cover, background, songs, play, sn, -1);
            // std::cout << "REWIND" << std::endl;
        }
        
        // PLAY PAUSE BTTN
        if(GuiButton((Rectangle){BTTN_X + BTTN_WIDTH,BTTN_Y,BTTN_WIDTH,BTTN_HEIGHT}, play ? "PAUSE" : "PLAY") && !songs.empty()){
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

        // SKIP BTTN
        if(GuiButton((Rectangle){float(BTTN_X + (BTTN_WIDTH * 2.000)), BTTN_Y, BTTN_WIDTH,BTTN_HEIGHT}, "SKIP") && !songs.empty()){

            // update the current song & cover
            updateCurrentSong(sound, img, cover, background, songs, play, sn, 1);
            // std::cout << "SKIPPED" << std::endl;
        }
        
        // display every possible playlist
        for(int i = 0; i < (int)playlists.size(); i++){

            if(GuiButton((Rectangle){10 , float(i * WINDOW_SIZE * 0.125) + 10,BTTN_WIDTH * 0.750, BTTN_HEIGHT * 0.750}, englishFormat(playlists[i]).c_str()) && pn != i){
    
                // update the current playlist position
                pn = i;
                
                // increment so that pressing play wont reset the current song to beginning
                ppc++;

                // load new songs in selected playlist
                songs.clear();
                loadSongs(playlists[i], songs);
   
                // update the current song & cover
                updateCurrentSong(sound, img, cover, background, songs, play, sn, -(sn));
            }
        }
        
        // Format time 
        std::stringstream timeString;
        timeString << std::put_time(ct, "%I:%M");

        std::stringstream dayMonth;
        dayMonth << std::put_time(ct, "%A, %B %d");
        
        // display current time
        DrawText(timeString.str().c_str(), WINDOW_SIZE * 0.500 - (MeasureText(timeString.str().c_str(), 70) * 0.500), WINDOW_SIZE * 0.080, 70, RAYWHITE);
        DrawText(dayMonth.str().c_str(), (WINDOW_SIZE * 0.500) - (MeasureText(dayMonth.str().c_str(), 25) * 0.500), WINDOW_SIZE * 0.020, 25, WHITE);
        
        // display song name
        if(!songs.empty())
            DrawText(englishFormat(songs[sn]).c_str(), (WINDOW_SIZE * 0.500) - (MeasureText(englishFormat(songs[sn]).c_str(), 20) * 0.500), WINDOW_SIZE * 0.750, 20, WHITE);

        else
            DrawText("---", WINDOW_SIZE * 0.500, WINDOW_SIZE * 0.750, 20, WHITE);

        DrawFPS(WINDOW_SIZE * 0.825, WINDOW_SIZE * 0.050);

        ClearBackground(BLANK);

        // end rendering
        EndDrawing();
    }

    // prevent leak
    if(!IsSoundPlaying(sound))
        UnloadSound(sound);
    UnloadImage(img);
    UnloadTexture(cover);
    UnloadTexture(background);
    
    CloseWindow();

    // only save to file if user was playing any music
    if(!songs.empty())
        saveMusic(playlists[pn], sn, pn);

    return 0;
}
