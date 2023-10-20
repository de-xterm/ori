#include <iostream>
#include <sstream>

#include <unistd.h>
#include <term.h>
#include <cstring>
#include <fcntl.h>
#include <ncurses.h>

unsigned get_term_width( void ) {
  char const *const term = getenv( "TERM" );
  if ( term == NULL ) {
    fprintf( stderr, "TERM environment variable not set\n" );
    return 100;
  }

  char const *const cterm_path = ctermid( NULL );
  if ( cterm_path == NULL || cterm_path[0] == '\0' ) {
    fprintf( stderr, "ctermid() failed\n" );
    return 100;
  }

  int tty_fd = open( cterm_path, O_RDWR );
  if ( tty_fd == -1 ) {
    fprintf( stderr,
      "open(\"%s\") failed (%d): %s\n",
      cterm_path, errno, strerror( errno )
    );
    return 100;
  }

  int cols = 0;
  int setupterm_err;
  if ( setupterm( (char*)term, tty_fd, &setupterm_err ) == ERR ) {
    switch ( setupterm_err ) {
      case -1:
        fprintf( stderr,
          "setupterm() failed: terminfo database not found\n"
        );
        goto done;
      case 0:
        fprintf( stderr,
          "setupterm() failed: TERM=%s not found in database\n",
          term
        );
        goto done;
      case 1:
        fprintf( stderr,
          "setupterm() failed: terminal is hardcopy\n"
        );
        goto done;
    } // switch
  }

  cols = tigetnum( (char*)"cols" );
  if ( cols < 0 )
    fprintf( stderr, "tigetnum() failed (%d)\n", cols );

done:
  if (tty_fd != -1)
    close( tty_fd );
  return cols < 0 ? 0 : cols;
}

unsigned ceil_uint_div(unsigned num, unsigned denom) {
    return (num+denom-1)/denom;
}

template<typename T>
void wrap_print(unsigned indent, unsigned right_padding, bool hyphenate_cutoffs, const T& val) {
    std::stringstream ss;
    ss << val;
    std::string str = ss.str();

    std::string out_str(indent, ' ');
    unsigned term_width = get_term_width();
    unsigned max_line_width = term_width-indent-right_padding;

    unsigned dest_i = 0;
    unsigned current_line_start_i  = 0;
    for(unsigned source_i = 0; source_i < str.size(); ++source_i) {
        if(str[source_i] == '\n') {
            out_str += str.substr(current_line_start_i, source_i - current_line_start_i);

            if(source_i && (str[source_i - 1] != '\n')) {
                out_str += '\n';
            }

            for(unsigned space_i = 0; space_i < indent; ++space_i) {
                out_str += ' ';
            }

            if((source_i + 1 < str.size()) && (str[source_i + 1] != '\n')) {
                ++source_i;
            }

            //++source_i; // so we don't erroneously process the '\n' again next iteration
            dest_i = 0;
            current_line_start_i = source_i;
        }
        else if(dest_i == max_line_width) {
            for(unsigned j = source_i; j > current_line_start_i; --j) {

                if(std::isspace(str[j])) {
                    out_str += str.substr(current_line_start_i, j-current_line_start_i);
                    out_str += '\n';
                    for(unsigned space_i = 0; space_i < indent; ++space_i) {
                        out_str += ' ';
                    }

                    for(unsigned k = j; k < str.size(); ++k) {
                        if(!std::isspace(str[k])) {
                            source_i = k;

                            dest_i = 0;
                            current_line_start_i = source_i;
                            goto end_of_first_loop;
                        }
                    }
                }
            }
            // if we reach this code, that means that the whole line had no spaces, so we have no choice but to cut it off
            out_str += str.substr(current_line_start_i, max_line_width-1);
            if(hyphenate_cutoffs) {
                out_str += '-';
                if(source_i) {
                    --source_i;
                }
            }
            else {
                //--source_i;
                //--dest_i;
            }
            out_str += '\n';
            for(unsigned space_i = 0; space_i < indent; ++space_i) {
                out_str += ' ';
            }

            dest_i = 0;
            current_line_start_i = source_i;
            goto end_of_first_loop;
        }

        end_of_first_loop:;
        ++dest_i;
    }
    if(str.size()-current_line_start_i) {
        out_str += str.substr(current_line_start_i, str.size()-current_line_start_i-1);
    }

    std::cout << out_str;
}

const char* lorem =
        #include "lorem.txt"
;

const char* frost =
        #include "frost.txt"
;

const char* iroha =
        #include "iroha.txt"
               ;

int main(int argc, char** argv) {
    const char* str = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\nUt enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\n";
    const char* a   = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz\n";

    if(argc < 2 || argc > 4) {
        std::cerr << "wrong number of args!";
        return -1;
    }
    unsigned right_padding = 0;
    if(argc >= 3) {
        right_padding = std::stoi(argv[2]);
    }

    unsigned hyphenate_cutoffs = false;
    if(argc == 4) {
        hyphenate_cutoffs = std::stoi(argv[3]);
    }
    unsigned indent = std::stoi(argv[1]);
    wrap_print(indent*2, right_padding, hyphenate_cutoffs, frost);

    return 0;
}
