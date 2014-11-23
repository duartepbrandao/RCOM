#include "application.h"

app_layer_T settings;
int corrupted_data = 0;

int startApp() {
	start_settings();
	setup();
}

int setup() {
	//need to setup status(sender/receiver), port, timeout and retries

	settings.status = cliStatus();

	while (settings.status != EXIT) {
		settings.port = cliPort();
		setBR(cliBaudrate());
		if (settings.status == SENDER) {
			setTO(cliTimeout());
			setNT(cliRetries());
			//TODO:check this var
			unsigned char *path = cliChooseFile();
			settings.fileSize = getFileSize(path);

			settings.packetSize = cliPacket();
			settings.fileDescriptor = open(path, O_RDONLY);
			sendFile();
			free(path);
		} else {
			receiveFile();
		}
	}
	return 0;
}

int receiveFile() {
	printf("Waiting for connection!\n");
	settings.serialPortDescriptor = llopen(settings.port, settings.status);
	if (settings.serialPortDescriptor == -1) {
		printf("error opening connection! \n");
		return -1;
	}

	receiveControl(2);

	printf("Name: %s\n",settings.name);
	if ((settings.fileDescriptor = createFile(settings.name)) == -1) {
		printf("error creating file!\n");
		return -1;
	}

	int value = 0;
	int last_size = 0;
	unsigned char packet[PACKET_MAX_SIZE];

	while (TRUE) {
		value = llread(settings.serialPortDescriptor, packet);
		if (value == DISCONECTED) {
			if (verifyFile(packet, last_size)) {
				printf("Error receiving packets!\n");
			}
			break;
		} else {
			last_size = value;
			processPacket(packet);
		}
	}

	close(settings.fileDescriptor);

	if (corrupted_data || value == 1) {
		printf("Data corrupted, discarding!\n");
	}

	llclose(settings.serialPortDescriptor, settings.status);
	return 0;

}

int verifyFile(unsigned char * packet, unsigned int size) {
	unsigned int pos = 0;
	struct stat stat_file;

	unsigned int ctrlField = packet[pos++];

	if (ctrlField != 3) {
		return 1;
	}

	unsigned char fileName[MAX_STRING_SIZE] = { 0 };
	unsigned int fileSize = 0;

	while (pos < size) {
		switch (packet[pos]) {
		case 0:
			pos++;
			memcpy(&fileSize, &packet[pos + 1], packet[pos]);
			pos += packet[pos] + 1;

			if (settings.fileSize != fileSize) {
				return 1;
			}

			if (fstat(settings.fileDescriptor, &stat_file) == -1) {
				return 1;
			}

			if (stat_file.st_size != fileSize) {
				return 1;
			}

			break;

		case 1:

			pos++;
			memcpy(&fileName, &packet[pos + 1], packet[pos]);
			pos += packet[pos] + 1;

			if (strcmp((char*) fileName, (char *) settings.name)
					!= 0) {
				return 1;
			}
			break;
		}
	}
	return 0;
}

int processPacket(unsigned char * packet) {
	unsigned int pos = 0;
	unsigned int number = 0;
	unsigned int temp = 0;
	unsigned int size = 0;
	unsigned char * info;

	unsigned int ctrlField = packet[pos++];
	if (ctrlField == 1) {

		number = packet[pos++];
		temp = number - settings.currentNum;
		if (temp == -254) {
			temp = 1;
		}
		if (temp == 1) {
			size = packet[pos++]*256;
			size += packet[pos++];

			info = malloc(size + 1);
			memset(info, 0, size + 1);
			memcpy(info, &packet[pos], size);
			
			if (writeToFile(info, size) == -1) {
				printf("Error writing to file\n");
				return -1;
			}
			settings.currentNum = number;

			free(info);
		}
	}/* else {
		corrupted_data = -1;
	}*/

	return 0;
}

int writeToFile(unsigned char * info, unsigned int byn_number) {
	if (write(settings.fileDescriptor, info, byn_number)
			!= byn_number) {
		printf("error in write! \n");
		return -1;
	}
	return byn_number;
}

