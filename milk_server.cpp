#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstring>
#include <vector>
#include <map>
#include <atomic>
#include <csignal>
#include <openssl/sha.h>
#include <iomanip>
#include "milk_server.h"
#include "milk_matrix.h"

const int BUFFER_SIZE = 1024;
const std::string milk_storage_file = "milk_storage.txt";
const std::string SALT = "11111111";

std::unordered_map<std::string, int> milk_storage;
std::unordered_map<std::string, Matrix> matrix_storage;
int global_milk;

ThreadPool pool;
std::atomic<bool> stop = false;
std::mutex milk_storage_mutex;

void signal_handler(int sig) {
    std::cout << "server exit: " << sig << "\n";
    stop = true;
}

int tokenize(const char (&buffer)[], std::vector<std::string>& tokens) {
    std::stringstream iss(buffer);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
        if (token == "matrix") {
            iss >> token;
            tokens.push_back(token);
            std::string data;
            getline(iss, data);
            if (data.size() <= 1) {
                return tokens.size();
            }
            data = data.substr(1, data.size() - 1);
            std::cout << "matrix data: " << data << "\n";
            tokens.push_back(data);
            break;
        }
    }

    return tokens.size();
}

int write_milk_storage_unsafe() {
    std::ofstream file;
    std::stringstream oss;

    file.open(milk_storage_file);

    oss << "global_milk: " << global_milk << "\n";

    for (auto& line : milk_storage) {
        oss << "user: " << line.first << " milk: " << line.second << "\n";
    }

    for (auto& line : matrix_storage) {
        std::string d;
        line.second.deconstruct(d);
        oss << "matrix: " << line.first << " data: " << d << "\n";
    }
    
    file << oss.str();
    file.close();

    return 0;
}

int read_milk_storage_unsafe() {
    std::ifstream file;
    std::string line;
    std::string temp;
    std::string user;
    std::string matrix;
    std::string data;
    int milk;
    milk_storage.reserve(4096);

    file.open(milk_storage_file);

    getline(file, line);
    std::stringstream iss(line);
    iss >> temp >> global_milk;

    while (getline(file, line)) {
        std::stringstream iss(line);
        iss >> temp;
        if (temp == "user:") {
            iss >> user >> temp >> milk;
            milk_storage[user] = milk;
        } 
        else if (temp == "matrix:" )
        {
            iss >> matrix >> temp;
            getline(iss, data);
            data = data.substr(1, data.size() - 1);
            matrix_storage[matrix] = Matrix();
            matrix_storage[matrix].construct(data);
        }
        else {
            continue;
        }
    }

    file.close();

    return 0;
}

void init_milk_storage() {
    std::unique_lock<std::mutex> milk_lk(milk_storage_mutex);
    
    if (std::filesystem::exists(milk_storage_file)) {
        read_milk_storage_unsafe();
        return;
    }
    
    std::fstream file(milk_storage_file);
    file << "global_milk: 0\n";
}

