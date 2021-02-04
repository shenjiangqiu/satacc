//
// Created by sjq on 2/4/21.
//

#include "boost/program_options.hpp"
#include "iostream"

void print_int(int a) {
    std::cout << a << std::endl;
}

using namespace boost::program_options;

int main(int argc, char **argv) {
    int result;
    options_description desc("test");
    desc.add_options()("help", "produce help message")("comp",
                                                       value<int>()->default_value(10)->notifier(
                                                               [&](int v) { result = v; }),
                                                       "set compress level");
    options_description cmd;
    cmd.add(desc);

    variables_map vm;
    store(command_line_parser(argc, argv).options(cmd).run(), vm);
    store(parse_config_file("conf.txt",desc),vm);
    notify(vm);
    std::cout << "v=" << result << std::endl;
    if (vm.count("help")) {
        std::cout << desc << std::endl;
    }
}