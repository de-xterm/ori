#pragma once

#include <iostream>
#include <sstream>
#include <cassert>

#ifdef __unix__
    #include <unistd.h>
    #include <sys/ioctl.h>
#elif defined(_WIN32) || defined(__WIN32__)
    #include <windows.h>
    #include <io.h>
#else
    #error "Platform not supported. Sorry!\n";
#endif

namespace ori {
    namespace detail {
        #ifdef ORI_IMPL
            #ifdef __unix__
                int get_file_descriptor_(const std::ostream& stream) {
                    int fd;
                    if (&stream == &std::cout) {
                        if (isatty(fileno(stdout))) {
                            fd = fileno(stdout);
                        }
                        else {
                            throw std::runtime_error("get_term_width error: output stream did not refer to a tty");
                        }
                    }
                    else if (&stream == &std::cerr) {
                        if (isatty(fileno(stderr))) {
                            fd = fileno(stderr);
                        }
                        else {
                            throw std::runtime_error("get_term_width error: output stream did not refer to a tty");
                        }
                    }
                    else {
                        throw std::runtime_error("get_term_width error: output stream was not cout or cerr");
                    }

                    return fd;
                }

                unsigned get_term_width_(const std::ostream& stream) {
                    struct winsize w;
                    assert(ioctl(get_file_descriptor_(stream), TIOCGWINSZ, &w) >= 0);

                    return w.ws_col;
                }
            #endif

            #if defined(_WIN32) || defined(__WIN32__)
                HANDLE get_file_handle_(const std::ostream& stream) {
                    DWORD handle;
                    if (&stream == &std::cout) {
                        if (_isatty(fileno(stdout))) {
                            handle = STD_OUTPUT_HANDLE;
                        }
                        else {
                            throw std::runtime_error("get_term_width error: output stream did not refer to a tty");
                        }
                    }
                    else if (&stream == &std::cerr) {
                        if (_isatty(fileno(stderr))) {
                            handle = STD_ERROR_HANDLE;
                        }
                        else {
                            throw std::runtime_error("get_term_width error: output stream did not refer to a tty");
                        }
                    }
                    else {
                        throw std::runtime_error("get_term_width error: output stream was not cout or cerr");
                    }

                    return GetStdHandle(handle);
                }

                unsigned get_term_width_(const std::ostream& stream) {
                    CONSOLE_SCREEN_BUFFER_INFO csbi; // implementation comes from https://stackoverflow.com/a/12642749

                    assert(GetConsoleScreenBufferInfo(get_file_handle_(stream), &csbi));
                    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
                }
            #endif

            static unsigned current_indent_ = 0;

