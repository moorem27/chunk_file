#include <string.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <stdio.h>

// TODO Do more error checking all around
namespace {
    // Adjust this for performance as needed
    const int MAX_BUFFER_SIZE = 4096;
    char buffer[ MAX_BUFFER_SIZE ];
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


/**
 * This function splits a file into separate chunks. The file chunks are stored in the
 * same directory as the original file.
 * @param num_chunks - The number of chunks to split the file specified by file_path
 * @param file_path - The file path to the file
 * @return A vector of strings containing the file paths to the newly created chunks
 */
std::vector<std::string> create_file_chunks( const int num_chunks, const std::string& file_path ) {
    if( !file_path.empty() && num_chunks > 0 ) {
        std::ifstream file;
        std::vector<std::string> paths{};
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

        // For each chunk, do:
        for ( int i = 1; i <= num_chunks; ++i ) {
            memset( &buffer, 0, sizeof( buffer ) );

            // Calculate the ending byte of the current chunk
            long long int current_chunk_last_byte = ( ( i * file_size ) / num_chunks );

            // Create chunk file name
            std::ostringstream out_file_path;
            out_file_path << file_name << i << extension;
            std::string out_file_name( out_file_path.str() );

            FILE* outfile = nullptr;
            outfile = fopen( out_file_name.c_str(), "wb" );
            if ( outfile && file.good() ) {
                paths.push_back( out_file_name );
                while( last_byte_read < current_chunk_last_byte && current_chunk_last_byte <= file_size ) {
                    long long int bytes = 0;
                    if( last_byte_read + MAX_BUFFER_SIZE <= current_chunk_last_byte ) {
                        bytes = MAX_BUFFER_SIZE;
                    } else if( last_byte_read + MAX_BUFFER_SIZE > current_chunk_last_byte ) {
                        bytes = current_chunk_last_byte - last_byte_read;
                    }
                    file.read( buffer, bytes );
                    fwrite( buffer, static_cast<size_t>( bytes ), 1, outfile );
                    memset( &buffer, 0, static_cast<size_t>( bytes ) );
                    last_byte_read += bytes;
                }
            } else {
                std::cout << "Bad input or output file" << std::endl;
                return {};
            }
            fclose( outfile );
        }
        if( last_byte_read == file_size ) {
            std::cout << "Successfully split " << file_size << " bytes into " << num_chunks << " chunks" << std::endl;
            return paths;
        } else {
            return {};
        }
    } else {
        return {};
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
 * A function to test all of the above code
 * @param file_path - The path to the file that will be split
 * @param chunks - The number of chunks to split the file into
 * @return - Return 0 for success, -1 on error
 */
int test_chunks( const std::string& file_path, const unsigned int chunks ) {
    if( !file_path.empty() && chunks > 0 ) {
        auto begin = std::chrono::high_resolution_clock::now();
        std::vector<std::string> paths = create_file_chunks( chunks, file_path );
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "File chunking took: " << std::chrono::duration_cast<std::chrono::milliseconds>( end - begin ).count() << " ms" << '\n';
        std::cout << "                    " << std::chrono::duration_cast<std::chrono::seconds>( end - begin ).count() << " s" << '\n';

        std::string new_name = clone_name( file_path );
        if( !paths.empty() )
            create_file_from_chunks( paths, new_name );
        return 0;
    }

    return -1;
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
    
    auto begin = std::chrono::high_resolution_clock::now();    
    create_file_chunks( chunks, path );
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "File chunking took: " << std::chrono::duration_cast<std::chrono::milliseconds>( end - begin ).count() << " ms" << '\n';

    return 0;
}



