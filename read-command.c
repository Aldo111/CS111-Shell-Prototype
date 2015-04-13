// UCLA CS 111 Lab 1 command reading

// Copyright 2012-2014 Paul Eggert.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <error.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
////
struct node {
  char * word;
  struct node * next_node;
  struct node * prev_node;
};

typedef struct node node;
node * stack_head;
node * stack_tail;
int (*get_byte) (void *);
void *get_byte_argument;
char next_byte = '\0';
/*
*/
command_t evaluate_command ();

char * nest_stack_top ();
char * nest_stack_pop ();
int nest_stack_push (char * word);


/*
  returns 0 if no "loop" word is found
  returns 1 on failure  
*/
char * get_word();

////
struct command_stream {
  int line_number;
  command_t current_command;
  command_stream_t next_command;
  command_stream_t prev_command;
};

//typedef struct command_stream command_stream;
void init_command_stream (command_stream_t command_stream_struct);
void free_command_stream (command_stream_t command_stream_Struct);
void init_command (command_t command_struct);
void free_command (command_t command_struct);
void free_everything (char * buff, command_stream_t command_stream_struct,
		     command_t command_struct);
int isLegalChar(char c);

int init_command_type(command_t parent_command,char next_byte);
char * remove_semicolons ();
int isLegalWord(char *c);

command_stream_t stream_tail = NULL;
command_stream_t stream_head = NULL;
command_stream_t global_stream = NULL;
command_t SUBSHELL_parent = NULL;
command_t old_parent = NULL;
int line_number;
int EOF_flag = 0;
int return_flag = 0;
int fi_flag = 0;
int else_flag = 0;
int command_flag = 0;
const char null_string = '\0';


command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  line_number = 1;
  command_stream_t stream = checked_malloc(sizeof(struct command_stream));
  init_command_stream(stream);
  command_t command_struct;
  stack_head = checked_malloc(sizeof(node));
  stack_tail = stack_head;
  stack_tail->next_node = NULL;  stack_tail->prev_node = NULL;  stack_tail->word = NULL; 
  
  stream_head = stream;
  
  get_byte=get_next_byte;
  get_byte_argument=get_next_byte_argument;
  //char * word;
    ////printf("stack tail is: {%p}\n", stack_tail->word);

  while(EOF_flag != 1) {
    command_struct = evaluate_command();
    if (command_struct != NULL) {
      command_stream_t temp_stream = checked_malloc(sizeof(struct command_stream));
      init_command_stream(temp_stream);
      stream->current_command = command_struct;
      stream->next_command = temp_stream;
      temp_stream->prev_command = stream;
      stream = temp_stream;

      //printf("%d is the word of command struct\n",command_struct->type);
      //line_number++;
    }
  }
  ////printf("EOF REACHED\n");
  stream_tail = stream;
  stream = stream_head;

  /*
  int count = 1;
    while (stream->current_command != NULL) {
    //printf("# %d\n", count++);
    print_command(stream->current_command);
    stream = stream->next_command;
    }
  */
  ////printf("total input lines: %d\n", line_number);
  return stream;//return command stream
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  //error (1, 0, "command reading not yet implemented");

  if (s == NULL) {
    //printf("YOLO");
    return NULL;
  }
  command_t return_command = NULL;
  return_command = stream_head->current_command;
  stream_head = stream_head->next_command;

  return return_command;
}






//////////////////////////////////////
///////// AUXILARY FUNCTIONS /////////
//////////////////////////////////////


