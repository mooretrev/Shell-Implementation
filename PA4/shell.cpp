#include<iostream>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>

using namespace std;

void execute (string);
void execute(string, string);
string trim (string);
char** vec_to_char_array (vector<string>);

string trim (string input){
    int i=0;
    while (i < input.size() && input [i] == ' ')
        i++;
    if (i < input.size())
        input = input.substr (i);
    else{
        return "";
    }
    
    i = int(input.size() - 1);
    while (i>=0 && input[i] == ' ')
        i--;
    if (i >= 0)
        input = input.substr (0, i+1);
    else
        return "";
    
    return input;
}

vector<string> split (string line, string separator=" "){
    vector<string> result;
    while (line.size()){
        size_t found = line.find(separator);
        if (found == string::npos){
            string lastpart = trim (line);
            if (lastpart.size()>0){
                result.push_back(lastpart);
            }
            break;
        }
        string segment = trim (line.substr(0, found));
        //cout << "line: " << line << "found: " << found << endl;
        line = line.substr (found+1);

        //cout << "[" << segment << "]"<< endl;
        if (segment.size() != 0) 
            result.push_back (segment);

        
        //cout << line << endl;
    }
    return result;
}

char** vec_to_char_array (vector<string> parts){
    char ** result = new char * [parts.size() + 1]; // add 1 for the NULL at the end
    for (int i=0; i<parts.size(); i++){
        // allocate a big enough string
        result [i] = new char [parts [i].size() + 1]; // add 1 for the NULL byte
        strcpy (result [i], parts[i].c_str());
    }
    result [parts.size()] = NULL;
    return result;
}

void execute (string command){
    vector<string> argstrings = split (command, " "); // split the command into space-separated parts
//    argstrings.insert(argstrings.begin(), "-c");
//    argstrings.insert(argstrings.begin(), "/bin/sh");
    char** args = vec_to_char_array (argstrings);// convert vec<string> into an array of char*
    execvp (args[0], args);
}

void is_exit(string str){
    if(str.find("exit", 0) <= str.length()){
        exit(0);
    }
}

bool is_relative_path(string filename){
    return false;
}

void cd(string command){
    if(command.find("cd", 0) <= command.length()){
        int pos = command.find("cd", 0);
        string filename = command.substr(pos + 3);
        if(is_relative_path(filename)){
            cout << "need to implement \n";
        }else{
            chdir(filename.c_str());
            cout << "error number " << errno << endl;
        }
    }
}

int main (){
    while (true){
        bool is_cd = false;
        int in, out;
        in = dup(0);
        out = dup(1);
        string commandline = "";
        cout << "trevormoore: ";
        getline(cin, commandline);
        is_exit(commandline);
        cd(commandline);
        vector<string> tparts = split (commandline, "|");

        // for each pipe, do the following:
        for (int i=0; i<tparts.size() && !is_cd; i++){
            int fd[2];
            pipe(fd);
            if (!fork()){
                string command = tparts[i];
                if(i == 0 && command.find("<", 0) <= command.length()){
                    int pos = int(command.find("<", 0));
                    string filename = command.substr(pos + 1);
                    filename = trim(filename);
                    filename.erase(remove_if(filename.begin(), filename.end(), ::isspace), filename.end());
                    command = command.substr(0, pos);
                    
                    int f = open(filename.c_str(), O_RDONLY);
                    dup2(f, 0);
                    close(f);
                }
                
                if (i < tparts.size() - 1){
                    dup2(fd[1],1);
                    close (fd[1]);
                }else if(i == tparts.size() - 1 && command.find(">", 0) <= command.length()){
                    int pos = int(command.find(">", 0));
                    string filename = command.substr(pos + 1);
                    filename = trim(filename);
                    filename.erase(remove_if(filename.begin(), filename.end(), ::isspace), filename.end());
                    command = command.substr(0, pos);
                    
                    int f = open(filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC);
                    dup2(f, 1);
                    close(f);
                }else{
                    dup2(out, 1);
                }
                
                execute (command);
                
            }else{
                wait(0);
                if (i < tparts.size() - 1){
                    dup2(fd[0],0);
                }else{
                    dup2(in, 0);
                }
                close(fd[1]);

            }
        }

    }
}
