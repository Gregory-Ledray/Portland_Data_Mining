#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "/project/ledra004/Data_Mining_Comp/shapelib-1.3.0/shapefil.h"

//Gregory Ledray
//notes:
//this version does not support triangles
//this code is working! :D

//example command line argument:
//./pinhole /project/ledra004/Data_Mining_Comp/Large_Sq/Large\ Squares.shp /project/ledra004/Data_Mining_Comp/2013/NIJ2013_JAN01_DEC31.csv /project/ledra004/Data_Mining_Comp/Large_Sq/out.csv rectangle /project/ledra004/Data_Mining_Comp/Large_Sq/Large_Sq.csv

/* *********************************************************************
 *                   Description of this program      
 *
 * Inputs: Shapefile defining geographic boundaries, .csv file denoting
 * the coordinates of various crimes using the same coordinate system
 * as the shapefile, the word 'rectangle' and an output .csv file
 * 
 * Function: 
 * 
 * Outputs: A new .csv file with all of the previous geographic
 * information and a list of crimes by type and date in that location
 * he shapefile denotes the boundaries of the geographic areas, .csv
 * file 
 * The second file is a crime input file
 * This takes the data in the crime input file and compares coordinates 
 * to the shapefile, putting each data piece into one of the shapefile's 
 * areas.
 * *********************************************************************
*/

// Enable extra output
#define VERBOSE 0


//structs for storing input data
typedef struct crime_data
{
	int month;
	int year;
	int day;
	int x_coor;
	int y_coor;
	char cfs;//b for burglary, s for street crime, a for auto; all others o
}crime_data_t;

typedef struct shapefile_data
{
	//coordinate order:
	//lower left
	//upper left
	//lower right
	//upper right
	double x_coords[4];
	double y_coords[4];
	
	int id; // note line is id+1 if identifiers start at 1
}shapefile_data_t;

//note: this may very well contain more than one attribute - 
//just make sure the header and data are comma delineated
typedef struct attributes
{
	char attribute[1200]; //length of attributes may not exceed 500
	int length; //the real length of this attribute
}attributes_t;

typedef struct attribute_header
{
	char header[5000];
	int length;
}attribute_header_t;

//convert a number to a char array for printing
char * toCharArray(int number)
{
	int n = log10(number) + 2;
	if (number == 0){
		char *numberArray = calloc(2, sizeof(char));
		numberArray[0] = '0';
		numberArray[1] = '\0';
		return numberArray;
	}
	char *numberArray = calloc(n, sizeof(char));
	for (int i = n-2; i > -1; i--, number /= 10 )
	{
		numberArray[i] = '0' + (number % 10);
	}
	numberArray[n-1] = '\0';
	return numberArray;
}//works
//convert a number to a char array for printing
char * toCharArray_noNull(int number)
{
	if (number == 0){
		char *numberArray = calloc(1, sizeof(char));
		numberArray[0] = '0';
		return numberArray;
	}
	int n = log10(number) + 1;
  char *numberArray = calloc(n, sizeof(char));
	for (int i = n-1; i > -1; i--, number /= 10 )
	{
		numberArray[i] = '0' + (number % 10);
	}
	return numberArray;
}//works

//several functions
int input_processing(char* shapefile, char* crime_file, int shapetype, crime_data_t** crime_data_array, int* crime_data_array_size, shapefile_data_t** shapefile_data_array, int* shapefile_data_array_size);
int convert_crime_data_to_attribute_array(attribute_header_t* at_header, attributes_t** attribute_array, int attribute_array_length, crime_data_t* crime_data_array, int crime_data_array_size, shapefile_data_t* shapefile_data_array, int shapefile_data_array_size);
int populate_shapefile_with_additional_attributes(attribute_header_t at_header, char* DBFfile, attributes_t* attribute_array, int attribute_array_length, char* oFile);
void self_destruct(crime_data_t* crime_data_array, shapefile_data_t* shapefile_data_array, attributes_t* attribute_array);

