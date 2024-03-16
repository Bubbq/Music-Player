#include <fstream>
#include <raylib.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
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


bool checkValidDir(std::string path) {
   if (fs::is_empty(path)) return false;
   else {
       for(const auto& entry : fs::directory_iterator(path)){
           if(entry.path().extension() == ".mp3")
               return true;
       }
   }
   return false;
}


// tranlastes strings to linux file format
std::string linuxFormat(std::string input){


   // change special characters
   input = ReplaceString(input, " ", "\\ ");
   input = ReplaceString(input, "(", "\\(");
   input = ReplaceString(input, ")", "\\)");
   input = ReplaceString(input, "&", "\\&");
   input = ReplaceString(input, "'", "\\'");
   input = ReplaceString(input, "`", "\\`");
   input = ReplaceString(input, "’", "\\’");
   input = ReplaceString(input, "\"", "\\\"");


   return input;
}


// converts file paths to readable form
std::string englishFormat(std::string input){
  
   size_t srt = input.find_last_of('/') + 1;
   size_t end = input.find_last_of('.');


   return input.substr(srt, end - srt);
}


// to load all playlists from a user (ONLY LOOKING FOR DIRECTORIES IN MUSIC FOLDER)
void loadPlaylists(std::vector<std::string>& playlists){


   // get the filepath of every directory and load into playlists vector
   for(const auto& entry : fs::directory_iterator("/home/" + USERNAME + "/Music")){
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
   std::sort(songs.begin(), songs.end());
}


// load the cover of the current song for viewing
void loadCover(Image& img, Texture2D& cover, Texture2D& background,  std::vector<std::string>& songs, int& sn){
  
   // unload the previous cover and background textures 
   UnloadTexture(cover);
   UnloadTexture(background);
  
   // check if img exists already
   std::ifstream inFile(COVERS_PATH + englishFormat(songs[sn]) + ".jpg");


   if(!inFile){
       std::cerr << "NO COVER TO LOAD \n";
       return;
   }


   img = LoadImage((COVERS_PATH + englishFormat(songs[sn]) + ".jpg").c_str());
   ImageResize(&img, IMG_WIDTH, IMG_HEIGHT);


   Image cp = ImageCopy(img);
   ImageResize(&cp, WINDOW_SIZE, WINDOW_SIZE);
   ImageBlurGaussian(&cp, 30);


   cover = LoadTextureFromImage(img);
   background = LoadTextureFromImage(cp);
  
   UnloadImage(cp);
}


// saves the cover of teh current song for veiwing
void saveCover(std::vector<std::string>& songs, int& sn){
      
   // check if 'covers' folder exists, if not, then create
   std::ifstream covers(COVERS_PATH);
   std::string command = "mkdir " + COVERS_PATH;
  
   if(!covers)
       system(command.c_str());


   // TODO: FIX FOR MULTIPLE OS
   std::string fs = linuxFormat(songs[sn]);


   // check if the img has been saved already
   std::ifstream inFile(COVERS_PATH + englishFormat(songs[sn]) + ".jpg");


   if(!inFile){
       std::string command = "ffmpeg -i " + fs + " -vf scale=250:-1 " + COVERS_PATH + englishFormat(fs) + ".jpg -loglevel error"; 
       std::system(command.c_str());
   }


   else
       std::cerr << "FILE HAS ALREADY BEEN SAVED \n";
}


// saves the last music user played by writing to the cache
void saveMusic(std::string playlistPath, int& sn, int& pn, int& cst, float& volume){


   std::ofstream outFile("/home/" + USERNAME + "/.cache/Music-Player-SRC/savedMusic.txt");
   if(!outFile){
       std::cerr << "ERROR SAVING MUSIC" << std::endl;
       return;
   }


   outFile << playlistPath << std::endl;
   outFile << pn << std::endl;
   outFile << sn << std::endl;
   outFile << cst << std::endl;
   outFile << volume << std::endl;


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
void drawProgressBar(Music& music, int& cst, int& cml, double& dur){


       // the main bar
       Rectangle musicC = (Rectangle){(WINDOW_SIZE * 0.500) - (IMG_WIDTH * 0.500), (WINDOW_SIZE * 0.775 + (BTTN_HEIGHT * 2.250) * 0.250), IMG_WIDTH, 12};
      
       // when user clicks on progress bar, move them to that point in music
       if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){


           Vector2 mc = GetMousePosition();


           if(CheckCollisionPointRec(mc, musicC)){


               // click width percentage
               double cwp = (mc.x - ((WINDOW_SIZE * 0.500) - (IMG_WIDTH * 0.500))) / IMG_WIDTH;


               // get the current song time at that percentage
               cst = cml * cwp;


               // move to that point in time in music
               SeekMusicStream(music, cst);


               music.looping = false;
           }
       }


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


       // current time
       DrawText(currTime.c_str(), ((WINDOW_SIZE * 0.500) - (IMG_WIDTH * 0.500)) - 35, (WINDOW_SIZE * 0.775 + (BTTN_HEIGHT * 2.250) * 0.250), 15, WHITE);
      
       // time left
       DrawText(remainTime.c_str(), ((WINDOW_SIZE * 0.500) + (IMG_WIDTH * 0.500)) + 10, (WINDOW_SIZE * 0.775 + (BTTN_HEIGHT * 2.250) * 0.250), 15, WHITE);


       // progress bar
       DrawRectangleRounded(musicC, 20, 10, WHITE);
      
       // bar of music completed
       DrawRectangleRounded((Rectangle){(WINDOW_SIZE * 0.500) - (IMG_WIDTH * 0.500), (WINDOW_SIZE * 0.775 + (BTTN_HEIGHT * 2.250) * 0.250), float(IMG_WIDTH * dur), 12}, 20, 10, (Color){160, 160, 160, 160});
}


// loads the last song and playlist a user was listening to
bool loadMusic(Music& music, Image& img, Texture2D& cover, Texture2D& background, std::vector<std::string>& songs, int& sn, int& pn, int& cst, float& volume){


   std::ifstream inFile("/home/" + USERNAME + "/.cache/Music-Player-SRC/savedMusic.txt");
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
  
   getline(inFile, info);
   volume = std::stof(info);
  
   // load the last sound to play
   music = LoadMusicStream(songs[sn].c_str());


   // move to last position
   SeekMusicStream(music, cst);


   // prevent looping
   music.looping = false;


   // set music volume
   SetMusicVolume(music, volume);


   // load the song cover
   loadCover(img, cover, background, songs, sn);


   std::cout << "LOADED MUSIC" << std::endl;


   return true;
}


int main() {


   // disables the info upon execution
   SetTraceLogLevel(LOG_ERROR);
  
   // init
   SetTargetFPS(60);
   InitWindow(WINDOW_SIZE, WINDOW_SIZE, "Music Player");
   InitAudioDevice();
  
   std::string comm = "mkdir /home/" + USERNAME + "/.cache/Music-Player-SRC";
   system(comm.c_str());
  
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


   // volume
   float volume = 1.0;


   // the percentage of music completed
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
   loadMusic(music, img, cover, background, songs, sn, pn, cst, volume);


   // game loop
   while (!WindowShouldClose()) {
      
       // start rendering
       BeginDrawing();


       // making cover and background image
       if(cover.id != 0 && background.id != 0){
           DrawTexture(background, 0, 0, RAYWHITE);
           DrawTexture(cover, (WINDOW_SIZE * 0.500)  - (IMG_WIDTH * 0.500), (WINDOW_SIZE * 0.465) - (IMG_HEIGHT * 0.500), RAYWHITE);
       }


   // VOL UP BTTN
   if (GuiButton((Rectangle){WINDOW_SIZE - BTTN_HEIGHT - 10, 10, BTTN_HEIGHT, BTTN_HEIGHT}, "+")) {
       if (volume < 1.0) {
           std::cout << "Volume up! " << volume << '\n';
           volume += 0.1;
               SetMusicVolume(music, volume);
       } else {
           std::cout << "Volume at maximum value " << volume << '\n';
       }


   }
   // VOL DOWN BTTN
   if (GuiButton((Rectangle){WINDOW_SIZE - BTTN_HEIGHT*2 - 15, 10, BTTN_HEIGHT, BTTN_HEIGHT}, "-")) {
       if (volume > 0.1) {
           std::cout << "Volume down! " << volume << '\n';
           volume -= 0.1;
               SetMusicVolume(music, volume);
       } else {
           std::cout << "Volume at minimum value " << volume << '\n';
       }
   }
      
       // REWIND BTTN
       if(GuiButton((Rectangle){BTTN_X,BTTN_Y,BTTN_WIDTH,BTTN_HEIGHT}, "REWIND") && !songs.empty()){
          
           if(cst > 3)
               SeekMusicStream(music, 0);
           else
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


           if(GuiButton((Rectangle){10 , float(i * WINDOW_SIZE * 0.075) + 10,BTTN_WIDTH * 0.750, BTTN_HEIGHT}, englishFormat(playlists[i]).c_str()) && pn != i){
       if (checkValidDir(playlists[i])) {
                   // update the current playlist position
                   pn = i;
                  
                   // increment so that pressing play wont reset the current song to beginning
                   ppc++;


                   // load new songs in selected playlist
                   songs.clear();
                   loadSongs(playlists[i], songs);
 
                   // update the current song & cover
                   updateCurrentSong(music, img, cover, background, songs, play, sn, -(sn));
           } else {
           std::cout << "Invalid directory " << playlists[i] << '\n';
           }
       }
       }
      
       if(IsMusicReady(music)){


           // moving onto the next song when current one finished
           if(!IsMusicStreamPlaying(music) && play)
               updateCurrentSong(music, img, cover, background, songs, play, sn, 1);
          
           if(play)
               UpdateMusicStream(music);


           // init the cml & cst
           cst = GetMusicTimePlayed(music);
           cml = GetMusicTimeLength(music);
          
           // show bar with current time
           drawProgressBar(music, cst, cml, dur);
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
  
   // only save to file if user played any music at all
   if(!songs.empty())
       saveMusic(playlists[pn], sn, pn, cst, volume);


   CloseWindow();
   CloseAudioDevice();


   return 0;
}



