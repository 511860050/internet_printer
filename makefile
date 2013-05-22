g++ -Wall ipp_header.h public_function.h print.h print.cpp -lpthread -o client
g++ -Wall ipp_header.h public_function.h print_daemon.h print_daemon.cpp -pthread -o server
g++ -Wall ipp_header.h public_function.h printer.h printer.cpp -pthread -o chen
