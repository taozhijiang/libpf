
#include <iostream>

#include <cstdlib>
#include <unistd.h>
#include <signal.h>

#include "libpf.h"
#include "time_util.h"

volatile bool terminate = false;

static void signal_handler(int signal) {
    switch(signal) {
        case SIGUSR1: {
            std::string msg {};
            if(libpf::message(msg))
                std::cout << msg << std::endl;
        }
            break;

        case SIGINT: {
            std::string msg {};
            if(libpf::message(msg))
                std::cout << msg << std::endl;

            // terminate system
            libpf::terminate();
            terminate = true;
        }
            break;

        default:
            std::cout << "[ERROR] signal not processed: " << signal << std::endl;
            break;
    }
}

int main(int argc, char* argv[]) {

    std::cout << "[INFO] begin libpf demo test." << std::endl;

    if(!libpf::init(20, 2)) {
        std::cout << "[ERROR] init libpf failed." << std::endl;
        return EXIT_FAILURE;
    }

    ::signal(SIGUSR1, ::signal_handler);
    ::signal(SIGINT, ::signal_handler);

    struct timespec delay {};
    delay.tv_sec = 0;
    delay.tv_nsec = 1; // avoid CPU 100%

    while(true) {

        libpf::submit("metric-101", ::rand() % 200);
        libpf::submit("metric-102", ::rand() % 200);
        libpf::submit("metric-101", ::rand() % 200);
        libpf::submit("metric-103", ::rand() % 200);

        ::nanosleep(&delay, NULL);
        // ::usleep(1);

        if(terminate) {
            std::cout << "[INFO] terminate main thread." << std::endl;
            break;
        }
    }

    ::sleep(1);
    std::cout << "[INFO] DONE." << std::endl;
    return EXIT_SUCCESS;
}