#include "linux-dmabuf-client-protocol.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "CHECK failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static int check_message(const struct wl_message *message, const char *name,
                         const char *signature) {
    CHECK(message != NULL);
    CHECK(strcmp(message->name, name) == 0);
    CHECK(strcmp(message->signature, signature) == 0);
    return 0;
}

int main(void) {
    CHECK(strcmp(zwp_linux_dmabuf_v1_interface.name, "zwp_linux_dmabuf_v1") == 0);
    CHECK(zwp_linux_dmabuf_v1_interface.version == 6);
    CHECK(zwp_linux_dmabuf_v1_interface.method_count == 4);
    CHECK(zwp_linux_dmabuf_v1_interface.event_count == 2);
    CHECK(check_message(&zwp_linux_dmabuf_v1_interface.methods[1], "create_params", "n") == 0);
    CHECK(check_message(&zwp_linux_dmabuf_v1_interface.events[1], "modifier", "3uuu") == 0);

    CHECK(strcmp(zwp_linux_buffer_params_v1_interface.name, "zwp_linux_buffer_params_v1") == 0);
    CHECK(zwp_linux_buffer_params_v1_interface.version == 6);
    CHECK(zwp_linux_buffer_params_v1_interface.method_count == 5);
    CHECK(zwp_linux_buffer_params_v1_interface.event_count == 2);
    CHECK(check_message(&zwp_linux_buffer_params_v1_interface.methods[1], "add", "huuuuu") == 0);
    CHECK(check_message(&zwp_linux_buffer_params_v1_interface.methods[2], "create", "iiuu") == 0);
    CHECK(check_message(&zwp_linux_buffer_params_v1_interface.events[0], "created", "n") == 0);

    CHECK(strcmp(zwp_linux_dmabuf_feedback_v1_interface.name,
                 "zwp_linux_dmabuf_feedback_v1") == 0);
    CHECK(zwp_linux_dmabuf_feedback_v1_interface.method_count == 1);
    CHECK(zwp_linux_dmabuf_feedback_v1_interface.event_count == 7);
    CHECK(check_message(&zwp_linux_dmabuf_feedback_v1_interface.events[2], "main_device", "a") == 0);

    puts("PASS host-protocol interfaces=3 messages=7");
    return 0;
}
