
 #define POLYNOMIAL 0x180D  /* 1100000001101 generator polyomial */
#include <stdint.h>
#include <stdio.h>
//#define GEN 1100000001101 C(x) = x^12 + x^11 + x^3 + x^2 + 1

int crcChecker(char *buf, char *check);

int main()
{
        
        char message[10] = "FUCK SHELA";
        message[8] = '\0';
        char crc[12] = "000110111011";
        int reply;
        
        
        reply = crcChecker(message, crc);
        if (reply == 0)
                printf ("\nCRC failed");
        else
                printf ("\nCRC passed");
        return 0;
}
        

int crcChecker(char *buf, char *check)
{
        int i, j, sizebit, e, c,l, m, k, z;
        int count=0, countd=1;
        char g[13] = "1100000001101";
        char checksum[12];
        
        
        
        sizebit = (sizeof(buf) * 8);//1022*8
        char remainder[24];
        char message[sizebit];
        
        
        
        for(i=0; i<12; i++)
        {
                checksum[i] = check[i];
                
        }
        
        for(j = 0; j < 9; j++)
        {               
                for( i=0; i < 8; i++)
                {                       
                        if( buf[j] >> i & 1 == 1)
                                message[count] = '1';
                        else
                                message[count] = '0';
                                
                        ++count;                
                }
                
        }
        k = 0;
        //append 12 zeros at end of data
        for( k = count; k<count+12; k++)
        {
                message[k] = '0';
        }
        message[k] = '\0';
		 
        for(z=0; z<count+12; z++)
                printf("%c",message[z]);
        
        //take first 13 bits at a time and xor them with generator polynomial
        m=0;
        
        //loop through the mesasge until a leading 1 is found (MSB = 1)
        //once found, assign the first 13 bits as the remainder
        while(1)
        {
		     if(message[m] == '1')
		     {
				  for(e=0;e<13;e++)
				          remainder[e]=message[e];
				  break;
		     }
		     else
		     		m+=1;
		  }   
        
                
        
        
        
        

        do{
        //only run the xor function once the MSB of the remainder is a 1
        //if not, it adds the next bit of the message and runs again with the next bit of the remainder
        
        
        
        if(remainder[0]=='1')
        {       //xor function
                for(c = 1;c < 13; c++)
                remainder[c] = (( remainder[c] == g[c])?'0':'1'); 
        }       


        for(c=0;c<12;c++)
        {
                remainder[c]=remainder[c+1];
                remainder[c]=message[e++];
        }
        
        
        }while(e<=sizebit+12);
        
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
        
		}
        

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

