#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdio.h>
  
#define PORT         8080 
#define MAX_LINE     1024 
#define NAME_LEN     20
#define CREATION_LEN 50

typedef struct 
{
    int index;
    char name[NAME_LEN];
    char mat[64]; // glm::mat4
} payload;

typedef struct
{
    int index;
    char name[NAME_LEN];
    char gltf[CREATION_LEN];
    char start; // when all players set start to 1 the game will be started
} creation_payload;

creation_payload* resize(creation_payload* data, int count, int capacity)
{
    creation_payload* new_data = (creation_payload*)malloc(sizeof(creation_payload) * capacity);
    memcpy(new_data, data, count * sizeof(creation_payload));
    free(data);
    return new_data;
}

int main() 
{ 
    int players_capacity = 2;
    int current_index = 0;
    int started_players = 0;

    payload* players = NULL;
    creation_payload *players_data = malloc(sizeof(creation_payload) * players_capacity); 

    int sockfd; 
    char buffer[MAX_LINE]; 
    struct sockaddr_in servaddr, cliaddr; 
      
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
      
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    socklen_t len;
    int n; 
  
    len = sizeof(cliaddr);
    creation_payload p;

    // prepare stage
    while(1) 
    {
        memset(buffer, 0, MAX_LINE);
        n = recvfrom(sockfd, (char *)buffer, MAX_LINE,  
                    MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                    &len); 

        memcpy(&p, buffer, sizeof(p));

        if (p.index == -1) 
        {
            printf("%s\n", p.name);
            p.index = current_index;
            players_data[current_index] = p;    
            sendto(sockfd, &p.index, sizeof(p.index), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
            ++current_index;
        }

        if (current_index == players_capacity) {
            players_capacity *= 2;
            players_data = resize(players_data, current_index, players_capacity);
        }

        if (p.start == 1) 
        {
            sendto(sockfd, (void*)players_data, sizeof(creation_payload) * current_index, MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
            printf("%d\n", (int)sizeof(creation_payload) * current_index);
            
            ++started_players;
        }

        if (started_players == current_index) break;

    }

    players = malloc(sizeof(payload) * current_index);
    for (int i = 0; i < current_index; ++i) 
    {
        players[i].index = players_data[i].index;
        memcpy(players[i].name, players_data[i].name, NAME_LEN);
        memset(players[i].mat, 0, 64);
    }

    // main loop
    while (1)
    {
        memset(buffer, 0, MAX_LINE);
        n = recvfrom(sockfd, (char *)buffer, MAX_LINE,  
                    MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                    &len); 
        payload p;
        memcpy(&p, buffer, sizeof(p));
        memcpy(players[p.index].mat, p.mat, sizeof(p.mat));

        sendto(sockfd, (void*)players, sizeof(payload) * current_index,  
            MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
                len); 
    }

    return 0; 
}