int createFile(char * name) {
	int fd = open((char *) name, O_CREAT | O_EXCL | O_WRONLY,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

	return fd;
}

int receiveControl(int var) {
	unsigned char packet[PACKET_MAX_SIZE] = { 0 };
	int size = 0;
	int pos = 0;

	size = llread(settings.serialPortDescriptor, packet);
	if (size == -1) {
		return -1;
	}

	if (packet[pos] != var) {
		printf("Error in receive control!\n");
		return -1;
	}

	pos++;

	while (pos < size) {
		switch (packet[pos]) {
		case 0:
			pos++;
			memcpy(&settings.fileSize, &packet[pos + 1], packet[pos]
			);
			pos += sizeof(unsigned int) + 1;
			printf("Filesize %d\n",settings.fileSize);
			break;
		case 1:
			pos++;
			memcpy(&settings.name, &packet[pos + 1],
					packet[pos]);
			pos += strlen((char *) settings.name) + 2;

			printf("Filename %s\n",settings.name);
			break;
		}
	}
	return 0;
}

int sendFile() {
	settings.serialPortDescriptor = llopen(settings.port,
			settings.status);
	if (settings.serialPortDescriptor == -1) {
		perror("error opening link!!\n\n");
		close(settings.fileDescriptor);
		return -1;
	}

	if (sendControl(2) == -1) {
		printf("Error in control packet\n");
		close(settings.fileDescriptor);
		return -1;
	}
	printf("control returned!\n");
	int stop = 0;
	unsigned char* packet = malloc(settings.packetSize);
	unsigned int size = 0;

	while (!stop) {
		printf("read!\n");
		size = read(settings.fileDescriptor, packet,
				settings.packetSize);
		if (size != settings.packetSize) {
			stop = -1;
		}
	printf("PACKET SIZE :%d\n",size);
		if (sendPacket(packet, size) == -1) {
			printf("Error transmiting packet \n");
			close(settings.fileDescriptor);
			return -1;
		}
	}

	if (sendControl(3) == -1) {
		printf("Error closing link\n");
		close(settings.fileDescriptor);
		return -1;
	}

	llclose(settings.serialPortDescriptor, settings.status);
	return 0;
}

int sendPacket(unsigned char * message, unsigned int size) {
	unsigned char packet[PACKET_MAX_SIZE];
	unsigned int pos = 0;

	packet[pos++] = 1;
	settings.currentNum = (settings.currentNum % 255) + 1;
	packet[pos++] = settings.currentNum;

	packet[pos++] = size/256;
	packet[pos++] = size%256;
	memcpy(&packet[pos], message, size);
	return llwrite(settings.serialPortDescriptor, packet,
			(size + 4));
}

int sendControl(int var) {
	unsigned char packet[PACKET_MAX_SIZE] = { 0 };
	unsigned int size = 0;
	packet[0] = var;
	size++;

	uint8_t fileSize = 0;
	packet[size++] = fileSize;
	uint8_t length = sizeof(unsigned int);
	packet[size++] = length;
	memcpy(&packet[size], &settings.fileSize,
			sizeof(unsigned int));
	size += sizeof(unsigned int);

	uint8_t fileName = 1;
	packet[size++] = fileName;
	length = strlen((char *) settings.name) + 1;
	packet[size++] = length;
	memcpy(&packet[size], &settings.name,
			strlen((char *) settings.name) + 1);
	size += strlen((char *) settings.name) + 1;
	printf("filename: %s\n",settings.name);
	return llwrite(settings.serialPortDescriptor, packet, size);

}

int cliBaudrate() {

	unsigned int choice = 0;
	int baud[] = { B110, B300, B600, B1200, B2400, B4800, B9600};
	printf("Select the baudrate: \n");
	printf("1) B110 \n");
	printf("2) B300 \n");
	printf("3) B600 \n");
	printf("4) B1200 \n");
	printf("5) B2400 \n");
	printf("6) B4800 \n");
	printf("7) B9600 \n");

	char temp[MAX_STRING_SIZE];
	fgets((char *) temp, MAX_STRING_SIZE, stdin);
	sscanf(temp, "%d", &choice);

	while (choice < 1 || choice > 7) {
		printf("Incorrect option\n");
		fgets((char *) temp, MAX_STRING_SIZE, stdin);
		sscanf(temp, "%d", &choice);
	}
	return baud[choice - 1];
}

int cliPacket() {
	printf("Enter the size of the data packets: \n\n");
	int choice = -1;
	char temp[MAX_STRING_SIZE];

	fgets((char*) temp, MAX_STRING_SIZE, stdin);
	sscanf(temp, "%d", &choice);

	while (choice < 1 || choice > PACKET_MAX_SIZE
			|| choice > (settings.fileSize) * pow(2, 8)) {
		printf("Invalid Size!\n\n");
		fgets((char*) temp, MAX_STRING_SIZE, stdin);
		sscanf(temp, "%d", &choice);
	}

	return choice;

}

int cliRetries() {
	printf("Enter the number of tries: \n\n");
	int choice = -1;
	char temp[MAX_STRING_SIZE];

	fgets((char*) temp, MAX_STRING_SIZE, stdin);
	sscanf(temp, "%d", &choice);

	while (choice < 0) {
		printf("Tries must be positive!\n\n");
		fgets((char*) temp, MAX_STRING_SIZE, stdin);
		sscanf(temp, "%d", &choice);
	}

	return choice;
}

char* cliChooseFile() {
	printf("What is the path for the file?\n\n");

	unsigned char *path = malloc(PATH_MAX);
	memset(path, 0, PATH_MAX);
	fgets((char *) path, PATH_MAX, stdin);
	path[strlen((char *) path) - 1] = '\0';
	printf("%s\n", path);
	while (validPath(path)) {
		memset(path, 0, PATH_MAX);
		fgets((char *) path, PATH_MAX, stdin);
		path[strlen((char *) path) - 1] = '\0';
		printf("Invalid path!");
	}
struct stat file_stat;
	stat((char ) path, &file_stat);
		strcpy((char *) settings.name, basename((char *) path));
	printf("path is valid\n");
	printf("filename %s", settings.name);
	return path;
}

int getFileSize(char* path) {
	FILE* file = fopen(path, "rb");

	fseek(file, 0L, SEEK_END);
	int sz = ftell(file);

	fseek(file, 0L, SEEK_SET);

	return sz;
}

int validPath(unsigned char * path) {
	FILE* file = fopen(path, "rb");

	return !file;
}

int cliTimeout() {
	printf("Enter a timeout value to use(seconds): \n\n");
	int choice = -1;
	char temp[MAX_STRING_SIZE];

	fgets((char*) temp, MAX_STRING_SIZE, stdin);
	sscanf(temp, "%d", &choice);

	while (choice < 0) {
		printf("Time must be positive!\n\n");
		fgets((char*) temp, MAX_STRING_SIZE, stdin);
		sscanf(temp, "%d", &choice);
	}

	return choice;
}

int cliPort() {
	printf("Enter a Serial Port to use: \n\n");

	int choice = -1;
	char temp[MAX_STRING_SIZE];

	fgets((char*) temp, MAX_STRING_SIZE, stdin);
	sscanf(temp, "%d", &choice);

	while (choice < 0 || choice > 4) {
		printf("Invalid Port!\n\n");
		fgets((char*) temp, MAX_STRING_SIZE, stdin);
		sscanf(temp, "%d", &choice);
	}

	return choice;
}

int cliStatus() {
	printf("\n Select a role: \n");
	printf("1) Sender \n");
	printf("2) Receiver \n");
	printf("3) EXIT \n\n");

	int choice = -1;

	char temp[MAX_STRING_SIZE];

	fgets((char*) temp, MAX_STRING_SIZE, stdin);
	sscanf(temp, "%d", &choice);

	while (choice != SENDER && choice != RECEIVER
			&& choice != EXIT) {
		printf("\n NOT A VALIDE CHOICE! \n\n");
		fgets((char*) temp, MAX_STRING_SIZE, stdin);
		sscanf(temp, "%d", &choice);
	}

	return choice;
}

int start_settings() {
	settings.port = -1;
	settings.serialPortDescriptor = 0;
	settings.status = -1;
	settings.fileSize = 0;
	settings.packetSize = 0;
	settings.fileDescriptor = 0;
	settings.currentNum = 0;
	memset(settings.name, 0, PATH_MAX);

	return 0;
}
