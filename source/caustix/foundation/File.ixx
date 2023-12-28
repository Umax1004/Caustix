module;

#include <stdio.h>

#include <filesystem>

export module Foundation.File;

import Foundation.Memory.Allocators.Allocator;
import Foundation.Memory.MemoryDefines;
import Foundation.Platform;

export namespace Caustix {
    struct FileReadResult {
        char*                       data;
        sizet                       size;
    };

    char*           FileReadBinary(cstring filename, Allocator* allocator, sizet* size);
    char*           FileReadText(cstring filename, Allocator* allocator, sizet* size);

    FileReadResult  FileReadBinary(cstring filename, Allocator* allocator);
    FileReadResult  FileReadText(cstring filename, Allocator* allocator);
}

namespace Caustix {
    char* FileReadBinary(cstring filename, Allocator* allocator, sizet* size) {
        char* outData = 0;

        FILE* file = fopen( filename, "rb" );

        if ( file ) {
            sizet filesize = std::filesystem::file_size(filename);

            outData = ( char* )calloca( filesize + 1, allocator );
            fread( outData, filesize, 1, file );
            outData[filesize] = 0;

            if ( size )
                *size = filesize;

            fclose( file );
        }

        return outData;
    }

    char* FileReadText(cstring filename, Allocator* allocator, sizet* size) {
        char* text = 0;

        FILE* file = fopen( filename, "r" );

        if ( file ) {
            sizet filesize = std::filesystem::file_size(filename);
            text = (char*)calloca( filesize + 1, allocator );
            // Correct: use elementcount as filesize, bytes_read becomes the actual bytes read
            // AFTER the end of line conversion for Windows (it uses \r\n).
            sizet bytesRead = fread(text, 1, filesize, file);

            text[bytesRead] = 0;

            if ( size )
                *size = filesize;

            fclose(file);
        }

        return text;
    }

    FileReadResult FileReadBinary(cstring filename, Allocator* allocator) {
        FileReadResult result { nullptr, 0 };

        FILE* file = fopen( filename, "rb" );

        if ( file ) {
            sizet filesize = std::filesystem::file_size(filename);

            result.data = ( char* )calloca(filesize, allocator);
            fread(result.data, filesize, 1, file);

            result.size = filesize;

            fclose(file);
        }

        return result;
    }

    FileReadResult  FileReadText(cstring filename, Allocator* allocator) {
        FileReadResult result{ nullptr, 0 };

        FILE* file = fopen( filename, "r" );

        if ( file ) {
            sizet filesize = std::filesystem::file_size(filename);

            result.data = ( char* )calloca(filesize + 1, allocator);
            // Correct: use elementcount as filesize, bytes_read becomes the actual bytes read
            // AFTER the end of line conversion for Windows (it uses \r\n).
            sizet bytesRead = fread( result.data, 1, filesize, file );

            result.data[bytesRead] = 0;

            result.size = bytesRead;

            fclose(file);
        }

        return result;
    }
}
