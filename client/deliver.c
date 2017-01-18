#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    printf("Server Started: \n");
    fflush(stdout);
    
    char* command;
    for(;;) {
        scanf("%c", &command);
    }
    
    return (EXIT_SUCCESS);
}