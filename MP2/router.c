

#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <sys/wait.h>
#include <signal.h>


#include "uthash.h"


pthread_mutex_t lock;

void *get_in_addr(struct sockaddr *sa)
{
        if (sa->sa_family == AF_INET) {
                return &(((struct sockaddr_in*)sa)->sin_addr);
        }

        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

struct nodeInfo 
{
        int ID; 
        char hostName[15];
        char udpPort[15];
        char cost[15];  
        UT_hash_handle hh;
        int nextHop;
};
struct threadArgs
{
        char listen[15];
        char adr[15];
        int fd;
        char hst[15];
        
};
struct nodeInfo *table = NULL;
char ROUTER[15];		//ID for this->router
char listenOn[15];		//listener port for this->router
int ackflag = 0;

struct nodeInfo *findHash(int ID);
int saveHash(char *buf, int next);
int updateHash(int id, char *cost);
int sendLog(int sendID, char *mes, int type, int fd);
int logMe(int type, int fd);
void *udpConnection(void *args);
int sendMessage(int type);
int sendMessageHelper(char *buf, char *udp);	//Sending the udp message
void reverse(char s[]);
void itoa(int n, char s[]);
int receiveAck();


int main(int argc, char *argv[])
{

        char host[20];
        char managerPort[20]; // TCP port for Mr.Manager
        char listenOn[20]; //UDP port for this router
        int msgLen, msgRecv;
        
        strcpy(host, argv[1]);
        strcpy(managerPort, argv[2]);
        strcpy(listenOn, argv[3]);


/////////////////////////////////////////////////////////////////////////////////////// 
//TCP connection with Mr. Manager       
        struct addrinfo hints, *res, *p;        
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        int status,sockfd;
        char s[INET6_ADDRSTRLEN];
        char *hostname; 


          
        struct hostent *hent;
        hent = gethostbyname(argv[1]);
        hostname = hent -> h_name;

    if ( (status = getaddrinfo (hostname , managerPort, &hints, &res)   ) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s \n", gai_strerror(status));
        return 2;
    }
        
    //cycle through the blah list to find a valid entries
    for(p = res; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
        {
                perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
                close(sockfd);
                perror("client: connect");
            continue;
        }

        break;
        }
        
    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, (void *)get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);


//////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////

//CONNECTION ESTABLISHED WITH MR. MANAGER

        //Initiate handshake:  Helo to Mr. Manager    
    char *msg = "HELO\n";
    msgLen = strlen(msg);
    
    msgRecv = send(sockfd, msg, msgLen, 0);
    if(msgRecv != 5)
    {
        printf("Message is FUCKED UP, SON ! \n");
    }
    
    
    //Mr. Manager makes us self-aware of our identifier
    char recvBuf[1024];
    char *addr;
    char *space = " ";
    
    int recvLen = recv(sockfd, recvBuf, 1024, 0);
    strtok( recvBuf , space );
    addr = strtok( '\0' , "\n" );
    strcpy( ROUTER , addr);
    printf("manager replied with address %s\n", addr);     
    
    
    ///THREADING PARAMS
	struct threadArgs *threadArg = (struct threadArgs*) malloc (sizeof (struct threadArgs));
	strcpy(threadArg->listen, listenOn);
	strcpy(threadArg->hst, host);
	strcpy(threadArg->adr, addr); 
	threadArg->fd = sockfd;
        
        
        
                                              
    //Router informs Mr. Manager of it's UDP Port from main's param list
    //HOST localhost udpport
    char UDPinfo[50];
    strcpy(UDPinfo , "HOST localhost ");
    strcat(UDPinfo , listenOn);
         int UDPLen = strlen(UDPinfo);
    UDPinfo[UDPLen] = '\n';
    UDPLen = UDPLen+1;
    
    msgRecv = send(sockfd, UDPinfo, UDPLen, 0);
    if(msgRecv != UDPLen)
    {
        printf("UDP is FUCKED UP, SON ! \n");
    }
   

        //MR. Manager says "OK"     
    memset(recvBuf, 0, sizeof(recvBuf));
    recvLen = 0;
        recvLen = recv(sockfd, recvBuf, 1024, 0);
        
        //And then we say, "Who my neighbros??"
        char neighbors[50];
    strcpy(neighbors , "NEIGH?\n");                                                                     ////// '\n'??
    int neighLen = strlen(neighbors);
        //printf("MR.MANAGER NOT SENDING US SHIT 0");
    msgRecv = send(sockfd, neighbors, neighLen, 0);
    //printf("MR.MANAGER NOT SENDING US SHIT -1");
    if(msgRecv != neighLen)
    {
        printf("Neighbor request is FUCKED UP, SON ! \n");
    }
        
        //And then Mr. Manager stalls & thinks for a while until the rest of the nodes/terminals connect & then responds.


        int i,j;
        
        int success;    
        char *DONE = "DONE\n";
//   	printf("threadArg addr: %s\n" , addr);     
        while(1)                                                        ///////SPACE AT THE END? 
        {           
            j=0;
            
            
            memset(recvBuf, 0, sizeof(recvBuf));
                
                recvLen = recv(sockfd, recvBuf, 1024, 0);
                char newRecv[1024]; 
                strcpy(newRecv,recvBuf);
                if (recvLen == 0)
                {
                        printf("MR.MANAGER NOT SENDING US SHIT");
                        break;
                
                }
                
                
                //checking for "DONE" 
                for (i=0; i < recvLen ; i++)
                {
                        if ( recvBuf[i] == 'D')
                        {
                                j=i;
                                break;  
                        }
                }
                        

                if (strcmp(&recvBuf[j], DONE) == 0)
                {
                      //  memset( &recvBuf[j], 32 , (sizeof(recvBuf)-j));
                        strtok(recvBuf, space);
                        char *id = strtok('\0', space);
                        //Udp send to neigh's addr with it's own addr 
                        //Use udp send function call
                        
                        
                        if(j != 0)
                        {
			                printf(" ID is : %d\n", atoi(id));     //debug message                   
                        	if( saveHash(newRecv, atoi(id)) == 1 )
		                           printf("Final saved\n");                
		                }
                                               
                        break;
                }       
                strtok(recvBuf, space);
            
                char *id = strtok('\0', space);
                printf(" ID is : %d\n", atoi(id));	//debug message
                //Udp send to neigh's addr with it's own addr 
                if( saveHash(newRecv, atoi(id)) == 1)
                        printf("Saved\n");
                        
        }

/////////////////////////////////////THREADING BIDNESS////////////////////////////////  

        
        pthread_t thread_ID;
        void *exit_status;
        pthread_create(&thread_ID, NULL, udpConnection, threadArg);
/////////////////////////////////TCP THREAD///////////////////////////////////////////
		
		//UDP send neighs by iterating through hash
		//send all neighbor's linkcosts except the one we're informing
		int reply = sendMessage(0); 
		
		
		//Sleep till all control messages are passed
	   	sleep(60);

		//Send message "READY"
		msg = "READY\n";
		msgLen = strlen(msg);

		msgRecv = send(sockfd, msg, msgLen, 0);
		if(msgRecv != 6)
			printf("Message is FUCKED UP, SON ! \n");
		
    
    
		//Recieve Message "OK"
		memset(recvBuf, 0, sizeof(recvBuf));
		recvLen = 0;
		recvLen = recv(sockfd, recvBuf, 1024, 0);
		printf("Server says: %s \n", recvBuf);  //debug message
        

        

//////////////////////////////THIS PART REQUIRES IT'S OWN THREAD//////////////////////  

        
        
		//Enable logging at the router

		success = logMe(0, sockfd);
		if(success == 0)
			printf("Logging error\n");
        
        while(1)
        {
                //Recieve message "LINKCOST node1 node2 cost"
                memset(recvBuf, 0, sizeof(recvBuf));
                recvLen = 0;
                sleep(2);
                recvLen = recv(sockfd, recvBuf, 1024, 0);
                printf("Server says: %s \n", recvBuf); //debug message
                
                char message[15];
                strcpy(message, (strtok (recvBuf, space)));
        
                
                
                if( strcmp( message , "END\n") == 0)
                {
                        printf("END\n");
                        //Send BYE to Mr. Manager
								msg = "BYE\n";
								msgLen = strlen(msg);

								msgRecv = send(sockfd, msg, msgLen, 0);
								if(msgRecv != 4)
									printf("BYE is FUCKED UP, SON ! \n");
								free(threadArg);
								pthread_cancel(thread_ID);
                        
                        break;
                }
                
                if( strcmp( message , "LINKCOST") == 0)
                {
                        printf("LINKCOST command\n");                   //debug message
                
        
                        //Save the two node values i.e. node1 and node2
                        char *temptempID = strtok( '\0' , space );      
                        int node1 = atoi(temptempID );
                        temptempID = strtok( '\0' , space );    
                        int node2 = atoi(temptempID );
        
                        //Save the cost value
                        char tempCost[15];
                        strcpy( tempCost , strtok( '\0' ,space ) );
                        printf("Node1:%d Node2:%d Cost:%s",node1, node2, tempCost); //debug message
                        int node;
                        if(atoi(addr) == node1)
                                node = node2;
                        else
                                node = node1; 
                
                        success = updateHash(node, tempCost);
                        if(success != 1)
                                printf("\n Error updating Hash\n");
                        
                        //Udp send function call with the new linkcost, with its neighbors
                        //this-> Figure out way to break router message looping 
                        //is your job, roman
                        //Cool?
                        //Reply with message "COST cost OK"
                        
                        char msg[20] ;
                        memset(&msg , 0 , strlen(msg));
                        strcpy(msg, "COST ");
                        strcat(msg, tempCost);
                        char tempmsg[20];
                        memset(&tempmsg , 0 , strlen(tempmsg));
                        strncpy(tempmsg , msg , (strlen(msg) - 1) );
                        strncat(tempmsg, " OK\n" , 4);
                        msgLen = strlen(tempmsg);
                        msgRecv = send(sockfd, tempmsg, msgLen, 0);
                        
                        
                }
        }
        
        pthread_join(thread_ID, &exit_status);
        /*
        //Send message Bye to terminate all connections
        msg = "BYE\n";
        msgLen = strlen(msg);
        msgRecv = send(sockfd, msg, msgLen, 0);
        */
        
        //DO WE HAVE TO TERMINATE CONNECTIONS AS WELL??
        
        
        //Recieve Message "END"
/*      memset(recvBuf, 0, sizeof(recvBuf));
    recvLen = 0;
        recvLen = recv(sockfd, recvBuf, 1024, 0);
        printf("Server says: %s \n", recvBuf);*/
        

        
    freeaddrinfo(res); // all done with this structure
        
        return 0;
           
}  


