# chunk_file
An example of breaking a file into separate chunks and reassembling
the chunks into a clone of the original file to prove no loss of data.

To compile and run this program:   

'cd' into chunk_file directory  
'mkdir build'  
'cd build'  
'cmake ..'  
'make'  
'./chunk /path/to/file num_chunks'  

where /path/to/file is the full file path to the file
you would like to split, and num_chunks is the number 
( > 0 ) of chunks you would like to create 


