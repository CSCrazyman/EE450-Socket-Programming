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

#define SERVERA_PORT 21143
#define SERVERB_PORT 22143
#define AWS_UDP 23143
#define CLIENT_PORT 24143
#define HOST "127.0.0.1"
#define BACKLOG 10   // how many pending connections queue will hold

using namespace std;

void showResultA(string &s);
void showResultB(string &s);

int main(int argc, char *argv[]) {

    /* --------------------------------------------------------------- */
    /* -------------------- INITIALIZES VARIABLES -------------------- */
    /* --------------------------------------------------------------- */
    int socTCP, socUDP; // receives data from client

    char mapID[1];
    char information[1000];
    char distances[2000];
    char results[8000];

    string idx;
    string filesize;

    struct sockaddr_in awsTCPaddr;
    struct sockaddr_in client_addr;
    struct sockaddr_in awsUDPaddr;
    struct sockaddr_in serverAaddr;
    struct sockaddr_in serverBaddr;

    socklen_t client_size;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    
    /* ---------------------------------------------------------------- */
    /* ---------------------- ESTABLISHES SOCKET ---------------------- */
    /* ---------------------------------------------------------------- */
    socTCP = socket(PF_INET, SOCK_STREAM, 0);
    if (socTCP == -1) {
        perror("AWS-socket");
        exit(-1);
    }

    memset(&awsTCPaddr, 0, sizeof awsTCPaddr);
    awsTCPaddr.sin_family = AF_INET;
    awsTCPaddr.sin_port = htons(CLIENT_PORT);
    inet_pton(AF_INET, HOST, &(awsTCPaddr.sin_addr));

    socUDP = socket(PF_INET, SOCK_DGRAM, 0);
    if (socUDP == -1) {
        perror("AWS-socket");
        exit(-1);
    }

    memset(&awsUDPaddr, 0, sizeof awsUDPaddr);
    awsUDPaddr.sin_family = AF_INET;
    awsUDPaddr.sin_port = htons(AWS_UDP);
    inet_pton(AF_INET, HOST, &(awsUDPaddr.sin_addr));

    memset(&serverAaddr, 0, sizeof serverAaddr);
    serverAaddr.sin_family = AF_INET;
    serverAaddr.sin_port = htons(SERVERA_PORT);
    inet_pton(AF_INET, HOST, &(serverAaddr.sin_addr));

    memset(&serverBaddr, 0, sizeof serverBaddr);
    serverBaddr.sin_family = AF_INET;
    serverBaddr.sin_port = htons(SERVERB_PORT);
    inet_pton(AF_INET, HOST, &(serverBaddr.sin_addr));

    /* ----------------------------------------------------------------- */
    /* -------------------- BINDS PORT AND LISTENS --------------------- */
    /* ----------------------------------------------------------------- */
    if (bind(socTCP, (struct sockaddr *)&awsTCPaddr, sizeof awsTCPaddr) == -1) {
        close(socTCP);
        perror("AWS-bind");
        exit(-1);
    }

    if (listen(socTCP, BACKLOG) == -1) {
        close(socTCP);
        perror("AWS-listen");
        exit(-1);
    }

    if(bind(socUDP, (struct sockaddr*)&awsUDPaddr, sizeof awsUDPaddr) == -1){
        close(socUDP);
        perror("AWS-bind");
        exit(-1);
    }

    printf("\nThe aws is up and running.\n");
    
    /* ----------------------------------------------------------------- */
    /* ---------------------------- STARTS ----------------------------- */
    /* ----------------------------------------------------------------- */
    while(1) {
        // accepts from client side
        client_size = sizeof client_addr;
        int client_accept = accept(socTCP, (struct sockaddr *)&client_addr, &client_size);
        if (client_accept == -1){
            close(socTCP);
            perror("AWS-client-accept");
            exit(-1);
        }

        // receives mapID
        if (recv(client_accept, mapID, 1, 0) == -1) {
            close(socTCP);
            perror("AWS-receive-mapID");
            exit(-1);
        }
        string map(mapID);

        // receives sourceIdx and fileSize
        if (recv(client_accept, information, 1000, 0) == -1) {
            close(socTCP);
            perror("AWS-receive-sourceIdx-fileSize");
            exit(-1);
        }
        string info(information);

        // extracts the source idx and file size
        for (int i = 0 ; i < info.length() ; i++) {
            if (info.at(i) == 'f') {
                idx = info.substr(0, i);
                filesize = info.substr(i + 1);
                break;
            }
        }

        printf("\nThe AWS has received map ID <%s>, start vertex <%s> and file size <%s> from the client using TCP over port <%d>\n", 
        map.c_str(), idx.c_str(), filesize.c_str(), CLIENT_PORT);

        // sends to server A
        if (sendto(socUDP, map.c_str(), sizeof map.c_str(), 0, (struct sockaddr *)&serverAaddr, sizeof serverAaddr) == -1) {
            close(socUDP);
            perror("AWS-send-map");
            exit(-1);
        }

        if (sendto(socUDP, idx.c_str(), 1000, 0, (struct sockaddr *)&serverAaddr, sizeof serverAaddr) == -1) {
           close(socUDP);
            perror("AWS-send-idx");
            exit(-1);
        }
        
        printf("\nThe AWS has sent map ID and starting vertex to server A using UDP over port <%d>\n", SERVERA_PORT);

        if (recvfrom(socUDP, distances, 2000, 0, (struct sockaddr *)&serverAaddr, &addrlen) == -1) {
            perror("AWS-receive-fromA");
            exit(-1);
        }
        string dists(distances);
        showResultA(dists);

        // sends to server B
        if (sendto(socUDP, filesize.c_str(), 1000, 0, (struct sockaddr *)&serverBaddr, sizeof serverBaddr) == -1){
            perror("AWS-send-toB");
            exit(-1);
        }

        if (sendto(socUDP, dists.c_str(), 2000, 0, (struct sockaddr *)&serverBaddr, sizeof serverBaddr) == -1){
            perror("AWS-send-toB");
            exit(-1);
        }
        printf("\nThe AWS has sent path length, propagation speed and transmission speed to server B using UDP over port <%d>\n", SERVERB_PORT);

        if (recvfrom(socUDP, results, 8000, 0, (struct sockaddr *)&serverBaddr, &addrlen) == -1) {
            perror("AWS-receive-fromB");
            exit(-1);
        }
        string res(results);
        showResultB(res);

        // Sends the result back to client
        if (send(client_accept, dists.c_str(), 2000, 0) == -1){
            perror("AWS-sendRes-toClient");
            exit(-1);
        }

        if (send(client_accept, res.c_str(), 8000, 0) == -1){
            perror("AWS-sendRes-toClient");
            exit(-1);
        }
        printf("\nThe AWS has sent calculated delay to client using TCP over port <%d>.\n", CLIENT_PORT);

        memset(mapID, 0, sizeof mapID);
        memset(information, 0, sizeof information);
        memset(distances, 0, sizeof distances);
        memset(results, 0, sizeof results);
    }

    return 0;
}

