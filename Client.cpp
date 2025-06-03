#include <zmq.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <thread>  // for std::this_thread::sleep_for

void print_menu()
{
    std::cout << "\n===== CarGame Menu =====\n";
    std::cout << "1. Speel CarGuess\n";
    std::cout << "2. Bekijk CarShop\n";
    std::cout << "3. Bekijk mijn collectie\n";
    std::cout << "4. Koop auto (index)\n";
    std::cout << "0. Afsluiten\n";
    std::cout << "> ";
}

std::string send_and_receive(zmq::socket_t& pub, zmq::socket_t& sub, const std::string& send_topic, const std::string& receive_topic, const std::string& message) {
    std::string full_msg = send_topic + message;
    zmq::message_t zmq_msg(full_msg.begin(), full_msg.end());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "SEND: '" << full_msg << "'\n";
    pub.send(zmq_msg, zmq::send_flags::none);

    while (true) {
        zmq::message_t reply;
        sub.recv(reply, zmq::recv_flags::none);
        std::string rcv_msg(static_cast<char*>(reply.data()), reply.size());
        if (rcv_msg.find(receive_topic) == 0) {
            return rcv_msg.substr(receive_topic.size());
        }
    }
}



int main() {
    zmq::context_t context(1);
    zmq::socket_t pub(context, ZMQ_PUSH);
    zmq::socket_t sub(context, ZMQ_SUB);

    sub.connect("tcp://benternet.pxl-ea-ict.be:24042");
    pub.connect("tcp://benternet.pxl-ea-ict.be:24041");

    std::string naam;
    std::cout << "Voer je spelersnaam in: ";
    std::getline(std::cin, naam);

    std::string topic_carGuess = "Loic>CarGuess!>" + naam + ">";
    std::string topic_carShop = "Loic>CarShop!>" + naam + ">";

    sub.setsockopt(ZMQ_SUBSCRIBE, topic_carGuess.c_str(), topic_carGuess.size());
    sub.setsockopt(ZMQ_SUBSCRIBE, topic_carShop.c_str(), topic_carShop.size());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    bool running = true;
    while (running) {
        print_menu();
        int keuze;
        std::cin >> keuze;
        std::cin.ignore();
        
        switch (keuze) {
            case 1: { // CarGuess spelen
                std::vector<std::string> hints;
                int guesses_used = 0;

                for (int i = 0; i < 5; ++i) {
                    std::string req =  naam + ">skip>";
                    std::string hint = send_and_receive(pub, sub, "Loic>CarGuess?>", topic_carGuess, req);
                    std::cout << "Hint " << (i + 1) << ": " << hint << "\n";

                    if (guesses_used < 3) {
                        std::cout << "Wil je raden? (ja/nee): ";
                        std::string choice;
                        std::getline(std::cin, choice);

                        if (choice == "ja") {
                            std::cout << "Voer je gok in (merk model): ";
                            std::string gok;
                            std::getline(std::cin, gok);
                            std::string guess_req = naam + ">" + gok + ">";
                            std::string antwoord = send_and_receive(pub, sub, "Loic>CarGuess?>", topic_carGuess, guess_req);
                            std::cout << "Antwoord: " << antwoord << "\n";
                            ++guesses_used;

                            if (antwoord.find("Correct") != std::string::npos) {
                                break;
                            }
                        }
                    }
                }
                break;
            }
            case 2: { // Bekijk shop
                std::string req = naam + ">list>";
                std::string antwoord = send_and_receive(pub, sub, "Loic>CarShop?>", topic_carShop, req);

                std::cout << antwoord << "\n";
                break;
            }
            case 3: { // Bekijk collectie
                std::string req = "Loic>CarShop?>" + naam + ">mijn>";
                std::string antwoord = send_and_receive(pub, sub, "Loic>CarShop?>", topic_carShop, req);

                std::cout << antwoord << "\n";
                break;
            }
            case 4: { // Koop auto
                std::cout << "Voer index in van auto die je wil kopen: ";
                int index;
                std::cin >> index;
                std::cin.ignore();
                std::ostringstream req;
                req << "Loic>CarShop?>" << naam << ">buy_" << index << ">";
                std::string antwoord = send_and_receive(pub, sub, "Loic>CarShop?>", topic_carShop, req.str());
                std::cout << antwoord << "\n";
                break;
            }
            case 0:
                running = false;
                break;
            default:
                std::cout << "Ongeldige keuze.\n";
                break;
        }
    }

    std::cout << "Tot ziens, " << naam << "!\n";
    return 0;
}
