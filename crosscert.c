/********************************************************************
 *  myASN1 cross certificate Pair Generation tool: crosscert        *
 *  Written by Kiyoshi Watanabe                                     *
 *  Last Modified: Time-stamp: <02/01/10 20:34:09 kiyoshi>          *
 *                                                                  *
 *  compile: gcc -o crosscert crosscert.c -Wall -pedantic -ansi     *
 *                                                                  *
 *  run: crosscert filename1 filename2                              *
 *                                                                  *
 *  help: crosscert -h                                              *
 *                                                                  *
 ********************************************************************/

/* 
 
 Copyright(C) Kiyoshi Watanabe (kiyoshi@tg.rim.or.jp)
 
 Permission is granted for use, copying, modification, distribution,
 and distribution of modified versions of this work as long as the
 above copyright notice is included somewhere... .

 THIS SOFTWARE IS PROVIDED ``AS IS'' AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>

typedef char BYTE;
typedef int BOOLEAN;

typedef struct{
  BYTE *value;
  FILE *fileptr;
  char *filename;
}STREAM;

#define SEQUENCE_TYPE                     0x10 
#define SEQUENCE_OF_TYPE                  0x10 
#define SEQUENCE_TYPE_W_CONSBIT           0x30 
#define SEQUENCE_OF_TYPE_W_CONSBIT        0x30 

/* Prototypes for functions in crosscert.c */

void writeTag( STREAM *stream, int tag, char asn1class );
void writeLength( STREAM *stream, const long length );
void writeSequence( STREAM *stream, int length );
void writeCharacterString(STREAM *stream, BYTE *charString, int length);
void writeContext( STREAM *stream, unsigned int tagNumber, char formParameter, int tagLength );
int sizeofTag( const long tagNumber );
int sizeofLength( const long valueLength );
long sizeofContextObject( const long tagNumber, const long valueLength );

void s2fwrite(STREAM *stream, BYTE *buffer, int length);
static void printUsage();
void error_warn( const char *fmt, ... );
void error_exit( const char *fmt, ... ); 