int main(int argc, char* argv[])
{
	if (argc != 6) //if not all information is being provided
	{
		fprintf(stderr, "Incorrect number of command line arguments\n");
		fprintf(stderr, "%d\n", argc);
		exit(0); //exit the program
	}
	
	//check file integrity
	FILE *fp;
	char *mode = "r";
	for (int i=1;i<2;i++)
	{
		fp = fopen(argv[i], mode);
		if (fp == NULL) {
			fprintf(stderr, "Can't open file %s\n", argv[i]);
			exit(1);
		}
		fclose(fp);
	}
	
	//check integrity of the type of shape - 
	//1=>rectangle
	//2=>left triangle
	//3=>top triangle
	//4=>left-right triangle
	//5=> top-bottom triangle
	int shapetype = 0;
	if (argv[4][0] == 'r')
	{
		shapetype = 1;
	}
	else if (argv[4][0] == 't')
	{
		if (argv[4][9] == 'l') shapetype = 2;
		else if (argv[4][9] == 't') shapetype = 3;
	}
	if (shapetype==0)
	{
		fprintf(stderr, "Shapetype was not one of defined types. Shapetype: %s\n", argv[4]);
		exit(0);
	}
	
	fprintf(stderr, "Command line input checked, now running input_processing\n");
	
	crime_data_t* crime_data_array;
	shapefile_data_t* shapefile_data_array;
	int shapefile_data_array_size;
	int crime_data_array_size;
	if (input_processing(argv[1], argv[2], shapetype, &crime_data_array, &crime_data_array_size, &shapefile_data_array, &shapefile_data_array_size) == -1)
	{
		fprintf(stderr, "Input processing failed\n");
		exit(0);
	}
	//debug - shapefile_data_array is being populated correctly
	/*for (int i=0;i<shapefile_data_array_size/100; i++)
	{
		fprintf(stderr, "%d ", shapefile_data_array[i].id);
	}*/
	
	//fprintf(stderr, "\nNow checking crime data array integrity\n");
	
	//debug - crime_data_array is being populated correctly
	/*for (int i=0;i<crime_data_array_size/100; i++)
	{
		fprintf(stderr, "%c ", crime_data_array[i].cfs);
	}
	fprintf(stderr, "\n");*/
	
	//create the attribute_array which will eventually be used to print to the file
	int attribute_array_length = shapefile_data_array_size;
	attributes_t* attribute_array;//deprecated - now malloced in the function = (attributes_t*) calloc(attribute_array_length, sizeof(attributes_t));
	attribute_header_t at_header;
	
	//at this point all of the input crime and location data is stored in cimre_data_array and shapefile_data_array
	//the next step is to go through the crime data, and place that data as new attributes in the shapefile
	if (convert_crime_data_to_attribute_array(&at_header, &attribute_array, attribute_array_length, crime_data_array, crime_data_array_size, shapefile_data_array, shapefile_data_array_size)==-1)
	{
		fprintf(stderr, "Populating attribute array failed\n");
		self_destruct(crime_data_array, shapefile_data_array, attribute_array);
	}
    
    /*
    //debug
    //need to go through the attribute_array data structure and see what's there! it should be the same
    //length as the DBF file as the attributes should be sorted by location
    //attribute_array is composed of attributes_t which are literally char arrays and 
    
    for (int i=0;i<3;i++)
    {
        for (int c=0;c<attribute_array[i].length;c++)
        {
            fprintf(stderr, "%c", attribute_array[i].attribute[c]);
        }
        fprintf(stderr, "\n");
    }
    self_destruct(crime_data_array, shapefile_data_array, attribute_array);
    exit(1);
    * */
	
    //debug
    //things seem to work up to here, and yet the length of attribute_array is larger than 
    //the number of squares that are created by the shapefile/seen in the dbf file
    
	if (populate_shapefile_with_additional_attributes(at_header, argv[5], attribute_array, attribute_array_length, argv[3])==-1)
	{
		fprintf(stderr, "Populating attribute array failed\n");
		self_destruct(crime_data_array, shapefile_data_array, attribute_array);
	}
	
	fprintf(stderr, "Program complete\n");
	self_destruct(crime_data_array, shapefile_data_array, attribute_array);
}

//Destroy allocated data structures
void self_destruct(crime_data_t* crime_data_array, shapefile_data_t* shapefile_data_array, attributes_t* attribute_array)
{
	fprintf(stderr, "Self destructing\n");
	free(crime_data_array);
	free(shapefile_data_array);
	free(attribute_array);
	exit(0);
}

//returns 1 if it's the correct shape; 0 if not
int check_if_correct_shape(shapefile_data_t shape, crime_data_t crime)
{
	//crime_data_t struct information
	//lower left
	//upper left
	//lower right
	//upper right
	if ( ( ((double) crime.x_coor) <= shape.x_coords[2]) && ( ((double) crime.x_coor) >= shape.x_coords[0]) &&
	( ((double) crime.y_coor) <= shape.y_coords[1]) && ( ((double) crime.y_coor) >= shape.y_coords[0]) )
	return 1;
	
	//fprintf(stderr, "crime.x_coor: %d\n", crime.x_coor);
	//fprintf(stderr, "shape right bound: %f\n", shape.x_coords[2]);
	//fprintf(stderr, "shape left bound: %f\n", shape.x_coords[0]);
	//fprintf(stderr, "crime.y_coor: %d\n", crime.y_coor);
	//fprintf(stderr, "shape top bound: %f\n", shape.y_coords[1]);
	//fprintf(stderr, "shape bottom bound: %f\n", shape.y_coords[0]);
	return 0;
}//works

int find_week_number_non_leap_year(crime_data_t crime) //aka not 2012 or 2016
{
	if ( (crime.month == 12) && (crime.day > 26) ) return 52;//avoid week 53
	else if (crime.month >11) return ( ((crime.month-1)*31 - 7 + crime.day)/7 + 1);
	else if (crime.month > 9) return ( ((crime.month-1)*31 - 6 + crime.day)/7 + 1);
	else if (crime.month > 6) return ( ((crime.month-1)*31 - 5 + crime.day)/7 + 1);
	else if (crime.month > 4) return ( ((crime.month-1)*31 - 4 + crime.day)/7 + 1);
	else if (crime.month > 2) return ( ((crime.month-1)*31 - 3 + crime.day)/7 + 1);
	else return ((crime.month-1)*31 + crime.day)/7;
}

