#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

/*
 * Redes de Computadores - Lab 2
 * Simple FTP download client
 *
 * Usage:
 *   ./download ftp://[user:password@]host/path/to/file
 *
 * Examples:
 *   ./download ftp://ftp.netlab.fe.up.pt/pub/file.txt
 *   ./download ftp://anonymous:anonymous@mirrors.up.pt/debian/README.html
 *
 * The program implements the basic FTP sequence:
 *   connect -> USER/PASS -> TYPE I -> PASV -> data connection -> RETR -> save file -> QUIT
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define FTP_PORT 21
#define BUFFER_SIZE 4096
#define RESPONSE_SIZE 8192

struct ftp_url {
    char user[256];
    char password[256];
    char host[256];
    int port;
    char path[1024];
    char filename[256];
};

static void usage(const char *program) {
    fprintf(stderr, "Usage: %s ftp://[user:password@]host/path/to/file\n", program);
}

static void fail(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static int starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static void url_decode(char *dst, size_t dst_size, const char *src) {
    size_t i = 0;

    while (*src != '\0' && i + 1 < dst_size) {
        if (*src == '%' && isxdigit((unsigned char) src[1]) && isxdigit((unsigned char) src[2])) {
            int high = hex_value(src[1]);
            int low = hex_value(src[2]);
            dst[i++] = (char) ((high << 4) | low);
            src += 3;
        } else {
            dst[i++] = *src++;
        }
    }

    dst[i] = '\0';
}

static void copy_checked(char *dst, size_t dst_size, const char *src, const char *field_name) {
    if (strlen(src) >= dst_size) {
        fprintf(stderr, "Error: %s is too long.\n", field_name);
        exit(EXIT_FAILURE);
    }
    strcpy(dst, src);
}

static void parse_url(const char *url, struct ftp_url *parsed) {
    const char *prefix = "ftp://";
    char *work = NULL;
    char *slash = NULL;
    char *host_part = NULL;
    char *path_part = NULL;
    char *at = NULL;
    char *colon = NULL;
    char decoded[1024];

    memset(parsed, 0, sizeof(*parsed));
    strcpy(parsed->user, "anonymous");
    strcpy(parsed->password, "anonymous@");
    parsed->port = FTP_PORT;

    if (!starts_with(url, prefix)) {
        fprintf(stderr, "Error: URL must start with ftp://\n");
        exit(EXIT_FAILURE);
    }

    work = strdup(url + strlen(prefix));
    if (work == NULL) fail("strdup");

    slash = strchr(work, '/');
    if (slash == NULL || slash[1] == '\0') {
        fprintf(stderr, "Error: URL must include a file path after the host.\n");
        free(work);
        exit(EXIT_FAILURE);
    }

    *slash = '\0';
    host_part = work;
    path_part = slash + 1;

    at = strrchr(host_part, '@');
    if (at != NULL) {
        char *auth = host_part;
        *at = '\0';
        host_part = at + 1;

        colon = strchr(auth, ':');
        if (colon != NULL) {
            *colon = '\0';
            url_decode(decoded, sizeof(decoded), auth);
            copy_checked(parsed->user, sizeof(parsed->user), decoded, "username");
            url_decode(decoded, sizeof(decoded), colon + 1);
            copy_checked(parsed->password, sizeof(parsed->password), decoded, "password");
        } else {
            url_decode(decoded, sizeof(decoded), auth);
            copy_checked(parsed->user, sizeof(parsed->user), decoded, "username");
            parsed->password[0] = '\0';
        }
    }

    if (host_part[0] == '\0') {
        fprintf(stderr, "Error: missing host in URL.\n");
        free(work);
        exit(EXIT_FAILURE);
    }

    /* Optional host:port support, useful for tests. */
    colon = strrchr(host_part, ':');
    if (colon != NULL && colon[1] != '\0') {
        char *endptr = NULL;
        long port = 0;

        *colon = '\0';
        port = strtol(colon + 1, &endptr, 10);
        if (*endptr != '\0' || port <= 0 || port > 65535) {
            fprintf(stderr, "Error: invalid FTP port in URL.\n");
            free(work);
            exit(EXIT_FAILURE);
        }
        parsed->port = (int) port;
    }

    url_decode(decoded, sizeof(decoded), host_part);
    copy_checked(parsed->host, sizeof(parsed->host), decoded, "host");

    url_decode(decoded, sizeof(decoded), path_part);
    copy_checked(parsed->path, sizeof(parsed->path), decoded, "path");

    {
        const char *last_slash = strrchr(parsed->path, '/');
        const char *name = (last_slash != NULL) ? last_slash + 1 : parsed->path;

        if (name[0] == '\0') {
            fprintf(stderr, "Error: URL path must end with a file name.\n");
            free(work);
            exit(EXIT_FAILURE);
        }
        copy_checked(parsed->filename, sizeof(parsed->filename), name, "filename");
    }

    free(work);
}

