#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h> 
#include <windows.h>
#include <windef.h>
#include <winnt.h>
#include <winbase.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{
	//If the user fails to give us two arguments yell at him.	
	if ( argc != 2 ) {
		fprintf ( stderr, "Usage: %s \n", argv[0] );
		exit ( EXIT_FAILURE );
	}
	// Data array
	uint8_t HEX_array[32768];

	// Bytes read into array.
	int HEX_array_size;
	
	//File to be loaded.	
	FILE *hex_file;

	//Open file using command-line info; for reading.
	hex_file = open_file (argv[0], "rb" );

	// Load the data from file
	HEX_array_size = hex_file_to_array(hex_file, HEX_array);


}

//Open file for reading, function.
FILE *open_file ( uint8_t *file, uint8_t *mode )
{
  FILE *fileOpen = fopen ( file, mode );

  if ( fileOpen == NULL ) {
    perror ( "Unable to open file" );
    exit (EXIT_FAILURE);
  }

  return fileOpen;
}

uint8_t read_byte_from_file(FILE * file, uint8_t * char_to_put, int * total_chars_read)
{
	//Holds combined nibbles.
	uint8_t hexValue;
	//Get first nibble.
	*char_to_put = fgetc (file);
	clear_special_char(file, char_to_put, total_chars_read);
	//Put first nibble in.
	hexValue = (Ascii2Hex(*char_to_put));
	//Slide the nibble.
	hexValue = ((hexValue << 4) & 0xF0);
	//Put second nibble in.
	*char_to_put = fgetc (file);
	clear_special_char(file, char_to_put, total_chars_read);
	//Put the nibbles together.
	hexValue |= (Ascii2Hex(*char_to_put));
	//Return the byte.
	*total_chars_read += 2;

	return hexValue;
}

void clear_special_char(FILE * file, uint8_t * charToPut, int * totalCharsRead)
{
	//Removes CR, LF, ':'  --Bdk6's
	while (*charToPut == '\n' || *charToPut == '\r' || *charToPut ==':'){
		(*charToPut = fgetc (file));
		*totalCharsRead++;
	}
}

uint8_t Ascii2Hex(uint8_t c)
{
	if (c >= '0' && c <= '9')
	{
		return (uint8_t)(c - '0');
	}
	if (c >= 'A' && c <= 'F')
	{
		return (uint8_t)(c - 'A' + 10);
	}
	if (c >= 'a' && c <= 'f')
	{
        return (uint8_t)(c - 'a' + 10);
	}

	return 0;  // this "return" will never be reached, but some compilers give a warning if it is not present
}

bool read_line_from_hex_file(FILE * file, uint8_t line_of_data[], long int * combined_address, int * bytes_this_line)
{
		int data_index = 0;
		uint8_t char_to_put;
		int total_chars_read = 0;

		//To hold file hex values.
		uint8_t byte_count;
		uint8_t datum_address1;
		uint8_t datum_address2;
		uint8_t datum_record_type;
		uint8_t datum_check_sum;

		//BYTE COUNT
		byte_count = read_byte_from_file(file, &char_to_put, &total_chars_read);

		// No need to read, if no data.
		if (byte_count == 0){return false;}

		//ADDRESS1 //Will create an 8 bit shift. --Bdk6's
		datum_address1 = read_byte_from_file(file, &char_to_put, &total_chars_read);

		//ADDRESS2
		datum_address2 = read_byte_from_file(file, &char_to_put, &total_chars_read);

		//RECORD TYPE
		datum_record_type = read_byte_from_file(file, &char_to_put, &total_chars_read);		

		// No need to read, if not data.
		if (datum_record_type != 0){return false;}

		*combined_address = ((uint16_t)datum_address1 << 8) | datum_address2;

		// DATA
		while(data_index < byte_count)
		{
			line_of_data[data_index] = read_byte_from_file(file, &char_to_put, &total_chars_read);
			data_index++;
		}			
		*bytes_this_line = data_index;

		// CHECKSUM
		datum_check_sum = read_byte_from_file(file, &char_to_put, &total_chars_read);

		return true;
}

int hex_file_line_count(FILE * file_to_count)
{
	int line_count = 0;
	char got_char;

	while(got_char != EOF)
	{
		got_char = fgetc(file_to_count);
		if (got_char == ':'){line_count++;}
	}
	rewind(file_to_count);
	return line_count;
}

int hex_file_to_array(FILE * file, uint8_t hex_data[])
{
	// 1. Get line count.
	// 2. Read a line. From ':' to '\n'
	// 3. Parse a line. 
	//   Repeat for all lines.

	// Data per line.
	uint8_t line_of_data[32];
	long int combined_address[4096];

	// Indices and counters
	int hex_line_index = 0;
	int chars_this_line = 0;
	int total_chars_read = 0;
	// How many lines in the hexfile?
	int hex_lines_in_file = 0;
	int bytes_this_line[4096];

	// Let's count how many lines are in this file.
	hex_lines_in_file = hex_file_line_count(file);

	// Indices for parsing.
	int line_index = 0;
	int byte_index = 0;
	bool read_line_ok = false;

	// Parse all lines in file.
	while(line_index < hex_lines_in_file)
	{
		read_line_ok = read_line_from_hex_file(file, line_of_data, &combined_address[line_index], &bytes_this_line[line_index]);
		if (!read_line_ok)
		{
			printf("Line#: %i. Dude, that's not data!\n", line_index);
			read_line_ok = true;
		}
		while(byte_index < bytes_this_line[line_index])
		{
			hex_data[combined_address[line_index] + byte_index] = line_of_data[byte_index];
			line_of_data[byte_index] = '\0';
			byte_index++;
		}
		byte_index = 0;
		line_index++;
	}

	// Print out parsed data.
	int k = 0;
	int j = 0;
	int printed_bytes = 0;
	while (k < hex_lines_in_file)
	{
		while(j < bytes_this_line[k])
		{
			printf("%02X ", hex_data[j+(printed_bytes)]);
			j++;
		}
		printed_bytes += bytes_this_line[k];
		j=0;
		printf("\n");
		k++;
	}
	
	return total_chars_read;
} // End hex_file_to_array