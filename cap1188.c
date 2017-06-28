/*
 *	Aimed at the Gertboard, but it's fairly generic.
 *
 * Alireza Bahremand
 * Copyright (c) 2012-2013 Gordon Henderson. <projects@drogon.net>
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <stdint.h>
#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h> 

#define DEBUG 0                 // 1 true 0 false
#if DEBUG
# define pause() getchar()      // Pause execution sort of like a break point, getchar waits for enter press.
#else
# define pause()                // Else blank method.
#endif

#define WAIT 500
#define ERROR -1

/**
 * If you're using multiple sensors, 
 * or you just want to change the I2C address to something else, 
 * you can choose from 5 different options - 
 * 0x28, 0x29 (default), 0x2A, 0x2B, 0x2C and 0x2D
 *
 */
#define I2C_ADDRESS_1 0x28
 /******** CAP 1188 Registers *********/
// INPUTSTATUS is sensing 3v input touch.
#define CAP1188_INPUTSTATUS 0x3
// The sensor is by default at 0, we read for input and if we receive input
// we write a 1 using the MAIN_INT constant with a writeReg8
#define CAP1188_MAIN 0x00
#define CAP1188_MAIN_INT 0x01

// Pre-define method headers.
uint8_t touched();
void setup();

// Class attributes/variables.
int fileDescriptor1 = -1;
// Volatile because it toggles repeatedly from 0 to 1.
// We also use unit8_t to guarantee we are using minimal data size.
volatile uint8_t touchedCaps;
uint8_t i;



/**
 * Read inputs from bus address and determine if we recieved voltage input from human 
 * touch, if we have write a 1 to the addresses for the process polling that will 
 * be performed in the main method.
 */
uint8_t touched(int fileDescriptor) {
    // Read from address if we have received 3v input.
    touchedCaps = wiringPiI2CReadReg8(fileDescriptor,CAP1188_INPUTSTATUS);
    // If we have received touch.
    if (touchedCaps) {
        // Write the touch to the address specified by toggling it with the ~ operator for writing 1.
        // Write in fie descriptor at register with input the toggled output.
        wiringPiI2CWriteReg8(fileDescriptor,CAP1188_MAIN, wiringPiI2CReadReg8(fileDescriptor,CAP1188_MAIN) & ~CAP1188_MAIN_INT);
    }
    return touchedCaps;
}

/** Setting up Wiring Pi, and I2c communication */
void setup() {
    if (wiringPiSetup() == ERROR) {
        printf("Wiring pi set up failure\n");
        exit(1);
    }
    // Setup file descriptor to configure bus at starting address which is 0x28
    fileDescriptor1 = wiringPiI2CSetup(I2C_ADDRESS_1);
//    printf("fd - %d\n", fileDescriptor1);
    if (fileDescriptor1  == ERROR) {           // Check for error.
        printf("Could not find cap1188 num 1\n");
        exit(1);
    }
}


/**
 * The touchedCaps & (1 << i) is basically taking 1 and multiplying it by whatever
 * iterated i value we are in w/in for loop, and bitwise and's it with 
 * the touchedCaps value to determine we have valid touch based on read & writes from
 * touched method.
 * In summary, as the for loop iterates we are bitwise and'n it with a shifted index 
 * the for loop and conditionally &&'n it to determine if we have a match with the 
 * return value of the touched method call.
 */
int main() {
    setup();
    // Forks
    pid_t childPID;

    while (1) {
        if (touched(fileDescriptor1)) {
            i = 0;
            for (; i < 4; i++) {
                // If received touch and it matches touch from touchedCaps
                if ((touched(fileDescriptor1)) && touchedCaps & (1 << i)) {
                    childPID = fork();
                    if (!(childPID >= 0)) {
                        perror("Fork failure");
                        exit(ERROR);
                    }
                    // Forked child will die after executing system audio call.
                    if (i == 0 && childPID == 0) {
                        execl("/usr/bin/omxplayer", " ", "./sounds/OhhAhh.wav", NULL);
                    } else if (i == 1 && childPID == 0) {
                        execl("/usr/bin/omxplayer", " ", "./sounds/AhhOhh.wav", NULL);
                    } else if (i == 2 && childPID == 0) {
                        execl("/usr/bin/omxplayer", " ", "./sounds/highPitch.wav",NULL);
                    } else if (i == 3 && childPID == 0) {
                        execl("/usr/bin/omxplayer", " ", "./sounds/airHorn.wav",NULL);
                    }
                    // Delay input so multiple inputs aren't registered all at once.
                    delay(WAIT);
                    printf("%d touched, ", i);
//                    wait(NULL); // Wait for child process to join parent process.
//                    if (getpid() != getppid()) {
//                        kill(getpid(), SIGTERM);
//                    }
                }
                printf("\n");   // Print out touched buttons by issuing a flush.
            }
        }
    }
    return 0;
}