void *udpConnection(void *args)
{
        
  	//	sleep(5);
        //TCP communication w/ Mr. Manager
        struct threadArgs *thrdArg = (struct threadArgs *)args;
        int sockfd = thrdArg->fd;
        char addr[15];
        strcpy(addr, thrdArg->adr);
        char host[15];
        strcpy(host, thrdArg->hst);
        
        strcpy(listenOn , thrdArg->listen);
        
        //UDP listen sockfd_udp
        int sockfd_udp;
        struct addrinfo hints_udp, *servinfo_udp, *p_udp;
        struct sockaddr_storage their_addr;
        int MAXBUFLEN = 100;
        char listenBuf[MAXBUFLEN],sendBuf[MAXBUFLEN];
        socklen_t addr_len;
        char s_udp[INET6_ADDRSTRLEN];
        int rv, numbytes;

        memset(&hints_udp, 0, sizeof hints_udp);
        hints_udp.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
        hints_udp.ai_socktype = SOCK_DGRAM;
        hints_udp.ai_flags = AI_PASSIVE; // use my IP

        if ((rv = getaddrinfo(NULL, listenOn , &hints_udp, &servinfo_udp)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        }

        // loop through all the results and bind to the first we can
        for(p_udp = servinfo_udp; p_udp != NULL; p_udp = p_udp->ai_next) {
                if ((sockfd_udp = socket(p_udp->ai_family, p_udp->ai_socktype,
                                p_udp->ai_protocol)) == -1) {
                        perror("listener: socket");
                        continue;
                }

                if (bind(sockfd_udp, p_udp->ai_addr, p_udp->ai_addrlen) == -1) {
                        close(sockfd_udp);
                        perror("listener: bind");
                        continue;
                }

                break;
        }

        if (p_udp == NULL) {
                fprintf(stderr, "listener: failed to bind socket\n");
                
        }

        freeaddrinfo(servinfo_udp);
////////////////////UDP THREAD/////////////////////////////////////////////////////////


		while(1)
			{

		    //recieve message/packet from mr. manager to fwd
		    //udp recieve 
		    
		    printf("listener: waiting to recvfrom...\n");

			//setup select to wait for connection ack
				  struct timeval tv;
			 fd_set readfds;

			 tv.tv_sec = 2;
			 tv.tv_usec = 500000;

			 FD_ZERO(&readfds);
			 FD_SET(sockfd_udp, &readfds);

			select(sockfd_udp+1, &readfds, NULL, NULL, &tv);
                
         if (FD_ISSET(sockfd_udp, &readfds))
			{
		    addr_len = sizeof their_addr;
		    if ((numbytes = recvfrom(sockfd_udp, listenBuf, MAXBUFLEN-1 , 0,
		            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
		            perror("recvfrom");
		            exit(1);
		    }
			}


		    if (numbytes == 0)
		    	continue;
		    
		    char sendBuf[100];
		    int i = 0;
		    //debug messages
		    printf("listener: got packet from %s\n",
		    inet_ntop(their_addr.ss_family,
		    get_in_addr((struct sockaddr *)&their_addr),
		                                    s_udp, sizeof s_udp));  
		    printf("listener: packet is %d bytes long\n", numbytes);
		    listenBuf[numbytes] = '\0';
		    
		    
		    //Copy listenBuf to sendBuf
		    memcpy ( (void *) sendBuf, (void *) listenBuf, numbytes);
		    
		    sendBuf[numbytes] = '\0';
		    
		 //   strcpy(sendBuf, listenBuf);
		    printf("listener: packet contains \"%s\"\n", listenBuf);	//debug message
		    printf("sendBuf contains : %s \n", sendBuf);				//debug message
		    

		    
		    
		    char type = listenBuf[0];
		    if((int)type == 1)
		    {
		            uint send_id = (int) ( listenBuf[2] |  (listenBuf[1] << 8) );
		            printf("send id is : %d\n", send_id);	//debug message


		            char data[50];
		            int k,l;
		            for(k =3,l=0 ; k< numbytes; k++, l++)
		            {
		                    data[l] = listenBuf[k] ; 
		                    printf("%c\n" , data[l]);       //debug message
		            }
		            data[l] = '\0';
		            
		            
		            printf("atoi of addr: %s\n", addr);		//debug message
		            if((int)send_id == atoi(addr))
		            //Message arrived at destination
		            {
		             		printf("LOG DICKS\n");
		                    sendLog(send_id, data, 1, sockfd);
		            }
		            else
		            {
		            
		            
		            	
		            	
		            	struct nodeInfo * sendPort = findHash(send_id);
		    			
		    			//Send only if ID of next hop == ID of send_id
		    			//Else lookup info for next hop
		    			while(sendPort->ID != sendPort->nextHop)
		    				sendPort = findHash(sendPort->nextHop);		//if(sendPort->ID != sendPort->nextHop)
		    				
		    			
		    			
							char dstPort[15];
							for(k = 0; k < 15; k++ )
						   		dstPort[k] = sendPort->udpPort[k];
		                   
							//UDP forward
							sendLog(sendPort->ID, data, 0, sockfd);
							
							

		                                            
                    		//Get external ip address
							struct ifaddrs * ifAddrStruct=NULL;
							struct ifaddrs * ifa=NULL;
							void * tmpAddrPtr=NULL;
							char addressBuffer[INET_ADDRSTRLEN];

							getifaddrs(&ifAddrStruct);

							for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
								if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
									// is a valid IP4 Address
									tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
								
									inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
									//printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
								} 
								else if (ifa->ifa_addr->sa_family==AF_INET6) { // check it is IP6
									// is a valid IP6 Address
									tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
									char addressBuffer[INET6_ADDRSTRLEN];
									inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
									//printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
								} 
							}
							if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);                       
		                    
		                    
		                     
		                    //UDP send sockfd_send
		                    struct addrinfo hints, *p;    
		                    int sockfd_send;
		                    struct addrinfo *servinfo_send;
		                    memset(&hints, 0, sizeof hints);
		                    hints.ai_family = AF_UNSPEC;
		                    hints.ai_socktype = SOCK_DGRAM;

		                    if ((rv = getaddrinfo(addressBuffer, dstPort, &hints, &servinfo_send)) != 0) {
		                      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		                     
		                    }

		                    // loop through all the results and make a socket
		                    for(p = servinfo_send; p != NULL; p = p->ai_next) {
		                      if ((sockfd_send = socket(p->ai_family, p->ai_socktype,
		                                             p->ai_protocol)) == -1) {
		                                    perror("talker: socket");
		                                    continue;
		                      }
		                      break;
		                   }

		                    if (p == NULL) {
		                      fprintf(stderr, "talker: failed to bind socket\n");
		                    
		                    }
		                    
		                    for(k = 0 ; k<numbytes; k++)
		            			printf("%c\n" , listenBuf[k]);
		                    
							printf("listenBuf length is: %d\n", strlen(listenBuf)); 	//debug message
		                    if ((numbytes = sendto(sockfd_send, listenBuf, numbytes, 0,
		                                             p->ai_addr, p->ai_addrlen)) == -1) {
		                              perror("talker: sendto");
		                              exit(1);
		                     }
		                    
		                     freeaddrinfo(servinfo_send);

		                     printf("talker: sent %d bytes to %s\n", numbytes, dstPort);
		                     close(sockfd_send);
		    
		            }
		    }       
		    else
		    //if((int)type == 0)
		    {
		    	char *space = " ";
		    	char message[10];
		    	char temp[100];
		    	char propogate[100];
		    	int identify, lookup;
				char tempID[10];
				char cst[10];
				char templookup[10];
				char savehsh[100];
				
		    	strcpy(temp, listenBuf);	
		    	//strcpy(propogate, listenBuf);	    	
		    	strcpy(savehsh, listenBuf);
				strcpy(message, strtok(listenBuf, space));
				if(strcmp(message, "ACK") != 0)
				{
					strtok(temp, space);
					strcpy(tempID, strtok('\0', space));
					strtok('\0', space);
					strtok('\0', space);
					strcpy(cst, strtok('\0', space));	
					strcpy(templookup, strtok('\0', space));
				
					if(strcmp(message, "NEW") == 0)
						saveHash(savehsh, 0);
					else if(strcmp(message, "UPDATE") == 0)
					{
						identify = atoi(tempID);
						updateHash(identify, cst);
					}
					//Send the ack
					lookup = atoi(templookup);
					struct nodeInfo *look = findHash(lookup);
				
					sendMessageHelper("ACK ", look->udpPort);
				
				}
				else
					ackflag = 1;
			
				//propagateControl(propogate);	
				

		    }
		    
		}		    

        close(sockfd_udp);
        return NULL;
} 

