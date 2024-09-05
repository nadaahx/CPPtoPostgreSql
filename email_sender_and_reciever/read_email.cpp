#include <iostream>
#include <string>
#include <curl/curl.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cctype>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <algorithm>

#define USERNAME "email@gmail.com"
#define PASSWORD "password"
#define IMAP_SERVER "imaps://imap.gmail.com:993"

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if(!ptr) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}


// Base64 decode function
std::string base64_decode(const std::string& input) {
    static const std::string base64_chars = 
                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                 "abcdefghijklmnopqrstuvwxyz"
                 "0123456789+/";

    std::string decoded;
    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (c == '=') break;
        if (std::find(base64_chars.begin(), base64_chars.end(), c) != base64_chars.end()) {
            val = (val << 6) + base64_chars.find(c);
            valb += 6;
            if (valb >= 0) {
                decoded.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
    }
    return decoded;
}

// Quoted-printable decode function
std::string decode_quoted_printable(const std::string& input) {
    std::string decoded;
    for (size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '=' && i + 2 < input.length()) {
            char hex[3] = { input[i+1], input[i+2], 0 };
            decoded += static_cast<char>(std::stoi(hex, nullptr, 16));
            i += 2;
        } else {
            decoded += input[i];
        }
    }
    return decoded;
}

// Function to read the file and decode its content
void process_email_file(const std::string& input_filename, const std::string& output_filename) {
    std::ifstream file(input_filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file " << input_filename << std::endl;
        return;
    }

    std::ofstream output_file(output_filename);
    if (!output_file.is_open()) {
        std::cerr << "Error: Could not open the file " << output_filename << std::endl;
        return;
    }

    std::string line;
    bool is_base64 = false;
    bool is_quoted_printable = false;

    while (std::getline(file, line)) {
        if (line.find("Content-Transfer-Encoding: base64") != std::string::npos) {
            is_base64 = true;
            continue;
        } else if (line.find("Content-Transfer-Encoding: quoted-printable") != std::string::npos) {
            is_quoted_printable = true;
            continue;
        }

        if (is_base64) {
            std::string decoded = base64_decode(line);
            output_file << decoded << std::endl;
        } else if (is_quoted_printable) {
            std::string decoded = decode_quoted_printable(line);
            output_file << decoded << std::endl;
        } else {
            output_file << line << std::endl;
        }
    }

    file.close();
    output_file.close();
}

int main(void) {
    CURL *curl;
    CURLcode res = CURLE_OK;

    struct MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if(curl) {
        // Specify the mailbox and the message to fetch
        curl_easy_setopt(curl, CURLOPT_URL, IMAP_SERVER "/INBOX;UID=28");  // Fetch the first email
        curl_easy_setopt(curl, CURLOPT_USERNAME, USERNAME);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, PASSWORD);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "Email content fetched successfully.\n";

            // Save the email content to a .txt file
            std::ofstream outFile("email_output.txt");
            if(outFile.is_open()) {
                outFile << chunk.memory;
                outFile.close();
                std::cout << "Email content has been saved to email_output.txt" << std::endl;
            } else {
                std::cerr << "Error opening output file!" << std::endl;
            }
        }


        curl_easy_cleanup(curl);
    }

    free(chunk.memory);

   std::string input_filename = "email_output.txt";      // Input file path
    std::string output_filename = "decoded_email.txt"; // Output file path
    process_email_file(input_filename, output_filename);

    return (int)res;
}
