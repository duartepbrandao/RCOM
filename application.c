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
      //set retransmitions
      //send
    }
    else{
      //receive
    }
  }
  return 0
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
