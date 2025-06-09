/*
* IZP projekt 1
* author: Matúš Fignár
* login: xfignam00
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>


#define max_allowed_characters 102  //maximum allowed number of characters is 100 + '\n' and '\0'
#define ASCII_characters 128 


void CapitalizeLetteters(char string[]){  //capitalizing all letters in string
	int pos = 0;
	while(string[pos]){ //cycle in string values until the end of the string
		string[pos] = toupper(string[pos]);
		pos++;
		}
	}


int IsDuplicate(char output[], char value[], int position){  //funcion for checking if value for output is already in variable
	int output_lenght = strlen(output); 
	for(int i = 0; i < output_lenght; i++){
		if(value[position] == output[i]){ //in case of match stop the funcion and return 0
			return 0;
		}
	}
	return 1; //if the value is not in the output variable return 1
}


int main(int argc, char *argv[]){
	
	if(argc > 2){ //check if there is valid count of arguments 
		fprintf(stderr, "Too many arguments\n");  //in case of invalid count of arguments return error
		return 1;
	}

	char input[max_allowed_characters] = "";
	for(int i = 1; i < argc; i++){  //put argument into variable and capitalize all characters
		strcat(input, argv[i]);
		CapitalizeLetteters(input);
		}

	int  input_lenght = strlen(input); 
	int  destination_counter = 0;
	char final_destination[max_allowed_characters] = "";
	char buffer[max_allowed_characters];
	char enable[ASCII_characters] = "";
	
	while(fgets(buffer, max_allowed_characters, stdin)){ //load values from standard input into variable
		CapitalizeLetteters(buffer);

		if(buffer[strlen(buffer)-1] != '\n' && strlen(buffer) > 100){  //check if last countable character in string that reaches 100 cahracters is end of the line
			fprintf(stderr, "Too many characters in line\n");
			return 1;
		}

		if(strlen(buffer) == 1){  //if line is empty skip iteration
			continue;
		}


		if(strncmp(input, buffer, input_lenght) == 0){ //compare arguments given by user with loaded value
			destination_counter++;	
			if(destination_counter == 1){
				strcat(final_destination, buffer);
			}
			if(IsDuplicate(enable, buffer, input_lenght) == 1){ //if the value is not in the variable put it into it
				strncat(enable, &buffer[input_lenght], 1);
			}
		}
	}
	
	
	int enable_lenght = strlen(enable);
	for (int char1 = 0; char1 < enable_lenght; char1++) {  //sort characters for next possible arguments with bubble sort
    	for (int char2 = char1 + 1; char2 < enable_lenght; char2++){
        	if (enable[char1] > enable[char2]) {
        		char temp = enable[char1]; 
        		enable[char1] = enable[char2]; 
     	    	enable[char2] = temp; 
        	}
		}
    }
	
	//check conditions for output based on a count of available destiantions
	if(destination_counter == 0){
		fprintf(stdout, "Not found\n"); 
		}
	else if(destination_counter == 1){  
		fprintf(stdout, "Found: %s", final_destination);
		}
	else{
		fprintf(stdout, "Enable: %s\n", enable); 
		}

	return 0;
}