void produce_milk() {
    while (!stop) {
        {
            std::unique_lock<std::mutex> milk_lk(milk_storage_mutex);    
            global_milk += 100;
            write_milk_storage_unsafe();
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool hash_user(const std::string& user, std::string& hashed) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    const std::string salted = user + SALT;
    SHA256(reinterpret_cast<const unsigned char*>(salted.c_str()), salted.size(), hash);
    
    std::stringstream oss;
    for (int i = 0; i < 16; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    hashed = oss.str();

    return 1;
}

bool user_exists(std::string& user) {
     std::string hash;
    hash_user(user, hash);
    std::unique_lock<std::mutex> milk_lk(milk_storage_mutex);

    return milk_storage.count(hash);
}

int user_get_global_milk() {
    std::unique_lock<std::mutex> milk_lk(milk_storage_mutex);
    std::stringstream oss;

    oss << "[milk] global count is " << global_milk << "\n";
    std::cout << oss.str();

    return global_milk;
}

int user_register(std::string& user) {
    std::string hash;
    hash_user(user, hash);
    
    std::unique_lock<std::mutex> milk_lk(milk_storage_mutex);
    std::stringstream oss;
    int registered;

    if (!milk_storage.count(hash)) {
        milk_storage[hash] = 0;
        oss << "[register] user " << hash << "\n";
        registered = 1;
        write_milk_storage_unsafe();
    }
    else {
        throw std::runtime_error("user already exists\n");
        return -1;
    }
    
    std::cout << oss.str();
    
    return registered;
}

int user_add_milk(std::string& user, int milk) {
    std::string hash;
    hash_user(user, hash);

    std::unique_lock<std::mutex> milk_lk(milk_storage_mutex);
    std::stringstream oss;

    if (!milk_storage.count(hash)) {
        throw std::runtime_error("user does not exist\n");
        return -1;
    }

    milk_storage[hash] += milk;
    global_milk -= milk;

    oss << "[add] " << milk << " milk to user " << hash << "\n";
    std::cout << oss.str();
    write_milk_storage_unsafe();

    return 1;
}

int user_set_milk(std::string& user, int milk) {
    std::string hash;
    hash_user(user, hash);

    std::unique_lock<std::mutex> milk_lk(milk_storage_mutex);
    std::stringstream oss;

    if (!milk_storage.count(hash)) {
        throw std::runtime_error("user does not exist\n");
        return -1;
    }

    int previous_value = milk_storage[hash];
    int new_value = milk;

    milk_storage[hash] = new_value;
    global_milk -= (new_value - previous_value);

    oss << "[set] milk of user " << user << " to " << milk << "\n";
    std::cout << oss.str();
    write_milk_storage_unsafe();

    return 1;
}

int user_get_milk(std::string& user) {
    std::string hash;
    hash_user(user, hash);

    std::unique_lock<std::mutex> milk_lk(milk_storage_mutex);
    std::stringstream oss;

    if (milk_storage.count(hash)) {
        oss << "[get] milk of user " << user << " (" << milk_storage[hash] << ")" << "\n";
        std::cout << oss.str();
        return milk_storage[hash];
    }
    else {
        throw std::runtime_error("user does not exist\n");
        return 0;
    }

    return 0;
}

Matrix& user_set_matrix(std::string& identifier, std::string& data) {
    std::unique_lock<std::mutex> milk_lk(milk_storage_mutex);
    std::stringstream oss;

    try {
        matrix_storage[identifier] = Matrix();
        matrix_storage[identifier].construct(data);
        oss << "[initialize] matrix " << identifier << "\n";
    }
    catch (std::invalid_argument &e) {
        matrix_storage.erase(identifier);
        write_milk_storage_unsafe();
        oss << "[matrix error] " << e.what() << "\n";
    }
    
    std::cout << oss.str();
    write_milk_storage_unsafe();
    
    return matrix_storage[identifier];
}

Matrix& user_get_matrix(std::string& identifier) {
    std::unique_lock<std::mutex> milk_lk(milk_storage_mutex);
    std::stringstream oss;

    if (matrix_storage.count(identifier)) {
        oss << "[get] matrix " << identifier << "\n";
        std::cout << oss.str();
        return matrix_storage[identifier];
    }
    else {
        throw std::runtime_error("matrix does not exist\n");
    }
}

Matrix& user_multiply_matrix(std::string& identifier_A, std::string& identifier_B, std::string& identifier_C) {
    std::unique_lock<std::mutex> milk_lk(milk_storage_mutex);
    std::stringstream oss;

    if (!matrix_storage.count(identifier_A) || !matrix_storage.count(identifier_B)) {
        throw std::runtime_error("operand does not exist\n");
    }

    Matrix* A = &matrix_storage[identifier_A];
    Matrix* B = &matrix_storage[identifier_B];
    Matrix* A_temp = nullptr;
    Matrix* B_temp = nullptr;

    if (identifier_A == identifier_C) {
        A_temp = new Matrix();
        *A_temp = *A;
        A = A_temp;
    }
    if (identifier_B == identifier_C) {
        B_temp = new Matrix();
        *B_temp = *B;
        B = B_temp;
    }

    bool C_exists = false;

    if (matrix_storage.count(identifier_C)) {
        C_exists = true;
    }

    try {
        matrix_storage[identifier_C].multiply(*A, *B, matrix_storage[identifier_C], pool);
        write_milk_storage_unsafe();

        delete A_temp;
        delete B_temp;

        return matrix_storage[identifier_C];
    }
    catch (std::invalid_argument &e) {
        oss << "[matrix multiplication error] " << e.what() << "\n";
        if (!C_exists) {
            matrix_storage.erase(identifier_C);
        }
        throw std::invalid_argument(oss.str());

        delete A_temp;
        delete B_temp;
    }
}

int read_buffer(const char (&buffer)[], std::string& response) {
    int status = 0;
    std::stringstream oss;
    oss << "\n>>>> received: " << buffer << "\n";
    std::cout << oss.str();
    oss = std::stringstream();

    std::vector<std::string> tokens;
    tokens.reserve(8);

    tokenize(buffer, tokens);

    if (tokens[0] == "milk") {
        if (tokens.size() != 1) {
            oss << "usage: milk\n";
        }
        else {
            oss << "global milk is " << user_get_global_milk() << "\n";
        }
    }
    else if (tokens[0] == "register") {
        if (tokens.size() != 2) {
            oss << "usage: register [user]\n";
        }
        else {
            try {
                user_register(tokens[1]);
                oss << "user " << tokens[1] << " was registered\n";
            }
            catch (const std::exception& e) {
                oss << "[client error] " << e.what() << "\n";
            }
        }
    }
    else if (tokens[0] == "add") {
        if (tokens.size() != 3) {
            oss << "usage: add [user] [milk]\n";
        }
        else {
            try {
                user_add_milk(tokens[1], std::stoi(tokens[2]));
                oss << tokens[2] << " milk was added to user " << tokens[1] << "\n";
            }
            catch (const std::exception& e) {
                oss << "[client error] " << e.what() << "\n";
            }
        }
    }
    else if (tokens[0] == "set") {
        if (tokens.size() != 3) {
            oss << "usage: set [user] [milk]\n";
        }
        else {
            try {
                user_set_milk(tokens[1], std::stoi(tokens[2]));
                oss << "user " << tokens[1] << " milk was set to " << tokens[2] << "\n";
            }
            catch (const std::exception& e) {
                oss << "[client error] " << e.what() << "\n";
            }
        }
    }
    else if (tokens[0] == "get") {
        if (tokens.size() != 2) {
            oss << "usage: get [user]\n";
        }
        else {
            try {
                int milk = user_get_milk(tokens[1]);
                oss << "user " << tokens[1] << " has " << milk << " milk\n";
            }
            catch (std::runtime_error &e) {
                oss << "[client error] " << e.what() << "\n";
            }
        } 
    }
    else if (tokens[0] == "matrix") {
        if (tokens.size() == 2) {
            try {
                Matrix& m = user_get_matrix(tokens[1]);
                oss << "Matrix " << tokens[1] << " = \n";
                oss << m << "\n";
            }
            catch (std::runtime_error &e) {
                oss << "[client error] " << e.what() << "\n";
            }
        }
        else if (tokens.size() == 3) {
            try {
                Matrix& m = user_set_matrix(tokens[1], tokens[2]);
                oss << "Matrix " << tokens[1] << " = \n";
                oss << m << "\n";
            }
            catch (std::runtime_error &e) {
                oss << "[client error] " << e.what() << "\n";
            }
        }
        else {
            oss << "usage: matrix [matrix] [[data]]\n";
        } 
    }
    else if (tokens[0] == "multiply") {
        if (tokens.size() != 4) {
            oss << "usage: multiply [matrix] [matrix] [matrix]\n";
        }
        try {
            Matrix& m = user_multiply_matrix(tokens[1], tokens[2], tokens[3]);
            oss << "Matrix " << tokens[3] << " = \n";
            oss << m << "\n";
        }
        catch (std::runtime_error &e) {
            oss << "[client error] " << e.what() << "\n";
        }
    }
    else if (tokens[0] == "exit") {
        oss << "goodbye\n";
        status = 1;
    }
    else {
        oss << "unknown com\n";
        oss << "coms: register, add, set, get, matrix, multiply\n";
    }

    std::cout << "<<<< sent: " << oss.str() << "\n";
    response = oss.str();

    return status;
}

void handle_accept_client(int serverSocket) {
    while (!stop) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);

        if (clientSocket < 0) {
            perror("accept error");
            exit(1);
        }

        if (stop) {
            break;
        }

        std::cout << "accept connection\n";
        
        pool.enqueue([clientSocket]() {
            handle_client(clientSocket);
        });
    }
}

void handle_client(int clientSocket) {
    try {
        while (!stop) {
            int status = 0;
            char buffer[BUFFER_SIZE] = {0};
            int bytes_receieved = recv(clientSocket, buffer, sizeof(buffer), 0);

            if (bytes_receieved <= 0) {
                break;
            }

            std::string response = "";
            
            try {
                status = read_buffer(buffer, response);
            }
            catch (const std::exception& e) {
                response = std::string(e.what()) + "\n";
                std::cerr << "[client error] " << e.what() << "\n";
            }

            if (send(clientSocket, response.c_str(), response.size(), 0) < 0) {
                break;
            }
            
            if (status == 1) {
                break;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[exception] [handle_client] " << e.what() << "\n";
    }
    catch (...) {
        std::cerr << "[exception] [handle_client]\n";
    }

    shutdown(clientSocket, SHUT_RDWR);
    close(clientSocket);
}

int main() {
    signal(SIGINT, signal_handler);

    init_milk_storage();
    pool.init();

    for (int i = 0; i < 8; i++) {
        pool.enqueue([]() {
            produce_milk();
        });
        std::cout << "started milk producer\n";
    }

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0) {
        perror("socket error");
        exit(1);
    }
    
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    int reuse = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt error");
        exit(1);
    }

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("bind error");
        exit(1);
    }

    if (listen(serverSocket, 100) < 0) {
        perror("listen error");
        exit(1);
    }

    std::cout << "listen on port " << ntohs(serverAddress.sin_port) << "\n";

    std::thread accept_thread(handle_accept_client, serverSocket);

    while (!stop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "close server\n";

    close(serverSocket);
    milk_storage.clear();
    matrix_storage.clear();
    
    return 0;
}