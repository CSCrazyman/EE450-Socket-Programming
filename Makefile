# Complies all code files and creates executables
all: runA runB runAWS runClient
# ------------------------
# Server A compile and run
# ------------------------
serverA: servera
	./servera
runA: serverA.cpp
	g++ -std=c++11 -o servera serverA.cpp
# ------------------------
# Server B compile and run
# ------------------------
serverB: serverb
	./serverb
runB: serverB.cpp
	g++ -std=c++11 -o serverb serverB.cpp
# ------------------------
# AWS compile and run
# ------------------------
aws: AWS
	./AWS
runAWS: aws.cpp
	g++ -std=c++11 -o AWS aws.cpp
# ------------------------
# client compile
# ------------------------
runClient: client.cpp
	g++ -std=c++11 -o client client.cpp
.PHONY: serverA serverB aws