/*
struct nodeInfo 
{
        int ID; 
        char hostName[15];
        char udpPort[15];
        char cost[15];  
        UT_hash_handle hh;
        int nextHop;
};
*/

int propagateControl(char *stuff)
{
	//loop through hash table
	struct nodeInfo *curr , *neigh;
	char control[100];
	char *space = " ";
	char temp[100];
	strcpy(temp, stuff);
	int identify;
	char recieved[15];
	int recv;
	strtok(temp, space);
	strtok('\0', space);
	strtok('\0', space);
	strtok('\0', space);
	strtok('\0', space);
	strcpy(recieved, strtok('\0', space));
	recv = atoi(recieved);
	for(curr = table; curr != NULL; curr=curr->hh.next)
	{
		if(recv == curr->ID)
			continue;
		sendMessageHelper(stuff , curr->udpPort);
	}
}

int saveHash(char *buf, int next)
{
        
        printf("BUFFAH! = %s\n", buf);
        
        char *space = " ";
        struct nodeInfo *temp=NULL;
        struct nodeInfo *sendPort;
		char message[15];
		strcpy(message, strtok (buf, space ));
        int sendPortCost, tempCost;        
        char *temptempID = strtok( '\0' , space );      
        int tempID = atoi(temptempID );
        //HASH_FIND_INT(table, &tempID, temp);   /* id already in the hash? */
        
        
        if(temp == NULL)
        {
                temp = (struct nodeInfo*)malloc(sizeof(struct nodeInfo));
                sendPort = temp;
                //ENTRY item;   
                char *space = " " ;
                temp->ID = tempID;
                char *nline = "\n";
                strcpy( temp->hostName , strtok( '\0' ,space ) );
                strcpy(temp->udpPort , strtok( '\0' ,space ) );                         
                     
                if( strcmp(message, "NEW") == 0 || strcmp(message, "UPDATE") == 0)
				{
					strcpy (temp->cost ,  strtok( '\0' , space));
					tempCost = atoi(temp->cost);
					
					
					
					char *tempn = strtok('\0',space);
					printf(" Temp (next) is : %s\n", tempn); 	
					next = atoi(tempn);  
					printf("next is : %d \n", next); 		//debug message    
					temp->nextHop = next;
					
					while(sendPort->ID != sendPort->nextHop)
					{
		    			sendPort = findHash(sendPort->nextHop);
		    			sendPortCost = atoi(sendPort->cost);
		    			tempCost += sendPortCost;			 		
		    		}
					itoa(tempCost, temp->cost);
				}
				else
				{
					strcpy (temp->cost ,  strtok( '\0' , nline));
					temp->nextHop = next;
				}
				
						
				
		    	
        		
                //item.key =  strdup(temp.ID);
                printf(" Temp ID = %d\n", temp->ID);
                //item.data = &temp;
                printf( " Cost is : %s \n" , temp->cost );
        


                //store in Hash Table
                pthread_mutex_lock(&lock);
                HASH_ADD_INT(table, ID, temp);
                findHash(tempID);
                pthread_mutex_unlock(&lock);
                return 1;
        }	//loop through hash table

        
        else
                return 0;
        
}


      

