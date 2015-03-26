/******************************************************************
*
*	Sam Opdahl
*	Smash Interpreter
*
*	A simple command interpreter for linux.
*	When run, type 'help' to list the available commands.
*
*******************************************************************/
#include <iostream>
#include <limits>
#include <string.h>
#include <map>
#include <cstdlib>
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

using namespace std;

//a type definition used with the 'cmdFunctions' map.
//each function will be passed the array of cstrings and the array's 
//	length and they will return void type.
typedef void (*function)(char**, int);


// -- Constants/Globals -- 

//holds the max parameters that the interpreter will attempt to read.
//**keep the actual length one greater than the amount of params,
//to help test if user entered too many args.
#define MAX_PARAMS 4
//holds the max size allowed of a parameter, which is also the size of each 
//	cstring in the array.
#define MAX_PARAM_LENGTH 100
//Holds the prompt that will display to the user
#define PROMPT "user@smash $ "
//Version number of the program
#define PROGRAM_VERSION "1.0"
//dictionary that will convert input (cstrings) into functions for each command.
//this effectively converts any valid command the user correctly entered 
//	directly into a function. 
map<string, function> cmdFunctions;


// -- Function Headers --

// * Command Functions *

//	~Pre/Post conditions below apply to all *_cmd functions~
//Pre:	'a' is the array of cstrings to be evaluated and
//			'len' is that value that hold 'a's length
//Post:	An error message is printed if the user entered an invalid number of 
//			params, excluding help and quit commands.
//		This is on top of what each individual command does, as described below.

//Post: A help message has been printed to the user
void help_cmd(char** a, int len);

//Post: the program has closed.
void quit_cmd(char** a, int len);

//Post:	Creates a copy of the source file.
//		If the source file doesn't exist or the destination file already exists,
//			A warning will be pritned to the user.
void copy_cmd(char** a, int len);

//Post:	the contents of the current or specified directory have been 
//		listed to the user.
void list_cmd(char** a, int len);

//Post: The specified program has been run, and this program will wait for 
//			the new child process to finish executing.
void run_cmd(char** a, int len);

// * Helper Functions *

//Pre:	'a' is a dynamic 2d array that will hold the user's input.
//		'len' is a reference to an int that will hold the length of 'a'
//Post:	'len' > 0 means we have successfully obtained 'len' amount of params
//			and 'a' will contain the params, converted to lower case.
//		'len' will be 0 if no parameters were retrieved.
//		'len' will be set to -1 and an error will print if MAX_PARAM_LENGTH
//			is exceeded.
void parse(char** a, int &len);

//Post:	The cin buffer will be cleared, 
//		ignoring all input up to a newline character.
void clearInput();

//Post:	'cmdFunctions' has been populated with the commands' names
//		and their functions. 
void initMap();

//Pre: 'iStream' is the memory address of the ifstream to open.
//      'fileName' is the location/name of the file to open the stream with.
//Post: 'iStream' has been created OR opening of stream failed 
//			and program has exited.
bool createInStream(ifstream &iStream, const char fileName[]);

//Pre: Location/name of the file to validate.
//Post: Checks if the output file exists. If it does, a warning is prompted 
//			to the user. If user opts to quit, the program exits.
bool validateOutFile(const char fileName[]);

//Pre:  'oStream' is the memory address of the ofstream to open.
//      'fileName' is the location/name of the file to open the stream with.
//Post: 'oStream' has been created OR opening of stream failed and 
//			program has exited.
bool createOutStream(ofstream &oStream, const char fileName[]);

//Pre: 'fileName' is the cstring of a file name
//Post: true is returned if files is found, otherwise false.
bool fileExists(char* fileName);



// -- Program Entry Point --

int main() {
	int aLength;
	//Create a 2d array for the parse function
	//This will hold the user's input once parse is ran.
	char** a = new char*[MAX_PARAMS];
	for (int i = 0; i < MAX_PARAMS; i++) {
		a[i] = new char[MAX_PARAM_LENGTH + 1];
	}

	//populate 'cmdFunctions', the string to function map
	initMap();

	//Main loop for the interpreter.
	while (true) {
		cout << PROMPT;
		//parse and retrieve user's input.
		parse(a, aLength);
		//if a parameter's length is too long, an error will have printed,
		//so we will skip processing the user's input for this line.
		if (aLength == -1)
			continue;

		map<string, function>::iterator iter;
		//Check if the command the user entered exists as a key in the map
		if ((iter = cmdFunctions.find(a[0])) != cmdFunctions.end()) {
			iter->second(a, aLength);
		}
		else {
			cout << "Unrecognized command: \"" << a[0] <<"\".\n";
		 	cout << "Type \"help\" to view a list of valid commands.\n";
		}
	}
}

// -- Command Functions --

void help_cmd(char** a, int len) {
	cout << "\tWelcome to smash v" << PROGRAM_VERSION << "!\n\n";
	cout << "\tThe following is a list of valid commands:\n\n";
	cout << "\trun <executable-file>\n";
	cout << "\tlist\n";
	cout << "\tlist <directory>\n";
	cout << "\tcopy <old-filename> <new-filename>\n";
	cout << "\thelp\n";
	cout << "\tquit\n\n";
	cout << "\tNote: All commands are case insensitive (arguments are not).\n";
}

void quit_cmd(char** a, int len) {
	cout << "Thanks for choosing smash!\n";
	exit(EXIT_SUCCESS);
}