int find_week_number_leap_year(crime_data_t crime) //aka 2012 or 2016
{
	if ( (crime.month == 12) && (crime.day > 26) ) return 52;//avoid week 53
	else if (crime.month >11) return ( ((crime.month-1)*31 - 8 + crime.day)/7 + 1);
	else if (crime.month > 9) return ( ((crime.month-1)*31 - 7 + crime.day)/7 + 1);
	else if (crime.month > 6) return ( ((crime.month-1)*31 - 6 + crime.day)/7 + 1);
	else if (crime.month > 4) return ( ((crime.month-1)*31 - 5 + crime.day)/7 + 1);
	else if (crime.month > 2) return ( ((crime.month-1)*31 - 4 + crime.day)/7 + 1);
	else return ((crime.month-1)*31 + crime.day)/7;
}

typedef struct crime_in_area{
	int attribute_array_position; //also position in shapefile_data_array; ranges from 0 to attribute_array_length
	//this attribute is also the id of a given shapefile_data type
	//and the index in the wider array
	
	int year;
	
	//keeping track of the crimes committed in this area per week 
    //(a little over 52 weeks in a year - last week pooled with second to last)
	int total_crimes[52];
	int street_crimes[52];
	int burglaries[52];
	int motor_vehicle_theft[52];
	int other[52];
}crime_in_area_t;

typedef struct y_shapefiles{
	shapefile_data_t* y_array;
	int y_array_length;
}y_shapefiles_t;

//Helper function for formatting the header
void at_header_time_append(attribute_header_t* at_header, int i, int* x, char* year)
{
	int x_temp = *x;
	if (i<10) at_header->header[x_temp++] = i+'0';
	else {
		at_header->header[x_temp++] = '0'+i/10;
		at_header->header[x_temp++] = '0'+i%10;
	}
	at_header->header[x_temp++] = '/';
	
	for (int i=0;i<4;i++){
		at_header->header[x_temp++] = year[i];
	}
	*x = x_temp;
}//works

//helper function for converting from the crime_in_area array to the attribute_array
static void inline append_attribute(char* arr_in, int* arr_index, attributes_t** attribute_array, int i)
{
	for (int a=0;a<5;a++)
	{
		if (arr_in[a] == '\0') break;//check for early terminating string
		(*attribute_array)[i].attribute[*arr_index] = arr_in[a];
		*arr_index+=1;
        (*attribute_array)[i].length+=1;
	}
	(*attribute_array)[i].attribute[*arr_index] = ',';
    (*attribute_array)[i].length+=1;
}

