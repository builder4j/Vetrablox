/*
Copywrite (C) stupid idiot known as alen or Astro on discord.
i will hunt you down if you steal my code if you do not credit me
im super duper evil >:) totaly totaly lol

Names of programers contrubuting to this file:
- Alen

*/

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <microhttpd.h>
#include <stdlib.h>
#include "relay.h"

#define PORT 80
#define BASE_URL "http://localhost"

// simple reply, just sends text back
static enum MHD_Result send_text(struct MHD_Connection *conn, int status, const char *text) {
    struct MHD_Response *res = MHD_create_response_from_buffer(
        strlen(text), (void *)text, MHD_RESPMEM_MUST_COPY
    );
    MHD_add_response_header(res, "Content-Type", "text/plain");
    enum MHD_Result ret = MHD_queue_response(conn, status, res); 
    MHD_destroy_response(res); 
    return ret;
}

static char *read_file(const char *path, long *out_size) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(size + 1);
    if (buf == NULL) {
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, size, f) != (size_t)size) {
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[size] = '\0';
    fclose(f);
    if (out_size != NULL)
        *out_size = size;
    return buf;
}
static enum MHD_Result send_file(struct MHD_Connection *conn, const char *path) {
    char *text = read_file(path, NULL);
    if (text == NULL) {
        printf("missing file: %s\n", path);
        return send_text(conn, 404, "print(\"script not found... uh, please report this to me pls\") local msg = instance.New(\"Hint\") msg.Text = \"The game just tryed calling a script that hasnt been implemented yet, please report this to vetrablox staff or contributers!\" msg.Parent = game.Workspace"); // reminder to self, rembember that its Instance.new not instance.New
    }
    enum MHD_Result ret = send_text(conn, 200, text);
    free(text);
    return ret;
}

static enum MHD_Result send_asset(struct MHD_Connection *conn) {
    const char *id = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "id");
    if (id == NULL || id[0] == '\0' || strlen(id) > 15)
        return send_text(conn, 400, "bad asset id, ya need to upload it!");
    for (const char *c = id; *c; c++)
        if (*c < '0' || *c > '9')
            return send_text(conn, 400, "uhh what the hell are you trying?");
    
    char path[64];
    snprintf(path, sizeof path, "Assets/%s", id);

    long size; // very long size
    char *data = read_file(path, &size);
    if (data == NULL) {
        printf("missing asset: %s\n", path);
        return send_text(conn, 404, "asset not found, oof, please upload this if its not uploaded");
    }
    struct MHD_Response *res = MHD_create_response_from_buffer(size, data, MHD_RESPMEM_MUST_COPY); // very long line... jezus
    MHD_add_response_header(res, "Content-Type", "application/octet-stream");
    enum MHD_Result ret = MHD_queue_response(conn, 200, res);
    MHD_destroy_response(res);
    free(data);
    return ret;
}

static int is_id(const char *s) {
    if (s == NULL || s[0] == '\0' || strlen(s) > 15)
        return 0;
    for (; *s; s++)
        if (*s < '0' || *s > '9')
            return 0;
    return 1;
}

static enum MHD_Result send_body_colors(struct MHD_Connection *conn) {
    const char *uid = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "userId");
    if (!is_id(uid))
        return send_text(conn, 400, "bad userId");

    // hardcoded for now
    int head = 24, torso = 21, left_arm = 24, right_arm = 24, left_leg = 119, right_leg = 119;

    char out[64];
    snprintf(out, sizeof out, "%d;%d;%d;%d;%d;%d", head, torso, left_arm, right_arm, left_leg, right_leg);
    return send_text(conn, 200, out);
}



// dont uh remove this at all becuase this will be called every request
static enum MHD_Result handler(void *cls, struct MHD_Connection *conn,
                                const char *url, const char *method,
                                const char *version, const char *upload_data,
                                size_t *upload_data_size, void **con_cls) {

    // log requests
    printf("%s %s\n", method, url);
    // insert end points here please
    if (strcasecmp(url, "/health") == 0) {
        return send_text(conn, 200, "ok, why are you even checking this though?");
    }

    if (strcasecmp(url, "/game/join.ashx") == 0)
        return send_file(conn, "Scripts/join.lua");

    if (strcasecmp(url, "/game/gameserver.ashx") == 0)
        return send_file(conn, "Scripts/gameserver.lua");

    if (strcasecmp(url, "/game/visit.ashx") == 0)
        return send_file(conn, "Scripts/visit.lua");

    if (strcasecmp(url, "/asset/") == 0 || strcasecmp(url, "/asset") == 0)
        return send_asset(conn);

    if (strcasecmp(url, "/Game/BodyColors.ashx") == 0)
        return send_body_colors(conn);
    
    if (strcasecmp(url, "/cursor") == 0) { // pls dont remove this
        return send_text(conn, 200, "OwO");
    }
    


    
    // if they accessed somthing that just, didnt exist, beem em to the fucking horizen
    return send_text(conn, 404, "Not found, were the hell did you this url?");

}

int main(void) {
    // start server
    struct MHD_Daemon *d = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD, PORT,
        NULL, NULL, &handler, NULL, MHD_OPTION_END
    );

    if (d == NULL) {
        fprintf(stderr, "failed to start on port %d\n", PORT); // maybe add a list of programs using said port?
        return 1;
    }
    if (relay_start(53640, 53641) != 0) {
        fprintf(stderr, "relay failed to start (is UDP 53640 in use?)\n");
        return 1;
    }

    printf("Running, listening on localhost:%d, ect. idk what the hell to put here\n", PORT);
    getchar();

    // shutdown
    MHD_stop_daemon(d);
    return 0;
}