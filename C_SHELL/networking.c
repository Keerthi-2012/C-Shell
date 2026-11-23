
#define _GNU_SOURCE

#include "networking.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include <sys/socket.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define HOST "man.he.net"
#define PORT "443"

/* Send complete HTTP request */
int send_all(SSL *ssl, const char *data, size_t len)
{
    size_t sent = 0;

    while (sent < len)
    {
        int remaining = len - sent > INT_MAX
                            ? INT_MAX : (int)(len - sent);

        int n = SSL_write(ssl, data + sent, remaining);

        if (n <= 0)
        {
            int err = SSL_get_error(ssl, n);

            if (err == SSL_ERROR_WANT_READ ||
                err == SSL_ERROR_WANT_WRITE)
                continue;

            ERR_print_errors_fp(stderr);
            return -1;
        }

        sent += n;
    }

    return 0;
}

/* Decode chunked HTTP response */
char *decode_chunked(const char *body, size_t len,
                     size_t *output_len)
{
    size_t capacity = len + 1;
    size_t used = 0;

    char *output = malloc(capacity);

    if (!output)
        return NULL;

    const char *p = body;
    const char *limit = body + len;

    while (p < limit)
    {
        char *end;
        unsigned long long chunk_size = strtoull(p, &end, 16);

        if (end == p || end > limit)
            break;

        const char *line_end = memchr(p, '\n', limit - p);

        if (!line_end)
            break;

        p = line_end + 1;

        if (chunk_size == 0)
            break;

        if (chunk_size > (unsigned long long)(limit - p))
            break;

        if (used + chunk_size + 1 > capacity)
        {
            capacity = used + chunk_size + 1;

            char *temp = realloc(output, capacity);

            if (!temp)
            {
                free(output);
                return NULL;
            }

            output = temp;
        }

        memcpy(output + used, p, chunk_size);
        used += chunk_size;

        p += chunk_size;

        if (p < limit && *p == '\r')
            p++;

        if (p < limit && *p == '\n')
            p++;
    }

    output[used] = '\0';
    *output_len = used;

    return output;
}

/* Decode common HTML entities */
void print_entity(const char *entity, size_t len)
{
    if (len == 4 && !strncmp(entity, "&lt;", 4))
        putchar('<');

    else if (len == 4 && !strncmp(entity, "&gt;", 4))
        putchar('>');

    else if (len == 5 && !strncmp(entity, "&amp;", 5))
        putchar('&');

    else if (len == 6 && !strncmp(entity, "&quot;", 6))
        putchar('"');

    else if (len == 6 && !strncmp(entity, "&nbsp;", 6))
        putchar(' ');

    else if (len == 6 && !strncmp(entity, "&apos;", 6))
        putchar('\'');

    else if (len == 5 && !strncmp(entity, "&#39;", 5))
        putchar('\'');

    else
        fwrite(entity, 1, len, stdout);
}

/* Convert HTML into readable terminal text */
void html_to_text(const char *html, size_t len)
{
    int skip_script = 0;
    int skip_style = 0;

    for (size_t i = 0; i < len;)
    {
        if (html[i] == '<')
        {
            const char *end = memchr(html + i, '>', len - i);

            if (!end)
                break;

            size_t tag_len = end - (html + i) + 1;

            if (strncasecmp(html + i, "<script", 7) == 0)
                skip_script = 1;

            else if (strncasecmp(html + i, "</script", 8) == 0)
                skip_script = 0;

            else if (strncasecmp(html + i, "<style", 6) == 0)
                skip_style = 1;

            else if (strncasecmp(html + i, "</style", 7) == 0)
                skip_style = 0;

            if (!skip_script && !skip_style)
            {
                if (strncasecmp(html + i, "<br", 3) == 0 ||
                    strncasecmp(html + i, "</p", 3) == 0 ||
                    strncasecmp(html + i, "</div", 5) == 0 ||
                    strncasecmp(html + i, "</pre", 5) == 0 ||
                    strncasecmp(html + i, "</li", 4) == 0 ||
                    strncasecmp(html + i, "</h", 3) == 0)
                {
                    putchar('\n');
                }
            }

            i += tag_len;
        }
        else if (!skip_script && !skip_style &&
                 html[i] == '&')
        {
            const char *end = memchr(html + i, ';', len - i);

            if (end && (size_t)(end - (html + i)) < 12)
            {
                size_t entity_len = end - (html + i) + 1;

                print_entity(html + i, entity_len);
                i += entity_len;
            }
            else
            {
                putchar(html[i++]);
            }
        }
        else
        {
            if (!skip_script && !skip_style)
                putchar(html[i]);

            i++;
        }
    }

    putchar('\n');
}

