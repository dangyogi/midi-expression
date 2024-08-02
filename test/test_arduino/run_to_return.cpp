// run_to_return.cpp

int
compare(const void *key, const void *element) {
    caller_stub_names_t *stubs = (caller_stub_names_t *)element;
    return strcmp((const char *)key, stubs->name);
}

void *
find_global(char *type, int max_type_len) {
    // copies type to type argument
    // does sendf for final_addr and value (with '\n' terminator).  Caller must do sendf for command
    // with trailing space.
    // returns final addr from function
    char *next = next_word();
    if (strlen(next) >= max_type_len) {
        fprintf(stderr, "ERROR: type='%s' too long\n", next);
        exit(2);
    }
    strcpy(type, next);
    char *addr = (char *)strtoul(next_word(), NULL, 10);

    // apply offsets
    int first = 1;
    while (*Word_ptr && *Word_ptr != '=') {
        unsigned int offset = (unsigned int)strtoul(next_word(), NULL, 10);
        if (first) {
            first = 0;
        } else {
            addr = *(char **)addr;
        }
        addr += offset;
    }

    // retrieve value
    if (type[0] == 's') {
        // signed int
        long l;
        switch (type[1]) {
        case '1': l = (long)*(char *)addr; break;
        case '2': l = (long)*(short *)addr; break;
        case '4': l = (long)*(int *)addr; break;
        default:
            fprintf(stderr, "ERROR: find_global illegal type='%s'\n", type);
            exit(2);
        }
        sendf("%lu %ld\n", (unsigned long)addr, l);
    } else if (type[0] == 'u') {
        // unsigned int
        unsigned long l;
        switch (type[1]) {
        case '1': l = (unsigned long)*(unsigned char *)addr; break;
        case '2': l = (unsigned long)*(unsigned short *)addr; break;
        case '4': l = (unsigned long)*(unsigned int *)addr; break;
        default:
            fprintf(stderr, "ERROR: find_global illegal type='%s'\n", type);
            exit(2);
        }
        sendf("%lu %lu\n", (unsigned long)addr, l);
    } else if (type[0] == 'f') {
        // float
        double d;
        switch (type[1]) {
        case '4': d = (double)*(float *)addr; break;
        case '8': d = (double)*(double *)addr; break;
        default:
            fprintf(stderr, "ERROR: find_global illegal type='%s'\n", type);
            exit(2);
        }
        sendf("%lu %g\n", (unsigned long)addr, d);
    } else if (strcmp(type, "str") == 0) {
        // string (null terminated)
        sendf("%lu %s\n", (unsigned long)addr, addr);
    } else if (isdigit(type[0])) {
        int len = atoi(type);
        send_data((const byte *)addr, len);
        sendf("\n");
    } else {
        fprintf(stderr, "ERROR: find_global illegal type='%s'\n", type);
        exit(2);
    }

    return addr;
}

void
set_global(void *addr, char *type) {
    // stores value at end of command line in addr
    if (type[0] == 's') {
        // signed int
        long l = strtol(next_word(), NULL, 10);
        switch (type[1]) {
        case '1': *(char *)addr = (char)l; break;
        case '2': *(short *)addr = (short)l; break;
        case '4': *(int *)addr = (int)l; break;
        default:
            fprintf(stderr, "ERROR: set_global illegal type='%s'\n", type);
            exit(2);
        }
    } else if (type[0] == 'u') {
        // unsigned int
        unsigned long l = strtoul(next_word(), NULL, 10);
        switch (type[1]) {
        case '1': *(unsigned char *)addr = (unsigned char)l; break;
        case '2': *(unsigned short *)addr = (unsigned short)l; break;
        case '4': *(unsigned int *)addr = (unsigned int)l; break;
        default:
            fprintf(stderr, "ERROR: set_global illegal type='%s'\n", type);
            exit(2);
        }
    } else if (type[0] == 'f') {
        // float
        double d = atof(next_word());
        switch (type[1]) {
        case '4': *(float *)addr = (float)d; break;
        case '8': *(double *)addr = (double)d; break;
        default:
            fprintf(stderr, "ERROR: set_global illegal type='%s'\n", type);
            exit(2);
        }
    } else if (strcmp(type, "str") == 0) {
        // string (null terminated)
        strcpy((char *)addr, Word_ptr);
    } else if (isdigit(type[0])) {
        int len = atoi(type);
        int len_loaded = load_data((byte *)addr, Word_ptr);
        if (len != len_loaded) {
            fprintf(stderr, "ERROR: set_global load_data len=%d != command len=%d\n", len_loaded, len);
            exit(2);
        }
    } else {
        fprintf(stderr, "ERROR: set_global illegal type='%s'\n", type);
        exit(2);
    }
}

#define MAX_TYPE_LEN    10

const char *
run_to_return(void) {
    for (;;) {
        char *line = sock_readline();
        char *command = next_word();
        char type[MAX_TYPE_LEN];

        if (strcmp(command, "return") == 0) {
            // return [value]
            if (*Word_ptr == '\0') return NULL;
            return next_word();
        }
        if (strcmp(command, "call") == 0) {
            // call name arguments
            char *name = next_word();
            caller_stub_names_t *ans =
              (caller_stub_names_t *)bsearch(name, Caller_stub_names, NUM_CALLER_STUBS,
                                             sizeof(caller_stub_names_t), compare);
            if (!ans) {
                fprintf(stderr, "ERROR function='%s' in 'call' command not found\n", name);
                exit(2);
            }
            ans->caller_stub(Word_ptr);
        } else if (strcmp(command, "get_global") == 0) {
            // get_global type addr offsets
            // each offset is to next pointer.
            // type is: s[1,2,4] or u[1,2,4] or f[4,8] or str (null terminated)
            //          or <int> (for data returned as hex string)
            sendf("get_global ");
            find_global(type, MAX_TYPE_LEN);
        } else if (strcmp(command, "set_global") == 0) {
            // set_global type addr offsets = value
            // each offset is to next pointer.
            // type is: s[1,2,4] or u[1,2,4] or f[4,8] or str (null terminated)
            //          or <int> (for data returned as hex string)
            sendf("set_global ");
            void *final_addr = find_global(type, MAX_TYPE_LEN);
            if (strcmp(next_word(), "=")) {
                fprintf(stderr, "ERROR set_global command missing '='\n");
                exit(2);
            }
            set_global(final_addr, type);
        } else if (strcmp(command, "exit") == 0) {
            // exit exit_status
            int exit_status = atoi(next_word());
            exit(exit_status);
        } else if (strcmp(command, "display") == 0) {
            // display text
            printf("%s\n", line);
        } else {
            fprintf(stderr, "ERROR unrecognized test_driver command: '%s'\n", command);
            exit(2);
        }
    }
}