//Converts input crime data in coordinate format to a format that can be 
//written into the output .csv
int convert_crime_data_to_attribute_array(attribute_header_t* at_header, attributes_t** attribute_array, int attribute_array_length, crime_data_t* crime_data_array, int crime_data_array_size, shapefile_data_t* shapefile_data_array, int shapefile_data_array_size)
{
	//debug
	//validate input: year, month, day for 50th (arbitrary) entry
	//fprintf(stderr, "year: %d\n", crime_data_array[50].year);
	//fprintf(stderr, "month: %d\n", crime_data_array[50].month);
	//fprintf(stderr, "day: %d\n", crime_data_array[50].day);
	
/* ******************************************************************** */
/* Creating the header 
 *                                                                      */
/* ******************************************************************** */
	
	/* Formatting:
	 * Header:
	 * sc[wk/yr],b[wk/yr],mvt[wk/yr],o[wk/yr],tc[wk/yr]
     * Street Crimes, Burglaries, Motor Vehicle Thefts, Other Crimes, Total Crimes
	 * for every row:
	 * 5,0,0,0,5
	 * */
	
    #if VERBOSE
	fprintf(stderr, "Creating the header file\n");
    #endif
	
	int x=0;
	char* year = toCharArray(crime_data_array[0].year);
	//fprintf(stderr, "new year value: %s\n", year);
	for (int i=1;i<53;i++)//for each week
	{
		at_header->header[x++] = ',';
		at_header->header[x++] = 's';
		at_header->header[x++] = 'c';
		at_header_time_append(at_header, i, &x, year);
		at_header->header[x++] = ',';
		at_header->header[x++] = 'b';
		at_header_time_append(at_header, i, &x, year);
		at_header->header[x++] = ',';
		at_header->header[x++] = 'm';
		at_header->header[x++] = 'v';
		at_header->header[x++] = 't';
		at_header_time_append(at_header, i, &x, year);
		at_header->header[x++] = ',';
		at_header->header[x++] = 'o';
		at_header_time_append(at_header, i, &x, year);
		at_header->header[x++] = ',';
		at_header->header[x++] = 't';
		at_header->header[x++] = 'c';
		at_header_time_append(at_header, i, &x, year);
	}
	at_header->length = x;
    
    #if VERBOSE
	fprintf(stderr, "the length of the header is: %d\n", x);
    #endif
	
	//checking whta the header is
	//fprintf(stderr, "header:\n%s\n", at_header->header);
	
/* ******************************************************************** */
/* go through the crime data and find out what shape it belongs to, 
 * populating the attribute array appropriately                         */
/* ******************************************************************** */

/* ******************************************************************** */
/*                            Strategy                                  */
/*	brute force: go through every shape and see if it's the right one for this location
	runtime ~ O(attribute_array_length * crime_data_array_length)
	
	index by X and Y coordinate
	1. Find the lowest and highest x_coord and y_coord.
	2. Find the largest x_coordinate low to x_coor high swing, and same for y_coor
	3. Sort the shapefile data by x_coordinate.
	This is possible because the shapefile data keeps its id in its id 
	attribute, so I can mix up the array and it'll still be a okay
	
	4. If there's an identical starting x-coordinate, sort by y_coord.
	
	5. since know the size of each square, place the object directly into the right spot
	6. Aka if know that largest x_coord swing is 600 and y_coord swing is 600
	7. Subtract X_coord by smallest x_coord, and divide by 600. this gives
	an exact position in an array. The position in that array points to the 
	y_coord array, which is in turn indexable the same way.
	
	the size of the x-array is (largest x_coord - smallest x_coord) / 600
	 * y_array (the actual array size) + an int for y_array_size aka that whole struct's size
	
	creation runtime: about O(attribute_array_length)
	insertion runtime: O(crime_data_array_length)
	aka total runtime is an addition instead of a multiplication. A major improvement over brute force
*/
/* ******************************************************************** */
	
	// 1. Find the lowest and highest x_coord and y_coord in shapefile_data_array
	//indexing will be done by the lower left coordinate ONLY
	//also, 2. Find the largest x_coordinate low to x_coordinate high swing, and same for y_coor
	
	double lowest_x_coord = 1000000000000.0;
	double lowest_y_coord = 1000000000000.0;
	double highest_x_coord = 0.0;
	double highest_y_coord = 0.0;
	double largest_x_swing = 0.0;
	double largest_y_swing = 0.0;
	
	for (int i=0;i<shapefile_data_array_size;i++)
	{
		//find the lowest and highest values from the lower left
		//position 0 is lower left
		if (shapefile_data_array[i].x_coords[0] < lowest_x_coord)lowest_x_coord = shapefile_data_array[i].x_coords[0];
		if (shapefile_data_array[i].y_coords[0] < lowest_y_coord)lowest_y_coord = shapefile_data_array[i].y_coords[0];
		if (shapefile_data_array[i].x_coords[0] > highest_x_coord)highest_x_coord = shapefile_data_array[i].x_coords[0];
		if (shapefile_data_array[i].y_coords[0] > highest_y_coord)highest_y_coord = shapefile_data_array[i].y_coords[0];
		
		//find the largest x_coordinate and y_coordinate swings
		double x_swing = shapefile_data_array[i].x_coords[3] - shapefile_data_array[i].x_coords[0];
		if (x_swing < 0.0) x_swing = (-1.0) * x_swing;//control for neg values
		if (x_swing > largest_x_swing) largest_x_swing = x_swing;
		double y_swing = shapefile_data_array[i].y_coords[3] - shapefile_data_array[i].y_coords[0];
		if (y_swing < 0.0) y_swing = (-1.0) * y_swing;//control for neg values
		if (y_swing > largest_y_swing) largest_y_swing = y_swing;
	}
	
    #if VERBOSE
	//debug - check pulled values - x_swing should be close to an int
	//check files for relevance of lowest_x_coord and lowest_y_coord
	fprintf(stderr, "lowest_x_coord: %f\n", lowest_x_coord);
	fprintf(stderr, "lowest_y_coord: %f\n", lowest_y_coord);
	fprintf(stderr, "largest_x_swing: %f\n", largest_x_swing);
	fprintf(stderr, "largest_y_swing: %f\n", largest_y_swing);
    #endif
	
	//Create the data structure for holding sorted data - shapefile_data_sorted_by_x
	int x_array_length = ( (highest_x_coord - lowest_x_coord) / largest_x_swing ) + 1;
	
	y_shapefiles_t* shapefile_data_sorted_by_x = calloc( x_array_length, sizeof(y_shapefiles_t) );
	int y_array_length_in = ( (highest_y_coord - lowest_y_coord) / largest_y_swing ) + 1;
	
    #if VERBOSE
	fprintf(stderr, "y_array_length: %d\n", y_array_length_in);
	fprintf(stderr, "x_array_length: %d\n", x_array_length);
	fprintf(stderr, "sizeof(shapefile_data_t): %ld\n", sizeof(shapefile_data_t));
    #endif
	
    #if VERBOSE
	fprintf(stderr, "init each member of shapefile_data_sorted_by_x\n");
    #endif
    
	//init each member of shapefile_data_sorted_by_x
	for (int i=0;i < x_array_length; i++)
	{
		y_shapefiles_t generic_y_shapefiles_array;
		generic_y_shapefiles_array.y_array_length = y_array_length_in;
		generic_y_shapefiles_array.y_array = calloc( y_array_length_in, sizeof(shapefile_data_t) );
		shapefile_data_sorted_by_x[i] = generic_y_shapefiles_array;
	}
    
	#if VERBOSE
	fprintf(stderr, "Sort the shapefile by x_coordinate by populating the x_array_length array\n");
    #endif
    
	//3 - 7. Sort the shapefile by x_coordinate by populating the x_array_length array
	for (int i=0;i<shapefile_data_array_size;i++)
	{
		int x_position = (shapefile_data_array[i].x_coords[0] - lowest_x_coord) / largest_x_swing;
		int y_position = (shapefile_data_array[i].y_coords[0] - lowest_y_coord) / largest_y_swing;
		shapefile_data_sorted_by_x[x_position].y_array[y_position] = shapefile_data_array[i];
	}
	
    #if VERBOSE
	fprintf(stderr, "Creating mammoth array to store all attribute information\n");
    #endif
	
	//create the mammoth array to store all of the attribute information prior to final storage as char arrays
	int crime_in_area_array_length = attribute_array_length;
	crime_in_area_t* crime_in_area_array = (crime_in_area_t*) calloc(attribute_array_length, sizeof(crime_in_area_t));
	
    #if VERBOSE
	fprintf(stderr, "Initializing the mammoth attribute information array\n");
    #endif
	
    //Initialize
	for (int i=0;i<attribute_array_length;i++)
	{
		for (int x=0;x<52;x++)
		{
			crime_in_area_array[i].total_crimes[x] = 0;
			crime_in_area_array[i].street_crimes[x] = 0;
			crime_in_area_array[i].burglaries[x] = 0;
			crime_in_area_array[i].motor_vehicle_theft[x] = 0;
			crime_in_area_array[i].other[x] = 0;
		}
		crime_in_area_array[i].year = crime_data_array[0].year;
		crime_in_area_array[i].attribute_array_position = i;
	}
	
/*  ******************************************************************* */
/*	Place each crime in the correct geographic location, and
	update the place in the crime_in_area array					        */
/*  ******************************************************************* */

    #if VERBOSE
	fprintf(stderr, "Placing crimes in the correct geographic locations\n");
    #endif
    
	int leapyear = 0;
	if (crime_data_array[0].year == 2012 || crime_data_array[0].year == 2016) leapyear = 1;
	
    #if VERBOSE
	fprintf(stderr, "crime_data_array_size: %d\n", crime_data_array_size);
    #endif

	for (int i=0;i<crime_data_array_size;i++)
	{
		int fail = 0;
		int attribute_array_position_in;
		
		int x_position = ( ( ((double) crime_data_array[i].x_coor ) - lowest_x_coord) / largest_x_swing ) - 1.0;
		int y_position = ( ( ((double) crime_data_array[i].y_coor ) - lowest_y_coord) / largest_y_swing ) - 1.0;
		
		//small bounds corrections
		if (x_position == x_array_length) x_position --;
		else if (x_position==-1) x_position++;
		if (y_position == y_array_length_in) y_position--;
		else if (y_position==-1) y_position++;
		
		if ( (x_position < 0) || (x_position >= x_array_length) ||  (y_position < 0) || (y_position >= y_array_length_in) )
		{
			//fprintf(stderr, "out of bounds\n");
			fail = 1;
		}
		else
		{
			attribute_array_position_in = shapefile_data_sorted_by_x[x_position].y_array[y_position].id;//seg faults here @ 30940
		}
		
		//run a quick check to make sure this element is being sorted properly; improper sorting renders this
		//whole exercise useless
		if (fail == 0)
		{
			JOT: if ( check_if_correct_shape(shapefile_data_sorted_by_x[x_position].y_array[y_position], crime_data_array[i]) )
			{
				//fprintf(stderr, "i1: %d\n", i);
				//Calculate the week to update
				int week;
				if (leapyear)
				{
					week = find_week_number_leap_year(crime_data_array[i]);
				}
				else
				{
					week = find_week_number_non_leap_year(crime_data_array[i]);
				}
				
				//find the crime type to know which arrays to update and update them
				char ct = crime_data_array[i].cfs;
				if (ct == 'b') crime_in_area_array[attribute_array_position_in].burglaries[week] += 1;
				else if (ct == 's') crime_in_area_array[attribute_array_position_in].street_crimes[week] += 1;
				else if (ct == 'a') crime_in_area_array[attribute_array_position_in].motor_vehicle_theft[week] += 1;
				else if (ct == 'o') crime_in_area_array[attribute_array_position_in].other[week] += 1;
				
				//always update the totals
				crime_in_area_array[attribute_array_position_in].total_crimes[week] += 1;
			}
			else if (x_position+1 == x_array_length) continue;//fprintf(stderr, "out of bounds 2\n");
			else if ( check_if_correct_shape(shapefile_data_sorted_by_x[x_position + 1].y_array[y_position], crime_data_array[i]) )
			{
				x_position++;
				goto JOT;
			}
			else if (y_position+1 == y_array_length_in) continue;//fprintf(stderr, "out of bounds 3\n");
			else if ( check_if_correct_shape(shapefile_data_sorted_by_x[x_position].y_array[y_position + 1], crime_data_array[i]) )
			{
				y_position++;
				goto JOT;
			}
			else if ( check_if_correct_shape(shapefile_data_sorted_by_x[x_position + 1].y_array[y_position + 1], crime_data_array[i]) )
			{
				x_position++;
				y_position++;
				goto JOT;
			}
			else
			{
                //note sorting failures
                #if VERBOSE
				fprintf(stderr, "i5: %d\n", i);
				fprintf(stderr, "failure to choose the right shape for this crime:\n");
				fprintf(stderr, "x_position: %d y_position: %d\narray x_position: %d array y_position: %d\n", 
				crime_data_array[i].x_coor, crime_data_array[i].y_coor, x_position, y_position);
				fprintf(stderr, "Position in crime_data_array: %d\n", i);
                #endif
                continue;
			}
		}
	}
	
	//free shapefile_data_sorted_by_x since the data has passed the point where that's needed
    #if VERBOSE
	fprintf(stderr, "Freeing shapefile_data_sorted_by_x since it's not needed anymore\n");
    #endif
    
	//free each member of shapefile_data_sorted_by_x
	for (int i=0;i<x_array_length;i++)
	{
		free(shapefile_data_sorted_by_x[i].y_array);
	}
	//free the overall array
	free(shapefile_data_sorted_by_x);
	
/* ******************************************************************** */
/* Convert the crime_in_area_array to an attribute_array for easy printing
 *                                                                      */
/* ******************************************************************** */
    
    #if VERBOSE
	fprintf(stderr, "Converting the crime_in_area_array to an attribute_array for easy printing\n");
    #endif
	 
	attributes_t* attribute_array_local = (attributes_t*) calloc(attribute_array_length, sizeof(attributes_t));

	//for every member of the attribute array 
    //need to write out 52 weeks' worth of crime
    //with every week having all of the crime types and total crimes
    //to do so, pass indices in by reference.
	for (int i=0;i<attribute_array_length;i++)
	{
		int arr_index=0; // 'attribute' array's index for adding another character
		//adding information to the attribute array's 'attribute' by length
		for (int x=0;x<52;x++)
		{
			append_attribute(toCharArray(crime_in_area_array[i].street_crimes[x]), &arr_index, &attribute_array_local, i);
			arr_index+=1;
			append_attribute(toCharArray(crime_in_area_array[i].burglaries[x]), &arr_index, &attribute_array_local, i);
			arr_index+=1;
			append_attribute(toCharArray(crime_in_area_array[i].motor_vehicle_theft[x]), &arr_index, &attribute_array_local, i);
			arr_index+=1;
			append_attribute(toCharArray(crime_in_area_array[i].other[x]), &arr_index, &attribute_array_local, i);
			arr_index+=1;
			append_attribute(toCharArray(crime_in_area_array[i].total_crimes[x]), &arr_index, &attribute_array_local, i);
            arr_index+=1;
		}
	}
    
    /*
    //debug
    //check that the attribute_array_local is being populated correctly with ones and zeros
    for (int i=50;i<60;i++)
    {
        fprintf(stderr, "%d\n", attribute_array_local[i].length);
        for (int c=0;c<attribute_array_local[i].length; c++)
        {
            fprintf(stderr, "%c", attribute_array_local[i].attribute[c]);
        }
        fprintf(stderr, "\n");
    }
    */
	
	//update the attribute_array with attribute_array_local
	*attribute_array = attribute_array_local;
	 
	//free locally created but now not used variables
	free(crime_in_area_array);
	return 1;
}//works

