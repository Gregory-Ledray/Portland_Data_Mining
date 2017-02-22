all: pinhole

pinhole: pinhole.c
	gcc -o pinhole ./shapelib-1.3.0/shpopen.o ./shapelib-1.3.0/dbfopen.o ./shapelib-1.3.0/safileio.o ./shapelib-1.3.0/shptree.o pinhole.c -lm

clean:
	rm -rf pinhole

debug: pinhole.c
	gcc -o pinhole ./shapelib-1.3.0/shpopen.o ./shapelib-1.3.0/dbfopen.o ./shapelib-1.3.0/safileio.o ./shapelib-1.3.0/shptree.o pinhole.c -lm -g
#gdb --args ./pinhole /project/ledra004/Data_Mining_Comp/EX/NAME/ACFS/1MO/NAME_ACFS_1MO.shp /project/ledra004/Data_Mining_Comp/2013/NIJ2013_JAN01_DEC31.csv out.txt rectangle /project/ledra004/Data_Mining_Comp/EX/NAME/ACFS/1MO/NAME_ACFS_1MO.csv