static void resolve_host(const char *host, char *ip, size_t ip_size) {
    struct hostent *h = gethostbyname(host);

    if (h == NULL) {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    if (h->h_addrtype != AF_INET || h->h_addr_list[0] == NULL) {
        fprintf(stderr, "Error: host does not have an IPv4 address.\n");
        exit(EXIT_FAILURE);
    }

    snprintf(ip, ip_size, "%s", inet_ntoa(*((struct in_addr *) h->h_addr_list[0])));
}

static int open_tcp_socket(const char *ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t) port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "Error: invalid IP address %s\n", ip);
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) fail("socket");

    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        fail("connect");
    }

    return sockfd;
}

static ssize_t send_all(int sockfd, const char *data, size_t length) {
    size_t sent = 0;

    while (sent < length) {
        ssize_t n = send(sockfd, data + sent, length - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) break;
        sent += (size_t) n;
    }

    return (ssize_t) sent;
}

static void ftp_send_command(int sockfd, const char *format, ...) {
    char command[2048];
    char full_command[2050];
    va_list args;
    int n;

    va_start(args, format);
    n = vsnprintf(command, sizeof(command), format, args);
    va_end(args);

    if (n < 0 || (size_t) n >= sizeof(command)) {
        fprintf(stderr, "Error: FTP command is too long.\n");
        exit(EXIT_FAILURE);
    }

    n = snprintf(full_command, sizeof(full_command), "%s\r\n", command);
    if (n < 0 || (size_t) n >= sizeof(full_command)) {
        fprintf(stderr, "Error: FTP command is too long.\n");
        exit(EXIT_FAILURE);
    }

    printf("> %s\n", command);

    if (send_all(sockfd, full_command, strlen(full_command)) != (ssize_t) strlen(full_command)) {
        fail("send");
    }
}

static int read_line(int sockfd, char *line, size_t line_size) {
    size_t i = 0;

    while (i + 1 < line_size) {
        char c;
        ssize_t n = recv(sockfd, &c, 1, 0);

        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) {
            if (i == 0) return 0;
            break;
        }

        line[i++] = c;
        if (c == '\n') break;
    }

    line[i] = '\0';
    return 1;
}

static int is_response_code_line(const char *line) {
    return isdigit((unsigned char) line[0]) &&
           isdigit((unsigned char) line[1]) &&
           isdigit((unsigned char) line[2]) &&
           (line[3] == ' ' || line[3] == '-');
}

static int response_code(const char *line) {
    if (!is_response_code_line(line)) return -1;
    return (line[0] - '0') * 100 + (line[1] - '0') * 10 + (line[2] - '0');
}

static int ftp_read_response(int sockfd, char *response, size_t response_size) {
    char line[1024];
    int code = -1;
    int multiline = 0;
    int first_line = 1;
    size_t used = 0;

    if (response_size > 0) response[0] = '\0';

    while (1) {
        int r = read_line(sockfd, line, sizeof(line));
        size_t len;

        if (r < 0) fail("recv");
        if (r == 0) {
            fprintf(stderr, "Error: FTP server closed the control connection unexpectedly.\n");
            exit(EXIT_FAILURE);
        }

        printf("< %s", line);
        if (line[strlen(line) - 1] != '\n') printf("\n");

        len = strlen(line);
        if (used + len + 1 < response_size) {
            memcpy(response + used, line, len);
            used += len;
            response[used] = '\0';
        }

        if (first_line) {
            first_line = 0;
            code = response_code(line);
            if (code < 0) continue;
            multiline = (line[3] == '-');
            if (!multiline) return code;
        } else if (multiline &&
                   response_code(line) == code &&
                   line[3] == ' ') {
            return code;
        }
    }
}

static void expect_code(int code, int expected, const char *phase) {
    if (code != expected) {
        fprintf(stderr, "Error during %s: expected FTP code %d, got %d.\n", phase, expected, code);
        exit(EXIT_FAILURE);
    }
}

static void expect_positive_completion(int code, const char *phase) {
    if (code < 200 || code >= 300) {
        fprintf(stderr, "Error during %s: expected 2xx FTP reply, got %d.\n", phase, code);
        exit(EXIT_FAILURE);
    }
}

