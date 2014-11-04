#include "application.h"



app_layer_T settings;

int startApp(){
  start_settings();
  setup();
}


int setup(){
  //need to setup status(sender/receiver), port, timeout and retries

  settings.status = cliStatus();
  settings.port = cliPort();
  //TODO:baudrate
  while (settings.status != EXIT) {

    if(settings.status == SENDER){
      //set timeout
      //set retries
      //choose file
      //get file size
      //send
    }
    else{
      //receive
    }
  }
  return 0
}

int cliretries(){
  printf("Enter the number of tries: \n\n");
  int choice = -1;
  char temp[MAX_STRING_SIZE];

  fgets((char*) temp, MAX_STRING_SIZE, stdin);
  sscanf(temp, "%d", &choice);

  while ( choice < 0 ){
    printf("Tries must be positive!\n\n");
    fgets((char*) temp, MAX_STRING_SIZE, stdin);
    sscanf(temp, "%d", &choice);
  }

  return choice;
}

char* cliChooseFile(){
  printf("What is the path for the file?\n\n");

  unsigned char *path = malloc(PATH_MAX);
  memset(path,0,PATH_MAX);
  fgets((char *) path, PATH_MAX, stdin);
  path[strlen((char *) path) - 1] = '\0';
  printf("%s\n", path);

  while(validPath(path)){
    memset(path,0,PATH_MAX);
    fgets((char *) path, PATH_MAX, stdin);
    path[strlen((char *) path) - 1] = '\0';
    printf("Invalid path!");
  }

  return path;
}

int getFileSize(char* path){
  FILE* file = fopen(path,"rb");
  
  fseek(file, 0L, SEEK_END);
  int sz = ftell(fp);

  fseek(file, 0L, SEEK_SET);

  return sz;
}

int validPath (unsigned char * path){
  FILE* file = fopen(path,"rb");

  return !file;
}

int cliTimeout(){
  printf("Enter a timeout value to use(seconds): \n\n");
  int choice = -1;
  char temp[MAX_STRING_SIZE];

  fgets((char*) temp, MAX_STRING_SIZE, stdin);
  sscanf(temp, "%d", &choice);

  while ( choice < 0 ){
    printf("Time must be positive!\n\n");
    fgets((char*) temp, MAX_STRING_SIZE, stdin);
    sscanf(temp, "%d", &choice);
  }

  return choice;
}

int cliPort(){
  printf("Enter a Serial Port to use: \n\n");

  int choice = -1;
  char temp[MAX_STRING_SIZE];

  fgets((char*) temp, MAX_STRING_SIZE, stdin);
  sscanf(temp, "%d", &choice);

  while ( choice < 0 || choice > 4 ){
    printf("Invalid Port!\n\n");
    fgets((char*) temp, MAX_STRING_SIZE, stdin);
    sscanf(temp, "%d", &choice);
  }

  return choice;
}

int cliStatus(){
  printf("\n Select a role: \n");
  printf("1) Sender \n");
  printf("2) Receiver \n");
  printf("3) EXIT \n\n");

  int choice = -1;

  char temp[MAX_STRING_SIZE];

  fgets((char*) temp, MAX_STRING_SIZE, stdin);
  sscanf(temp, "%d", &choice);

  while (choice != SENDER && choice != RECEIVER && choice != EXIT) {
    prinf("\n NOT A VALIDE CHOICE! \n\n");
    fgets((char*) temp, MAX_STRING_SIZE, stdin);
    sscanf(temp, "%d", &choice);
  }

  return choice;
}

int start_settings(){
  settings.port = -1;
  settings.serialPortDescriptor = 0;
  settings.status = -1;
  settings.packetSize = 0;
  settings.fileSize= 0;
  settings.fileDescriptor = 0;
  settings.currentNum = 0;
  memset(settings.name,0,PATH_MAX);

  return 0;
}
