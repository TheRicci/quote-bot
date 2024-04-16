#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <signal.h>
#include <curl/curl.h>
#include <libnotify/notify.h>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>

std::string pid_file = "/var/run/mindfulness_daemon.pid"; // Change this to a suitable location

// Function to handle termination signal
void signalHandler(int signum) {
    if (signum == SIGTERM) {
        std::cout << "Received SIGTERM. Stopping the daemon." << std::endl;
        remove(pid_file.c_str()); // Remove the PID file
        exit(EXIT_SUCCESS);
    }
}

void writePidFile() {
    std::ofstream pid_stream(pid_file);
    if (pid_stream.is_open()) {
        pid_stream << getpid();
        pid_stream.close();
    } else {
        std::cerr << "Failed to open PID file for writing: " << strerror(errno) << std::endl;
    }
}

// Function to handle HTTP GET request using libcurl
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

std::string getQuote() {
    std::string url = "https://api.openai.com/v1/chat/completions";
    std::string api_key = "API_KEY"; 
    std::string prompt = "Generate a quote pretending to be Allan Wats:";
    std::string data = "{\"model\":\"gpt-3.5-turbo\",\"prompt\":\"" + prompt + "\",\"max_tokens\":50}";

    CURL *curl;
    CURLcode res;
    std::string response;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return response;
}

// Function to show notification using libnotify
void showNotification(const char* message) {
    notify_init("Alan Watts quote");
    NotifyNotification *notification = notify_notification_new("Alan says: ", message, nullptr);
    notify_notification_set_timeout(notification, 4000);
    notify_notification_show(notification, nullptr);
    g_object_unref(G_OBJECT(notification));
    notify_uninit();
}

int main(int argc, char *argv[])  {
    // Check for command-line arguments
    if (argc == 2 && strcmp(argv[1], "--shutdown") == 0) {
        // Read PID from file
        std::ifstream pid_stream(pid_file);
        if (pid_stream.is_open()) {
            pid_t pid;
            pid_stream >> pid;
            pid_stream.close();
            // Send termination signal
            if (kill(pid, SIGTERM) == 0) {
                std::cout << "Sent SIGTERM signal to PID " << pid << std::endl;
                return 0;
            } else {
                std::cerr << "Failed to send SIGTERM signal to PID " << pid << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Failed to open PID file: " << pid_file << std::endl;
            return 1;
        }
    }

    // Daemonize the process
    pid_t pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    umask(0);

    // Change the working directory to root
    if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
    }

    // Register signal handler
    signal(SIGTERM, signalHandler);

    // Write PID to file
    writePidFile();

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Main daemon loop
    while (true) {
        std::string quote = getQuote();
        Extracting the quote from the JSON response
        const char* quote_json = quote.c_str();
        const char* quote_start = strstr(quote_json, "\"choices\":[{\"text\":\"") + strlen("\"choices\":[{\"text\":\"");
        const char* quote_end = strstr(quote_start, "\"");
        char message[256];
        strncpy(message, quote_start, quote_end - quote_start);
        message[quote_end - quote_start] = '\0';
        
        showNotification(message);
        
        // Wait for 1 hour before fetching the next quote
        std::this_thread::sleep_for(std::chrono::hours(1));
    }

    return 0;
}
