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
#include "serverA.h"

#define SERVERA_PORT 21143
#define AWS_UDP 23143
#define HOST "127.0.0.1"

using namespace std;

void readMaps(map<char, Graph<int> > &mp, vector<char> &list, const string &filename);

int main(int argc, char *argv[]) {
  
    /* --------------------------------------------------------------- */
    /* -------------------- INITIALIZES VARIABLES -------------------- */
    /* --------------------------------------------------------------- */
    map<char, Graph<int> > mp = map<char, Graph<int> >();
    map<int, int> distances = map<int, int>();
    vector<char> list = vector<char>();

    int socUDP; // UDP socket
    char mapID[1];
    char sourceIdx[1000];

    string filename = "map.txt";
    string buffer;

    socklen_t addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in serverAaddr;
    struct sockaddr_in awsUDPaddr;

    /* ---------------------------------------------------------------- */
    /* ---------------------- ESTABLISHES SOCKET ---------------------- */
    /* ---------------------------------------------------------------- */
    socUDP = socket(PF_INET, SOCK_DGRAM, 0);
    if (socUDP == -1) {
        perror("ServerA-socket");
        exit(-1);
    }
    
    memset(&serverAaddr, 0, sizeof serverAaddr);
    serverAaddr.sin_family = AF_INET;
    serverAaddr.sin_port = htons(SERVERA_PORT);
    inet_pton(AF_INET, HOST, &(serverAaddr.sin_addr));

    /* ----------------------------------------------------------------- */
    /* -------------------- BINDS PORT AND LISTENS --------------------- */
    /* ----------------------------------------------------------------- */
    if (bind(socUDP, (struct sockaddr *)&serverAaddr, sizeof serverAaddr) == -1) {
        perror("ServerA-bind");
        exit(-1);
    }

    printf("\nThe Server A is up and running using UDP on port <%d>\n", SERVERA_PORT);
    
    /* ----------------------------------------------------------------- */
    /* ---------------------- READS THE MAP FILE ----------------------- */
    /* ----------------------------------------------------------------- */
    readMaps(mp, list, filename);
    printf("\nThe Server A has constructed a list of <%d> maps:\n", (int)list.size());
    printf("------------------------------------------------\n");
    printf("Map ID\tNum Vertices\tNum Edges\n");
    printf("------------------------------------------------\n");
    for (int i = 0 ; i < list.size() ; i++) 
        printf("%c\t%d\t\t%d\n", list[i], mp[list[i]].AN(), mp[list[i]].E());
    printf("------------------------------------------------\n");

    /* ----------------------------------------------------------------- */
    /* ---------------------------- STARTS ----------------------------- */
    /* ----------------------------------------------------------------- */
    while (1) {

        if (recvfrom(socUDP, mapID, sizeof mapID, 0, (struct sockaddr* ) &awsUDPaddr, &addrlen) == -1) {
            perror("ServerA-receive-map");
            exit(-1);
        }
        string map(mapID);

        if (recvfrom(socUDP, sourceIdx, sizeof sourceIdx, 0, (struct sockaddr* ) &awsUDPaddr, &addrlen) == -1) {
            perror("ServerA-receive-index");
            exit(-1);
        }
        string idx(sourceIdx);

        printf("\nThe Server A has received input for finding shortest paths: starting vertex %s of map %s\n", 
            idx.c_str(), map.c_str());

        int totalNum = 0;
        buffer = "";
        for (int i = 0 ; i < list.size() ; i++) {
            if (list[i] == mapID[0]) {
                Dijkstra<Graph<int>, int> dij(mp[list[i]], atoi(sourceIdx));
                buffer += "p" + to_string(mp[list[i]].prop()) + "t" + to_string(mp[list[i]].trans());
                totalNum = mp[list[i]].V();
                for (int j = 0 ; j < totalNum ; j++) {
                    if (dij.hasPathTo(j)) distances[j] = dij.shortestPathTo(j);
                    else distances[j] = 0;
                }
            }
        }

        printf("\nThe Server A has identified the following shortest paths:\n");
        printf("------------------------------------------------\n");
        printf("Destination\tMin Length\n");
        printf("------------------------------------------------\n");
        for (int i = 0 ; i < totalNum ; i++) {
            if (distances[i] != 0) {
                printf("%d\t\t%d\n", i, distances[i]);
                buffer += "f" + to_string(i) + "d" + to_string(distances[i]);
            }
        }
        printf("------------------------------------------------\n");

        // Sends the result back to AWS
        if (sendto(socUDP, buffer.c_str(), 2000, 0, (struct sockaddr *)&awsUDPaddr, sizeof awsUDPaddr) == -1) {
            perror("ServerA-send-failed");
            exit(-1);
        } 
        printf("\nThe Server A has sent shortest paths to AWS.\n");

        distances.clear();
        memset(sourceIdx, 0, sizeof sourceIdx);
        memset(mapID, 0, sizeof mapID);
    }

    return 0;
}

void readMaps(map<char, Graph<int> > &mp, vector<char> &list, const string &filename) {
    ifstream file(filename);
    string line;
    int v, count = 0;
    double trans, prop;
    bool has = false;
    char first_letter;
    Graph<int> g = Graph<int>();

    assert(file.is_open());

    while (getline(file, line)) {
        char temp = line.c_str()[0];
        if (temp >= 'A' && temp <= 'Z') {
            list.push_back(temp);
            if (has) {
                g.setV(v + 1);
                mp.insert(make_pair(first_letter, g));
            }
            first_letter = temp;
            has = true;
            count++;
        }
        else if (count == 1) {
            stringstream ss(line);
            ss >> prop;
            g.setProp(prop);
            count++;
        }
        else if (count == 2) {
            stringstream ss(line);
            ss >> trans;
            g.setTrans(trans);
            count = 0;
        }
        else {
            stringstream ss(line);
            int a, b, dist;
            ss >> a >> b >> dist;
            v = max(a, b);
        }
    }

    g.setV(v + 1);
    mp.insert(make_pair(first_letter, g));

    file.clear();
    file.seekg(0, ios::beg);

    while (getline(file, line)) {
        char temp = line.c_str()[0];
        if (temp >= 'A' && temp <= 'Z') {
            first_letter = temp;
            count++;
        }
        else if (count == 1) {
            count++;
        }
        else if (count == 2) {
            count = 0;
        }
        else {
            stringstream ss(line);
            int a, b, dist;
            ss >> a >> b >> dist;
            mp[first_letter].addV(a);
            mp[first_letter].addV(b);
            mp[first_letter].addEdge(a, b, dist);
        }
    }
}