static void parse_pasv_response(const char *response, char *ip, size_t ip_size, int *port) {
    const char *p = strchr(response, '(');
    int h1, h2, h3, h4, p1, p2;

    if (p == NULL || sscanf(p, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        fprintf(stderr, "Error: could not parse PASV response.\n");
        exit(EXIT_FAILURE);
    }

    if (h1 < 0 || h1 > 255 || h2 < 0 || h2 > 255 || h3 < 0 || h3 > 255 || h4 < 0 || h4 > 255 ||
        p1 < 0 || p1 > 255 || p2 < 0 || p2 > 255) {
        fprintf(stderr, "Error: invalid values in PASV response.\n");
        exit(EXIT_FAILURE);
    }

    snprintf(ip, ip_size, "%d.%d.%d.%d", h1, h2, h3, h4);
    *port = p1 * 256 + p2;
}

static void download_data(int data_sockfd, const char *filename) {
    FILE *file = fopen(filename, "wb");
    char buffer[BUFFER_SIZE];
    size_t total = 0;

    if (file == NULL) fail("fopen");

    while (1) {
        ssize_t n = recv(data_sockfd, buffer, sizeof(buffer), 0);

        if (n < 0) {
            if (errno == EINTR) continue;
            fclose(file);
            fail("recv data");
        }
        if (n == 0) break;

        if (fwrite(buffer, 1, (size_t) n, file) != (size_t) n) {
            fclose(file);
            fprintf(stderr, "Error: failed to write to output file.\n");
            exit(EXIT_FAILURE);
        }

        total += (size_t) n;
    }

    if (fclose(file) != 0) fail("fclose");
    printf("Saved %zu bytes to %s\n", total, filename);
}

int main(int argc, char **argv) {
    struct ftp_url url;
    char server_ip[64];
    char pasv_response[RESPONSE_SIZE];
    char response[RESPONSE_SIZE];
    char data_ip[64];
    int data_port;
    int control_sockfd;
    int data_sockfd;
    int code;

    if (argc != 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    parse_url(argv[1], &url);

    printf("Host: %s\n", url.host);
    printf("Path: %s\n", url.path);
    printf("Output file: %s\n", url.filename);

    resolve_host(url.host, server_ip, sizeof(server_ip));
    printf("Server IP: %s\n", server_ip);

    control_sockfd = open_tcp_socket(server_ip, url.port);

    code = ftp_read_response(control_sockfd, response, sizeof(response));
    expect_code(code, 220, "initial connection");

    ftp_send_command(control_sockfd, "USER %s", url.user);
    code = ftp_read_response(control_sockfd, response, sizeof(response));

    if (code == 331) {
        ftp_send_command(control_sockfd, "PASS %s", url.password);
        code = ftp_read_response(control_sockfd, response, sizeof(response));
        expect_positive_completion(code, "login");
    } else if (code != 230) {
        fprintf(stderr, "Error during USER: expected FTP code 331 or 230, got %d.\n", code);
        close(control_sockfd);
        return EXIT_FAILURE;
    }

    ftp_send_command(control_sockfd, "TYPE I");
    code = ftp_read_response(control_sockfd, response, sizeof(response));
    expect_positive_completion(code, "TYPE I");

    ftp_send_command(control_sockfd, "PASV");
    code = ftp_read_response(control_sockfd, pasv_response, sizeof(pasv_response));
    expect_code(code, 227, "PASV");

    parse_pasv_response(pasv_response, data_ip, sizeof(data_ip), &data_port);
    printf("Passive data address: %s:%d\n", data_ip, data_port);

    data_sockfd = open_tcp_socket(data_ip, data_port);

    ftp_send_command(control_sockfd, "RETR %s", url.path);
    code = ftp_read_response(control_sockfd, response, sizeof(response));
    if (code != 150 && code != 125) {
        fprintf(stderr, "Error during RETR: expected FTP code 150 or 125, got %d.\n", code);
        close(data_sockfd);
        close(control_sockfd);
        return EXIT_FAILURE;
    }

    download_data(data_sockfd, url.filename);
    close(data_sockfd);

    code = ftp_read_response(control_sockfd, response, sizeof(response));
    expect_positive_completion(code, "file transfer completion");

    ftp_send_command(control_sockfd, "QUIT");
    code = ftp_read_response(control_sockfd, response, sizeof(response));
    expect_positive_completion(code, "QUIT");

    close(control_sockfd);
    return EXIT_SUCCESS;
}
