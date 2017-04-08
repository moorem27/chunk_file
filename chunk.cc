#include <string.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>


// TODO Move all helper functions into a library file
// TODO Do more error checking all around
namespace {
    // Adjust this for performance as needed
	const int max_buffer_size = 4096;
	char buffer[ max_buffer_size ];
}


std::string get_extension( const std::string& file_path ) {
	size_t last_index = file_path.find_last_of( "." );
	std::string extension = file_path.substr( last_index, file_path.length() );
	return extension;
}


std::string remove_extension( const std::string& file_path ) {
	size_t dot = file_path.find_last_of( "." );
	if ( dot == std::string::npos ) return file_path;
	return file_path.substr( 0, dot );
}


std::string clone_name( const std::string& file_path ) {
	return remove_extension( file_path ) + "_clone" + get_extension( file_path );
}


void erase_chunks( const std::vector<std::string>& paths ) {
    for( const auto& path : paths ) {
        std::remove( path.c_str() );
    }
}


// TODO Refactor this monstrosity some day
std::vector<std::string> create_file_chunks( const int chunks, const std::string& file_path ) {
    if( !file_path.empty() && chunks > 0 ) {
        std::ifstream file;
        std::vector<std::string> paths{};
        std::string extension = get_extension( file_path );
        std::string file_name = remove_extension( file_path );

        // Open the file to be read as a binary file
        file.open( file_path, std::ios_base::binary );

        // Seek to position counter to end to grab size
        file.seekg( 0, std::ifstream::end );

        // Grab total file size
        long long int size = file.tellg();
        std::cout << "Total file size: " << size << std::endl;

        // Seek back to beginning;
        file.seekg( 0, std::ifstream::beg );

        // Calculate chunk size
        long long int chunk_size = size / chunks;

        // The last amount of bytes read
        long long int last_bytes_read = 0;

        // Next byte position if a read were to occur
        long long int next_byte_position = max_buffer_size;

        std::cout << "Number of chunks: " << chunks << std::endl;
        std::cout << "Individual chunk size: " << chunk_size << std::endl;
        std::cout << "Read buffer size: " << max_buffer_size << std::endl;

        // For each chunk, do:
        for ( int i = 1; i <= chunks; ++i ) {
            memset( &buffer, 0, sizeof( buffer ) );

            // Calculate the ending byte of the current chunk
            long long int last_chunk_byte = ( ( i * size ) / chunks );

            // Create chunk file name and push to the vector of chunk names
            std::ostringstream out_file_path;
            out_file_path << file_name << i << extension;
            std::string out_file_name( out_file_path.str() );
            paths.push_back( out_file_name );

            // Open output file
            std::ofstream out_file;
            out_file.open( out_file_name, std::ios_base::binary | std::ios::out );

            if ( out_file.is_open() && file.good() ) {
                out_file.seekp( 0, std::ios_base::beg );

                // Ensure that calling read won't put us past the current chunk length by checking
                // next_byte_position vs last_chunk_byte
                while ( next_byte_position < last_chunk_byte && last_bytes_read < last_chunk_byte ) {
                    file.read( buffer, max_buffer_size );
                    out_file.write( buffer, sizeof( buffer ) );
                    memset( &buffer, 0, sizeof( buffer ) );
                    last_bytes_read = file.tellg();
                    next_byte_position += max_buffer_size;
                }
                memset( &buffer, 0, sizeof( buffer ) );

                // If there are still bytes to be read:
                if ( last_chunk_byte - last_bytes_read > 0 ) {
                    last_bytes_read = file.tellg();
                    // Read them in one character at a time until current chunk length TODO: Make this great again?
                    while ( last_bytes_read != last_chunk_byte ) {
                        file.read( buffer, sizeof( char ) );
                        out_file.write( buffer, sizeof( char ) );
                        last_bytes_read = file.tellg();
                    }
                }

                // Increment next_byte_position by max_buffer_size for the next_byte_position iteration
                next_byte_position += max_buffer_size;
            }
            out_file.close();
        }

        if (last_bytes_read == size) std::cout << "Chunked all bytes successfully!" << std::endl;

        return paths;
    } else {
        return {};
    }
}


void create_file_from_chunks( const std::vector<std::string> &paths, const std::string& out_path ) {
	std::ofstream output( out_path, std::ios_base::binary | std::ios::out );

	for( const auto& path : paths ) {
		std::ifstream file( path, std::ios_base::binary );
		output << file.rdbuf();
	}
}


int test_chunks( const std::string& file_path, const int chunks ) {
	auto begin = std::chrono::high_resolution_clock::now();
	std::vector<std::string> paths = create_file_chunks( chunks, file_path );
	std::string new_name = clone_name( file_path );
    create_file_from_chunks( paths, new_name );

    // Comment this in to erase chunks
    // erase_chunks( paths );

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Chunking took: " << std::chrono::duration_cast<std::chrono::milliseconds>( end - begin ).count() << " ms" << std::endl;
    std::cout << "               " << std::chrono::duration_cast<std::chrono::seconds>( end - begin ).count() << " s" << std::endl;
	return 0;
}


int main( void ) {
    // Adjust file path and number of chunks
	test_chunks( "/Users/matthewmoore/Desktop/network.pdf" , 4 );
    return 0;
}



