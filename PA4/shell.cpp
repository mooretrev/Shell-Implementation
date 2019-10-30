#include<iostream>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>


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
    
    i = input.size() - 1;
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

int main (){
    cout << "Shell Started" << endl;

    while (true){ // repeat this loop until the user presses Ctrl + C
        string commandline = "";/*get from STDIN, e.g., "ls  -la |   grep Jul  | grep . | grep .cpp" */
        getline(cin, commandline);
        // split the command by the "|", which tells you the pipe levels
        vector<string> tparts = split (commandline, "|");

        // for each pipe, do the following:
        for (int i=0; i<tparts.size(); i++){
            // make pipe
            int fd[2];
            pipe(fd);
            if (!fork()){
                // redirect output to the next level
                // unless this is the last level
                if (i < tparts.size() - 1){
                    // redirect STDOUT to fd[1], so that it can write to the other side
                    dup2(fd[1],1);
                }
                close (fd[1]);   // STDOUT already points fd[1], which can be closed
                //execute function that can split the command by spaces to
                // find out all the arguments, see the definition
                execute (tparts [i]); // this is where you execute
            }else{
                wait(0);
                dup2(fd[0], 0);
                close(fd[0]);
                
                // then do other redirects
            }
        }

    }
}

///////////TESTER MAIN

//int main(){
//
//    execute("ls > a.txt");
//
//}
