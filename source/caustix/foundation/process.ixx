export module foundation.process;

import foundation.platform;

//export namespace caustix {
//
//    cstring                         process_get_output();
//}

namespace caustix {

    static char         k_process_output_buffer[ 1025 ];

    export bool process_execute( cstring working_directory, cstring process_fullpath, cstring arguments, cstring search_error_string = "" )
    {
        return false;
    }

    export cstring process_get_output() {
        return k_process_output_buffer;
    }
}

