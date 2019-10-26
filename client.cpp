#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <fstream>
#include <map>
#include <fstream>
#include <sstream>

#define PORT 24143         // client port: dynamically
#define HOST "127.0.0.1"    // local host

using namespace std;

void showResult(string &s, map<int, int> &mp);
void processDist(string &s, map<int, int> &mp);

int main(int argc, char *argv[]) {

    /* --------------------------------------------------------------- */
    /* -------------------- INITIALIZES VARIABLES -------------------- */
    /* --------------------------------------------------------------- */
    int socTCP; // TCP socket: client -> aws
    int dataSend;
    int indicator = 0;

    char *mapID;
    char *sourceIdx;
    char *fileSize;
    char distances[2000];
    char results[8000];
    string buffer;

    map<int, int> mp = map<int, int>();

    struct sockaddr_in awsTCPaddr;
    struct sockaddr_in my_addr;
    socklen_t my_addr_size;

    stringstream strs_1, strs_2;

    /* ---------------------------------------------------------------- */
    /* ----------------------- PROCESSES INPUT ------------------------ */
    /* ---------------------------------------------------------------- */
    mapID = argv[1];
    strs_1 << atoi(argv[2]);
    sourceIdx = (char *)strs_1.str().c_str();
    strs_2 << atol(argv[3]);
    fileSize = (char *)strs_2.str().c_str();
    buffer = strs_1.str() + "f" + strs_2.str();

    /* ---------------------------------------------------------------- */
    /* ---------------------- ESTABLISHES SOCKET ---------------------- */
    /* ---------------------------------------------------------------- */
    socTCP = socket(PF_INET, SOCK_STREAM, 0);
    if (socTCP == -1) {
        perror("Client-socket");
        exit(-1);
    }

    memset(&awsTCPaddr, 0, sizeof awsTCPaddr);
    awsTCPaddr.sin_family = AF_INET;
    awsTCPaddr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST, &(awsTCPaddr.sin_addr));

    /* ----------------------------------------------------------------- */
    /* ---------------------- ESTABLISHES CONNECT ---------------------- */
    /* ----------------------------------------------------------------- */
    if (connect(socTCP, (struct sockaddr *)&awsTCPaddr, sizeof awsTCPaddr) == -1) {
        close(socTCP);
        perror("Client-connect");
        exit(-1);
    }
    printf("\nThe client is up and running.\n");

    my_addr_size = sizeof my_addr;
    if (getsockname(socTCP, (struct sockaddr *)&my_addr, &my_addr_size) == -1) {
        close(socTCP);
        perror("Client-getSocketName");
        exit(-1);
    }
    
    /* ----------------------------------------------------------------- */
    /* ----------------------- SENDS QUERY INFO ------------------------ */
    /* ----------------------------------------------------------------- */
    dataSend = send(socTCP, mapID, strlen(mapID), 0); // Send Map ID
    if (dataSend <= 0) {
        close(socTCP);
        perror("Client-mapID");
        exit(-1);
    }

    dataSend = send(socTCP, buffer.c_str(), 1000, 0); // Send Source Vertex Idx and File Size
    if (dataSend <= 0) {
        close(socTCP);
        perror("Client-sourceIdxAndFileSize");
        exit(-1);
    }
    printf("\nThe client has sent query to AWS using TCP over port <%d>: start vertex <%s>; map <%s>; file size <%s>\n", 
        ntohs(my_addr.sin_port), sourceIdx, mapID, fileSize);

    // Gets the result from AWS
    if (recv(socTCP, distances, 2000, 0) == -1) {
        close(socTCP);
        perror("Client-receive-results");
        exit(-1);
    }
    string dists(distances);

    if (recv(socTCP, results, 8000, 0) == -1) {
        close(socTCP);
        perror("Client-receive-results");
        exit(-1);
    }
    string res(results);

    processDist(dists, mp);
    showResult(res, mp);

    close(socTCP);
    return 0;
}

void showResult(string &s, map<int, int> &mp) {
    size_t sign_f = s.find('f');
    double tt = atof(s.substr(1, sign_f).c_str());
    int dest = -1;
    double tp = 0.0, delay = 0.0;
    printf("\nThe client has received results from AWS: \n");
    printf("----------------------------------------------------------------------------\n");
    printf("Destination\tMin Length\tTt\t\tTp\t\tDelay\n");
    printf("----------------------------------------------------------------------------\n");
    for (size_t i = sign_f + 1, begin = sign_f + 1; i < s.length() ; i++) {
        string sub_dest_len = "";
        if (s.at(i) == 'f') {
            sub_dest_len = s.substr(begin, i - begin);
            begin = i + 1;
            for (int j = 0 ; j < sub_dest_len.length() ; j++) {
                if (sub_dest_len.at(j) == 'y') {
                    dest = atoi(sub_dest_len.substr(0, j).c_str());
                    tp = atof(sub_dest_len.substr(j + 1).c_str());
                    delay = tp + tt;
                    printf("%d\t\t%d\t\t%.2f\t\t%.2f\t\t%.2f\n", dest, mp[dest], tt, tp, delay);
                }
            }
        }
        else if (i == s.length() - 1) {
            sub_dest_len = s.substr(begin);
            for (int j = 0 ; j < sub_dest_len.length() ; j++) {
                if (sub_dest_len.at(j) == 'y') {
                    dest = atoi(sub_dest_len.substr(0, j).c_str());
                    tp = atof(sub_dest_len.substr(j + 1).c_str());
                    delay = tp + tt;
                    printf("%d\t\t%d\t\t%.2f\t\t%.2f\t\t%.2f\n", dest, mp[dest], tt, tp, delay);
                }
            }
        }
    }   
    printf("----------------------------------------------------------------------------\n");
}

void processDist(string &s, map<int, int> &mp) {
    size_t sign_f = s.find('f');
    for (size_t i = sign_f + 1, begin = sign_f + 1; i < s.length() ; i++) {
        string sub_dest_len = "";
        if (s.at(i) == 'f') {
            sub_dest_len = s.substr(begin, i - begin);
            begin = i + 1;
            for (int j = 0 ; j < sub_dest_len.length() ; j++) {
                if (sub_dest_len.at(j) == 'd')
                    mp[atoi(sub_dest_len.substr(0, j).c_str())] = 
                        atoi(sub_dest_len.substr(j + 1).c_str());
            }
        }
        else if (i == s.length() - 1) {
            sub_dest_len = s.substr(begin);
            for (int j = 0 ; j < sub_dest_len.length() ; j++) {
                if (sub_dest_len.at(j) == 'd') 
                    mp[atoi(sub_dest_len.substr(0, j).c_str())] = 
                        atoi(sub_dest_len.substr(j + 1).c_str());
            }
        }
    }   
}