command_t evaluate_command () {
  char * word = get_word();
  //printf("GOT first word is {%s}\n", word);
  int size_of_word = 0;
  
  command_t current_command = checked_malloc (sizeof(struct command));
  init_command(current_command);
  current_command->u.word = checked_malloc (sizeof(char**));
  current_command->u.word[0] = NULL;
  
  char single_char = '\0';
  //printf("evaluate function called {%p}\n", current_command);
  
  while (1) {

    //BEGIN STACK WORDS
    if (word[0] == EOF) {
      //IF stack is not empty, syntax error
      if (stack_tail->word != NULL) {
	//printf("reached EOF before stack was emptied\n");
	error (1, 0, "syntax error not all elements have been closed\n");
      }
      EOF_flag = 1;
      if (size_of_word <= 0 && current_command->type == SIMPLE_COMMAND) {
	//printf("YOLO from (EOF)\n");
	return NULL;
      }
      return current_command;
    }

    if (command_flag == 0) { // if  not in a currently passed command
      if (!strcmp(word, "if")) {
	current_command->type = IF_COMMAND;
	nest_stack_push("if");
	command_t A_command = evaluate_command();
	if (A_command == NULL ){
	  //printf("error,  A_command_if stack {%p}\n",current_command);
	  fprintf(stderr, "%d: syntax error A_command_if == NULL", line_number); exit(-1);
	}
	//printf("finished  A_command_if stack {%p} with {%p}\n", current_command, A_command);
	return_flag = 0;
	//printf("reset return_flag = 0\n");
	command_t B_command = evaluate_command();
	if ( !strcmp(nest_stack_top(), "then" ) && B_command == NULL) { //top does not have a then at this point, look for one
	  B_command = evaluate_command();
	}
	if (B_command == NULL ){
	  //printf("error,  B_command_if stack {%p}\n",current_command);
	  fprintf(stderr, "%d: syntax error B_command_if == NULL", line_number); exit(-1);
	}
	//printf("finished  B_command_if stack {%p} with {%p}\n", current_command, B_command);
	return_flag = 0;
	//printf("reset return_flag = 0\n");

	command_t C_command = NULL;
	if (fi_flag <= 0 || else_flag) {
	  C_command = evaluate_command();
	  //printf("making C_command\n");
	  if ( !strcmp(nest_stack_top(), "else" ) && C_command == NULL) { //top does not have a else at this point, look for one
	    C_command = evaluate_command();
	  }
	  if (C_command == NULL ){
	    //printf("error,  C_command_if stack {%p}\n",current_command);
	    fprintf(stderr, "%d: syntax error C_command_if == NULL", line_number); exit(-1);
	  }
	}
	return_flag = 0;
	fi_flag = 0;
	else_flag = 0;
	//printf("reset return_flag = 0\n");
	//  //printf("finished  C_command_if stack {%p} with {%p}\n", current_command, C_command);
	current_command->u.command[0] = A_command;
	current_command->u.command[1] = B_command;
	current_command->u.command[2] = C_command;

      
	word = get_word();
	//printf("if command continue word is {%s}\n",word);
	continue;
	//return current_command;
      }
    
      if (!strcmp(word, "until") || !strcmp(word, "while")) {
	if (!strcmp(word, "while")) {
	  current_command->type = WHILE_COMMAND;
	}
	else {
	  current_command->type = UNTIL_COMMAND;
	}
	nest_stack_push("loop");
	command_t B_command = evaluate_command();

	if (B_command == NULL ){
	  //printf("error,  B_command stack {%p}\n",current_command);
	  fprintf(stderr, "%d: syntax error B_command == NULL", line_number); exit(-1);
	}
	//printf("finished  B_command stack {%p} with {%p}\n", current_command, B_command);
	return_flag = 0;
	//printf("reset return_flag = 0\n");
	//REMOVE THIS
	//print_command(B_command);
	//REMOVE THIS
	command_t C_command = evaluate_command();;
	if ( !strcmp(nest_stack_top(), "do" ) && C_command == NULL) { //top does not have a do at this point, look for one
	  C_command = evaluate_command();
	}
      
	if (C_command == NULL ){
	  //printf("error, C_command stack {%p}\n", current_command);
	  fprintf(stderr, "%d: syntax error C_command == NULL", line_number); exit(-1);
	}
	//printf("finished  C_command stack (%p} with {%p}\n", current_command, C_command);
	return_flag = 0;
	//printf("reset return_flag = 0\n");
	current_command->u.command[0] = B_command;
	current_command->u.command[1] = C_command;

	char temp_byte = '\0';
	if (next_byte != '\0') {
	  temp_byte=next_byte;
	}
	else {
	  temp_byte = get_byte(get_byte_argument);
	}
	while (temp_byte == ' ' || temp_byte == '\t' || temp_byte == '\0') {
	  temp_byte = get_byte(get_byte_argument);
	}
	//printf("printing {%c}\n", temp_byte);
	if ((temp_byte != '\n' && temp_byte != ';' && temp_byte != '<' && temp_byte != '>' && 
	     temp_byte != '|' && temp_byte != EOF) || isLegalChar(temp_byte)) {
	  fprintf(stderr, "%d: syntax error, done was not closed properly", line_number); exit(-1);
	}
	next_byte = temp_byte;

	word = get_word();
	//printf("until command continue word is {%s}\n",word);
	continue;
	//return current_command;
    
      }

      else if (!strcmp(word, "do") || !strcmp(word, "then") || !strcmp(word, "else")) {
	nest_stack_push(word);
	if (size_of_word <=0 && current_command->type == SIMPLE_COMMAND) { ////printf("error, no command found before {do/then/else}");
	  //fprintf(stderr, "%d: syntax error", line_number); exit(-1);
	  //printf("returned NULL after pushing {%s} onto stack\n", word);
	  //return NULL;
	  return_flag = 1;
	  //printf("set return flag = 1\n");
	  return NULL;
	}
	else {
	  return current_command;
	}
      }
      else if (!strcmp(word, "done")) { // DONE
	//remove semicolons function
	//word = remove_semicolons();
	//remove semicolon function end
	if ( !strcmp(nest_stack_pop(),"do")) {
	  if (!strcmp(nest_stack_pop(),"loop")) {
	    if (size_of_word <=0 && current_command->type == SIMPLE_COMMAND) { ////printf("error, no command found before {done}");
	      //fprintf(stderr, "%d: syntax error", line_number); exit(-1);
	      ////printf("returning from {done} and continuing\n", size_of_word);
	      //continue;
	      return_flag = 1;
	      //printf("set return flag = 1\n");
	      return NULL;
	    }
	    //printf("returning with size_of_word {%d} {done}\n", size_of_word);
	    return current_command;
	  }//printf("{loop} not found\n");
	  fprintf(stderr, "%d: syntax error, expected loop", line_number); exit(-1);
	}//printf("{do} not found\n");
	fprintf(stderr, "%d: syntax error, expected do", line_number); exit(-1);    
      }
      else if (!strcmp(word, "fi")) { /// FI
	//remove semicolons function
	//word = remove_semicolons();
	//remove semicolon function end
	char * popped_word = nest_stack_pop();
	fi_flag = 1;
	if ( !strcmp(popped_word,"else")) {
	  //printf("{else} found\n");
	  else_flag = 1;
	  if (!strcmp(nest_stack_pop(),"then")) {
	    if (!strcmp(nest_stack_pop(),"if")) {
	      if (size_of_word <=0 && current_command->type == SIMPLE_COMMAND) { ////printf("error, no command found before {done}");
		//fprintf(stderr, "%d: syntax error", line_number); exit(-1);
		return_flag = 1;
		//printf("set return flag = 1\n");
		return NULL;
	      }
	      //printf("returning with size_of_word {%d} {FI-ELSE}\n", size_of_word);
	      return current_command; // works!
	    }
	    //printf("{if} not found (else)\n");
	    fprintf(stderr, "%d: syntax error, expected if", line_number); exit(-1);
	  }
	  //printf("{then} not found (else)\n");
	  fprintf(stderr, "%d: syntax error, expected then",line_number); exit(-1);
	}
	else if (!strcmp(popped_word,"then")) {
	  if (!strcmp(nest_stack_pop(),"if")) {
	    if (size_of_word <=0 && current_command->type == SIMPLE_COMMAND) { ////printf("error, no command found before {done}");
	      //fprintf(stderr, "%d: syntax error", line_number); exit(-1);
	      return_flag = 1;
	      //printf("set return flag = 1\n");
	      return NULL;
	    }
	    //printf("returning with size_of_word {%d} {FI-THEN}\n", size_of_word);
	    return current_command; // works!
	  }
	  //printf("{if} not found (then)\n");
	  fprintf(stderr, "%d: syntax error, expected if (2)", line_number); exit(-1);
	}      
	//printf("{then} not found (then)\n");
	fprintf(stderr, "%d: syntax error, expected then (2)", line_number); exit(-1);
    
      }
      //END STACK WORDS
    }
    //BEGIN NORMAL OPERATION
    command_flag = 1;
    
    if (strlen(word) == 1) {
      single_char = word[0];
      if (single_char == ' ') {
	while (!strcmp(word," ")) {
	  word = get_word();
	}
      }
      else if(single_char == '<') {
	if (current_command->input != NULL || current_command->output != NULL) {
	  //printf("command already has input!\n");
	  fprintf(stderr, "%d: syntax error, prexisting input/ouput", line_number);	  //fprintf(stderr, "%d: syntax error", line_number); exit(-1);
	  exit(-1);
	}
	else if (size_of_word <= 0 && current_command->type == SIMPLE_COMMAND) {
	  fprintf(stderr, "%d: syntax error, no word before <", line_number);	  //fprintf(stderr, "%d: syntax error", line_number); exit(-1);
	  exit(-1);
	}
	next_byte = '\0';
	current_command->input = get_word();
	//printf("command input set to: {%s}\n",current_command->input);
	word = get_word();
	continue;
      }
      else if (single_char == '>') {
	if (current_command->output != NULL) {
	  //printf("command already has output!\n");
	  fprintf(stderr, "%d: syntax error, output already full", line_number); exit(-1);
	}
	else if (size_of_word <= 0 && current_command->type == SIMPLE_COMMAND) {
	  fprintf(stderr, "%d: syntax error, nothing to output", line_number); exit(-1);
	}
	next_byte='\0';
	current_command->output = get_word();
	if (!isLegalWord(current_command->output))
	  {
	    fprintf(stderr, "%d: syntax error, no file provided for output", line_number);
	    exit(-1);
	  }

	//printf("command output set to: {%s}\n",current_command->output);
	word = get_word();

	continue;
      }
      
      else if (single_char == '|' || single_char == ';') { //or semicolons
	// if newline, which if the nest stack is empty or not
	// if it isn't, treat \n like a semicolon
	//printf("Creating {%c} function! {%p}\n",single_char,current_command);
	//printf("return flag is: %d\n",return_flag);
	command_flag = 0;
	if (size_of_word <= 0 &&
	    current_command->type == SIMPLE_COMMAND) {
	  if ( stack_tail->word == NULL) {
	    //printf("for pipe/sequence command, size_of_word == 0\n");
	    error (1, 0, "syntax error, no command for pipe/semicolon");
	  }
	  else if (return_flag == 0) {
	    word = get_word();
	    continue;
	  }
	}
	if (return_flag == 1) {
	  //printf("\n\nreturned from return flag!\n\n");
	  return current_command;
	}
	command_t new_command = evaluate_command ();       
	if (single_char == '|') {
	  if (new_command == NULL || current_command == NULL) {
	    //printf("for pipe command, child commands are NULL");
	    error (1, 0, "syntax error, no following command for pipe");
	  }
	}
	if (new_command != NULL) {
	  command_t parent_command = checked_malloc (sizeof(struct command));
	  init_command(parent_command);
	  init_command_type(parent_command, single_char);
	  parent_command->u.command[0] = current_command;	  
	  parent_command->u.command[1] = new_command;
	  //printf(", created parent with 2 children current:{%p} new: {%p}\n",parent_command,current_command,new_command);
	  current_command =  parent_command;
	  return current_command;
	}
	//printf(", nevermind return current command, new command was NULL {%p}\n",current_command);
	return current_command;
      }
      
      else if (single_char == '\n') {
	line_number++;
	command_flag = 0;
	if (size_of_word <= 0) {
	  if (stack_tail->word == NULL &&
	      current_command->type == SIMPLE_COMMAND) {
	    
	    //printf("return NULL from (newline), stack empty\n");
	    return NULL; // BASE CASE: when stack is empty, and size of word == 0
	  }/*
	  else {
	    word = get_word();
	    continue;
	  }*/
	}
	if (stack_tail->word != NULL) {
	  //printf("from (newline), continuing semicolon like command\n");
	  word[0] = ';'; word[1] = '\0';
	  continue;
	}
	return current_command;
      }
      
      else if (single_char == '(') {
	command_flag = 0;
	init_command_type(current_command, '(');
	nest_stack_push("(");
	command_t new_command = evaluate_command ();
	if (size_of_word > 0) { //what is this useful for?
	  //printf("error: subshell not created\n");
	  fprintf(stderr, "%d: syntax error, word before opening shell\n", line_number); exit(-1);
	}
	  
	current_command->u.command[0] = new_command;
	if (new_command == NULL) {
	  //printf("error: empty subshell\n");
	  fprintf(stderr, "%d: syntax error, empty shell", line_number); exit(-1);
	}
	word = get_word();
	continue;
      }
	
      else if (single_char == ')') {
	command_flag = 0;
	char * stack_element = nest_stack_top();
	if (strcmp(stack_element,"(")) {
	  //printf("error: subshell not terminated\n");
	  fprintf(stderr, "%d: syntax error, subshell not terminated", line_number); exit(-1);
	}
	nest_stack_pop();
	if (size_of_word <= 0 && current_command->type == SIMPLE_COMMAND) {
	  //printf("YOLO from closing subshell\n");
	  return NULL;
	}
	return current_command;
      }
    }
    if (strlen(word) > 0) {
      char ** temp_word = checked_realloc(current_command->u.word, sizeof(char**)*(size_of_word+2));
      current_command->u.word = temp_word;
      if (!isLegalWord(word)) {
	  fprintf(stderr, "%d: syntax error, not legal word", line_number); exit(-1);
      }
      current_command->u.word[size_of_word] = word;
      //printf("word is {%s}\n", word);
      current_command->u.word[size_of_word+1] = NULL;
      size_of_word++;
	
    }
    word = get_word();
	
  }
 
  return NULL;
}