/********************************************************************

  MAIN function
*********************************************************************/
int main(int argc, char *argv[])
{

  FILE *fout,*fin_forward,*fin_reverse; 
  int size = sizeof(char);
  STREAM streamOut;
  char x;
  char buffer;
  char *programname;
  char *forward_filename = NULL, *reverse_filename = NULL;

  int fwd=-1;
  int rvs=-1;
  int count_forward=0;
  int count_reverse=0;
  int verbose_flag=0;
  int length=0;

  int total_size=0;

  char outfile[82] = "crosscert.der";

  programname = argv[0];
  argv++;
  argc--;

  while( argc > 0 ){
    if(strcmp(*argv,"-h") == 0){
      printUsage();
    }
    else if(strcmp(*argv, "-out") == 0){
      if(--argc < 1){
	error_warn("find: illegal usage: %s", *argv);
	printUsage();
      }
      strcpy(outfile, *++argv);
    }
    else if(strcmp(*argv, "-v") == 0){
      verbose_flag = 1;
    }
    else if(strcmp(*argv, "-f") == 0){
      if(--argc < 1){
	error_warn("find: illegal usage: %s", *argv);
	printUsage();
      }
      if(forward_filename == NULL){
	forward_filename = *(++argv);
	printf("%s\n", forward_filename);
	fwd=1;
      }
    }
    else if(strcmp(*argv, "-r") == 0){
      if(--argc < 1){
	error_warn("find: illegal usage: %s", *argv);
	printUsage();
      }
      if(reverse_filename == NULL){
	reverse_filename = *(++argv);
	printf("%s\n",reverse_filename);
	rvs=1;
      }
    }
    else {
      if(forward_filename == NULL){
	forward_filename = *argv;
	fwd=1;
      }
      else if(reverse_filename == NULL){
	reverse_filename = *argv;
	rvs=1;
      }
      else{
	error_warn("find: illegal usage: %s", *argv);
	printUsage();
      }
    }
    argc--;
    argv++;
  }
  
  /* check whether both files are not specified or not */
  if(fwd<0 && rvs<0){
    error_warn("find: illegal usage: files are not correctly specified.");
    printUsage();
  }

  /* OK' Let's count the size of two certficates */

  if(fwd>0){
    fin_forward = fopen(forward_filename,"rb");
    while(fread(&x,size,1,fin_forward)>0){
      count_forward++;
    }
  }
  if(rvs>0){
    fin_reverse = fopen(reverse_filename,"rb");
    while(fread(&x,size,1,fin_reverse)>0){
      count_reverse++;
    }    
  }
  
  fout = fopen(outfile,"wb");
  if(fout == NULL){
    error_exit("%s:File cannot open:", argv[0]);
  }

  streamOut.fileptr = fout;

  /* Get the file pointer back to the starting point */
  if(fwd>0)
    fseek( fin_forward, 0, SEEK_SET );
  if(rvs>0)
    fseek( fin_reverse, 0, SEEK_SET );

/* The crossCertifcatePair will be constructed as follows, 

    Cited from ITU-T Rec. X.509 (1997 E)

    CertificatePair::= SEQUENCE{
      forward [0] Certificate OPTIONAL,
      reverse [1] Certificate OPTIONAL
        -- at least one of the pair shall be present
    }

  OK let's write it down to the file */

  if(fwd>0 && rvs>0){
    total_size=sizeofContextObject(0,count_forward)+sizeofContextObject(1,count_reverse);
  }
  else if(fwd>0 && rvs<0){
    total_size=sizeofContextObject(0,count_forward);
  }
  else if(fwd<0 && rvs>0){
    total_size=sizeofContextObject(0,count_reverse);
  }
  else{
    error_warn("find: illegal usage: files are not correctly specified.");
    printUsage();
  }
  
  writeSequence(&streamOut,total_size);

  if(fwd>0){
    writeContext(&streamOut, 0, 'C',count_forward);
    while(fread(&buffer,size,1,fin_forward)>0){
      length=sizeof(buffer);
      writeCharacterString(&streamOut,&buffer,length);
    }
  }
  
  if(rvs>0){
    writeContext(&streamOut, 1, 'C',count_reverse);
    while(fread(&buffer,size,1,fin_reverse)>0){
      length=sizeof(buffer);
      writeCharacterString(&streamOut,&buffer,length);
    }
  }

  if(verbose_flag == 1){
    if(fwd>0)
      printf("ForwardCertififate %s: size is %d\n",forward_filename,count_forward);
    if(rvs>0)
      printf("ReverseCertificate %s: size is %d\n",reverse_filename,count_reverse);
    puts("done.");
  }

  if(fwd>0)
    fclose(fin_forward);
  if(rvs>0)
    fclose(fin_reverse);
  fclose(fout);

  exit(0);
}


/********************************************************************

  crosscert printUsage function: to print usage and exit eventually
  Last Modified: Wednesday 9th Monday 2002
*********************************************************************/
void printUsage(){
  puts("Usage: crosscert [-hv][-out filename] [-rf]forward_file reverse_file");
  puts(" -h              - help");
  puts(" -out            - specify the output file name");
  puts("                   (crosscert.der is used as default)");
  puts(" -v              - verbose function to dispaly some");
  puts(" -f              - only specify the forward certificate");
  puts(" -r              - only specify the reverse certificate");
  puts("");
  puts("Report bugs to Kiyoshi Watanabe <kiyoshi@tg.rim.or.jp>.");
  exit(2);
}

/********************************************************************

  myASN1 writeSequence function
  Last Modified: Monday 2nd October 2000
*********************************************************************/
/* This function writes the ASN.1 SEQUENCE TYPE. This function takes
   two variables. You must obtain the length of the sequence
   for this, by calling the sizeof... functions beforehand. */

void writeSequence( STREAM *stream, int length )
{

  writeTag( stream, SEQUENCE_TYPE_W_CONSBIT, 'U');
  writeLength( stream, length );
  
}

