// CarGuessService.cpp
// g++ CarGuessService.cpp -o carguess_service -lzmq

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>
#include <ctime>
#include <zmq.hpp>

struct Car {
    std::string brand;
    std::string model;
    std::vector<std::string> hints;
    int rarity_points;
};

struct GameState {
    int hint_index = 0;
    int guesses_used = 0;
    Car current_car;
    bool in_progress = false;
};

std::vector<Car> cars = {
    {"Ferrari", "F40", {"Italiaans", "V8-Turbomotor", "Jaren 80", "Rood", "Supercar icoon"}, 100},
    {"Toyota", "Hilux", {"Japans", "Betrouwbaar", "Groot", "Veel verkocht", "Pick-up"}, 20},
    {"Lamborghini", "Aventador", {"V12 motor", "Luxe", "Snelle acceleratie", "Opvallend design", "Dure supercar"}, 90},
    {"Volkswagen", "Golf", {"Duits", "Compact", "Populair", "Verschillende varianten, GTi, R, GTD, ...", "Veelzijdig"}, 40},
    {"Tesla", "Cybertruck", {"Elektrisch", "Onverwoestbaar", "Amerikaans", "Pick-up", "Innovatief"}, 70}
};

std::unordered_map<std::string, int> player_points;
std::unordered_map<std::string, GameState> game_states;

std::string trim(const std::string& str) {
    const std::string whitespace = " \t\n\r";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return ""; // only whitespace
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::string to_lower(const std::string& str) {
    std::string lowered = str;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);
    return lowered;
}

std::string normalize(const std::string& str) {
    return to_lower(trim(str));
}

Car get_random_car() {
    static std::mt19937 rng(static_cast<unsigned>(time(nullptr)));
    std::uniform_int_distribution<size_t> dist(0, cars.size() - 1);
    return cars[dist(rng)];
}

std::pair<std::string, std::string> parse_message(const std::string& message) {
    size_t naam_start = message.find("?>");
    if (naam_start == std::string::npos) return {"", ""};
    size_t naam_end = message.find(">", naam_start + 2);
    std::string naam = message.substr(naam_start + 2, naam_end - (naam_start + 2));

    size_t gok_start = naam_end + 1;
    size_t gok_end = message.find(">", gok_start);
    std::string gok = message.substr(gok_start, gok_end - gok_start);

    return {naam, gok};
}

int main() {
    zmq::context_t context{1};
    zmq::socket_t sub_socket{context, zmq::socket_type::sub};
    zmq::socket_t push_socket{context, zmq::socket_type::push};

    sub_socket.connect("tcp://benternet.pxl-ea-ict.be:24042");
    push_socket.connect("tcp://benternet.pxl-ea-ict.be:24041");
    
    //sub_socket.connect("tcp://127.0.0.1:5555");
    //push_socket.connect("tcp://127.0.0.1:5556");

    std::string topic = "Loic>CarGuess?>";
    sub_socket.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.size());
    std::cout << "[CGService] Subscribed op topic: '" << topic << "'" << std::endl;

    while (true) {
        zmq::message_t msg;
        sub_socket.recv(msg, zmq::recv_flags::none);
        std::string received_msg(static_cast<char*>(msg.data()), msg.size());
        std::cout << "[CGService] Ontvangen bericht: '" << received_msg << "'" << std::endl;

        auto [name, guess] = parse_message(received_msg);
        GameState& state = game_states[name];

        if (!state.in_progress) {
            state = GameState();
            state.current_car = get_random_car();
            state.in_progress = true;
        }

        std::string response_topic = "Loic>CarGuess!>" + name + ">";
        std::string response;

        if (guess != "skip") {
            state.guesses_used++;
            std::string correct = state.current_car.brand + " " + state.current_car.model;
            
            std::cout << "[DEBUG] Guess (raw): '" << guess << "', Correct (raw): '" << correct << "'\n";
            std::cout << "[DEBUG] Guess (normalized): '" << normalize(guess) << "', Correct (normalized): '" << normalize(correct) << "'\n";
            
            if (normalize(guess) == normalize(correct)) {
                int points_awarded = state.current_car.rarity_points - (state.guesses_used - 1) * 10;
                if (points_awarded < 10) points_awarded = 10;
                player_points[name] += points_awarded;
                response = "Correct! Je wint " + std::to_string(points_awarded) + " punten.";
                state = GameState(); // Reset state
            } else if (state.guesses_used >= 3) {
                response = "Helaas, geen pogingen meer. Het juiste antwoord was: " + correct;
                state = GameState(); // Reset state
            } else {
                response = "Fout geraden. Je hebt nog " + std::to_string(3 - state.guesses_used) + " pogingen.";
            }
        } else {
            response = "Hint: " + state.current_car.hints[state.hint_index];
            state.hint_index++;
        }

        std::string full_response = response_topic + response + ">";
        std::cout << "[CGService] Verzenden naar client: '" << full_response << "'" << std::endl;
        zmq::message_t reply(full_response.begin(), full_response.end());
        push_socket.send(reply, zmq::send_flags::none);
    }

    return 0;
}
