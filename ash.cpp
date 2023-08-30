/*
 * Thiphilius Mills
 * 07/26/2023
 * COMP350-03A
 * Description: ASH is a shell with built-in functions cd, path, and exit.
 * This shell has both an interactive mode and a batch mode.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
//Structure for passing information from parsing function
struct ArgC_V {
    int argc;
    char **argv;
    char *cmd;
    char *fileName;
    bool outputToFile;

};
//initializing a array of strings for storing paths
char **path;


//Outputs error message
void error(void){
    char error_message[30]= "An error has occurred\n";
    write(STDERR_FILENO,error_message,strlen(error_message));
}
//Parses input in to argc and argv
//Determines whether the output is redirected or not
struct ArgC_V parse(char* input){
    int i=0,argc = 0,argsPastLT=0;
    bool stopArgcCount = false;
    char **argv =NULL;
    char *cmd=NULL,*outfileName=NULL;
    char *splitInput=NULL,*splitInput2=NULL,*copyOfInput =NULL;
    //make a copy of the input so that we can traverse it twice
    copyOfInput=strdup(input);
    /*The first time we iterate through the input we find the argc.
     * Additionally we determine whether the redirect character is present*/
    splitInput= strtok(input," \n");
    while(splitInput){
        if(strcmp(splitInput,">")==0){
            stopArgcCount =true;
        }
        if (!stopArgcCount){
            argc++;
        }
        splitInput= strtok(NULL," \n");
    }

    /*The second iteration argv is determined and
     * checks whether too many arguments are after the
     * redirect character*/
    argv = (char**) malloc(sizeof(char *)*(argc +1));
    splitInput2= strtok(copyOfInput," \n");
    while(splitInput2){
        if (i==0){
            cmd = splitInput2;
            argv[i]=splitInput2;
        }
        else{
            if (i<argc){
                argv[i]=splitInput2;
            }
            else if (i==argc){
                argv[i]=NULL;
            }
            if (i>argc){
                argsPastLT++;
                outfileName = splitInput2;
                if (argsPastLT>1){
                    error();
                }
            }
        }
        splitInput2= strtok(NULL," \n");
        i++;
    }
    //check to make sure there is a argument after the redirect character
    if (outfileName=="\n" && stopArgcCount) {
        error();
    }



    //declare an instance of the ArgC_V struct
    ArgC_V args;
    //determine what is outputted for outputToFile and fileName
    if(stopArgcCount){
        args.outputToFile = stopArgcCount;
        args.fileName= outfileName;
        /*printf("%s\n",outfileName);
        int redirectFD = open(outfileName,O_CREAT | O_TRUNC | O_WRONLY);
        dup2(redirectFD,STDOUT_FILENO);
        close(redirectFD);*/
    }
    else{
        args.outputToFile =false;
        args.fileName =NULL;
    };


    args.argc=argc;
    args.argv=argv;
    args.cmd =cmd;
    //free storage
    free(splitInput2);
    free(splitInput);

    return args;
}
//initialize the path as /bin
void initPath(void){
    path = (char**) malloc(sizeof(char*)*2);
    path[0]=(char*)"/bin";
    path[1]=NULL;
}
//Function for changing what paths are searched for an instruction
void changePath(int numArgs,char** args){

    path=(char**)realloc(path,sizeof(char*)*numArgs);

    for (int i =0;i<numArgs;i++){
        path[i]=args[i+1];
    }

    printf("New search Path Created\n");
    int i =0;
    while(path[i]) {
        printf("%s\n", path[i]);
        i++;
    }
}
//concat concatinates two strings
char* concat(char* str1,char* str2){
    char * result = (char *)malloc(sizeof(str1)+sizeof("/")+sizeof(str2)+1);
    strcpy(result,str1);
    strcat(result,"/");
    strcat(result,str2);
    return result;
}
//checks to whether a path to a cmd is valid
char* checkPaths(char *cmd){
    int i = 0;
    //iterate through the possible paths until a valid path is found
    // or there are no more paths
    while(path[i]){
        if(access(concat(path[i],cmd),X_OK)==0){
            //printf("valid path: %s\n",concat(path[i],cmd));
            return concat(path[i],cmd);
        }
        i++;
    }
    error();
    //return Path not found (PNF)
    return (char *)"PNF";
}//If the command isn't built in execute using execv
void executeCommand(char *cmd,int argc,char **argv,bool outToFile,char *fileName){
    //check if there is a valid path to the command
    char *validpath = checkPaths(cmd);
    int redirectFD;
    if (strcmp(validpath,"PNF")==0){
        return;
    }
    //create a new process
    int pid = fork();
    //child process
    if (pid == 0){
        //if the redirect character is present redirect stdout to the file name
        if (outToFile){
            redirectFD = open(fileName,O_CREAT | O_TRUNC | O_WRONLY);
            dup2(redirectFD,STDOUT_FILENO);
            close(redirectFD);
            int redirectFD2 = open(fileName,O_APPEND);
            dup2(redirectFD2,STDERR_FILENO);
            close(redirectFD2);
        }

        char * args[argc+1];
        for (int i=0;i<=argc+1;i++){
            args[i]=argv[i];
        }
        //execute the command
        execv(validpath,args);
    }
    else{
        //wait for the child process to complete
        wait(NULL);
        //return the output back to stdout
        if (outToFile){
            dup2(STDOUT_FILENO,redirectFD);
        }
        free(validpath);
    }

}
//with the args from parse interpret determines what is done with the args
//which built in command if executed or whether the args are passed to execute
void interpretArgs(ArgC_V args){
    //dependent on the number of args determine
    //1. where the command is built in
    //2. if it is determine where that command is valid for that number of arguments
    if(args.argc==1){
        if (!strcmp(args.cmd,"exit")){
            free(path);
            exit(0);
        }
        else if (!strcmp(args.cmd,"cd")){
            error();
        }
        else if (!strcmp(args.cmd,"path")){
            changePath(args.argc,args.argv);
        }
        else{
            executeCommand(args.cmd,args.argc,args.argv,args.outputToFile,args.fileName);
        }
    }
    else if (args.argc==2){
        if (!strcmp(args.cmd,"exit")){
            error();
        }
        else if (!strcmp(args.cmd,"cd")){
            int check = chdir(args.argv[1]);
            if (check<0){
                error();
            }
        }
        else if (!strcmp(args.cmd,"path")){
            changePath(args.argc,args.argv);
        }
        else{
            executeCommand(args.cmd,args.argc,args.argv,args.outputToFile,args.fileName);
        }
    }
    else{
        if (!strcmp(args.cmd,"exit")){
            error();
        }
        else if (!strcmp(args.cmd,"cd")){
            error();
        }
        else if (!strcmp(args.cmd,"path")){
            changePath(args.argc,args.argv);
        }
        else{
            executeCommand(args.cmd,args.argc,args.argv,args.outputToFile,args.fileName);
        }
        free(args.cmd);
        free(args.fileName);
    }
}
//parses the file for the individual cmds and executes them
void batchmode(char *fileName){
    FILE *fPointer;
    struct ArgC_V args;
    //search through the file line by line
    fPointer= fopen(fileName,"r");
    char instruction[150];
    while(!feof(fPointer)){
        fgets(instruction,150,fPointer);
        //parse the line and attempt to execute the cmd
        args=parse(instruction);
        interpretArgs(args);
    }
    fclose(fPointer);
    free(path);
    exit(0);
}

//Determines whether the shell is in interactive mode or batch mode runs
//runs interactive mode receives input from the cmdline
int main(int argc,char *argv[]){
    //dependent on the number of arguments determine whether you are
    //in batched or interactive mode
    char prompt[6]="ash> ";
    initPath();
    if (argc>2){
        error();
    }
    //batched mode
    else if (argc==2){
        batchmode(argv[1]);

    }
    //interactive mode
    else{
        while(1){
            struct ArgC_V args;
            size_t sizeOfInputBuf;
            char *input =NULL;
            //write(STDOUT_FILENO,prompt,strlen(prompt));
            printf("%s",prompt);
            getline(&input,&sizeOfInputBuf,stdin);
            args = parse(input);
            interpretArgs(args);
            //printf("count: %d\n",args.argc);
            /*for (int i=0;i<args.argc;i++){
                printf("V: %s\n",args.argv[i]);
            }*/
            //free(*args.argv);
            free(input);
        }
    }
}
