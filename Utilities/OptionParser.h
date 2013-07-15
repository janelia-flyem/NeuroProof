#ifndef OPTIONPARSER_H
#define OPTIONPARSER_H

#include <boost/program_options.hpp>
#include "ErrMsg.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

namespace NeuroProof {
class OptionParser
{
  public:
    OptionParser(std::string description_) : description(description_), 
        generic_options("Options"), hidden_options("Hidden Options")
    {
        generic_options.add_options()
            ("help,h", "Show help message");
    }

    void parse_options(int num_args, char ** arg_vals)
    {
        boost::program_options::options_description cmdline_options; 
        cmdline_options.add(generic_options).add(hidden_options);
        boost::program_options::variables_map vm;

        try {
            boost::program_options::store(boost::program_options::command_line_parser(
                        num_args, arg_vals).options(cmdline_options).positional(positional_options).run(), vm);
            if (vm.count("help")) {
                print_help();
                exit(0);
            }
            notify(vm);
        } catch (std::exception& e) {
            std::cout << "ERROR: " << e.what() << std::endl;
            print_help();
            exit(-1);
        }
    }

    void add_positional(std::string& var_ref, std::string identifier, std::string description)
    {
        hidden_options.add_options()
            (identifier.c_str(), boost::program_options::value<std::string>(&var_ref)->required(),
             description.c_str());
        positional_options.add(identifier.c_str(), 1);
        positional_ids.push_back(identifier);
        positional_descr.push_back(description);
    }

    template <typename T>
    void add_option(T& var_ref, std::string identifier, std::string description,
            bool has_default=true, bool required=false, bool hidden=false)
    {
        assert((required && !has_default) || (!required && has_default));
        if (has_default) {
            std::stringstream sstr;
            sstr << " [DEFAULT: " << var_ref << "]";
            description += sstr.str(); 
        }
        if (!required) {
            description += " [OPTIONAL]";
        }

        boost::program_options::options_description* temp_options;
        if (hidden) {
            temp_options = &hidden_options;
        } else {
            temp_options = &generic_options;
        }
        if (required) {
            (*temp_options).add_options()
                (identifier.c_str(), boost::program_options::value<T>(&var_ref)->required(),
                 description.c_str());
        } else {
            (*temp_options).add_options()
                (identifier.c_str(), boost::program_options::value<T>(&var_ref),
                 description.c_str());
        }
    } 


  private:
    void print_help()
    {
        std::cout << "Program description: " << description << std::endl;
        std::cout << "Positionals: ";
        for (unsigned int i = 0; i < positional_ids.size(); ++i) {
            std::cout << "[" << positional_ids[i] << "] ";
        }
        std::cout << std::endl; 

        for (unsigned int i = 0; i < positional_ids.size(); ++i) {
            // TODO: proper multi-line formatting
            std::cout << "  " << std::setw(31) << std::left << positional_ids[i];
            std::cout << positional_descr[i] << std::endl;
        }
        std::cout << generic_options <<std::endl;
    }
    
    
    boost::program_options::options_description generic_options; 
    boost::program_options::options_description hidden_options; 
    
    boost::program_options::positional_options_description positional_options; 

    std::vector<std::string> positional_ids;
    std::vector<std::string> positional_descr;
    std::string description;
};
}




#endif
