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


void kill_all_background_children(vector<pid_t> processes);

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

void remove_quotes(string& command){
    command.erase(remove(command.begin(), command.end(), '\''), command.end());
    command.erase(remove(command.begin(), command.end(), '\"'), command.end());
}


bool in_quotes(string command, int pos_target){
    int quote1 = command.find('\"');
    int quote2 = command.find('\"', quote1+1);
    int _quote1 = command.find('\'');
    int _quote2 = command.find('\'', _quote1+1);
    return (pos_target > quote1 && pos_target < quote2) || (pos_target > _quote1 && pos_target < _quote2);
}

vector<string> split (string line, string separator=" "){
    vector<string> result;
    int pos_start = 0;
    while (line.size()){
        size_t found = line.find(separator, pos_start);
        

        if(in_quotes(line, found)){
            pos_start = found + 1;
            
        }
        else{
            
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
            pos_start = 0;
        }
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
    for(int i = 0; i < argstrings.size(); ++i){
        remove_quotes(argstrings.at(i));
    }
//    argstrings.insert(argstrings.begin(), "-c");
//    argstrings.insert(argstrings.begin(), "/bin/sh");
    char** args = vec_to_char_array (argstrings);// convert vec<string> into an array of char*
    execvp (args[0], args);
}

void is_exit(string str, vector<pid_t> &background_process){
    if(str.find("exit", 0) <= str.length()){
        kill_all_background_children(background_process);
        exit(0);
    }
}

bool is_relative_path(string filename){
    return false;
}

bool cd(string command){
    if(command.find("cd", 0) <= command.length()){
        int pos = int(command.find("cd", 0));
        string filename = command.substr(pos + 3);
        filename.erase(remove(filename.begin(), filename.end(), '\\'), filename.end());
        if(filename == "-"){
            filename = "..";
        }
        chdir(filename.c_str());
    
        return true;
    }
    return false;
}

bool child_alive(pid_t pid){
    int Stat;
    if (waitpid(pid, &Stat, WNOHANG) == pid) {
        if (WIFEXITED(Stat) || WIFSIGNALED(Stat)) {
            return false;
        }
    }
    return true;
}

bool is_background(string command){
    return command.find("&", 0) <= command.length();
}

void remove_amp(string &command){
    command.erase(remove(command.begin(), command.end(), '&'), command.end());
}

void check_for_dead_children(vector<pid_t>& processes, vector<string>& commands){
    for(int i = 0; i < processes.size(); ++i){
        if(!child_alive(processes.at(i))){
            waitpid(processes.at(i), nullptr, 0);
            processes.erase(processes.begin() + i);
            commands.erase(commands.begin() + i);
            
            
        }
    }
}

void kill_all_background_children(vector<pid_t> processes){
    for(int i = 0; i < processes.size(); ++i){
        waitpid(processes.at(i), nullptr, 0);
    }
}

bool jobs(string command, vector<pid_t>& bg_processes, vector<string>& bg_name){
    check_for_dead_children(bg_processes, bg_name);
    if(command == "jobs"){
        for(int i = 0; i < bg_processes.size(); ++i){
            cout << "[" << bg_processes.at(i) << "]" << " " << bg_name.at(i) << endl;
        }
        return true;
    }else{
        return false;
    }
    
}


int main (){
    vector<pid_t> background_proccess;
    vector<string> background_command_name;
    while (true){
        bool is_amp = false;
        bool is_cd = false;
        bool is_jobs = false;
        int in, out;
        in = dup(0);
        out = dup(1);
        string commandline = "";
        cout << "trevormoore: ";
        getline(cin, commandline);
        is_exit(commandline, background_proccess);
        is_cd = cd(commandline);
        is_jobs = jobs(commandline, background_proccess, background_command_name);
        vector<string> tparts = split (commandline, "|");

        // for each pipe, do the following:
        for (int i=0; i<tparts.size() && !is_cd && !is_jobs; i++){
            is_amp = false;

            int fd[2];
            pipe(fd);
            if(is_background(tparts.at(i))){
                is_amp = true;
                remove_amp(tparts.at(i));
                background_command_name.push_back(tparts.at(i));
            }
            pid_t pid = fork();
            if(is_amp && pid != 0) background_proccess.push_back(pid);
            if (!pid){
                string command = tparts[i];
                if(i == 0 && command.find("<", 0) <= command.length() && command.find(">", 0) <= command.length() && !in_quotes(command, command.find("<", 0)) && !in_quotes(command, command.find(">", 0))){
                    int input_pos = int(command.find("<", 0));
                    int output_pos = int(command.find(">", 0));
                    string input_filename = command.substr(input_pos + 1, output_pos - input_pos -1);
                    input_filename.erase(remove_if(input_filename.begin(), input_filename.end(), ::isspace), input_filename.end());
                    
                    string output_filename = command.substr(output_pos + 1);
                    output_filename.erase(remove_if(output_filename.begin(), output_filename.end(), ::isspace), output_filename.end());
                    
                    command = command.substr(0, input_pos);
                    
                    int f = open(input_filename.c_str(), O_RDONLY);
                    dup2(f, 0);
                    close(f);
                    
                    int f2 = open(output_filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC);
                    dup2(f2, 1);
                    close(f2);
                }
                else{
                    if(i == 0 && command.find("<", 0) <= command.length() && !in_quotes(command, command.find("<", 0))){
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
                    }else if(i == tparts.size() - 1 && command.find(">", 0) <= command.length() && !in_quotes(command, command.find(">", 0))){
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
                }
                
                execute (command);
                
            }else{
                if(!is_amp){
                    wait(0);
                    check_for_dead_children(background_proccess, background_command_name);
                }else{
                    check_for_dead_children(background_proccess, background_command_name);
                }
                
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
