#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>


void *get_in_addr(struct sockaddr *sa)
{
        if (sa->sa_family == AF_INET) {
                return &(((struct sockaddr_in*)sa)->sin_addr);
        }

        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


//header file with function & struct declarations?? or just put them at top of file
struct frame {
unsigned short int length;
unsigned long int file_posn;
char Data[1016]; 
char CRC_12[2]; 

struct frame * prev;
};


int enqueue (struct frame * new_frame);
struct frame * dequeue (void);
int crcChecker(char *buf);


//GLOBAL Vars for queue??
struct frame * front = NULL;   // index of front of queue
struct frame * back  = NULL;   // index of back of queue
int size= 0;                   // # frames in queue


// Queue implemented as a linked list

//ENQUEUE
//1 == success, 0 == failure
int enqueue (struct frame * new_frame) {
		printf("\n Size : %d\n ", size);

        if (new_frame == NULL) return 0;

                //if ( size == 1 ) fork(sleep) child write thread
           
                
        //empty queue , add 1st frame
        if (size == 0 ) {
                front = new_frame;
                back  = new_frame;
                new_frame->prev = NULL;
                //return 1;
                }

        back->prev = new_frame;
        back  = new_frame;
        new_frame->prev = NULL;
        size++;
        printf("\n Size : %d\n ", size);

        return 1;
}

//DEQUEUE
//returns pointer to front of queue , NULL if empty
struct frame * dequeue (void){

        if (front == NULL || size == 0) return NULL;
				struct frame * temp = front;

				front = front->prev;
				size--;

				//dequeued last frame
				if(size == 0) back = NULL;
        return temp;
}













///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char *argv[] )
{
        // mp1client <server> <port #> <output file>

        char server[40];
        char port[40];
        
        char output_file[40];
        
        //argv[0] == 'mp1client'
        strcpy(server , argv[1]);
        strcpy(port , argv[2]);
        strcpy( output_file , argv[3]);
        
        //converts string to int
        //port = atoi(port_char);
             
               
        // 1)
        //sockets stuff gets FD
        // Beej's guide & mp0 from CS 438

        // setting up the struct 
        struct addrinfo hints, *res, *p;
        int status,sockfd;
        char s[INET6_ADDRSTRLEN];
        char *hostname; 
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        
        struct hostent *hent;
        hent = gethostbyname(argv[1]);
        hostname = hent -> h_name;

        if ( (status = getaddrinfo ( hostname , port, &hints, &res)   ) != 0){
                    fprintf(stderr, "getaddrinfo: %s \n", gai_strerror(status));
                    return 2;
                        }
        
        //cycle through the blah list to find a valid entries
        for(p = res; p != NULL; p = p->ai_next) {
                                  if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
                                          perror("client: socket");
                                          continue;
                                  }

                                  if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                                          close(sockfd);
                                          perror("client: connect");
                                          continue;
                                  }

                                  break;
                        }
        
        if (p == NULL) {
                fprintf(stderr, "client: failed to connect\n");
                return 2;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
        printf("client: connecting to %s\n", s);

        freeaddrinfo(res); // all done with this structure

  


        //Should be connected by now!


                
         // 2)
         //Read frames from server! 1 iteration == 1 frame
                        //while loop, run while mp1_read is returning frames                              
                        
                                int numbytes, offset;
                                char * buf; 
                           buf = (char *) malloc(1025);
                                if(buf==NULL) exit (1); 
                        		 
                                 while(1)
                                 {
                                          int flag=0;
                                                
                                          int total=1024;
                                          offset=0;
                                          while(offset!=total)
                                          {
                                                       
                                                       numbytes = MP1_read(sockfd, buf+offset, 1024-offset);
                                                       
                                                        
                                                        offset+=numbytes;
                                                        //printf(" Numbytes : %d\n", offset);
                                                        if(numbytes==0)
                                                        {
                                                                 printf("%s","End of all data");
                                                                 flag=1;
                                                                 close(sockfd);
                                                                 break;
                                                        }
                                          }

                                          if(flag==1)
                                          {
                                                        break;
                                          }

                                          buf[1024]='\0';
                                          
                                          // 3) CRC CHECK
                                          int check=crcChecker(buf);

                                          if(check==1)
                                          {
                                                        continue;
                                                        printf("\n CRC Failed \n");
                                          }
                                          printf("\n CRC Passed \n");
                                          
                                          // 4) Create & populate Frame struct to be enqued once CRC passes
                                          
                                          struct frame * temp = (struct frame * ) malloc(sizeof(struct frame));
                                          
                                          memcpy(&temp->length,buf,2);
                                          temp->length = ntohs(temp->length);
                                          memcpy(&temp->file_posn,buf+2,4);
                                          temp->file_posn = ntohl(temp->file_posn);
                                          memcpy(temp->Data,buf+6,1016);
                                          memcpy(temp->CRC_12,buf+1022,2);
                                          int enq = enqueue(temp);
                                          if(enq == 1)
                                          	printf(" \nEnqueued\n ");
                                          
                                          
                                          
                                 }
                                 free(buf);

           


////////////////////////////////////////////////////////////////////////////////
/*
Done enquing the correct frames for the file
//deque them & write them to file @ correct file_posn!
*/
////////////////////////////////////////////////////////////////////////////////


                // 5)
                // Write frame
                // if EOF, send empty frame
             
                struct frame * write_frame;
                FILE * ret = fopen( output_file , "w" );
        
                while (size != 0){
                                 write_frame = dequeue();   
                                 
                                 //printf("%d\n" , size);
                                 
                                 //printf("%s" , write_frame->Data);     
                                 
                                 if(fseek(ret, write_frame->file_posn , SEEK_SET) != 0)
                                         printf("seek failed");

                                
                                //if ( fwrite(write_frame->Data, sizeof(char), write_frame->length , ret) == 0) printf("Dun with FIle");
                                 if( MP1_fwrite (write_frame->Data, sizeof(char), write_frame->length , ret) != write_frame->length )
                                 printf("fwrite failed");
                        
                                free(write_frame);

                }


}