//go through the crime data, and place that data as new attributes in the shapefile
//this can be done by following the following steps:
//1. Find the file (ex. ) and convert it using LibreOffice Calc to a .csv file
//Ex. NAME_ACFS_1MO.csv in /project/ledra004/Data_Mining_Comp/EX/NAME/ACFS/1MO
//2. Run this function
//3. Convert the file back to .dbf using LibreOffice Calc
int populate_shapefile_with_additional_attributes(attribute_header_t at_header, char* DBFfile, attributes_t* attribute_array, int attribute_array_length, char* oFile)
{
/* ******************************************************************** */
/*  open the file and write additional attributes to the specified line */
/* ******************************************************************** */
	
	FILE *fp;
	char *mode = "r+";
	fp = fopen(DBFfile, "r+");
	if (fp == NULL)
	{
		fprintf(stderr, "Can't open file %s\n", DBFfile);
		return -1;
	}
    
    FILE *ofp;
    ofp = fopen(oFile, "w");
    
    if (fp == NULL)
	{
		fprintf(stderr, "Can't open file %s\n", oFile);
		return -1;
	}
	
    #if VERBOSE
	fprintf(stderr, "DBFfile opened for editing\n");
    #endif
	
	//get general information about the DBFfile

    int lines=-1;
	while(!feof(fp))
	{
		char ch = fgetc(fp);
		if(ch == '\n')
		{
			lines++;
		}
        
	}
	rewind(fp);
    
    #if VERBOSE
	fprintf(stderr, "number of lines in the DBFfile: %d\n", lines);
    #endif
    
	if ((lines) != attribute_array_length) // there's a header line that must be ignored
	{
		fprintf(stderr, "attribute array length incompatible with DBFfile; attribute_array_length: %d\n", attribute_array_length);
		return -1;
	}
	
	//put the attribute header in the first line
	char c;
	while (1)
	{
		c=getc(fp);
		if (c=='\n')
		{
			//ungetc(c, fp);//go back one character
			//write the file header
			//at_header.header[at_header.length] = '\0';
            
			//fputs(at_header.header, fp); // write the string to the existing file
            
            for (int i=0;i<at_header.length; i++)
            {
                if (at_header.header[i] == '\n') return -1;
                fputc(at_header.header[i], ofp); // write to the new file
            }
            fputc(c, ofp); // write the newline character
			break; //get out of infinite loop
		}
        
        fputc(c, ofp); // write to the new file
	}
    
    
	// c=getc(fp);//get the /n char I put back
	
	//write the rest of the information
	
	int line_in_file = 0; // the line in the file - incremented only if already written the attribute
	int line = 0; // the position in the attribute array
	while ((c = getc(fp)) != EOF)
	{
		if (c=='\n'){ //when find the end of the line, start to append it with the new attribute
			// ungetc(c, fp); //go back
            fputc(',', ofp); // add a comma at the end of the line
			if (attribute_array[line].length != 0)
			{
				// put in new characters from the attribute array
				// attribute_array[line].attribute[attribute_array[line].length] = '\0';//cap end of line
				// fputs(attribute_array[line].attribute, fp);
                
                for (int i=0;i<attribute_array[line].length; i++)
                {
                    fputc(attribute_array[line].attribute[i], ofp);
                }
			}
			else
			{
				fputc('0', ofp);
                fputc('\n', ofp);
			}
			line++;
		}
		//else if (c=='\n') line_in_file++;
        
        fputc(c, ofp); // write character to new file
	}
	
	fclose(fp);
    fclose(ofp);
	
	return 1;
}//works