char * get_word () {
  char * buff;
  char temp_byte;
  int size = 1024;
  int x = 0;
  buff = checked_malloc(size);
  buff[0] = '\0';
  
  while (buff[0] != EOF) {
    
    if (x >= size-1) {
      size = size*2;
      char * temp_buff = checked_realloc (buff, size);
      buff = temp_buff;
    }
    
    if (next_byte != '\0') {
      if (!isLegalChar(next_byte)) {
	while (next_byte == ' ' || next_byte == '\t') {
	  next_byte = get_byte(get_byte_argument);
	}
	if (!isLegalChar(next_byte) && next_byte!='#') {
	  buff[x] = next_byte;
	  buff[x+1] = '\0';
	  x++;
	  next_byte = '\0';
	  return buff;
	}
	else if (next_byte =='#') {
	  while (temp_byte != '\n') {
	    
	    temp_byte = get_byte(get_byte_argument);
	    //printf("got {%c} as next_byte\n", temp_byte);
	  }
	  next_byte = '\n';
	  buff[x] = next_byte;
	  buff[x+1] = '\0';
	  x++;
	  next_byte = '\0';
	  return buff;
	
	}	
      }
      // character is legal
      buff[x] = next_byte;
      buff[x+1] = '\0';
      x++;
      next_byte = '\0';
     
    }
    
    temp_byte = get_byte(get_byte_argument);
    ////printf("got {%c} as next_byte\n", temp_byte);
   
    if (!isLegalChar(temp_byte)) {
      next_byte = temp_byte;
      if (temp_byte == EOF) {
	next_byte = EOF;
      }
      if (x == 0) {
	return get_word();
      }
      return buff;
    }
    buff[x]=temp_byte;
    buff[x+1]='\0';
    x++;
  }
  buff [1] = '\0';
  return buff;
  
}

