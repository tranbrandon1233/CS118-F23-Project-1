#include <regex.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }
    
    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = malloc(strlen(buffer) + 1);
    strcpy(request, buffer);
    puts(request);

    // TODO: Parse the header and extract essential fields, e.g. file name
    char* filePath;
    // Regex extraction to get filename
    int status;
    regex_t re;
    if (regcomp(&re, "GET /([^ ]*) .* ", REG_EXTENDED) != 0) {
      // Invalid request, want to return 400 error mayber
    } else {
      regmatch_t pmatch[2];
      status = regexec(&re, request, 2, pmatch, 0);
      printf("strlen(request): %lu\nsubmatch offsets are %lld %lld\n", strlen(request), pmatch[1].rm_so, pmatch[1].rm_eo);
      regfree(&re);
      int length = pmatch[1].rm_eo-pmatch[1].rm_so;
      filePath = malloc(length+1);
      strncpy(filePath, &request[5], length);
      filePath[length] = 0;
      puts(filePath);
    }
    // Hint: if the requested path is "/" (root), default to index.html
    if (filePath!=NULL && strcmp(filePath,"")==0) {
      serve_local_file(client_socket, "index.html");
    }
    else {
      serve_local_file(client_socket, filePath);
    }
    /* Temporarily removed section for proxy part, the rest of the if branches to determine content type belong in the function below
     else if(strstr(file_name,".ts") != NULL){
    	return proxy_remote_file(app,client_socket,file_name);
    }
    else {
      exit(EXIT_FAILURE);
    }
     */
    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    // Will run if file is valid and not .ts
     //}
}

// Small helper function that computes # of digits in the input n
int numlen(long n) {
  int count = 0;
  while (n>0) {
    n /= 10;
    count++;
  }
  return count;
}

void serve_local_file(int client_socket, const char *path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response

    // Determining content type from file extension
    char* content_type = "";
    if (strstr(path, ".") == NULL){
	    content_type = "Content-Type: application/octet-stream\r\n";
    }	
    else if(strstr(path, ".html") != NULL) {
	    content_type = "Content-Type: text/html\r\n";
    }
    else if(strstr(path, ".txt") != NULL) {
	    content_type = "Content-Type: text/plain\r\n";
    }
    else if(strstr(path, ".jpg") != NULL) {
	    content_type = "Content-Type: image/jpeg\r\n";
    }
    
    // Predefine the strings/content that may go into the response
    char* statusLine;
    char* content;
    // Try to read/open file
    FILE* ptr;
    long fsize;
    ptr = fopen(path, "r");
    if (ptr==NULL) {
      // Set status code to 404 (not found)
      statusLine = "HTTP/1.0 404 Not Found\r\n";
    } else {
      // Set status code to 200 (ok)
      statusLine = "HTTP/1.0 200 OK\r\n";
      fseek(ptr, 0, SEEK_END);
      fsize = ftell(ptr);
      fseek(ptr, 0, SEEK_SET);
      content = malloc(fsize + 1);
      fread(content, fsize, 1, ptr);
      fclose(ptr);
      content[fsize] = 0;
    }
    char* response = malloc(strlen(statusLine)+strlen(content_type)+strlen(content)+numlen(fsize)+21);
    sprintf(response, "%s%sContent-Length %ld\r\n\r\n%s", statusLine, content_type, fsize, content);

    /*
    char responseDemo[] = "HTTP/1.0 200 OK\r\n"
                      "Content-Type: text/plain; charset=UTF-8\r\n"
                      "Content-Length: 15\r\n"
                      "\r\n"
                      "Sample response";
    */
    send(client_socket, response, strlen(response), 0);
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response
	
    char response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    send(client_socket, response, strlen(response), 0);
    // Create the proxy server socket
    int proxySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (proxySocket < 0){
        perror("Error opening proxy socket");
	exit(EXIT_FAILURE);
    }
    struct sockaddr_in proxyAddr;
    bzero((char *) &proxyAddr, sizeof(proxyAddr));
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_addr.s_addr = INADDR_ANY;
    proxyAddr.sin_port = htons(app->server_port);

    if (bind(proxySocket, (struct sockaddr *) &proxyAddr, sizeof(proxyAddr) < 0)){
        perror("Error on binding");
	exit(EXIT_FAILURE);
    }
    listen(proxySocket, 5);
    printf("Proxy server is listening on port %d...\n", app->server_port);

    while (1) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(proxySocket, (struct sockaddr *) &clientAddr, &clientLen);
        if (clientSocket < 0){
            perror("Error accepting client connection");
	    exit(EXIT_FAILURE);
	}
        printf("Accepted connection from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        int remoteSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (remoteSocket < 0){
            perror("Error opening remote socket");
	    exit(EXIT_FAILURE);
	}
        struct sockaddr_in remoteAddr;
        bzero((char *) &remoteAddr, sizeof(remoteAddr));
        remoteAddr.sin_family = AF_INET;
        remoteAddr.sin_port = htons(app->remote_port);

        if (inet_pton(AF_INET, app->remote_host, &remoteAddr.sin_addr) <= 0){
            perror("Invalid remote host address");
	    exit(EXIT_FAILURE);
	}
        if (connect(remoteSocket, (struct sockaddr *) &remoteAddr, sizeof(remoteAddr)) < 0) {
            // Connection to the remote server failed, send a "Bad Gateway" response
            char badGateway[] = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
            send(clientSocket, badGateway, strlen(badGateway), 0);
	    exit(EXIT_FAILURE);
        } else {
            // Forward the original request to the remote server
            char buffer[BUFFER_SIZE];
            int bytesRead;

            while ((bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
                send(remoteSocket, buffer, bytesRead, 0);
                memset(buffer, 0, BUFFER_SIZE);

                bytesRead = recv(remoteSocket, buffer, BUFFER_SIZE, 0);
                send(clientSocket, buffer, bytesRead, 0);
                memset(buffer, 0, BUFFER_SIZE);
            }
        }

        close(clientSocket);
        close(remoteSocket);
    }

    close(proxySocket);
}
