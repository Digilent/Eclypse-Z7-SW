/**
 * @file transfer/eth/eth.c
 * @author Nita Eduard
 * @date 10 Nov 2021
 * @brief File containing implementations for sending data via a TCP/IP connection.
 */
#include "eth.h"

/**
 * Initializes a TCP socket and binds it to a given port.
 *
 * @param port the port used for the connection
 *
 * @return 	server socket if the connection was set up successfully,
 * 			-1 if an error occurred when creating the socket,
 * 			-2 if an error occurred when binding the socket
 */
int fnEthInitConnection(int port) {
	int rc, server_sock;
	struct sockaddr_in server_addr;
	uint32_t send_buffer_size;
	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(server_sock < 0) {
		perror("Error creating socket");
		return -1;
	}
	unsigned int option = 1;
	setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	rc = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(rc < 0) {
		perror("Error binding socket");
		close(server_sock);
		return -2;
	}

	send_buffer_size = 65536;
	setsockopt(server_sock, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, sizeof(send_buffer_size));

	return server_sock;
}

/**
 * Start listening for a single connection on a given socket and
 * then wait for a connection to it.
 *
 * @param sockfd the server socket of the application
 *
 * @return 	client socket if a client connected successfully,
 * 			-1 if an error occurred when listening for a client,
 * 			-2 if an error occurred when accepting the connection
 */
int fnEthWaitForConnection(int sockfd)
{
	int client_sock;
	struct sockaddr_in client_addr;
	socklen_t addr_size = sizeof(client_addr);

	if(listen(sockfd, 1) != 0) {
		perror("Error in listening for connection");
		return -1;
	}
	client_sock = accept(sockfd, (struct sockaddr*)&client_addr, &addr_size);
	if(client_sock < 0) {
		perror("Error accepting socket");
		return -2;
	}

	return client_sock;
}

/**
 * Send data to a client socket using a buffer.
 *
 * @param client_sock 	the client socket of the application
 * @param buffer 		a buffer holding the data to be sent
 * @param size 			the size of the buffer to be sent
 *
 * @return 	 0 if the data was sent successfully,
 * 			-1 if the buffer size was zero or less,
 * 			-2 if an error occurred when sending the data
 */
int fnEthSendData(int client_sock, uint8_t* buffer, size_t size)
{
	int rc;
    const uint8_t* current_pointer = buffer;

	while (size > 0) {
		rc = send(client_sock, current_pointer, size, 0);
		if(rc < 0) {
			perror("Failed to send data to client");
			return -2;
		}
		size -= rc;
		current_pointer += rc;
	}
	return 0;
}

/**
 * Send data to a client socket using a buffer.
 *
 * @param client_sock 	the client socket of the application
 * @param buffer 		a buffer holding the data to be sent
 * @param size 			the size of the buffer to be sent
 *
 * @return 	 0 if the data was sent successfully,
 * 			-1 if the buffer size was zero or less,
 * 			-2 if an error occurred when sending the data
 */
int fnEthRecvData(int client_sock, uint8_t* buffer, size_t size)
{
	int rc;

    while (size > 0) {
		rc = recv(client_sock, buffer, size, 0);
		if(rc < 0) {
			perror("Failed to receive data from client");
			return -2;
		}

		size -= rc;
		buffer += rc;

	}
	return 0;
}