/* Main iMan command */
void fetch_man_page(char *input)
{
    char *copy = strdup(input);

    if (!copy)
    {
        perror("strdup");
        return;
    }

    /* Parse command */
    char *saveptr;

    char *token = strtok_r(copy, " \t\n", &saveptr);

    if (!token || strcmp(token, "iMan") != 0)
    {
        printf("Invalid command\n");
        free(copy);
        return;
    }

    char *command = strtok_r(NULL, " \t\n", &saveptr);

    if (!command)
    {
        printf("Usage: iMan <command>\n");
        free(copy);
        return;
    }

    /* Resolve hostname */
    struct addrinfo hints, *result, *rp;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(HOST, PORT, &hints, &result);

    if (status != 0)
    {
        fprintf(stderr, "DNS error: %s\n", gai_strerror(status));
        free(copy);
        return;
    }

    /* Create TCP socket and connect */
    int sockfd = -1;

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sockfd = socket(rp->ai_family, rp->ai_socktype,
                        rp->ai_protocol);

        if (sockfd == -1)
            continue;

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(result);

    if (sockfd == -1)
    {
        perror("Connection failed");
        free(copy);
        return;
    }

    /* Initialize SSL */
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());

    if (!ctx)
    {
        ERR_print_errors_fp(stderr);
        close(sockfd);
        free(copy);
        return;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    if (SSL_CTX_set_default_verify_paths(ctx) != 1)
    {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        close(sockfd);
        free(copy);
        return;
    }

    SSL *ssl = SSL_new(ctx);

    if (!ssl)
    {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        close(sockfd);
        free(copy);
        return;
    }

    /* Set hostname for TLS and certificate verification */
    SSL_set_tlsext_host_name(ssl, HOST);
    SSL_set1_host(ssl, HOST);
    SSL_set_fd(ssl, sockfd);

    /* TLS handshake */
    if (SSL_connect(ssl) != 1)
    {
        fprintf(stderr, "TLS handshake failed\n");
        ERR_print_errors_fp(stderr);

        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        free(copy);
        return;
    }

    /* Construct HTTP request */
    char path[512];

    snprintf(path, sizeof(path),
             "/?topic=%s&section=all", command);

    char request[1024];

    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: " HOST "\r\n"
             "User-Agent: iMan/1.0\r\n"
             "Accept: text/html\r\n"
             "Accept-Encoding: identity\r\n"
             "Connection: close\r\n\r\n",
             path);

    /* Send request */
    if (send_all(ssl, request, strlen(request)) < 0)
    {
        printf("Request sending failed\n");

        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        free(copy);
        return;
    }

    /* Receive complete response */
    size_t capacity = 8192;
    size_t total = 0;

    char *response = malloc(capacity);

    if (!response)
    {
        perror("malloc");

        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        free(copy);
        return;
    }

    char buffer[4096];
    int n;

    while (1)
    {
        n = SSL_read(ssl, buffer, sizeof(buffer));

        if (n > 0)
        {
            if (total + n + 1 > capacity)
            {
                capacity = (total + n + 1) * 2;

                char *temp = realloc(response, capacity);

                if (!temp)
                {
                    perror("realloc");
                    free(response);
                    SSL_free(ssl);
                    SSL_CTX_free(ctx);
                    close(sockfd);
                    free(copy);
                    return;
                }

                response = temp;
            }

            memcpy(response + total, buffer, n);
            total += n;
        }
        else
        {
            int err = SSL_get_error(ssl, n);

            if (err == SSL_ERROR_ZERO_RETURN)
                break;

            /*
             * Some servers close the connection without
             * sending TLS close_notify. If data was already
             * received, treat this as end of response.
             */
            if (err == SSL_ERROR_SYSCALL &&
                errno == 0 && total > 0)
                break;

            if (err == SSL_ERROR_SSL && total > 0)
            {
                unsigned long ssl_err = ERR_peek_error();

                if (ssl_err == 0)
                    break;
            }

            if (err == SSL_ERROR_WANT_READ ||
                err == SSL_ERROR_WANT_WRITE)
                continue;

            fprintf(stderr, "SSL read error\n");
            ERR_print_errors_fp(stderr);

            free(response);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            close(sockfd);
            free(copy);
            return;
        }
    }

    response[total] = '\0';

    /* Close SSL connection */
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sockfd);

    if (total == 0)
    {
        printf("Empty response from server\n");
        free(response);
        free(copy);
        return;
    }

    /* Find HTTP header/body separator */
    char *body = strstr(response, "\r\n\r\n");
    size_t separator_length = 4;

    if (!body)
    {
        body = strstr(response, "\n\n");
        separator_length = 2;
    }

    if (!body)
    {
        printf("Could not find HTTP header separator.\n");
        printf("Raw server response:\n");
        fwrite(response, 1, total, stdout);

        free(response);
        free(copy);
        return;
    }

    /* Separate headers and body */
    size_t header_length = body - response;

    *body = '\0';
    body += separator_length;

    size_t body_len = total - header_length - separator_length;

    /* Check HTTP status */
    int http_status = 0;

    sscanf(response, "HTTP/%*s %d", &http_status);

    if (http_status != 200)
    {
        printf("HTTP error: %d\n", http_status);
        printf("Server response:\n");

        fwrite(body, 1, body_len, stdout);

        free(response);
        free(copy);
        return;
    }

    /* Handle chunked encoding */
    char *decoded_body = NULL;

    if (strcasestr(response, "Transfer-Encoding: chunked"))
    {
        decoded_body = decode_chunked(body, body_len, &body_len);

        if (!decoded_body)
        {
            printf("Chunk decoding failed\n");

            free(response);
            free(copy);
            return;
        }

        body = decoded_body;
    }

    /* Print readable manual page */
    html_to_text(body, body_len);

    free(decoded_body);
    free(response);
    free(copy);
}
