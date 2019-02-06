/*
CIS 3110 Operating Systems
This assignment creates a shell program
*/

//include statemennts
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

//defining bool for use of booleans
typedef enum { false, true } bool;

char ** getln(); // function that gets data from lex

//function prototypes
int gcdPair(int, int);
int getGCDorLCM(char **, int, bool);
int args(char **, int);
void signalHandler();

int main () {
    while (1) //loop to continuously get input from user
    {
        //Declare variables
        char ** uArgs; //user arguments
        char hostname[50], rootUser = '$';
        int uid, forkRes, nArgs = 1, outArg = 0, inArg = 0;
        struct passwd *userDetails;
        bool bg = false, notReady = false;

        //Get hostname, uid, username
        gethostname(hostname, 50);
        uid = getuid();
        userDetails = getpwuid(uid);

        //check if user is super user
        if (uid == 0) {
            rootUser = '#';
        }

        waitpid(-1, NULL, WNOHANG);
        //print prompt
        printf ("[%s@%s]%c ", userDetails->pw_name, hostname, rootUser);

        //Get user input, parsed via lex
        uArgs = getln();

        //Processing special cases (i.e. exit, redirected I/O, etc)
        if(!(uArgs[0])) { continue; } //checking if nothing is entered
        if(strcmp(uArgs[0], "exit") == 0) { exit(0); } //if exit was entered

        for(; nArgs < 100; nArgs++) {
          //printf("%d\n", nArgs);
            if(!(uArgs[nArgs])) {
                nArgs--;
                break;
            }
            //check in or out file arguments
            if(strcmp(uArgs[nArgs], ">") == 0) { //sees if argument is '>'
                if(!(uArgs[nArgs+1])) { //checks if filename is given
                    fprintf(stderr, "No output file defined\n");
                    notReady = true;
                } else {
                    outArg = nArgs + 1;
                    uArgs[nArgs] = NULL;
                }
            } else if(strcmp(uArgs[nArgs], "<") == 0) {
                if(!(uArgs[nArgs+1])) {
                    fprintf(stderr, "No input file defined\n");
                    notReady = true;
                } else {
                    inArg = nArgs + 1;
                    uArgs[nArgs] = NULL;
                }
            }
        }
        if (notReady) { continue; } //continue to next iteration if something went wrong

        //checking if to be run in background
        if(strcmp(uArgs[nArgs], "&") == 0) {
            bg = true;
            uArgs[nArgs] = NULL;
        }

        /* Launch executable */
        forkRes = fork ();
        if (bg) { sigset(forkRes, &signalHandler); }
        if(forkRes == 0) {
            bool failed = false;
            //open output/input files
            if((outArg) && (freopen(uArgs[outArg], "w", stdout) == NULL)) {
                perror(NULL);
                failed = true;
            } if((inArg) && (freopen(uArgs[inArg], "r", stdin) == NULL)) {
                perror(NULL);
                failed = true;
            }
            if (!failed) { //if successful
                if (strcmp(uArgs[0], "gcd") == 0) { //Greatest common divisor
                    exit(getGCDorLCM(&uArgs[1], inArg, false));
                } else if (strcmp(uArgs[0], "lcm") == 0) { //Greatest common divisor
                    exit(getGCDorLCM(&uArgs[1], inArg, true));
                } else if(strcmp(uArgs[0], "args") == 0) { //Arg counter
                    exit(args(&uArgs[1], inArg));
                } else {
                    int execRes = execvp(uArgs[0], uArgs);
                    //Should only reach here if error occurred
                    perror(NULL);
                    exit(execRes);
                }
            }
        } else {
            if (!bg) { //if not in background, wait for child
                wait(NULL);
            }
        } // end of if

    }

    return 0;
}


/*
Handles signals from child Processing
args: none
returns: none
*/
void signalHandler() {
  wait(NULL);
}

