
gps2root: gps2root.cc
	c++ -o $@ $< `root-config --cflags` `root-config --libs` `root-config --ldflags` -Wall -Wextra -g -Os 


clean: 
	rm -f gps2root 