int input_processing(char* shapefile, char* crime_file, int shapetype, crime_data_t** crime_data_array, int* crime_data_array_size, shapefile_data_t** shapefile_data_array, int* shapefile_data_array_size)
{
	
/* ******************************************************************** */
/* 	open the crime file and determine coordinates of vertices and id	*/
/* ******************************************************************** */

	//going opening the crime_file
	FILE *fp;
	char *mode = "r";
	fp = fopen(crime_file, mode);
	if (fp == NULL)
	{
		fprintf(stderr, "Can't open file %s\n", crime_file);
		return -1;
	}
	
	int lines=0;
	while(!feof(fp))
	{
		char ch = fgetc(fp);
		if(ch == '\n')
		{
			lines++;
		}
	}
	rewind(fp);
	fprintf(stderr, "number of lines in the crime file: %d\n", lines);
		
	//calloc array to hold the crimes
	crime_data_t* crime_data_array_local = (crime_data_t*) calloc(lines-1, sizeof(crime_data_t));
	
	//reading through the file
	char buf[200];
	fgets(buf, 200, fp);//throw away first line
	//fprintf(stderr, "%s\n", buf);
	
	int index=0;
	int lines_input = 0; // won't count garbage lines
	int skipped_lines = 0; // counting the garbage lines
	while (1){
		//read in a line
		if (fgets(buf, 200, (FILE*)fp)==NULL) break;
		
		int skip=0; // skip controls entering the for loop
		
		//store this line's data in a crime_data struct
		int c = 0;
		crime_data_t data;
		if (buf[0]=='S' || buf[0]=='s') data.cfs = 's';
		else if (buf[0]=='O' || buf[0] == 'o') data.cfs = 'o';
		else if (buf[0] == 'B'|| buf[0] == 'b') data.cfs = 'b';
		else if (buf[0] == 'M' || buf[0] == 'm') data.cfs = 'a';
		else{
			skip = 1; // activate skip to not copy lines with garbage
			skipped_lines++;
		}
		if (!skip)
		{
			//fprintf(stderr, "%s\n", buf);
			
			lines_input++;
			for (int i=0;i<200;i++)
			{
				if (buf[i]==',') c++;
				//handle month/day/year
				else if (c==4)
				{
					//fprintf(stderr, "c=4\n");
					if (buf[i+1]=='/'){
						data.month=buf[i] - '0';
						//fprintf(stderr, "month: %d\n", data.month);
					}
					else
					{
						int month;
						char in[3];
						in[0] = buf[i];
						i++;
						in[1] = buf[i];
						in[2] = '\0';
						sscanf(in, "%d", &month);
						data.month = month;
						//fprintf(stderr, "month: %d\n", month);
					}
					i+=2;
					if (buf[i+1]=='/'){
						data.day=buf[i] - '0';
						//fprintf(stderr, "day: %d\n", data.day);
					}
					else
					{
						int day;
						char in[3];
						in[0] = buf[i];
						i++;
						in[1] = buf[i];
						in[2] = '\0';
						sscanf(in, "%d", &day);
						data.day = day;
						//fprintf(stderr, "day: %d\n", day);
					}
					i+=2;
					int year;
					char in[4];
					in[0] = buf[i];
					i++;
					in[1] = buf[i];
					i++;
					in[2] = buf[i];
					i++;
					in[3] = buf[i];
					sscanf(in, "%d", &year);
					data.year = year;
					//fprintf(stderr, "yr: %d\n", year);
				}//this works
				
				//handle x-coordinate
				else if (c==5)
				{
					//fprintf(stderr, "c=5\n");
					char in[7];
					for (int x=0;x<7;x++)
					{
						in[x] = buf[i];
						i++;
					}
					int x_c;
					sscanf(in, "%d", &x_c);
					data.x_coor = x_c;
					i--;//don't pass the next comma
					//fprintf(stderr, "x_coor: %d\n", data.x_coor);
				}//works
				
				//handle y-coordinate
				else if (c==6)
				{
					//fprintf(stderr, "c=6\n");
					char in[7];
					for (int x=0;x<6;x++)
					{
						if (buf[i] != ',')
						{
							in[x] = buf[i];
						}
						else x--;
						i++;
					}
					in[6] = '\0';
					int y_c;
					sscanf(in, "%d", &y_c);
					data.y_coor = y_c;
					//fprintf(stderr, "y_coor: %d\n", data.y_coor);
					break;
				}//works
			}
		}
		
		//store that crime_data in the crime_data array
		crime_data_array_local[index] = data;
		index++;
	}//works
	
	fclose(fp);
	//debug - fprintf(stderr,"successfully finished modifying crime_data_array_local\n");
	*crime_data_array = crime_data_array_local;
	*crime_data_array_size = lines_input;
	
	//debug - now that the right command line argument is being fed, structs are being populated
	/*for (int i=0;i<lines_input; i++)
	{
		fprintf(stderr, "%c ", crime_data_array_local[i].cfs);
	}*/
	
	fprintf(stderr, "number of garbage lines: %d; num of lines_input: %d\n", skipped_lines, lines_input);

/* **************************************************************** */
/* determine coordinates of vertices and id by using shapefile api	*/
/* **************************************************************** */
	
	char *mode2 = "rb";
	//open the shapefiles
	SHPHandle shapefile_handle = SHPOpen(shapefile, mode2);
	//SHPHandle shapex_handle = SHPOpen(shapex, mode2);
    #if VERBOSE
	fprintf(stderr, "shapefile opened\n");
    #endif
	
	//get general information about both files
	int shapefile_pnEntities;
	int shapefile_pnShapeType;
	double shapefile_padfMinBound[4];
	double shapefile_padfMaxBound[4];
	SHPGetInfo( shapefile_handle, &shapefile_pnEntities, &shapefile_pnShapeType,
                 shapefile_padfMinBound, shapefile_padfMaxBound );
                 
    //calloc array
	shapefile_data_t* shapefile_data_array_local = (shapefile_data_t*) calloc(shapefile_pnEntities, sizeof(shapefile_data_t));
	//fprintf(stderr, "shapefile_data_array size: %ld sizeof(struct shapefile_data): %ld\n", sizeof(*shapefile_data_array), sizeof(struct shapefile_data));
	fprintf(stderr, "number of entities in shapefile: %d\n", shapefile_pnEntities);
	*shapefile_data_array_size = shapefile_pnEntities;//works
	
	//debug - check that array is correct - saw all zeros as expected
    
    for (int i=0;i<shapefile_pnEntities;i++)
    {
		shapefile_data_t sd;
		SHPObject *a = SHPReadObject( shapefile_handle, i);
		sd.id = a->nShapeId;//this works
		
		if (a->nSHPType == 5)//if shapetype is Polygons
		{
			sd.x_coords[0] = a->dfXMin;//lower left
			sd.x_coords[1] = a->dfXMin;//upper left
			sd.x_coords[2] = a->dfXMax;//lower right
			sd.x_coords[3] = a->dfXMax;//upper right
			sd.y_coords[0] = a->dfYMin;
			sd.y_coords[1] = a->dfYMax;
			sd.y_coords[2] = a->dfYMin;
			sd.y_coords[3] = a->dfYMax;
			//fprintf(stderr, "XMin %f, YMin %f, XMax %f, YMax %f\n", a->dfXMin,a->dfYMin, a->dfXMax, a->dfYMax);// exit(0);
		}
		else
		{
			fprintf(stderr, "shape type: %d\n", a->nSHPType);
			SHPDestroyObject(a);
			SHPClose(shapefile_handle);
			return -1;
		}
		shapefile_data_array_local[i] = sd;
		//fprintf(stderr, "sizeof(sd): %ld, sizeof(shapefile_data_array[i]): %ld", sizeof(sd), sizeof(shapefile_data_array[i]));
		//fprintf(stderr, "shapefile_data_array[i].id: %d \n", *shapefile_data_array[i].id);
		SHPDestroyObject(a);
	}
	
	//debug - check for population of this local array - works
	/*for (int i=0;i<*shapefile_data_array_size; i++)
	{
		fprintf(stderr, "%d ", shapefile_data_array_local[i].id);
	}*/
	
	SHPClose( shapefile_handle );
	
	*shapefile_data_array = shapefile_data_array_local;
    
	#if VERBOSE
	fprintf(stderr, "Shapefile processing finished\n");
    #endif

	return 1;
}//works