char * nest_stack_top () {
   if (stack_tail->word == NULL) {// || stack_tail->word == NULL {
    //printf("top word {pooped} on stack\n");//,stack_tail->word);
    return &null_string;
  }
   //printf("top word {%s} on stack\n", stack_tail->word);//,stack_tail->word);
  return stack_tail->word;
}
char * nest_stack_pop () {
  //printf("Popping!\n");
  if (stack_tail->word == NULL) {
    return &null_string;
  }
  char * word = stack_tail->word;
  //printf("Popped word: {%s}\n", word);
  if (stack_tail->prev_node == NULL) {
    free(stack_tail);
    stack_tail = NULL;
    stack_head = NULL;
      //printf("new tail is: {NULL}\n");
    return word;
  }
  else {
  stack_tail = stack_tail->prev_node;
  free(stack_tail->next_node);
  stack_tail->next_node = NULL;
  //printf("new tail is: {%s}\n", stack_tail->word);
  return word;
  }
}
int nest_stack_push (char * word) {

  if ( (!strcmp(word, "do") || !strcmp(word, "then") || !strcmp(word, "else") ) && stack_tail->word == NULL){
	//printf("using {do/then/else} before {anything}\n");
	fprintf(stderr, "%d: syntax error", line_number); exit(-1);
  }
  else if (!strcmp(word, "do") && strcmp(nest_stack_top(),"loop")){
	//printf("using {do} before {while/until}\n");
	fprintf(stderr, "%d: syntax error", line_number); exit(-1);
  }
  else if (!strcmp(word, "then") && strcmp(nest_stack_top(),"if")){
	//printf("using {then} before {if}\n");
	fprintf(stderr, "%d: syntax error", line_number); exit(-1);
  }
  else if (!strcmp(word, "else") && strcmp(nest_stack_top(),"then")){
	//printf("using {else} before {then}\n");
	fprintf(stderr, "%d: syntax error", line_number); exit(-1);  
  }
  if (stack_tail == NULL) {
    stack_tail->prev_node = NULL;
    stack_tail->next_node = NULL;
    stack_tail->word = checked_malloc (strlen(word)+1);
    strcpy(stack_tail->word,word);  
    //printf("create from null stack {%s} onto stack\n",stack_tail->word);
    return 0;
    
  }
  node * temp_node = stack_tail;
  stack_tail = checked_malloc (sizeof(node));
  temp_node->next_node = stack_tail;
  stack_tail->prev_node = temp_node;
  stack_tail->next_node = NULL;
  stack_tail->word = checked_malloc (strlen(word)+1);
  strcpy(stack_tail->word,word);  
  //printf("pushed {%s} onto stack\n",stack_tail->word);
  return 0;
}