/********************************************************************

  myASN1 writeTag function
  Last Modified: Monday 2nd October 2000
*********************************************************************/
void writeTag(STREAM *stream, int tag, char asn1class)
{

  BYTE buffer[5];
  int length = 0;
  int mask_application = 0x40;
  int mask_private = 0xC0;
  int mask_context = 0xA0;

  /* currently assume that the the number will be less than 31 */
  if(asn1class == 'U' || asn1class == 'u')
    buffer[length++] = (BYTE)tag;
  else if(asn1class == 'A' || asn1class == 'a')
    buffer[length++] = (BYTE)(tag | mask_application);
  else if(asn1class == 'P' || asn1class == 'p')
    buffer[length++] = (BYTE)(tag | mask_private);
  else
    buffer[length++] = (BYTE)(tag | mask_context);

  s2fwrite(stream, buffer, length);
}

/********************************************************************

  myASN1 writeLength function
  Last Modified: Monday 2nd October 2000
*********************************************************************/
void writeLength(STREAM *stream, const long length){

  BYTE buffer[5];
  int len=0;

  /* length is less than 255 */
  if(length < 0x00000080L){
    buffer[len] = (BYTE)length;
    len++;
  }
  /* When length is more than 255, determine the value of the 
     length of length */
  if(length > 0x00FFFFFFL)
    buffer[len++] = (BYTE)0x84;
  else if(length > 0x0000FFFFL)
    buffer[len++] = (BYTE)0x83;
  else if(length > 0x000000FFL)
    buffer[len++] = (BYTE)0x82;
  else if(length >= 0x00000080L)
    buffer[len++] = (BYTE)0x81;

  /* Determine the number of bytes to encode the lenght
     and encode it into a temporary buffer */

  if(length > 0x00FFFFFFL)
    buffer[len++] = (BYTE)(length >> 24);
  if(length > 0x0000FFFFL)
    buffer[len++] = (BYTE)(length >> 16);
  if(length > 0x000000FFL)
    buffer[len++] = (BYTE)(length >> 8);
  if(length >= 0x00000080L)
    buffer[len++] = (BYTE)length;

  s2fwrite(stream, buffer, len);

}

void writeCharacterString(STREAM *stream, BYTE *charString, int length)
{

  s2fwrite( stream, charString, length );

}

/********************************************************************

  s2fwrite function
  Last Modified: 2000/10/27
*********************************************************************/

/* This function write the content of the "buffer", which points to
   with the number of the "length" to a file, which is pointed
   by the "stream" pointer. The function is really straight forward.

  @ Input parameters: STREAM *stream, BYTE *buffer, int length
  @ Retrun          : None
  @ Restriction     : 
  */

void s2fwrite(STREAM *stream, BYTE *buffer, int length)
{
  int size;

  assert(stream != NULL);
  assert(buffer != NULL);
  assert(stream->fileptr != NULL);
  assert(length > 0);

  size = sizeof(char);
  if(fwrite(buffer,size,length,stream->fileptr) != (size_t)length){
    printf("An error occurs in s2fwrite function\n");
    exit(EXIT_FAILURE);
  }
}

/********************************************************************

  ERR_warn function
  Last Modified: 2001/01/08
*********************************************************************/

/* This function takes variable arguments to print the erorr.
   and continue the program.

   */

void error_warn( const char *fmt, ... )
{
 
  va_list args;
  assert(fmt);

  fflush(stderr);
  fprintf( stderr, "warning: " );

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  if( fmt[0] != '\0' && fmt[strlen(fmt) - 1] == ':')
    fprintf(stderr, " %s", strerror(errno));
  fprintf( stderr, "\n");

}

/********************************************************************

  error_exit function
  Last Modified: 2001/01/08
*********************************************************************/

/* This function takes variable arguments to print the erorr.
   and exit the program.

   */
void error_exit( const char *fmt, ... )
{
 
  va_list args;
  assert(fmt);

  fflush(stderr);

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  if( fmt[0] != '\0' && fmt[strlen(fmt) - 1] == ':')
    fprintf(stderr, " %s", strerror(errno));
  fprintf( stderr, "\n");
  exit(2);

}

