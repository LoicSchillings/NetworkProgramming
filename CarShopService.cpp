// CarShopService.cpp
// g++ CarShopService.cpp -o carshop_service -lzmq

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <zmq.hpp>

struct Car {
    std::string brand;
    std::string model;
    int price;
    std::string image_url;
};

std::vector<Car> showroom = {
    {"Ferrari", "F40", 120, "https://example.com/ferrari-f40.jpg"},
    {"Toyota", "Corolla", 30, "https://example.com/toyota-corolla.jpg"},
    {"Lamborghini", "Aventador", 110, "https://example.com/aventador.jpg"},
    {"Volkswagen", "Golf", 40, "https://example.com/golf.jpg"},
    {"Tesla", "Model S", 90, "https://example.com/model-s.jpg"}
};

std::unordered_map<std::string, int> player_points;
std::unordered_map<std::string, std::vector<Car>> player_cars;

std::pair<std::string, std::string> parse_shop_message(const std::string& msg) {
    // Format: Loic>CarShop?>Naam>Actie>
    size_t start = msg.find("?>") + 2;
    size_t mid = msg.find(">", start);
    std::string name = msg.substr(start, mid - start);

    size_t action_start = mid + 1;
    size_t action_end = msg.find(">", action_start);
    std::string action = msg.substr(action_start, action_end - action_start);

    return {name, action};
}

int main() {
    zmq::context_t context{1};
    zmq::socket_t sub_socket{context, zmq::socket_type::sub};
    zmq::socket_t push_socket{context, zmq::socket_type::push};

    sub_socket.connect("tcp://benternet.pxl-ea-ict.be:24042");
    push_socket.connect("tcp://benternet.pxl-ea-ict.be:24041");

    std::string topic = "Loic>CarShop?>";
    sub_socket.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.size());

    while (true) {
        zmq::message_t msg;
        sub_socket.recv(msg, zmq::recv_flags::none);
        std::string received_msg(static_cast<char*>(msg.data()), msg.size());

        auto [name, action] = parse_shop_message(received_msg);
        std::string response_topic = "Loic>CarShop!>" + name + ">";
        std::string response;

        if (action == "list") {
            response = "Te koop:\n";
            for (size_t i = 0; i < showroom.size(); ++i) {
                const Car& car = showroom[i];
                response += std::to_string(i) + ". " + car.brand + " " + car.model + " - " + std::to_string(car.price) + " punten\n";
            }
            response += "Gebruik: Loic>CarShop?>Naam>buy_<index> om te kopen.";
        } else if (action.rfind("buy_", 0) == 0) {
            int index = std::stoi(action.substr(4));
            if (index >= 0 && index < (int)showroom.size()) {
                Car car = showroom[index];
                int& points = player_points[name];

                if (points >= car.price) {
                    points -= car.price;
                    player_cars[name].push_back(car);
                    response = "Je hebt " + car.brand + " " + car.model + " gekocht!\nFoto: " + car.image_url +
                               "\nResterende punten: " + std::to_string(points);
                } else {
                    response = "Niet genoeg punten om deze auto te kopen.";
                }
            } else {
                response = "Ongeldige keuze.";
            }
        } else if (action == "mijn") {
            const auto& cars = player_cars[name];
            if (cars.empty()) {
                response = "Je hebt nog geen auto's gekocht.";
            } else {
                response = "Jouw collectie:\n";
                for (const Car& car : cars) {
                    response += "- " + car.brand + " " + car.model + "\n";
                }
            }
        } else {
            response = "Ongeldige actie. Gebruik 'list', 'buy_<index>' of 'mijn'.";
        }

        std::string full_response = response_topic + response + ">";
        zmq::message_t reply(full_response.begin(), full_response.end());
        push_socket.send(reply, zmq::send_flags::none);
    }

    return 0;
}
