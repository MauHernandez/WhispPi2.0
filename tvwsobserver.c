#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <stdlib.h>
#define BAUDRATE B500000
#define FALSE 0
#define TRUE 1
// define the number of measurements to calculate the maximum
// #define MAXHOLD 1 
// MAXHOLD in 5 can capture peaks in the activity within the 10secs period. 
#define MAXHOLD 1 
// 9242 is the equivalent of 2 hours
// 13 in 10 secs aprox 
// 77 in 1 min aprox

int fd;
FILE *f;
char fname[21];
char *sweep_data;
time_t t;
struct tm *localt;
char buf[255];
char sPortName[255];
//char fileName[255];
char shortFN[10]; //to pass to the handler

long lowfreq, highfreq;

long stepfreq;

void signal_handler_io (int status);   /* definition of signal handler */
void set_scale(char *argv[]);
void get_sample(float samples[], unsigned *size);
void reset_max_hold(float max_hold[], int size);
void calc_max_hold(float samples[], int size, float max_hold[]);
void print_spec(float samples[], float max_hold[], int size);

int main(int argc, char **argv)
{
  
  float samples[200];
  float max_hold[200];
  int size, i;
  int mh;
  
  // get the time
  t = time(0);
  localt = localtime(&t);
 
  // calculate interval
  lowfreq=atol(argv[2]);
  highfreq=atol(argv[3]);
  
  // set the RFExplorer frequency scale
  set_scale(argv);
  

  // the very first sample
  get_sample(samples, &size);
  reset_max_hold(max_hold, size);
  calc_max_hold(samples, size,  max_hold);

  if (argv[7] > 0)
     {
    	mh=atol(argv[7]);
     }
  else
     {  
      	mh=MAXHOLD;
     }
 
  // max_hold for N times
  for (i=0; i<mh-1; i++)
    {
      get_sample(samples, &size);
      calc_max_hold(samples, size,  max_hold);
    }

  print_spec(samples, max_hold, size);

 return 0;
}


// WHAT THE HELL IS THIS?
void signal_handler_IO (int status)
{
}

void print_spec(float samples[], float max_hold[], int size)
{
  unsigned freq;
  int i;

  for (i=0; i<size; i++)
    {
      freq=(lowfreq+i*((highfreq-lowfreq)/size));
      //  printf("%u\t%5.2f\t%5.2f\n", freq, samples[i], max_hold[i]);
      printf("%u\t%5.2f\n", freq, max_hold[i]);
    }
}

void reset_max_hold(float max_hold[], int size)
{
  int i;

  for (i=0; i<size; i++)
    {
      // a minimum value of -150 dBm
      max_hold[i]=-150;
    }
}

void calc_max_hold(float samples[], int size, float max_hold[])
{
  int i;

  for (i=0; i<size; i++)
    {
      if (samples[i]>max_hold[i])
	max_hold[i]=samples[i];
    }  
}

void get_sample(float samples[], unsigned *size)
{
  int res;
  int i;

  do 
    {
      memset(buf,0x0,sizeof(buf));
      // THERE WAS A: pause();
      res = read(fd,buf,255);
    } while(buf[0]!='$');
  
  *size=buf[2];

  // once received the signal the whole data should be available
  //  pause(); 

  /*  
  res=0;
  while(res < *size + 5) 
    {
      printf("enter second read");
      res += read(fd,buf+res,255);
    }
  */
  //printf("res = %i, size = %u\n",res,*size);

  for(i=0; i < *size; i++)
    {
      // be careful it starts at 3 not at 4 as before
      // since the string is $SXyyy where every y corresponds
      // to a different power value.

      samples[i]=((unsigned char*)buf)[i+3]/-2.0;
      
      // printf("%5.2f\t",samples[i]);
    }
}


void set_scale(char *argv [])
{
  struct sigaction saio;
  struct termios oldtio, newtio;

  // get port name 
  strcpy(sPortName, argv[1]);

 fd = open(sPortName, O_RDWR | O_NOCTTY | O_NONBLOCK);
 
if (fd <0) 
 {
     perror(sPortName); 
     exit(-1); 
 }

 /* install the signal handler before making the device asynchronous */
 memset(&saio,0x0,sizeof(saio));
 saio.sa_handler = signal_handler_IO;
 sigaction(SIGIO,&saio,NULL);
 
 /* allow the process to receive SIGIO */
 fcntl(fd, F_SETOWN, getpid());
 
 /* Make the file descriptor asynchronous (the manual page says only 
    O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
 fcntl(fd, F_SETFL, FASYNC);

 tcgetattr(fd,&oldtio); /* save current port settings */
 /* set new port settings for canonical input processing */
 newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
 newtio.c_iflag = IGNPAR | ICRNL;
 newtio.c_oflag = 0;
 newtio.c_lflag = ICANON;
 newtio.c_cc[VMIN]=1;
 newtio.c_cc[VTIME]=0;
 tcflush(fd, TCIFLUSH);
 tcsetattr(fd,TCSAFLUSH,&newtio);
 buf[0]='#';
 buf[1]=32;
 buf[2]='C';
 buf[3]='2';
 buf[4]='-';
 buf[5]='F';
 buf[6]=':';
 buf[7]=argv[2][0];
 buf[8]=argv[2][1];
 buf[9]=argv[2][2];
 buf[10]=argv[2][3];
 buf[11]=argv[2][4];
 buf[12]=argv[2][5];
 buf[13]=argv[2][6];
 buf[14]=',';
 buf[15]=argv[3][0];
 buf[16]=argv[3][1];
 buf[17]=argv[3][2];
 buf[18]=argv[3][3];
 buf[19]=argv[3][4];
 buf[20]=argv[3][5];
 buf[21]=argv[3][6];
 buf[22]=',';
 buf[23]='-';
 buf[24]=argv[4][0];
 buf[25]=argv[4][1];
 buf[26]=argv[4][2];
 buf[27]=',';
 buf[28]='-';
 buf[29]=argv[5][0];
 buf[30]=argv[5][1];
 buf[31]=argv[5][2];
 
 write(fd,buf,32);

}

/*  strftime(fname,21,"%Y_%m_%d_%H_%M.txt",localt); */

// strftime(fname,4,argv[6],localt);


// EXTRA CODE:
/*  fprintf(f,"--------------------\r\n");
 strftime(line2,20,"%Y-%m-%d-%H:%M:%S",localt);
 fprintf(f,"%s\r\n",line2);
 fprintf(f,"\r\n");
 fprintf(f,"\r\n");
 fprintf(f,"Frequency(mHZ)\tSignal (dBm)\r\n");
*/  

// STORE IN A FILE
/*
f = fopen(argv[6],"w"); 

for(i=0;i<sweep_steps;i++)
{
 fprintf(f,"%ld\t%6.6f\r\n",(atol(argv[2])+i*(atol(argv[3])-atol(argv[2]))/sweep_steps)*1000,-1*(((float)(unsigned char)buf[i+4]))/2);

}

fclose(f);
*/

