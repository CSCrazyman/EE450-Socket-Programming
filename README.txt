The code files in the project are client.cpp, aws.cpp, serverA.cpp, serverA.h, serverB.cpp. I have also written a README.txt (this file) and Makefile.

As these files named, each file contains source code of corresponding part of the project: 
1. The client.cpp is responsible to send requests and receive final results.
2. The aws.cpp is responsible to receive client's requests, forward necessary data to two servers, store intermediate results, and send results back to client.
3. The serverA.cpp is responsible to utilize Dijkstra algorithm to compute all shortest distances from a source vertex to all other vertexes in a map.
4. The serverA.h contains user-defined classes to implement Dijkstra algorithm.
5. The serverB.cpp is responsible to compute all end-to-end delays for transmitting a file via all distances computed by serverA.cpp. 

The format of all the messages exchanged between client/aws/servers is char array. I convert char array (message) to local string to process and then get results in each phase, and convert string to char array back while communicating between client/aws/servers.

Because I need to compute shortest distances via a map using Dijkstra algorithm, I write Edge class and Graph class to represent a map. If the source vertex does not exist in the map we looks up, my project will fail. Also, my project will not return any result because serverA cannot compute a valid result with a non-exist map.
