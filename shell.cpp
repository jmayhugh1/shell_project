#include <iostream>
#include "Tokenizer.h"
#include <cstring>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <sys/types.h>




// all the basic colours for a shell prompt
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define WHITE "\033[1;37m"
#define NC "\033[0m"

using namespace std;

int main()
{
   
    
   
    int original_stdin = dup(0);
    int orginal_stdout = dup(1);
    char previous_wd[2000];
    getcwd(previous_wd, sizeof(previous_wd));


    //need to keep track of background processes
    vector<int> ids;


    for (;;)
    {
        // implement iteration over vector or background pid (vector also declared outside loop)
        // waitpid() - using flag for non-blocking
        // implement date/time with TODO
        // implement username with getlogin()
        // implement date/time eith getcwd()
        // need date/time, usernawe, and absolute path to current dir
        for (size_t i = 0; i < ids.size(); i++)
        {
            int status = 0;
            int pid = waitpid(ids.at(i), &status, WNOHANG);
            if (pid > 0)
            {
                ids.erase(ids.begin() + i);
            }
            else if (pid < 0)
            {
                perror("wait for background process failed");
                exit(2);
            }
        }
        // need date/time, username, and absolute path to current dir
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        time_t curr_time = time(NULL);
        char *time = ctime(&curr_time);
        string str_time(time);
        // this removes the newline
        if (str_time.back() == '\n')
        {
            str_time.pop_back();
        }

        cout << YELLOW << "Shell$" << NC << " " << NC << str_time << NC << " " << NC << getenv("USER") << ": " << cwd << ">";

        // get user inputted command
        string input; // user input
        getline(cin, input);

        if (input == "exit")
        { // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl
                 << "Goodbye" << NC << endl;
            break;
        }


        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError())
        { // continue to next prompt if input had an error
            continue;
        }
        for (size_t i = 0; i < tknr.commands.size(); i++) {
            if (tknr.commands[i]->args[0] == "cd") {
                // get current working directory
                char curr_cwd[1024];
                getcwd(curr_cwd, sizeof(curr_cwd)); // get current working directory
                // check if there is a second argument
                if (tknr.commands[i]->args[1] == "-") {
                // change to prev
                if (chdir(previous_wd) != 0) {
                    perror("Failed to change directory");
                    return 1;
                }
                continue;
                }
                if (chdir(tknr.commands[i]->args[1].c_str()) != 0) {
                perror("Failed to change directory");
                return 1;
                }
                // reassign previous working directory
                strcpy(previous_wd, curr_cwd);
                continue;
            }

            FILE *OUT;
            FILE *IN;
            vector<string> args = tknr.commands[i]->args; // get arguments for command
            pid_t pid;
            int fd[2];
            if (pipe(fd) < 0) // create pipe
            {
                perror("pipe");
                exit(2);
            }
            if ((pid = fork()) < 0)
            {
                perror("fork");
                exit(2);
            }
            
            if (pid == 0){
                if (i < tknr.commands.size() - 1){
                    dup2(fd[1], STDOUT_FILENO); // output needs to be redirected to write end
                    
                }
                close(fd[0]);

                bool input = tknr.commands[i]->hasInput();
                bool output = tknr.commands[i]->hasOutput();
                if (input){


                    string inp = tknr.commands[i]->in_file;
                    IN = fopen(inp.c_str(), "r");
                    if (IN == NULL){
                        perror("fopen");
                        exit(2);
                    }
                    int fd_in = fileno(IN);
                    dup2(fd_in, STDIN_FILENO);
                }
                if (output){
                    string out = tknr.commands[i]->out_file;
                    OUT = fopen(out.c_str(), "w");
                    if (OUT == NULL){
                        perror("fopen");
                        exit(2);
                    }
                    int fd_out = fileno(OUT);
                    dup2(fd_out, STDOUT_FILENO);
                }
                vector<char *> args;
                for (size_t j = 0; j < tknr.commands[i]->args.size(); j++){ // convert vector of strings to vector of char *
                    args.push_back((char *)tknr.commands[i]->args[j].c_str());
                }
                args.push_back(NULL);
                if (execvp(args[0], args.data()) < 0){
                    perror("execvp");
                    exit(2);
                }
                
            }
            else{
                dup2(fd[0], STDIN_FILENO);
                close(fd[1]);
                int status = 0;
                int flag = 0;
                if (tknr.commands[i]->isBackground()){
                    flag = WNOHANG; //dont wait for child to finish
                    ids.push_back(pid);

                }
                if (i == tknr.commands.size() - 1){
                    waitpid(pid, &status, flag);
                }
                if (status > 1){
                    exit(status);
                }
            }



        }
        // reset stdin and stdout
        dup2(original_stdin, 0);
        dup2(orginal_stdout, 1);
    

     
    }
}