/*
Finds gcd of two numbers recursively
args: two integers
returns: Greatest common divisor
*/
int gcdPair(int one, int two) {
    if (one == 0) {
        return two;
    }
    return gcdPair(two % one, one);
}

/*
Gets GCD or LCM from array of strings
args: array of strings, input file, bool to select GCD/LCM
returns: error code, 0 if successful
*/
int getGCDorLCM(char ** gArgs, int inArg, bool isLCM){
    int min = -1, numArr[100], count=0;
    char cmd[4];
    bool valNum = true;

    if(inArg) { gArgs = getln(); } //gets from input if necessary
    if (isLCM) {
        strcpy(cmd, "lcm");
    } else {
        strcpy(cmd, "gcd");
    }
    //first parses arguments, stores in int array
    for(count = 0; count < 100; count++) {
        char * cNum = gArgs[count];
        int base = 10, index = 0;
        if (!cNum) {
            break;
        }
        if ((cNum[0] == '0') && ((cNum[1] == 'x') || cNum[1] == 'X')) { //checks if number is  hex
            base = 16;
            cNum = &cNum[2];
        }
        for (index=0; index < strlen(cNum); index++) {
            if ((!isdigit(cNum[index])) && ((base != 16) || (cNum[index]<'a')
                || (cNum[index]>'f'))) { //checks if number is valid hex/dec
                valNum = false;
                break;
            }
        }
        if (valNum) { //if number is valid, adds to array after conversion
            numArr[count] = strtol(cNum, NULL, base);
            if ((min == -1) || (min > numArr[count])) {
                min = numArr[count];
            }
        } else {
            fprintf(stderr, "Invalid number found\n");
            fprintf(stderr, "usage: %s number1 number2 ... numberN\n", cmd);
            break;
        }
    }
    if (valNum && (count < 2)) { // if no arguments given
        fprintf(stderr, "Too few arguments\n");
        fprintf(stderr, "usage: %s number1 number2 ... numberN\n", cmd);
    }
    if ((valNum) && (count >= 2)) {
        //finds GCD or LCM
        int fVal = numArr[0], index;
        printf("GCD (%d", fVal);
        for (index = 1; index < count; index++) {
            printf(", %d", numArr[index]);
            if (isLCM) { //finding LCM
                fVal = (numArr[index]*fVal) / (gcdPair(numArr[index], fVal));
            } else { //find GCD
                fVal = gcdPair(numArr[index], fVal);
            }
        }
        printf(") = %d\n", fVal);
    } else {
        exit(EXIT_FAILURE);
    }
    return 0;
}

/*
Gets number of args
args: array of strings, input file
returns: error code, 0 if successful
*/
int args(char ** gArgs, int inArg){
    //count is index, real count is value counted by function
    int count=0, realCount=0;
    char listArg[100][1000] = {};
    char  delim[3] = "";
    bool openBrack = false;

    if(inArg) { gArgs = getln(); }
    for(count = 0; count < 100; count++) {
        char * cArg = gArgs[count];
        if (!cArg) { break; }
        if ((openBrack) && (cArg[strlen(cArg)-1] == '"')) { //if a quote was opened and is closed by this  word
            openBrack = false;
            strcat(listArg[realCount], " ");
            strcat(listArg[realCount], gArgs[count]);
            realCount++;
        } else if (openBrack) { //if quote is still open
            strcat(listArg[realCount], " ");
            strcat(listArg[realCount], gArgs[count]);
        } else {
            if ((cArg[0] == '"') && (cArg[strlen(cArg)-1] != '"')) { //if quote is opened and not closed within same word
                openBrack = true;
                strcat(listArg[realCount], gArgs[count]);
            } else { //normal case
                strcat(listArg[realCount], gArgs[count]);
                realCount++;
            }
        }
    }
    printf("argc = %d, args = ", realCount);
    for (count = 0; count < realCount; count++) {
        printf("%s%s", delim, listArg[count]);
        strcpy(delim, ", ");
    }
    printf("\n");
    return 0;
}
