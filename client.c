#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>

#define PORT 2000

int main(void) {

    uid_t uid = getuid();
    char uid_str[10];
    snprintf(uid_str, sizeof(uid_str), "%d", uid);

    gid_t gid = getgid();
    char gid_str[10];
    snprintf(gid_str, sizeof(gid_str), "%d", gid);

    int socket_desc;
    struct sockaddr_in server_addr;
    FILE *file_to_send;
    char file_buffer[1024];
    size_t bytes_read;

    char filepath[256];

    char message_from_server[1024];
    ssize_t bytes_received;

    printf("Enter the file path and filename: ");
    scanf("%255s", filepath); 

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_desc < 0) {
        perror("Unable to create socket");
        return -1;
    }

    printf("Socket created successfully\n");

    // Set port and IP address to match the server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); // Change to the desired port
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Change to the server's IP

    // Send connection request to the server
    if (connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Unable to connect");
        return -1;
    }
    printf("Connected with server successfully\n");

    //check if it can send data
    char Authorization[1024]; 
    snprintf(Authorization, sizeof(Authorization), "%s:%s:%s", uid_str, gid_str, filepath);

    if (send(socket_desc, Authorization, strlen(Authorization), 0) < 0) {
    perror("Can't send data");
    return -1;
    }

    // Receive the authorazation message from the server
    bytes_received = recv(socket_desc, message_from_server, sizeof(message_from_server), 0);

    if (bytes_received < 0) {
        perror("Error while receiving status message");
        return -1;
    }

    // Null-terminate the received data
    message_from_server[bytes_received] = '\0';
    printf("Received message: [%s]\n", message_from_server);

    //Authorazation handling
    if(strcmp(message_from_server,"yes") == 0){
        printf("You are authorized to send file, sending now...\n");
        
    }else{
        printf("You are not authorized to send file\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(filepath, "wa");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Sending the filepath as metadata
    send(socket_desc, filepath, strlen(filepath), 0);

    // Open the file to send
    file_to_send = fopen(filepath, "rb"); // Open the file for binary reading

    if (file_to_send == NULL) {
        perror("Couldn't open the file for reading");
        return -1;
    }

    // Send the file data
    while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file_to_send)) > 0) {
        if (send(socket_desc, file_buffer, bytes_read, 0) < 0) {
            perror("Can't send file data");
            return -1;
        }
    }

    printf("File sent successfully: %s\n", filepath);

    // Close the file and socket
    fclose(file_to_send);
    

    // Receive the status message from the server
    bytes_received = recv(socket_desc, message_from_server, sizeof(message_from_server), 0);

    if (bytes_received < 0) {
        perror("Error while receiving status message");
        return -1;
    }

    // Null-terminate the received data
    message_from_server[bytes_received] = '\0';

    // Print the status message
    printf("Status from server: %s\n", message_from_server);
    close(socket_desc);
    return 0;
}