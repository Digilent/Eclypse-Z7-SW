/**
 * @file transfer/eth/eth.h
 * @author Nita Eduard
 * @date 10 Nov 2021
 * @brief Function declarations used for ethernet data logging.
 */
#ifndef ETH_H
#define ETH_H
#include <sys/resource.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>

int fnEthInitConnection(int port);
int fnEthWaitForConnection(int sockfd);
int fnEthSendData(int client_sock, uint8_t* buffer, size_t size);
int fnEthRecvData(int client_sock, uint8_t* buffer, size_t size);
int fnEthCloseConnection(int server_sock, int client_sock);

#endif
