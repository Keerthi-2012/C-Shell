#include "networking.h"
void fetch_man_page(char *input)
{
    input[strcspn(input, "\n")] = '\0';
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char request[buf_size], response[buf_size];
    int bytes_received;
    char* command = (char*)malloc(sizeof(char)*buf_size);
    char* token = (char*)malloc(sizeof(char)*buf_size);
    token = strtok(input," \t");
    int i = 0;
    while (token!=NULL)
    {
        if(i==0){
            if(strcmp(token,"iMan")!=0){
                printf("%s: not valid\n",token);
                return;
            }
            i++;
        }
        else if(i==1){
            strcpy(command,token);
            break;
        }
        token = strtok(NULL," \t");
    }
    char path[100];
    snprintf(path, sizeof(path), "/?topic=%s&section=all", command);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);//(Address family, Type of socket, Protocol)-“Hey, I want to create a TCP socket that uses IPv4 addressing.” to OS
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server = gethostbyname("man.he.net");
    if (server == NULL)
    {
        fprintf(stderr, "Error: No such host\n");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));//Initializes the entire server_addr structure to zeros to avoid garbage values.
    server_addr.sin_family = AF_INET;//Specifies the address family: AF_INET means IPv4. Required for IPv4 socket communication.
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);//Copy IP address from DNS result into socket structure
    server_addr.sin_port = htons(80);//Set port number to 80 for HTTP (with correct byte order)

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)//initiates the TCP connection to the web server
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             path, "man.he.net");
    // printf("%s\n", request);
    send(sockfd, request, strlen(request), 0);

    int n;
    int header_end = 0;
    char *body_start = (char *)malloc(sizeof(char) * buf_size);
    char *tag_start, *tag_end;

    while ((n = read(sockfd, response, sizeof(response) - 1)) > 0)
    {
        response[n] = '\0';
        const char *header_end = strstr(response, "\n\n");
        if (header_end)
        {
            strcpy(body_start, header_end + 2);
        }
        char *current_pos = body_start;
        while ((tag_start = strchr(current_pos, '<')) != NULL)// Text between tags is printed. Tags are skipped. Any remaining trailing content is also printed
        {
            if (tag_start > current_pos)
            {
                fwrite(current_pos, 1, tag_start - current_pos, stdout);
            }

            tag_end = strchr(tag_start, '>');
            if (tag_end)
            {
                current_pos = tag_end + 1;
            }
            else
            {
                fwrite(tag_start, 1, strlen(tag_start), stdout);
                break;
            }
        }

        if (*current_pos)
        {
            printf("%s", current_pos);
        }
    }

    if (n < 0)
    {
        perror("Error reading response");
    }

    close(sockfd);
}

// int main()
// {
//     char a[100];
//     fgets(a, 100, stdin);
//     fetch_man_page(a);
//     return 0;
// }
