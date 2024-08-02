// main.cpp

#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    Test_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (Test_socket == -1) {
        fprintf(stderr, "ERROR: socket creation failed %s\n", strerror(errno));
        exit(2);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(2020);

    int status = inet_aton("127.0.0.1", &serv_addr.sin_addr);
    if (status == 0) {
        fprintf(stderr, "ERROR: inet_aton got invalid inet addr\n");
        exit(2);
    }
    if (status < 0) {
        fprintf(stderr, "ERROR: inet_aton failed %s\n", strerror(errno));
        exit(2);
    }

    if (connect(Test_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
        fprintf(stderr, "ERROR: connect failed %s\n", strerror(errno));
        exit(2);
    }

    init_EEPROM();
    init_sketch();

    arch_send_defines();
    arch_send_classes();
    arch_send_structs();
    arch_send_globals();
    arch_send_arrays();

    send_defines();
    send_classes();
    send_structs();
    send_globals();
    send_arrays();

    sendf("ready\n");

    int ret = atoi(run_to_return());

    return ret;
}
