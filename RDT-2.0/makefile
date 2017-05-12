all:
	g++ -c rdt.cpp
	ar rvs rdt.a rdt.o
	g++ -o server server.cpp rdt.a
	g++ -o client client.cpp rdt.a