int init_command_type(command_t parent_command, char next_byte) {
  switch(next_byte) {
  case '|':
    parent_command->type = PIPE_COMMAND;
    break;
  case ';':
    parent_command->type = SEQUENCE_COMMAND;
    break;
  case '(':
    parent_command->type = SUBSHELL_COMMAND;
    break;
    
    //GET MORE CASES!
  default:
    break;
  }
  return 1;
}

void init_command_stream (command_stream_t command_stream_struct) {
  command_stream_struct->current_command = NULL;
  command_stream_struct->next_command = NULL;
  command_stream_struct->prev_command = NULL;
}

void free_command_stream (command_stream_t command_stream_struct) {
  ////printf("free_command_stream: starting to free command stream");
  if (command_stream_struct == NULL) {
    free(command_stream_struct);
    ////printf("free_command_stream (return): command stream was NULL\n");
    return;
  }
  command_stream_t next_node = NULL;
   while(command_stream_struct != NULL)  {
     free_command(command_stream_struct->current_command);
    next_node = command_stream_struct->next_command;
    free(command_stream_struct);
    ////printf("free_command_stream: freed command in command stream\n");
    command_stream_struct = next_node;
  }
  
  free(command_stream_struct);
  ////printf("free_command_stream (return): command stream was freed\n");
  return;
}


