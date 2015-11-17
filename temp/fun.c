#include<stdio.h>
#include "fun.h"
 

int check_prime(int num) /* User-defined function to check prime number*/
{
   int j,flag=0;
   for(j=2;j<=num/2;++j){
        if(num%j==0){
            flag=1;
            break;
        }
   }
   return flag;
}