struct nodeInfo *findHash(int ID)
{
        struct nodeInfo *s;
        HASH_FIND_INT(table, &ID, s);
        printf ("Hash # %d: %d , %s , %s,  %s, %d\n" , ID, s->ID , s->hostName ,s->udpPort , s->cost, s->nextHop); 
        //debug message
        return s;
} 



int updateHash(int id, char *cost)
{
        //First save the given node's updport and hostname
        struct nodeInfo *s;
        s = findHash(id);
        
        char tempUdpport[15];
        strcpy(tempUdpport, s->udpPort);
        char temphostName[15];
        strcpy(temphostName, s->hostName);
        int next = s->nextHop;
        
        int oldCost, newCost;
        oldCost = atoi(s->cost);
        newCost = atoi(cost);
        //Delete entry for the the particular node
        if( oldCost > newCost )
        {
		    HASH_DEL(table, s);
		    
		    //Create a new entry with updated info
		    struct nodeInfo *temp = NULL;
		    temp = (struct nodeInfo*)malloc(sizeof(struct nodeInfo));
		    temp->ID = id;
		    strcpy(temp->udpPort, tempUdpport);
		    strcpy(temp->hostName, temphostName);
		    strcpy(temp->cost, cost);
		    temp->nextHop = next;
		    HASH_ADD_INT(table, ID, temp);
		    findHash(id);					//debug message
		}
		else
			printf("Old cost is smallest\n");
			
        return 1;
}
        