/********************************************************************

  myASN1 writeContext Function
  Last Modified: 11/17 Friday 2000 
*********************************************************************/
/* 
   This is the function to write a context-specific tag, i.e. [0]...[N]
   The N is virtually infinite. However since the number of context-specific
   tag will not be over 2 bytes range (16383), I will process only the two
   identifier octets only. Normally the tag number is less than 127, but once 
   saw 999 for the tag! So the specification will be:

   In case the tag number is [0] and [30], the tag would be:
   
     8 7 6 5 4 3 2 1   
     1 0 1 . . . . .  (constructed)
   
     8 7 6 5 4 3 2 1   
     1 0 0 . . . . .  (primitive)

   In case the tag is [31] and [16383], the tag would be:

     8 7 6 5 4 3 2 1   
     1 0 1 1 1 1 1 1  (flag)

     8 7 6 5 4 3 2 1     
     1 . . . . . . .     2  

     8 7 6 5 4 3 2 1     
     0 . . . . . . .     N
    

   */

void writeContext( STREAM *stream, unsigned int tagNumber, char formParameter, int tagLength )
{

  BYTE buffer[2];
  int length = 0;
  int mask_contextSpecific = 0x80;
  int mask_constructedForm = 0x20;
  int mask_multileBytes = 0x1F;

  buffer[0] = (BYTE)0;
  buffer[length] |= mask_contextSpecific;

  /* When the formParameter is "Constructed" */
  if( formParameter == 'c' || formParameter == 'C' ){
    buffer[length] |= mask_constructedForm;
  }
  
  /* When the Tag number is 30 (which requires multiple bytes) */
  if ( tagNumber > 0x1E ){
    buffer[length] |= mask_multileBytes;
  }

  /* ----------------------------------------------------------------
   Here is the code to encode the number into the buffer along with 
   the ASN.1 specification. 
  ------------------------------------------------------------------*/

  if( tagNumber > 0x3FFF ){  /* over 16383 */
    printf("The number you specified is too big or too samll to encode!\n");
    exit(1);
  }
  if( tagNumber > 0x7F ){    /* over 127   */ 
    length++;
    buffer[length] = (BYTE)( tagNumber >> 7 ) | 0x80;
  }   
  if( tagNumber > 0x1E ){   /*  over 30    */
    length++;
    buffer[length] = (BYTE)~(~( tagNumber ) | 0x80 );
  }
  else{                     /* less than 30 */
    buffer[length] |= (BYTE)tagNumber; 
  }

  length++;

  s2fwrite( stream, buffer, length );   /* write this tag */
  writeLength( stream, tagLength );     /* write following any content */
 
}

/********************************************************************

  myASN1 sizeofContextObject function
  Last Modified: 19th November 2000
*********************************************************************/
long sizeofContextObject( const long tagNumber, const long valueLength )
{
  if (tagNumber < 0 || valueLength < 0)
    return -1; /* used for error */
  else
    return sizeofTag(tagNumber) + sizeofLength(valueLength) + valueLength;
}


/********************************************************************

  myASN1 sizeofLength function
  Last Modified: 19th November 2000
*********************************************************************/
int sizeofLength( const long valueLength )
{
  if(valueLength < 0)
    return -1; /* used for error checking */
  else if (valueLength > 0x00FFFFFFL)
    return 5;
  else if (valueLength > 0x0000FFFFL)
    return 4;
  else if (valueLength > 0x000000FFL)
    return 3;
  else if (valueLength > 0x0000007FL)
    return 2;
  else
    return 1;
}

/********************************************************************

  myASN1 sizeofTag function
  Last Modified: 19th November 2000
********************************************************************/
/*  This function determine the size of Tag Length. I think that
    most of Tag would be 1, i.e. sizeof(BYTE). However, when you
    think the case of Context-Specific, we need to find a way to
    calcualte the value 

    */

int sizeofTag( const long tagNumber )
{

  if(tagNumber < 0)
    return -1; /* used for error checking */
  else if(tagNumber > 0x0000007FL)
    return 3;
  else if(tagNumber > 0x0000001EL)
    return 2;
  else
    return 1;
}


