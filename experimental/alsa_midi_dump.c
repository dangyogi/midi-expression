// alsa_midi_in.c

#include <alsa/asoundlib.h>

static snd_seq_t *seq_handle;
static int in_port;
static int Dump_notes = 1;
static int Dump_control = 1;
static int Dump_connect = 1;
static int Skip_channel_0 = 0;


#define CHK(stmt, msg) if((stmt) < 0) { puts("ERROR: "#msg); exit(1);}
void
midi_open(void) {
    CHK(snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0),
        "Could not open sequencer");
    printf("client id: %d\n", snd_seq_client_id(seq_handle));


    CHK(snd_seq_set_client_name(seq_handle, "Midi Dump"),
        "Could not set client name");
    CHK(in_port = snd_seq_create_simple_port(seq_handle, "listen:in",
                      SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                      SND_SEQ_PORT_TYPE_APPLICATION),
        "Could not open port");
}


snd_seq_event_t *
midi_read(void) {
    snd_seq_event_t *ev = NULL;
    snd_seq_event_input(seq_handle, &ev);
    return ev;
}


char *
Notes[12] = {
    "C",
    "C#",
    "D",
    "Eb",
    "E",
    "F",
    "F#",
    "G",
    "Ab",
    "A",
    "Bb",
    "B",
};

char *
note_name(int midi_note) {
    return Notes[midi_note % 12];
}

int
octave(int midi_note) {
    return midi_note / 12 - 1;
}


void
midi_process(snd_seq_event_t *ev) {
    if ((ev->type == SND_SEQ_EVENT_NOTEON)||(ev->type == SND_SEQ_EVENT_NOTEOFF)) {
        if (Dump_notes && (ev->data.note.channel || !Skip_channel_0)) {
            const char *type = (ev->type == SND_SEQ_EVENT_NOTEON) ? "on " : "off";

            printf("[%d] Channel %#.2X -> Note %s: %s%d[%#.2X] vel(%#.2X)\n",
                   ev->time.tick, ev->data.note.channel, type,
                   note_name(ev->data.note.note),
                   octave(ev->data.note.note),
                   ev->data.note.note,
                   ev->data.note.velocity);
        }
    } else if(ev->type == SND_SEQ_EVENT_CONTROLLER || ev->type == SND_SEQ_EVENT_PGMCHANGE) {
        const char *type = (ev->type == SND_SEQ_EVENT_CONTROLLER)
                          ? "Control" : "Program Change";
        if (Dump_control && (ev->data.note.channel || !Skip_channel_0)) {
            printf("[%d] Channel %#.2X -> %s: %#.2X = %#.2X\n",
                   ev->time.tick, ev->data.control.channel, type,
                   ev->data.control.param, ev->data.control.value);
        }
    } else if(ev->type == SND_SEQ_EVENT_PORT_SUBSCRIBED ||
              ev->type == SND_SEQ_EVENT_PORT_UNSUBSCRIBED) {
        const char *type = (ev->type == SND_SEQ_EVENT_PORT_SUBSCRIBED)
                          ? "Subscribed" : "Unsubscribed";
        if (Dump_connect) {
            printf("[%d] Port %s: %d:%d -> %d:%d\n",
                   ev->time.tick, type,
                   ev->data.connect.sender.client, ev->data.connect.sender.port,
                   ev->data.connect.dest.client, ev->data.connect.dest.port);
        }
    } else
        printf("[%d] Unknown:  Unhandled Event %d Received\n", ev->time.tick, ev->type);
}


void
usage(char *msg) {
    fprintf(stderr, "%s\n", msg);
    fprintf(stderr, "alsa_midi_in: [-n] [-c] [-C] [-z]\n");
    exit(1);
}


int
main(int argc, char *argv[]) {
    int i;
    for (i = 1; i < argc; i++) {
        printf("%d: %s\n", i, argv[i]);
        if (argv[i][0] != '-') usage("expected '-'");
        else if (argv[i][1] == 'n') Dump_notes = 0;
        else if (argv[i][1] == 'c') Dump_control = 0;
        else if (argv[i][1] == 'C') Dump_connect = 0;
        else if (argv[i][1] == 'z') Skip_channel_0 = 1;
        else usage("expected -n or -c or -C or -z");
    }
    midi_open();
    while (1)
        midi_process(midi_read());
    return -1;
}