int logMe(int type, int fd)
{
        int msgRecv;
        char log[50];
        if(type == 0)
        {
                strcpy(log, "LOG ON\n");
        }
        else if(type == 1)
        {
                strcpy(log, "LOG OFF\n");
        }
        int logLen = strlen(log);
        msgRecv = send(fd, log, logLen, 0);
        if(msgRecv != logLen)   
        printf("LOGGIN is FUCKED UP, SON ! \n");
   
   		memset(log, 0, sizeof(log));
  		msgRecv = 0;
        msgRecv = recv(fd, log, 1024, 0);
                printf("Log Me says: %s \n", log);      //debug message
   
   return 1;
}

int sendLog(int sendID, char *mes, int type, int fd)
{
        int msgRecv;
        char logger[50];
        char *nline = "\n";
        char *space = " " ;
        if(type == 0)
        {
                char sendid[15];
                strcpy(logger, "LOG FWD ");
                sprintf(sendid, "%d", sendID);
                //itoa(sendID, sendid, 10);             
                strcat(logger, sendid);
                strcat(logger,space);
                strncat(logger, mes,strlen(mes));               
                strcat(logger,nline);
        }
        else if(type == 1)
        {
                strcpy(logger, "RECEIVED ");
                strncat(logger, mes, strlen(mes));
                //strcat(logger, mes);
                strcat(logger,nline);
        }
        int loggerLen = strlen(logger);
        printf("received is: %s" , logger);		//debug message
        printf("\nloggerlen is: %d \n" , loggerLen);	//debug message
        msgRecv = send(fd, logger, loggerLen, 0);
        sleep(2);
        printf("send return bytes in SendLog: %d \n" , msgRecv);
        if(msgRecv != loggerLen)
                printf("Log not sent\n");
        if(msgRecv != 0)
                return 1;
                
                
      return 0;
}