            void print_impl_(std::ostream& stream, const std::string& in_str, unsigned indent, unsigned right_padding, bool hyphenate_cutoffs) {
                if(indent == -1) {
                    indent = current_indent_;
                }

                std::string out_str(indent, ' ');
                unsigned term_width = detail::get_term_width_(stream);
                unsigned max_line_width = term_width - indent - right_padding;

                unsigned index_in_current_line = 0;
                unsigned current_line_start_i = 0;
                for (unsigned source_i = 0; source_i < in_str.size(); ++source_i) {
                    if (in_str[source_i] == '\n') {
                        out_str += in_str.substr(current_line_start_i, source_i - current_line_start_i);

                        if (source_i && (in_str[source_i - 1] != '\n')) {
                            out_str += '\n';
                        }

                        for (unsigned space_i = 0; space_i < indent; ++space_i) {
                            out_str += ' ';
                        }

                        if ((source_i + 1 < in_str.size()) && (in_str[source_i + 1] != '\n')) {
                            ++source_i;
                        }

                        index_in_current_line = 0;
                        current_line_start_i = source_i;
                    }
                    else if (index_in_current_line == max_line_width) {
                        for (unsigned j = source_i; j > current_line_start_i; --j) {

                            if (std::isspace(in_str[j])) {
                                out_str += in_str.substr(current_line_start_i, j - current_line_start_i);
                                out_str += '\n';
                                for (unsigned space_i = 0; space_i < indent; ++space_i) {
                                    out_str += ' ';
                                }

                                for (unsigned k = j; k < in_str.size(); ++k) {
                                    if (!std::isspace(in_str[k])) {
                                        source_i = k;

                                        index_in_current_line = 0;
                                        current_line_start_i = source_i;
                                        goto end_of_first_loop;
                                    }
                                }
                            }
                        }
                        // if we reach this code, that means that the whole line had no spaces, so we have no choice but to cut it off
                        out_str += in_str.substr(current_line_start_i, max_line_width - 1);
                        if (hyphenate_cutoffs) {
                            out_str += '-';

                        }
                        if (source_i) {
                            --source_i;
                        }

                        if (source_i < in_str.size() - 1) {
                            out_str += '\n';
                            for (unsigned space_i = 0; space_i < indent; ++space_i) {
                                out_str += ' ';
                            }
                        }

                        index_in_current_line = 0;
                        current_line_start_i = source_i;
                        goto end_of_first_loop;
                    }

                    end_of_first_loop:;
                    ++index_in_current_line;
                }
                if (in_str.size() - current_line_start_i) {
                    out_str += in_str.substr(current_line_start_i, in_str.size() - current_line_start_i);
                }

                std::cout << out_str;
            }

            const std::string& to_string_(const std::string& str) {
                return str;
            }

            std::string to_string(const char* cstr) {
                return {cstr};
            }

            std::string to_string(char c) {
                   return {1, c};
            }

        #else // end of #ifdef ORI_IMPL block
            void print_impl_(std::ostream& stream, unsigned indent, unsigned right_padding, bool hyphenate_cutoffs, const std::string& in_str);
            const std::string& to_string_(const std::string& str);
            std::string to_string(const char* cstr);
            std::string to_string(char c);
        #endif

        template<typename T>
        std::string to_string_(const T& arg) {
            std::stringstream ss; // uh oh probably very slow
            ss << arg;
            return ss.str();
        }
    }

    #ifdef ORI_IMPL
        void set_indent(unsigned indent) {
            detail::current_indent_ = indent;
        }

        unsigned get_indent() {
            return detail::current_indent_;
        }

        void change_increment(int difference) {
            detail::current_indent_ += difference;
        }
    #endif

    constexpr unsigned use_global_indent = -1; // this is well-defined

    void set_indent(unsigned indent);
    unsigned get_indent();

    /// adds difference to current increment
    void change_increment(int difference);


    template<typename T>
    std::ostream& print(std::ostream& stream, const T& val, unsigned indent = use_global_indent, unsigned right_padding = 0, bool hyphenate_cutoffs = false) {
        decltype(auto) in_str = detail::to_string_(val);
        detail::print_impl_(stream, in_str, indent, right_padding, hyphenate_cutoffs);
        return stream;
    }

    /// default stream is std::cout
    template<typename T>
    std::ostream& print(const T& val, unsigned indent = use_global_indent, unsigned right_padding = 0, bool hyphenate_cutoffs = false) {
        decltype(auto) in_str = detail::to_string_(val);
        return print(std::cout, in_str, indent, right_padding, hyphenate_cutoffs);
    }

    template<typename T>
    std::ostream& println(std::ostream& stream, const T& val, unsigned indent = use_global_indent, unsigned right_padding = 0, bool hyphenate_cutoffs = false) {
        return print(stream, val, indent, right_padding, hyphenate_cutoffs) << '\n';
    }

    /// default stream is std::cout
    template<typename T>
    std::ostream& println(const T& val, unsigned indent = use_global_indent, unsigned right_padding = 0, bool hyphenate_cutoffs = false) {
        return print(val, indent, right_padding, hyphenate_cutoffs) << '\n';
    }
}