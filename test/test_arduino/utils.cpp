// utils.cpp

#include <stdio.h>
#include <cstdarg>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>

int
calc_dim(int array_size, int element_size) {
    for (int i = 0; i <= 1; i++) {
        if (array_size % element_size == 0)
            return array_size / element_size;
        if (element_size & (1 << i)) {
            element_size += 1 << i;
        }
    }
    fprintf(stderr,
            "ERROR: calc_dim could not determine dimension for array_size %d, element_size %d\n",
            array_size, element_size);
    exit(2);
}

int Test_socket;

#define SENDF_BUFFER_SIZE    200

char Sendf_buffer[SENDF_BUFFER_SIZE];
char *Sendf_buf_end = Sendf_buffer;

void
sendf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(Sendf_buf_end, SENDF_BUFFER_SIZE - (Sendf_buf_end - Sendf_buffer), format, args);

    if (!strchr(Sendf_buffer, '\n')) {
        Sendf_buf_end = strchr(Sendf_buf_end, '\0');
    } else {
        int bytes_to_send = strlen(Sendf_buffer);
        const char *buffer = Sendf_buffer;
        ssize_t send_result; 

        while (bytes_to_send > 0) {
            send_result = send(Test_socket, buffer, bytes_to_send, 0);
            if (send_result == -1) {
                fprintf(stderr, "ERROR: sendf got send error %s\n", strerror(errno));
                exit(2);
            }
            buffer += send_result;
            bytes_to_send -= send_result;
        }
        Sendf_buf_end = Sendf_buffer;
    }
}

#define SOCK_RECV_SIZE       4096
#define SOCK_BUFFER_SIZE     (SOCK_RECV_SIZE + 200)

char Recv_buffer[SOCK_BUFFER_SIZE];
char *Recv_buffer_start = Recv_buffer;

char *
sock_readline() {
    if (Recv_buffer_start > Recv_buffer) {
        memmove(Recv_buffer, Recv_buffer_start, strlen(Recv_buffer_start) + 1); // include null at end
        Recv_buffer_start = Recv_buffer;
    }
    char *end = strchr(Recv_buffer, '\0');
    char *newline;
    while (!(newline = strchr(Recv_buffer, '\n'))) {
        size_t recv_len = recv(Test_socket, end, SOCK_RECV_SIZE, 0);
        if (!recv_len) {
            // socket closed
            fprintf(stderr, "ERROR: socket closed by test driver\n");
            exit(2);
        }
        end += recv_len;
        *end = '\0';
    }
    *newline = '\0';
    Recv_buffer_start = newline + 1;
    return Recv_buffer;
}

byte
hex_to_nibble(char c) {
    // Converts one hex digit to a "nibble" (between 0x0 and 0xF).
    switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': return c - '0';
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F': return c - 'A' + 10;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f': return c - 'a' + 10;
    default:
        fprintf(stderr, "ERROR: illegal hex digit, '%c' (0x%x), from test driver\n", c, c);
        exit(2);
    }
}

byte
hex_to_byte(const char *s) {
    return (hex_to_nibble(s[0]) << 4) | hex_to_nibble(s[1]);
}

int
load_data(byte *dest, const char *hex_str) {
    // Does not null terminate dest.
    // Skips initial spaces.
    //
    // Returns number of bytes copied.
    byte *dest_ptr = dest;
    while (*hex_str == ' ') hex_str++;
    while (*hex_str && *hex_str != ' ') {
        *dest_ptr++ = hex_to_byte(hex_str);
        hex_str += 2;
    }
    return dest_ptr - dest;
}

char to_hex_nibble(byte n) {
    switch (n) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9: return n + '0';
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15: return n + 'A' - 10;
    default:
        fprintf(stderr, "INTERNAL ERROR: illegal hex nibble, %u\n", n);
        exit(1);
    }
}

void
send_data(const byte *data, int len) {
    // Sends data as a string of hex chars.
    // Does NOT send intial or final spaces!
    while (len--) {
        *Sendf_buf_end++ = to_hex_nibble(*data >> 4);
        *Sendf_buf_end++ = to_hex_nibble(*data & 0x0F);
        data++;
    }
    *Sendf_buf_end = '\0';
}

const char *
run_to_return(void) {
    // FIX: Implement this
    return "";
}

