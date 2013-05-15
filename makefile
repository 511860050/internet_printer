g++ -Wall public_function.h print.h print.cpp -lpthread -o client
g++ -Wall public_function.h print_daemon.h print_daemon.cpp -pthread -o server
