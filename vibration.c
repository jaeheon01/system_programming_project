#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "shared.h"

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define PIR_PIN 17
#define VALUE_MAX 40
#define DIRECTION_MAX 128


static int GPIOExport(int pin) {
#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;
	
	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(-1 == fd) {
		fprintf(stderr, "Failed to open for writing!\n");
		return -1;
	}
	
	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return 0;
}

static int GPIODirection (int pin, int dir){
	static const char s_directions_str[]="int\0out";
	char path[DIRECTION_MAX];
	int fd;
	
	snprintf(path,DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if(-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return -1;
	}
	if (-1 == 
		write(fd, &s_directions_str[IN == dir ? 0 : 3], IN ==dir ? 2 : 3)) {
			fprintf(stderr, "Failed to set direction!\n");
			return -1;
	}
	
	close(fd);
	return 0;
}

static int GPIORead(int pin){
	char path[DIRECTION_MAX];
	char value_str[3];
	int fd;
	
	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value",pin);
	fd = open(path, O_RDONLY);
	if(fd == -1){
		perror("Error: GPIO read");
		return -1;
	}
	
	read(fd, value_str,3);
	close(fd);
	
	return(value_str[0] == '1') ? HIGH : LOW;
}


static int GPIOWrite(int pin, int value){
	static const char s_values_str[] = "01";
	
	char path[VALUE_MAX];
	int fd;
	
	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path,O_WRONLY);
	if(-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return -1;
	}
	
	if(1 != write(fd, &s_values_str[LOW == value ? 0: 1],1)) {
		fprintf(stderr, "Failed to write value!\n");
		return -1;
	}
	close(fd);
	return 0;
}

static int GPIOUnexport(int pin){
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;
	
	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if(-1 == fd){
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return -1;
	}
	
	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return 0;
}

void* vibration_sensor(void* arg) {
	if (GPIOExport(PIR_PIN) == -1) printf("Error_Exprot");
	if (GPIODirection(PIR_PIN, IN) == -1) printf("Error_Direction");
	
	while(1) {
		pthread_mutex_lock(&lock);
		while(step!=1){
			pthread_cond_wait(&cond,&lock);
		}
		
		printf("[Vibration Sensor] Checking for vibration...\n");
		usleep(200 * 1000);
		if(GPIORead(PIR_PIN) == HIGH){
			vibration_detected = 1;
			printf("[Vibration Sensor] Vibration detected!\n");
		}else{
			vibration_detected = 0;
			printf("[Vibration Sensor] No Vibration detected\n");
		}
		
		step=2;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&lock);
	}
	
	if(GPIOUnexport(PIR_PIN) == -1){ 
		printf("Error_Unexport");
	}
}




