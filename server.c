#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>


#define PORT 2000 // Change to the desired port

void handle_client(void *arg) {
    int client_sock = *((int *)arg); // Cast arg to int pointer and dereference to get client_sock
    FILE *received_file;
    char file_buffer[1024];
    size_t bytes_received;
    pthread_mutex_t lock;
    char Authorization[1024];  
    const char *message_to_client;

    pthread_mutex_lock(&lock);

    bytes_received = recv(client_sock, Authorization, sizeof(Authorization), 0);
        if (bytes_received < 0) {
        perror("Server: Error while receiving data");
    
        }

    Authorization[bytes_received] = '\0';

    char *uid_str = strtok(Authorization, ":");
    char *gid_str = strtok(NULL, ":");
    char *filepath = strtok(NULL, ":");

    if (uid_str == NULL || gid_str == NULL || filepath == NULL) {
    perror("Server: Error parsing received data");
    exit(EXIT_FAILURE);
    }

    // Convert UID and GID back to integer
    uid_t uid = atoi(uid_str);
    gid_t gid = atoi(gid_str);

    const char *directory_path = filepath; 
    struct stat dir_stat;

    if (stat(directory_path, &dir_stat) == 0) {
        if(dir_stat.st_uid != uid || dir_stat.st_gid != gid) {
            message_to_client = "Server: Authorization failure: UID or GID do not match";
            send(client_sock, message_to_client, strlen(message_to_client), 0);
            exit(EXIT_FAILURE);
        }else{
            printf("Server: UID and GID match, you are authorize to access\n");
            message_to_client = "yes";
            send(client_sock, message_to_client, strlen(message_to_client), 0);
        }
    } else {
        perror("Server: Error getting directory information");
        exit(EXIT_FAILURE);
    }

    printf("63\n");
    // Create or open the file for writing received data
    received_file = fopen("/Users/armando/Desktop/SSCA2/SSCA2/Manufacturing/test.txt", "w"); // Open the file for writing
    printf("66\n");
    if (received_file == NULL) {
        perror("Server: Couldn't create the file for writing");
        message_to_client = "Transfer failed";
        send(client_sock, message_to_client, strlen(message_to_client), 0); 
    }
    printf("72\n");
    // Receive data from the client and write it to the file
    bytes_received = recv(client_sock, file_buffer, sizeof(file_buffer), 0);
        printf("Reading data or waiting at least\n");
        size_t bytes_written = fwrite(file_buffer, 1, bytes_received, received_file);
        printf("Data got written\n");
        if (bytes_written != bytes_received) {
            perror("Server: Error writing to file");
        }
    
    printf("77\n");
    // Closing the socket and file:
    fclose(received_file);
    pthread_mutex_unlock(&lock);
    printf("81\n");
    message_to_client = "Server: Transfer sucessfully completed";
    send(client_sock, message_to_client, strlen(message_to_client), 0);
    printf("84\n");
}


int main(void) { 
    int socket_desc, client_sock;
    socklen_t client_size;
    struct sockaddr_in server_addr, client_addr;
    pthread_t thread_id;


    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_desc < 0) {
        perror("Error while creating socket");
        return -1;
    }
    printf("Socket created successfully\n");

    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind to the set port and IP:
    if (bind(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Couldn't bind to the port");
        exit(EXIT_SUCCESS);
    }
    printf("Done with binding\n");

    // Listen for clients:
    if (listen(socket_desc, 1) < 0) {
        perror("Error while listening");
         exit(EXIT_SUCCESS);
    }
    printf("\nServer: Listening for incoming connections.....\n");

    // Accept an incoming connection:
    while(1){
        
        client_size = sizeof(client_addr);
        client_sock = accept(socket_desc, (struct sockaddr *)&client_addr, &client_size);

        if (client_sock < 0) {
            perror("Server: Can't accept");
            exit(EXIT_SUCCESS);
        }
        printf("Server: Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Create a new thread to handle the client
        if (pthread_create(&thread_id, NULL, (void *(*)(void *))handle_client, &client_sock) < 0) {
            perror("Server: Couldn't create thread");
            exit(EXIT_SUCCESS);
        }
        // Wait for the thread to finish
        if (pthread_join(thread_id, NULL) != 0) {
            exit(EXIT_SUCCESS);
        }
        printf("Waiting for new client connections...\n");
    }
  
    close(client_sock);
    close(socket_desc);

    return 0;
}