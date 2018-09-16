# crosscert

myASN1 cross certificate Pair Generation tool: crosscert        
written by Kiyoshi Watanabe                                     
Last Modified: Time-stamp: <2018-09-15 23:20:32 kiyoshi>          
                                                                  
You can compile under Linux, FreeBSD, and Cygwin. I did not
test others, but it should work.
   
compile: gcc -o crosscert crosscert.c -Wall -pedantic -ansi     
                                                                   
run: crosscert -out filename fwd_cert.der rvs_cert.der                              
                                                                   
help: crosscert -h                                              
                                                                  