void showResultA(string &s) {
    string dest_len = "";
    for (int i = 0 ; i < s.length() ; i++) {
        if (s.at(i) == 'f') {
            dest_len = s.substr(i + 1);
            break;
        }
    }
    printf("\nThe AWS has received shortest path from server A: \n");
    printf("------------------------------------------------\n");
    printf("Destination\tMin Length\n");
    printf("------------------------------------------------\n");
    int begin = 0;
    for (int i = 0 ; i < dest_len.length() ; i++) {
        string sub_dest_len = "";
        if (dest_len.at(i) == 'f') {
            sub_dest_len = dest_len.substr(begin, i - begin);
            begin = i + 1;
            for (int j = 0 ; j < sub_dest_len.length() ; j++) {
                if (sub_dest_len.at(j) == 'd') 
                    printf("%s\t\t%s\n", 
                    sub_dest_len.substr(0, j).c_str(), sub_dest_len.substr(j + 1).c_str());
            }
        }
        else if (i == dest_len.length() - 1) {
            sub_dest_len = dest_len.substr(begin);
            for (int j = 0 ; j < sub_dest_len.length() ; j++) {
                if (sub_dest_len.at(j) == 'd') 
                    printf("%s\t\t%s\n", 
                    sub_dest_len.substr(0, j).c_str(), sub_dest_len.substr(j + 1).c_str());
            }
        }
    }
    printf("------------------------------------------------\n");
}

void showResultB(string &s) {
    size_t sign_f = s.find('f');
    double tt = atof(s.substr(1, sign_f).c_str());
    int dest = -1;
    double tp = 0.0, delay = 0.0;
    printf("\nThe AWS has received delays from server B: \n");
    printf("--------------------------------------------------------------------\n");
    printf("Destination\tTt\t\tTp\t\tDelay\n");
    printf("--------------------------------------------------------------------\n");
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
                    printf("%d\t\t%.2f\t\t%.2f\t\t%.2f\n", dest, tt, tp, delay);
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
                    printf("%d\t\t%.2f\t\t%.2f\t\t%.2f\n", dest, tt, tp, delay);
                }
            }
        }
    }   
    printf("--------------------------------------------------------------------\n");
}