// Steps to reproduce:
// 1. Start this app.
// 2. In the terminal run `netsh interface portproxy add v4tov4 listenaddress=localhost listenport=8005 connectaddress=<broker address> connectport=61616 protocol=tcp`
//   (replace <broker address> with your broker's address)
// 3. Wait a few seconds for the app to detect the connection.
// 4. In the terminal run `netsh interface portproxy delete v4tov4 listenaddress=localhost listenport=8005 protocol=tcp`
// 5. Wait a few seconds for the app to detect that the connection is dropped.
// 6. Do step 2. again to reestablish the connection. Notice that 2 new instances of the message "on_sender_open" appear in the terminal (there should be only one new instance).
// If you keep repeating steps 2-6 then more and more "on_sender_open" messages appear in the terminal, suggesting that there is a leak in the internals of qpid-proton.    

#include <thread>
#include <iostream>
#include <proton\connection.hpp>
#include <proton\connection_options.hpp>
#include <proton\reconnect_options.hpp>
#include <proton\container.hpp>
#include <proton\messaging_handler.hpp>

const std::string address = "tcp://localhost:8005";

class amq_proton_sender : public proton::messaging_handler
{
private:
    void on_container_start(proton::container& cont) override
    {
        proton::connection_options connection_options;
        connection_options.user("hello");
        connection_options.password("hello");
        connection_options.failover_urls(std::vector<std::string>{ address });
        cont.connect(address, connection_options);
    }
    void on_connection_open(proton::connection& conn) override
    {
        std::cout << "on_connection_open" << std::endl;
        conn.open_sender("com_ig_trade_v0_unsolicited_execution");
    }

    void on_sender_open(proton::sender&) override
    {
        std::cout << "on_sender_open" << std::endl;
    }

    void on_error(const proton::error_condition& e) override
    {
        std::cout << "on_error: " << e.description() << std::endl;
    }
};

int main()
{
    amq_proton_sender sender_ptr;
    proton::container container_ptr(sender_ptr);
    auto container_thread = std::make_unique<std::thread>([&container_ptr]() { container_ptr.run(); });
    std::this_thread::sleep_for(std::chrono::hours(1000));
}