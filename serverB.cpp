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

#define SERVERB_PORT 22143
#define AWS_UDP 23143
#define HOST "127.0.0.1"    // local host

using namespace std;

void readPaths(map<int, int> &mp, const size_t &sign, const string &str);
void computeDelay(map<int, int> &dists, map<int, double> &p_delays, const double &prop);

int main(int argc, char *argv[]) {

    /* --------------------------------------------------------------- */
    /* -------------------- INITIALIZES VARIABLES -------------------- */
    /* --------------------------------------------------------------- */
    int socUDP; // UDP socket
    double prop_speed, trans_speed;
    double tt, total_delay;
    long fs;
    char filesize[1000];
    char information[2000];

    map<int, int> dest_dist = map<int, int>();
    map<int, double> prop_delay = map<int, double>();
    map<int, int>::iterator iter_path;
    map<int, double>::iterator iter_delay;

    string buffer;

    socklen_t addrlen = sizeof(struct sockaddr_in);

    struct sockaddr_in serverBaddr;
    struct sockaddr_in awsUDPaddr;

    /* ---------------------------------------------------------------- */
    /* ---------------------- ESTABLISHES SOCKET ---------------------- */
    /* ---------------------------------------------------------------- */
    socUDP = socket(PF_INET, SOCK_DGRAM, 0);
    if (socUDP == -1) {
        perror("ServerB-socket");
        exit(-1);
    }
    
    memset(&serverBaddr, 0, sizeof serverBaddr);
    serverBaddr.sin_family = AF_INET;
    serverBaddr.sin_port = htons(SERVERB_PORT);
    inet_pton(AF_INET, HOST, &(serverBaddr.sin_addr));

    /* ----------------------------------------------------------------- */
    /* -------------------- BINDS PORT AND LISTENS --------------------- */
    /* ----------------------------------------------------------------- */
    if (bind(socUDP, (struct sockaddr *)&serverBaddr, sizeof serverBaddr) == -1) {
        perror("ServerB-bind");
        exit(-1);
    }

    printf("\nThe Server B is up and running using UDP on port %d\n", SERVERB_PORT);

    /* ----------------------------------------------------------------- */
    /* ---------------------------- STARTS ----------------------------- */
    /* ----------------------------------------------------------------- */
    while (1) {
        if (recvfrom(socUDP, filesize, 1000, 0, (struct sockaddr* ) &awsUDPaddr, &addrlen) == -1) {
            perror("ServerB-receive-filesize");
            exit(-1);
        }
        fs = atol(filesize);

        if (recvfrom(socUDP, information, 2000, 0, (struct sockaddr* ) &awsUDPaddr, &addrlen) == -1) {
            perror("ServerB-receive-data");
            exit(-1);
        }
        string info(information);
        size_t sign_t = info.find('t');
        size_t sign_f = info.find('f');
        prop_speed = atof(info.substr(1, sign_t).c_str());
        trans_speed = atof(info.substr(sign_t + 1, sign_f).c_str());
        tt = fs / (8.0 * trans_speed);
        buffer = "x" + to_string(tt);   // trans_time followed by 'x'

        readPaths(dest_dist, sign_f, info);
        printf("\nThe Server B has received data for calculation: \n");
        printf("* Propagation speed: <%.2f> km/s;\n", prop_speed);
        printf("* Transmission speed: <%.2f> Bytes/s;\n", trans_speed);
        for (iter_path = dest_dist.begin() ; iter_path != dest_dist.end() ; iter_path++) 
            printf("* Path length for destination <%d>: <%d> km;\n", iter_path->first, iter_path->second);

        computeDelay(dest_dist, prop_delay, prop_speed);
        printf("\nThe Server B has finished the calculation of the delays: \n");
        printf("------------------------------------------------\n");
        printf("Destination\tDelay\n");
        printf("------------------------------------------------\n");
        for (iter_delay = prop_delay.begin() ; iter_delay != prop_delay.end() ; iter_delay++) {
            buffer += "f" + to_string(iter_delay->first) + "y" + to_string(iter_delay->second); // y->prop
            total_delay = tt + iter_delay->second;
            printf("%d\t\t%.2f\n", iter_delay->first, total_delay);
        }
        printf("------------------------------------------------\n");

        // Sends the result back to AWS
        if (sendto(socUDP, buffer.c_str(), 8000, 0, (struct sockaddr *)&awsUDPaddr, sizeof awsUDPaddr) == -1) {
            perror("ServerB-send-failed");
            exit(-1);
        }
        printf("\nThe Server B has finished sending the output to AWS.\n");

        dest_dist.clear();
        prop_delay.clear();
        memset(information, 0, sizeof information);
        memset(filesize, 0, sizeof filesize);
    }

    return 0;
}

void readPaths(map<int, int> &mp, const size_t &sign, const string &str) {
    for (size_t i = sign + 1, begin = sign + 1; i < str.length() ; i++) {
        string sub_dest_len = "";
        if (str.at(i) == 'f') {
            sub_dest_len = str.substr(begin, i - begin);
            begin = i + 1;
            for (int j = 0 ; j < sub_dest_len.length() ; j++) {
                if (sub_dest_len.at(j) == 'd')
                    mp[atoi(sub_dest_len.substr(0, j).c_str())] = 
                        atoi(sub_dest_len.substr(j + 1).c_str());
            }
        }
        else if (i == str.length() - 1) {
            sub_dest_len = str.substr(begin);
            for (int j = 0 ; j < sub_dest_len.length() ; j++) {
                if (sub_dest_len.at(j) == 'd') 
                    mp[atoi(sub_dest_len.substr(0, j).c_str())] = 
                        atoi(sub_dest_len.substr(j + 1).c_str());
            }
        }
    }   
}

void computeDelay(map<int, int> &dists, map<int, double> &p_delays, const double &prop) {
    map<int, int>::iterator it;
    for (it = dists.begin() ; it != dists.end() ; it++)
        p_delays[it->first] = it->second / prop;
}