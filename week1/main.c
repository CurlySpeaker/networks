#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct Stack{
    int value;
    struct Stack *next;
};

struct Stack *stack;
int size;

int peek(){
    return stack->value;
}

void push(int data){
    struct Stack *new_head = malloc(sizeof(struct Stack));
    new_head->value = data;
    new_head->next = stack;
    stack = new_head;

    size += 1;
}

void pop(){
    struct Stack *tmp = stack;
    stack = stack->next;
    free(tmp);

    size -= 1;
}

int empty(){
    return !size;
}

void display(){
    struct Stack *tmp = stack;
    for(int i=0;i<size;i++){
        printf("%d ",tmp->value);
        tmp = tmp->next;
    }
    printf("\n");
}

void create(){
    stack = malloc(sizeof(struct Stack));
    size = 0;
}

void stack_size(){
    printf("%d\n", size);
}



int main(){
    int fd[2];
    int fd_sync[2];
    pipe(fd);
    pipe(fd_sync);
    printf("Available commands:\n"
            "0 - peek\n"
            "1 - push\n"
            "2 - pop\n"
            "3 - empty\n"
            "4 - display\n"
            "5 - create the stack\n"
            "6 - stack size\n"
          );

    int pid = fork();

    //Child process
    if (pid==0){
        int buffer;
        int value;
        while(1){
            read(fd[0], &buffer, sizeof(int));
            switch (buffer){
                case 0:
                    printf("%d\n", peek());
                    break;
                case 1:
                    read(fd[0], &value, sizeof(int));
                    push(value);
                    break;
                case 2:
                    pop();
                    break;
                case 3:
                    printf(empty() ? "True\n" : "False\n");
                    break;
                case 4:
                    display();
                    break;
                case 5:
                    create();
                    break;
                case 6:
                    stack_size();
                    break;
            }
            write(fd_sync[1], &value, sizeof(int));
        }
    }

    //Parent process
    else if (pid>0){
        int buffer;
        while (1){
            printf(">>> ");
            scanf("%d", &buffer);
            write(fd[1], &buffer, sizeof(int));
            if (buffer == 1){
                int value;
                scanf("%d", &value);
                write(fd[1], &value, sizeof(int));
            }
            read(fd_sync[0], &buffer, sizeof(int));
        }
    }

    return 0;
}
