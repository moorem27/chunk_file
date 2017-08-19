#include <string.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <stdio.h>
#include <utility>
#include <thread>

// TODO Do more error checking all around
namespace {
    // Adjust this for performance as needed
	const int MAX_BUFFER_SIZE = 4096;
	char buffer[ MAX_BUFFER_SIZE ];
    int FILE_NUMBER = 1;
}

/**
 * Returns the extension of file_path
 * /path/to/file.txt returns .txt
 * @param file_path - The file path to the file
 * @return The extension with the period prepended to the front
 */
std::string get_extension( const std::string& file_path ) {
    if( !file_path.empty() ) {
        size_t last_index = file_path.find_last_of( "." );
        std::string extension = file_path.substr( last_index, file_path.length() );
        return extension;
    } else {
        return "";
    }
}

/**
 * Removes the file extension from a file name
 * /path/to/file.txt becomes /path/to/file
 * @param file_path - The file path to the file
 * @return The file path with the extension removed
 */
std::string remove_extension( const std::string& file_path ) {
    if( !file_path.empty() ) {
        size_t dot = file_path.find_last_of( "." );
        if ( dot == std::string::npos ) return file_path;
        return file_path.substr( 0, dot );
    } else {
        return "";
    }
}

/**
 * Create a new file name to be assigned to a file cloned from another file
 * @param file_path - The path to the original file
 * @return - A string representing a newly created file
 */
std::string clone_name( const std::string& file_path ) {
    if( !file_path.empty() )
	    return remove_extension( file_path ) + "_clone" + get_extension( file_path );
    else
        return "";
}

/**
 * Iterate over a vector of strings that contain file paths and remove all files
 * @param paths - A vector of strings representing file paths
 */
void erase_chunks( const std::vector<std::string>& paths ) {
    if( !paths.empty() ) {
        for ( const auto &path : paths ) {
            std::remove( path.c_str() );
        }
    }
}


std::vector<std::pair<long long int, long long int>> create_chunk_pairs( const int num_chunks, const long long int file_size ) {
    std::vector<std::pair<long long int, long long int>> chunk_pairs{};

    long long int current = 0;
    
    std::cout << "FILE SIZE: " << file_size << std::endl;
    for ( int i = 1; i <= num_chunks; ++i ) {
        long long int new_end = ( ( i * file_size ) / num_chunks );
        
        chunk_pairs.emplace_back( std::make_pair( current, new_end ) );
        current = new_end;
    }

    return chunk_pairs;
}


std::string create_file_chunk( std::pair<long long int, long long int> pair, 
                               const std::string file_path,
                               const long long int file_size,
                               const int file_number  ) {
    std::ifstream file;
    const std::string extension = get_extension( file_path );
    const std::string file_name = remove_extension( file_path );

    long long int last_byte_read = pair.first;

    // Open file
    file.open( file_path, std::ios_base::binary );

    // Seek to calculated position
    file.seekg( pair.first );
    std::ostringstream out_file_path{};
    out_file_path << file_name << file_number << extension;
    std::string out_file_name( out_file_path.str() );
    FILE* outfile = fopen( out_file_name.c_str(), "wb" );
    if( outfile && file.good() ) {
        while( last_byte_read < pair.second  && pair.second <= file_size ) {
            long long int bytes = 0;
            if( last_byte_read + MAX_BUFFER_SIZE <= pair.second ) {
                bytes = MAX_BUFFER_SIZE;
            } else if( last_byte_read + MAX_BUFFER_SIZE > pair.second ) {
                bytes = pair.second - last_byte_read;
            }
            file.read( buffer, bytes );
            fwrite( buffer, static_cast<size_t>( bytes ), 1, outfile );
            memset( &buffer, 0, static_cast<size_t>( bytes ) );
            last_byte_read += bytes;
        }
    }
    if( outfile )
        fclose( outfile );
    return out_file_name;
}


// TODO: Figure out why files differ
/**
 * This function splits a file into separate chunks. The file chunks are stored in the
 * same directory as the original file.
 * @param num_chunks - The number of chunks to split the file specified by file_path
 * @param file_path - The file path to the file
 * @return A vector of strings containing the file paths to the newly created chunks
 */
void create_file_chunks( const int num_chunks, const std::string& file_path ) {
    if( !file_path.empty() && num_chunks > 0 ) {
        std::ifstream file;
        std::vector<std::string> paths{};
        std::vector<std::thread> threads{};

        const std::string extension = get_extension( file_path );
        const std::string file_name = remove_extension( file_path );

        // Open the file to be read as a binary file
        file.open( file_path, std::ios_base::binary );

        // Seek position counter to end to grab size
        file.seekg( 0, std::ifstream::end );

        // Grab total file size
        const long long int file_size = file.tellg();

        // Seek back to beginning;
        file.seekg( 0, std::ifstream::beg );

        // The last amount of bytes read
        long long int last_byte_read = 0;

        auto pairs = create_chunk_pairs( num_chunks, file_size );

        for( const auto& pair : pairs ) {
            threads.emplace_back( std::thread( create_file_chunk, pair, file_path, file_size, FILE_NUMBER ) );
            FILE_NUMBER++;
        }

        for( auto& thread : threads )
            thread.join();
    }
}

/**
 * Assembles a complete file from chunks of files. The cloned file is stored in the same directory as the
 * file chunks.
 * @param paths - The paths to the file chunks
 * @param out_path - The path where the assembled file will be created
 */
void create_file_from_chunks( const std::vector<std::string>& paths, const std::string& out_path ) {
    if( !paths.empty() && !out_path.empty() ) {
        auto begin = std::chrono::high_resolution_clock::now();
        std::ofstream output( out_path, std::ios_base::binary | std::ios::out );

        for ( const auto &path : paths ) {
            std::ifstream file( path, std::ios_base::binary );
            output << file.rdbuf();
        }
        output.close();
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Reassembly took: " << std::chrono::duration_cast<std::chrono::milliseconds>( end - begin ).count()
                  << " ms" << std::endl;
        std::cout << "                 " << std::chrono::duration_cast<std::chrono::seconds>( end - begin ).count()
                  << " s" << std::endl;
    }
}


/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main( int argc, char* argv[] ) {
    std::string path = argv[ 1 ];
    const unsigned int chunks = static_cast<unsigned int>( atoi( argv[ 2 ] ) );
    std::cout << "Path: " << std::endl;
    std::cout << path << std::endl;
        
    create_file_chunks( chunks, path );
    return 0;
}



