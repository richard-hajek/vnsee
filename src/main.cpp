#include "options.hpp"
#include "app/client.hpp"
#include "config.hpp"
#include "network.hpp"
#include "rmioc/buttons.hpp"
#include "rmioc/pen.hpp"
#include "rmioc/screen.hpp"
#include "rmioc/touch.hpp"
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept> // IWYU pragma: keep
// IWYU pragma: no_include <bits/exception.h>
#include <string>
#include <utility>
#include <vector>

/**
 * Print a short help message with usage information.
 *
 * @param name Name of the current executable file.
 */
auto help(const char* name) -> void
{
    std::cout << "Usage: " << name << " [IP [PORT]] [OPTION...]\n"
"Connect to the VNC server at IP:PORT.\n\n"
"If " PROJECT_NAME " is launched without a specific IP, it will scan\n"
"for VNC servers running on the default port in the local USB network.\n"
"By default, PORT is 5900.\n\n"
"Available options:\n"
"  -h, --help           Show this help message and exit.\n"
"  -v, --version        Show the current version of " PROJECT_NAME " and exit.\n"
"  --no-buttons         Disable buttons interaction.\n"
"  --no-pen             Disable pen interaction.\n"
"  --no-touch           Disable touchscreen interaction.\n";
}

/**
 * Print current version.
 */
auto version() -> void
{
    std::cout << PROJECT_NAME << ' ' << PROJECT_VERSION << '\n';
}

constexpr int default_server_port = 5900;
constexpr int min_port = 1;
constexpr int max_port = (1U << 16U) - 1;

auto main(int argc, const char* argv[]) -> int
{
    // Read options from the command line
    std::string server_ip;
    int server_port = default_server_port;
    bool enable_buttons = true;
    bool enable_pen = true;
    bool enable_touch = true;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char* const name = argv[0];
    auto [opts, oper] = options::parse(argv + 1, argv + argc);

    if ((opts.count("help") >= 1) || (opts.count("h") >= 1))
    {
        help(name);
        return EXIT_SUCCESS;
    }

    if ((opts.count("version") >= 1) || (opts.count("v") >= 1))
    {
        version();
        return EXIT_SUCCESS;
    }

    if (oper.size() > 2)
    {
        std::cerr << "Too many operands: at most 2 are needed, you gave "
            << oper.size() << ".\n"
            "Run “" << name << " --help” for more information.\n";
        return EXIT_FAILURE;
    }

    if (!oper.empty())
    {
        server_ip = oper[0];
    }
    else
    {
        // No IP provided, search for a VNC server on the local network
        auto local_ips = network::get_usb_hosts();
        bool found = false;

        for (const auto& local_ip : local_ips)
        {
            if (network::tcp_can_connect(
                local_ip,
                default_server_port
            ))
            {
                server_ip = network::ip_to_string(local_ip);
                found = true;
                break;
            }
        }

        if (!found)
        {
            std::cerr << "No server IP given and no VNC server running on\n"
                "port " << default_server_port << " found in the local USB "
                "network.\nPlease specify a VNC server IP.\n"
                "Run “" << name << " --help” for more information.\n";
            return EXIT_FAILURE;
        }
    }

    if (oper.size() == 2)
    {
        try
        {
            server_port = std::stoi(oper[1]);
        }
        catch (const std::invalid_argument&)
        {
            std::cerr << "“" << oper[1]
                << "” is not a valid port number.\n";
            return EXIT_FAILURE;
        }

        if (server_port < min_port || server_port > max_port)
        {
            std::cerr << server_port << " is not a valid port "
                "number. Valid values range from " << min_port << " to "
                << max_port << ".\n";
            return EXIT_FAILURE;
        }
    }

    if (opts.count("no-buttons") >= 1)
    {
        enable_buttons = false;
        opts.erase("no-buttons");
    }

    if (opts.count("no-pen") >= 1)
    {
        enable_pen = false;
        opts.erase("no-pen");
    }

    if (opts.count("no-touch") >= 1)
    {
        enable_touch = false;
        opts.erase("no-touch");
    }

    if (!opts.empty())
    {
        std::cerr << "Unknown options: ";

        for (
            auto opt_it = std::cbegin(opts);
            opt_it != std::cend(opts);
            ++opt_it
        )
        {
            std::cerr << opt_it->first;

            if (std::next(opt_it) != std::cend(opts))
            {
                std::cerr << ", ";
            }
        }

        std::cerr << "\n";
        return EXIT_FAILURE;
    }

    // Start the client
    try
    {
        rmioc::screen screen;
        std::unique_ptr<rmioc::buttons> buttons;
        std::unique_ptr<rmioc::pen> pen;
        std::unique_ptr<rmioc::touch> touch;

        if (enable_buttons)
        {
            buttons = std::make_unique<rmioc::buttons>();
        }

        if (enable_pen)
        {
            pen = std::make_unique<rmioc::pen>();
        }

        if (enable_touch)
        {
            touch = std::make_unique<rmioc::touch>();
        }

        std::cerr << "Connecting to "
            << server_ip << ":" << server_port << "...\n";

        app::client client{
            server_ip.data(), server_port,
            screen, buttons.get(), pen.get(), touch.get()
        };

        std::cerr << "\e[1A\e[KConnected to "
            << server_ip << ':' << server_port << "!\n";

        if (!client.event_loop())
        {
            std::cerr << "Connection closed by the server.\n";
            return EXIT_FAILURE;
        }
    }
    catch (const std::exception& err)
    {
        std::cerr << "Error: " << err.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