void init_command (command_t command_struct) {
  command_struct->type = SIMPLE_COMMAND;
  command_struct->input = NULL;
  command_struct->output = NULL;
  command_struct->u.word = NULL;
  command_struct->u.command[0] = NULL;
  command_struct->u.command[1] = NULL;
  command_struct->u.command[2] = NULL;
}

void free_command (command_t command_struct) {
  ////printf("free_command: started\n");
  int i=0;
  if (command_struct == NULL) {
    ////printf("free_command (return): command was NULL, returned\n");
    return;
  }
  free(command_struct->input);
  free(command_struct->output);
  if (command_struct->type == SIMPLE_COMMAND && command_struct->u.word != NULL) {
      while(command_struct->u.word[i] != NULL) {
      free(command_struct->u.word[i]);
      i++;
      }
      ////printf("free_command (return): command was SIMPLE_COMMAND, freed and returned\n");
    return;
  }
  free_command(command_struct->u.command[0]);
  free_command(command_struct->u.command[1]);
  free_command(command_struct->u.command[2]);
  ////printf("free_command (return): complex command was freed and returned\n");
  return;
}

void free_everything(char * buff, command_stream_t command_stream_struct,
		    command_t command_struct) {
  free (buff);
  free_command_stream(command_stream_struct);
  free_command(command_struct);
}

int isLegalChar(char c) {
  //printf("isLegalChar was passed: {%c}\n", c);
  return (isalnum(c) || c=='!' || c=='%' || c=='+' || c==',' || c=='-' || c=='.' || c== '/' || c==':' || c=='@' || c=='^' || c=='_');
}

char * remove_semicolons () {
  ////printf("removing semicolons\n");
  char * word = get_word(); //Get rid of all the white spaces and remove the last semicolon
  ////printf("1st  word is: {%s}\n", word);
  while ( !strcmp(word, " ") || !strcmp(word, "\t") || word[0]=='\0') {
    word = get_word();
    ////printf("(while)  word is: {%s}\n", word);
  }
  if ( !strcmp(word, ";") ) {
    word[0] = '\n';
    word[1] = '\0';
    ////printf("(if) word is: {%s}\n", word);
    ////printf("returning and removed semicolons\n");
    return word;
  }
  return word;
}

//check if a word is valid
int isLegalWord(char *c) {
  if (c==NULL)
    return 0;

  int x=0;
  char *k=c;
  if (k[0] == '\0')
    return 0;
  while(k[x]!='\0')
    {
      if (!isLegalChar(k[x]))
	return 0;
      x++;
    }
  return 1;
}