/*
crcChecker
return: 0 success , 1 failure
pseudo code revised from:
http://en.wikipedia.org/wiki/Computation_of_CRC#Implementation
*/
int crcChecker(char *buf)
     



{
    char *check=(char *)malloc(1025);
    char *pointer=check;
    check[1024]='\0';
    memcpy(check,buf,1024);
    short remainder=0;
    int i=0;
    for(i=0;i<1022;i++)
    {
        remainder= remainder ^(*check<<4);
        check++;
        int j;
        for(j=0;j<8;j++)
        {
            if(remainder & 0x800)
            {
                remainder=(remainder<<1)^0x180D;
            }
            else
            {
                remainder=remainder<<1;
            }
            remainder = remainder & 0xfff;
        }
    }
    short checksum=*((short *)(buf+1022));
    free (pointer);
        if(remainder == ntohs(checksum))
        {
                return 0;
        }
        return 1;
        
}


///Garbage code .... :C
         
     /*   int i, j, sizebit, e, c,l, m, k, z;
        int count=0, countd=0;
        char g[13] = "1100000001101";
        char checksum[12];

        sizebit = (1022 * 8);//1022*8
        char remainder[24];
        char message[sizebit];
        
        //Mapping check bytes to checksum bits
        //Discarding the first 4 bits of check using countd condition
  //////////////////////////////////////////////////////////////      
                          for(j=0; j<2; j++)
                          {        
          //printf (" CR TITIES! \n");
          
                                for( i=0; i < 8;countd++)
                                 {       
                                                                 if( countd > 3)
                                                        {    
                                                                         if( (check[j] & (0x01 << i)) > 0)
                                                                                 checksum[count] = '1';
                                                                         else
                                                                                checksum[count] = '0';
                                                                          i++;  
                                                                         --count;
                                                        }
                                                        else
                                                                 continue; 
                                                  printf(" countd : %d\n", countd);       
                                 }
                                 count = ((j+2) * 8) -1;
                                 
                                //countd+=countd;        
                        }
                        
                        ////////////////////////////////////////////////////
        
                        
                        
                //map CRC byte 1 into bits array
                //discard first 4 bits of byte 1023
                j=0;
                count = 3;
                for( i=0; i < 4;i++){       
                        if( (check[j] & (0x01 << i)) > 0)
                                          checksum[count] = '1';
                        else
                                         checksum[count] = '0';

                        --count;                          
                }
                
                // map CRC byte 2 into bits array
                // byte 1024
                count = 11;
                j=1;
                for( i=0; i < 8;i++){       
                        if( (check[j] & (0x01 << i)) > 0)
                                          checksum[count] = '1';
                        else
                                         checksum[count] = '0';
                          
                        --count;
       }

                // map data byte array into bits array
                // 0-1022 bytes       
                count = 7;
                for(j = 0; j < 1022; j++)
                {               
                        for( i=0; i < 8; i++)
                        {                       
                                        if( (buf[j] & (0x01 << i)) > 0)
                                                     message[count] = '1';
                                        else
                                                     message[count] = '0';
                                                     
                                        --count;                
                        }
                        count = ((j+2) * 8) -1;
                }
                k = 0;
                // count=16;
                
                
                //append 12 zeros at end of data
                for( k = sizebit; k<sizebit+12; k++)
                {
                                 message[k] = '0';
                }
                message[k] = '\0';
                                  
                
                // debug
                // for(z=0; z<sizebit+12; z++)
                //         printf("%c",message[z]);

                
                
                //take first 13 bits at a time and xor them with generator polynomial
                m=0;

                //loop through the mesasge until a leading 1 is found (MSB = 1)
                //once found, assign the first 13 bits as the remainder
      while(1)
                {
                        if(message[m] == '1')
                        {
                                                         for(e=0;e<13;e++,m++)
                                                                      remainder[e]=message[m];
                                                         break;
                        }
                        else
                                                  m+=1;
                }   
        
                do{
                //only run the xor function once the MSB of the remainder is a 1
                //if not, it adds the next bit of the message and runs again with the next bit of the remainder

                if(remainder[0]=='1'){       
                                //xor function
                                for(c =0 ;c < 13; c++)
                                        remainder[c] = (( remainder[c] == g[c])?'0':'1'); 
                }  
                         
                        c = 0;
                        while(remainder[c] == '0')
                        {
                          //for(c=0;c<12;c++)
                                remainder[c]=remainder[c+1];
                                ++c;
                        }
                        for( ; c > 0; c--, m++)
                                        remainder[13-c]=message[m];
                                 
                                 
                }while(m<sizebit+12);

                printf("\n");

                for(z=0; z<12; z++)
                                 printf("%c",checksum[z]);
                printf("\n");
                for(z=0; z<12; z++)
                                 printf("%c",remainder[z]);
        
        //check lil vs big END
        for(l = 0; l < 12; l++)
        {
                if(remainder[l] != checksum[l])
                        return 0;
        }
        return 1;  
        
        */
        

        
        
       
        

        
    /*  
                        // 2
                     //call  mp1_read
                                        
                                        //tempbuf points to beginning of frame as buf gets incremented in loop
                                        tempbuf = buf;
                                        
                                  while (numbytes <1024){
                                
                                                tempbytes = MP1_read (sockfd, read_buf, 1024-numbytes);
                                         memcpy ( buf, read_buf, tempbytes );   
                                    buf += tempbytes; //increment buf pointer
                                    printf("Number of bytes returned by MP1_\"read\" : %d\n" , tempbytes);
                                                                           
                                    if(tempbytes == 0)//indicates last frame , outer do while condition fails @ the end
                                    {
                                        break;
                                        close(sockfd);
                                     }
                                                                                                
                                                                                                //update running total of mp1_read
                                                                                                numbytes = tempbytes + numbytes;
                                                                                        
                                        }
                           
                           buf = tempbuf;*/
                           //child process should sleep while queue is being populated
                        //blank frame signals write to stop and terminate child process
     