/*
struct nodeInfo 
{
        int ID; 
        char hostName[15];
        char udpPort[15];
        char cost[15];  
        UT_hash_handle hh;
        int nextHop;
};
*/



//Sends control messages to neigh's
int sendMessage(int type)
{
	//loop through hash table
	struct nodeInfo *curr , *neigh;
	char control[100];
	char *array;
	char *space = " ";
	char id[15];
	
	for(curr = table; curr != NULL; curr=curr->hh.next)
	{
		//for each curr neighboring router, send the rest of the neighbors connected 
		for(neigh = table; neigh != NULL; neigh=neigh->hh.next)
		{
			if(curr == neigh)
				continue;
			//construct control message to distribute
			memset(control , 0 , 100);
			//strcat(control, "\0");
			//control = control+1;
			
			if(type == 0)
				strcat(control, "NEW ");
			else if(type == 1)
				strcat(control, "UPDATE ");
			
			itoa(neigh->ID, id);
			//snprintf (id, sizeof(id), "%d", neigh->ID); // print int 'n' into the char[] buffer
			strcat(control , id);
			strcat(control , space);
			strcat(control , neigh->hostName);
			strcat(control , space);
			strcat(control , neigh->udpPort);	
			strcat(control , space);
			strcat(control , neigh->cost);
			strcat(control , space);
			strcat(control , ROUTER);
			
			
			printf("control message is:%s \n" , control);
			ackflag = 0;
			sendMessageHelper(control , curr->udpPort);
			while(ackflag != 1)
				sleep(1);
			
			printf("ACK Received \n");					
		
		}
	}
	return 1;
}
/*
int receiveAck()
{

	int sockfd_udp;
	struct addrinfo hints_udp, *servinfo_udp, *p_udp;
	struct sockaddr_storage their_addr;
	int MAXBUFLEN = 100;
	char listenBuf[MAXBUFLEN];
	socklen_t addr_len;
	char s_udp[INET6_ADDRSTRLEN];
	int rv, numbytes;

	memset(&hints_udp, 0, sizeof hints_udp);
	hints_udp.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints_udp.ai_socktype = SOCK_DGRAM;
	hints_udp.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, listenOn , &hints_udp, &servinfo_udp)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}

	// loop through all the results and bind to the first we can
	for(p_udp = servinfo_udp; p_udp != NULL; p_udp = p_udp->ai_next) {
		if ((sockfd_udp = socket(p_udp->ai_family, p_udp->ai_socktype,
		                p_udp->ai_protocol)) == -1) {
		        perror("listener: socket");
		        continue;
		}

		if (bind(sockfd_udp, p_udp->ai_addr, p_udp->ai_addrlen) == -1) {
		        close(sockfd_udp);
		        perror("listener: bind");
		        continue;
		}

		break;
	}

	if (p_udp == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		
	}

	freeaddrinfo(servinfo_udp);
	
	printf("Ack: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd_udp, listenBuf, MAXBUFLEN-1 , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
    }
    
    
    
    //debug messages
    printf("listener: got packet from %s\n",
    inet_ntop(their_addr.ss_family,
    get_in_addr((struct sockaddr *)&their_addr),
                                    s_udp, sizeof s_udp));  
    printf("listener: packet is %d bytes long\n", numbytes);
    listenBuf[numbytes] = '\0';
    if(strcmp(listenBuf, "ACK") == 0)
    	return 1;
    else
    	printf("ACK not received \n");
    	
    return 0;
}*/

