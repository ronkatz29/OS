#include <iostream>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <fstream>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Constants
#define ERROR 1
#define FAILURE -1
#define NUM_ARG_NOT_EXTRA  3
#define BUFFER_SIZE  256
#define SERVER "server"
#define CLIENT "client"
#define SPACE " "
#define MAX_CLIENTS_TO_LISTEN 5

//Error massage constants
#define ERR_ARGUMENT "failure in the arguments"
#define ERR_WRITE "failure in the write function"
#define ERR_SOCKET_INIT_CLIENT "failure in the CalSocket function"
#define ERR_SOCKET_INIT_SERVER "failure in the Establish function"
#define ERR_REED "failure in the ReadData function"
#define ERR_SYSTEM "failure in the system function"


// typedef DECLARATION
using namespace std;
enum ProgramKind {
    SER, CLI, ERR
};
// ----- Helper Functions --------
/**
 *
 * @param problem - The string of the massage that need to be printed.
 */
void system_call_failure_printer(const std::string &problem) {
    std::cerr << "system error: " << problem << std::endl;
}

int Establish(unsigned short portNum) {
    char myName[BUFFER_SIZE + 1];
    int s;
    struct sockaddr_in sa{};
    struct hostent *hp;
    //hostnet initialization
    gethostname(myName, BUFFER_SIZE);
    hp = gethostbyname(myName);
    if (hp == nullptr)
        return(FAILURE);
    //sockaddrr_in initlization
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;
    /* this is our host address */
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
    /* this is our port number */
    sa.sin_port= htons(portNum);
    /* create socket */
    if ((s= socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return(-1);

    if (bind(s , (struct sockaddr *)&sa , sizeof(struct sockaddr_in)) < 0) {
        close(s);
        return(FAILURE);
    }

    listen(s, MAX_CLIENTS_TO_LISTEN); /* max # of queued connects */
    return(s);
}


int CalSocket(char *hostname, unsigned short portNum) {

    struct sockaddr_in sa{};
    struct hostent *hp;
    int s;

    if ((hp= gethostbyname (hostname)) == nullptr) {
        return(FAILURE);
    }

    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr , hp->h_addr ,hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portNum);
    if ((s = socket(hp->h_addrtype,SOCK_STREAM,0)) < 0) {
        return(FAILURE);
    }

    if (connect(s, (struct sockaddr *)&sa , sizeof(sa)) < 0) {
        close(s);
        return(FAILURE);
    }

    return(s);
}



void Client(int argc,  char *argv[], int portNum){
    string command;
    int numArgCom = argc - NUM_ARG_NOT_EXTRA;
    //Inserting the extra arguments to the array.
    for (int i = 0; i < numArgCom; ++i) {
        string curr = argv[NUM_ARG_NOT_EXTRA + i];
        if (i < numArgCom -1)
            command  += curr + SPACE;  //TODO see if the space in the end is not too much
        else
            command  += curr;

    } //TODO check if need to add 0 to the end

    char hostName[BUFFER_SIZE];
    gethostname(hostName, BUFFER_SIZE);
    int socket = CalSocket(hostName, portNum);
    if (socket == FAILURE){
        system_call_failure_printer(ERR_SOCKET_INIT_CLIENT);
        exit(ERROR);
    }

    if(write(socket, command.c_str(), BUFFER_SIZE) == FAILURE){
        system_call_failure_printer(ERR_WRITE);
        exit(ERROR);
    }
}

int get_connection(int s) {
    int t; /* socket of connection */

    if ((t = accept(s,NULL,NULL)) < 0)
        return FAILURE;
    return t;
}

int ReadData(int s, char *buf, int n) {
    int bCount = 0;       /* counts bytes read */
    int br = 0;               /* bytes read this pass */

    while (bCount < n) { /* loop until full buffer */
        br = read(s, buf, n - bCount);
        if (br > 0)  {
            bCount += br;
            buf += br;
        }
        if (br < 1) {
            return(FAILURE);
        }
    }
    return(bCount);
}
void Server(int portNum){

    char serverBuffer[BUFFER_SIZE];
    int socket = Establish(portNum);
    if (socket == FAILURE){
        system_call_failure_printer(ERR_SOCKET_INIT_SERVER);
        exit(ERROR);
    }

    while (true){
        int clientSocket;
        while ((clientSocket = get_connection(socket)) == FAILURE){
        }

        memset(serverBuffer, 0, BUFFER_SIZE);
        if (ReadData(clientSocket, serverBuffer, BUFFER_SIZE) == FAILURE){
            system_call_failure_printer(ERR_REED);
            exit(ERROR);
        }
        if (system(serverBuffer) != 0){
            system_call_failure_printer(ERR_SYSTEM);
            exit(ERROR);
        }
    }


}


ProgramKind ArgumentsChecker(int argc,  char *argv[]){
    if (argc < 3) return ERR;
    if (strcmp(argv[1], SERVER) ==0 ) return SER;
    if (strcmp(argv[1], CLIENT) == 0) return CLI;
    return ERR;
}


int main(int argc, char *argv[]) {
    ProgramKind programKind = ArgumentsChecker(argc, argv);

    if (programKind == ERR) {
        system_call_failure_printer(ERR_ARGUMENT);
        exit(1);
    }

    int portNum = (int) strtol(argv[2], nullptr, 10);

    switch (programKind) {
        case CLI:
            Client(argc, argv, portNum);
            break;

        case SER:
            Server(portNum);
            break;

        default:
            break;
    }
}