void copy_cmd(char** a, int len) {
	if (len != 3) {
		cout << "Invalid number of arguments.\n";
		cout << "Usage: copy <old-filename> <new-filename>\n";
		return;
	}

	//Check if the same file is trying to copied to itself.
	//if this were to happen, the file would be clobbered.
	if (strcmp(a[1], a[2]) == 0) {
		cout << "Cannot copy same file!\n";
		return;
	}
	
	ifstream inStream;
	ofstream outStream;

	//attempt open and validate the streams.
	if (!createInStream(inStream, a[1]) || 
		!validateOutFile(a[2]) || 
		!createOutStream(outStream, a[2])) {
		inStream.close();
		outStream.close();
		return;
	}

	//read each char from input file and write it to the output file.
	char c;
	while (inStream.get(c)) {
		outStream.put(c);
	}

	inStream.close();
	outStream.close();
}

void list_cmd(char** a, int len) {
	if (len > 2) {
		cout << "Too many arguments.\n";
		cout << "Usage: list [<directory>]\n";
		return;
	}

	//create objects to the directory pointer and directory entries
	DIR *dp;
	struct dirent *ep;

	//open user's supplied directory, 
	//if no directory was supplied, use current dir.
	dp = (len == 2) ? opendir(a[1]) : opendir("./");

	if (dp != NULL) {
		//while the dir pointer gives us entries...
		while ((ep = readdir(dp))) {
			//print out the entry's directory name
			cout << ep->d_name << endl;
		}
		closedir(dp);
	}
	else {
		cout << "Unable to open the directory.\n";
	}
}

void run_cmd(char** a, int len) {
	if (len != 2) {
		cout << "Invalid number of arguments.\n";
		cout << "Usage: run <executable-file>\n";
		return;
	}

	//Check if the file exists before attempting to run it.
	if (!fileExists(a[1])) {
		cout << "Unable to find executable file \"" << a[1] << "\".\n";
		return;
	}

	//create a process id object
	pid_t p = fork();

	//make sure fork was successful
	if (p == -1) {
		cout << "Fork failed.\n";
		return;
	}

	//run the program and make the interpreter 
	//wait for it to finish execution.
	if (p)
		wait(NULL);
	else
		execl(a[1], "", NULL);
}



// -- Helper Functions --

void parse(char** a, int &len) {
	len = 0;
	//clear the array of input so there is none left over from previous input.
	for (int i = 0; i < MAX_PARAMS; i++) {
		for (int j = 0; j < MAX_PARAM_LENGTH; j++) {
			a[i][j] = 0;
		}
	}

	//paramCount will keep track of which parameter we are currently reading.
	int paramCount = 0;
	//charCount will hold the amount of characters that the current parameter contains.
	int charCount = 0;
	//curRead will hold the current character being read.
	char curRead;
	//read the user's input until we hit a newline character.
	while ((curRead = cin.get()) != '\n') {
		if (curRead == ' ') {
			//if curRead contains a space and we are processing a parameter
			//(i.e. charCount > 0), we have reached the end of that parameter.
			//if charCount == 0, we aren't processing a parameter. There are
			//just a bunch of spaces next to each other, so ignore them.
			if (charCount > 0) {
				//add the null terminator to the cstring
				a[paramCount][charCount] = '\0';
				charCount = 0;
				len++;
				//if there are more parameters than we are trying to process,
				//ignore them. Otherwise continue processing for more params.
				if (++paramCount >= MAX_PARAMS) {
					clearInput();
					//exit function with first n parameters where n = MAX_PARAMS.
					return;
				}
			}
		}
		//if curRead is a non-space character...
		else {
			//convert the first argument (the command) to lower case for easier processing later
			if (paramCount == 0)
				curRead = tolower(curRead);
			//add it to the current parameter's cstring.
			a[paramCount][charCount++] = curRead;
			if (charCount > MAX_PARAM_LENGTH) {
				//param length is too long. Print error and exit function.
				clearInput();
				cout << "Parameter " << paramCount << " exceeds maximum allowed characters: " << MAX_PARAM_LENGTH << ".\n";
				//len of -1 means we errored out reading the input.
				len = -1;
				return;
			}
		}
	}

	//New line character has been hit.
	if (charCount > 0) {
		a[paramCount][charCount] = '\0';
		len++;
	}
}

void clearInput() {
	cin.clear();
	//ignore all input up until a newline is hit.
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void initMap() {
	//these will assign each command word to their command function in the program.
	cmdFunctions["help"] = *help_cmd;
	cmdFunctions["quit"] = *quit_cmd;
	cmdFunctions["copy"] = *copy_cmd;
	cmdFunctions["list"] = *list_cmd;
	cmdFunctions["run"]  = *run_cmd;
}

bool createInStream(ifstream &iStream, const char fileName[]) {
    iStream.open(fileName);
    if (!iStream.good()) {
        cout << "File \"" << fileName << "\" doesn't exist or has invalid permissions.\n";
        cout << "Cannot continue requested operation.\n";
        return false;
    }
    return true;
}

bool validateOutFile(const char fileName[]) {
    ifstream stream(fileName);
    //If stream is good, file already exists.
    if (stream.good()) {
    	stream.close();
        //Warn user that file already exists and if they continue, it will be overwritten.
        string userInput;
        cout << "File \"" << fileName << "\" already exists.\n";
        cout << "If you continue, this file will be overwritten.\n";
        cout << "Do you wish to continue (y/n)? ";
        cin >> userInput;
        clearInput();
        if (userInput != "y") {
        	cout << "Operation aborted.\n";
            return false;
        }
    }
    return true;
}

bool createOutStream(ofstream &oStream, const char fileName[]) {
    oStream.open(fileName);
    if (!oStream.good()) {
        cout << "Unknown error creating output file\"" << fileName << "\".\n";
        cout << "Cannot continue requested operation.\n";
        return false;
    }
    return true;
}

bool fileExists(char* fileName) {
	struct stat tmpBuffer;
	//if stat() is successful, file exists, otherwise it doesn't
	return stat(fileName, &tmpBuffer) == 0;
}