//Actual udp send
int sendMessageHelper(char *buf , char* udp)
{
	struct ifaddrs * ifAddrStruct=NULL;
	struct ifaddrs * ifa=NULL;
	void * tmpAddrPtr=NULL;
	char addressBuffer[INET_ADDRSTRLEN];

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
			// is a valid IP4 Address
			tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
	
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			//printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
		} 
		else if (ifa->ifa_addr->sa_family==AF_INET6) { // check it is IP6
			// is a valid IP6 Address
			tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
			char addressBuffer[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
			//printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
		} 
	}
	if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);                       
		   
		  //UDP send sockfd_send
		  int numbytes, rv;
		  struct addrinfo hints, *p;    
		  int sockfd_send;
		  struct addrinfo *servinfo_send;
		  memset(&hints, 0, sizeof hints);
		  hints.ai_family = AF_UNSPEC;
		  hints.ai_socktype = SOCK_DGRAM;

		  if ((rv = getaddrinfo(addressBuffer, udp, &hints, &servinfo_send)) != 0) {
		    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		   
		  }

		  // loop through all the results and make a socket
		  for(p = servinfo_send; p != NULL; p = p->ai_next) {
		    if ((sockfd_send = socket(p->ai_family, p->ai_socktype,
		                           p->ai_protocol)) == -1) {
		                  perror("control mess: socket");
		                  continue;
		    }
		    break;
		  }

		  if (p == NULL) {
		    fprintf(stderr, "control mess: failed to bind socket\n");
		  
		  }

		  if ((numbytes = sendto(sockfd_send, buf, strlen(buf), 0,
		                           p->ai_addr, p->ai_addrlen)) == -1) {
		            perror("control mess: sendto");
		            exit(1);
		   }
		   freeaddrinfo(servinfo_send);

		   printf("control mess: sent %d bytes to %s\n", numbytes, udp);
		   close(sockfd_send);
}		    

/* itoa:  convert n to characters in s */
void itoa(int n, char s[])
{
 int i, sign;

 if ((sign = n) < 0)  /* record sign */
     n = -n;          /* make n positive */
 i = 0;
 do {       /* generate digits in reverse order */
     s[i++] = n % 10 + '0';   /* get next digit */
 } while ((n /= 10) > 0);     /* delete it */
 if (sign < 0)
     s[i++] = '-';
 s[i] = '\0';
 reverse(s);
}

/* reverse:  reverse string s in place */
void reverse(char s[])
{
 int i, j;
 char c;

 for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
     c = s[i];
     s[i] = s[j];
     s[j] = c;
 }
}
 
 
