#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <thread>
#include <functional>
#include <chrono>
#include "AudioReader.h"



typedef websocketpp::client<websocketpp::config::asio_client> client;

// Global vector to store deserialized audio data
std::vector<std::vector<double>> globalAudioData;

// Function to deserialize binary data directly to a vector of vector of doubles
std::vector<std::vector<double>> deserializeAudioData(const std::vector<char>& data, size_t numChannels)
{
    size_t numDoubles = data.size() / sizeof(double);
    std::vector<std::vector<double>> audioData(numChannels);
    const double* dataPtr = reinterpret_cast<const double*>(data.data());

    for (size_t i = 0; i < numChannels; ++i) {
        audioData[i].assign(dataPtr + i * (numDoubles / numChannels), dataPtr + (i + 1) * (numDoubles / numChannels));
    }

    return audioData;
}

bool writeAudioFileToDisk()
{
	AudioFile<double>::AudioBuffer buffer;
	AudioFile<double> audioFile;

	buffer.resize(2); 
	audioFile.setBitDepth (24);
	audioFile.setSampleRate (48000);
	buffer[0] = globalAudioData[0]; 
	buffer[1] = globalAudioData[1]; 

	
	audioFile.setAudioBuffer (buffer);	

	// Wave file (implicit)
	bool status = audioFile.save ("/root/webrtc/websocketpp/programs/audio_example");
	return status; 
} 


class utility_client {
public:
    utility_client(const std::string& uri) : m_uri(uri), m_client() {
        // Initialize ASIO
        m_client.init_asio();

        // Set message handler
        m_client.set_message_handler(std::bind(&utility_client::on_message, this, std::placeholders::_1, std::placeholders::_2));

        // Create connection
        websocketpp::lib::error_code ec;
        m_connection = m_client.get_connection(m_uri, ec);
        if (ec) {
            std::cout << "Could not create connection because: " << ec.message() << std::endl;
            return;
        }

        // Connect to server
        m_client.connect(m_connection);
    }

    void run() {
        // Start ASIO io_service run loop in a separate thread
        std::thread t([this]() { m_client.run(); });

        // Wait for the connection to be established
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Main loop for user input
        main_loop();

        // Stop the client and join the thread
        m_client.stop();
        t.join();
    }

private:
    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg)
    {
   	if (msg->get_opcode() == websocketpp::frame::opcode::binary)
        {
            std::vector<char> receivedData(msg->get_payload().begin(), msg->get_payload().end());
            globalAudioData = deserializeAudioData(receivedData, 2);
            std::cout << "Received and deserialized audio data." << std::endl;
	
            if(writeAudioFileToDisk) 
		std::cout << "Successfully Wrote Audio to Disk" << std::endl;
	    else
		std::cout << "Error Writting to Disk" << std::endl;
 
        }
        else
        {
            std::cout << "Server: " << msg->get_payload() << std::endl;
        }     
	
	
    }

    void main_loop()
    {
        std::string input;
        
	while (true) {
            std::cout << "Enter command (or 'exit' to quit): ";
            std::getline(std::cin, input);

            if (input == "exit") {
                break;
            }

            if (input.rfind("SELECT_SONG:", 0) == 0 || input == "GET_SAMPLE_RATE" || input == "GET_LENGTH_IN_SECONDS" ||
                input == "GET_CHANNEL_COUNT" || input == "GET_BIT_DEPTH" || input == "GET_AUDIO_DATA" ||
                input.rfind("SET_BUFFER_SIZE:", 0) == 0) {
                m_client.send(m_connection, input, websocketpp::frame::opcode::text);
            } else {
                std::cout << "Invalid command. Valid commands are: " << std::endl;
                std::cout << " - SELECT_SONG:<song_name>" << std::endl;
                std::cout << " - GET_SAMPLE_RATE" << std::endl;
                std::cout << " - GET_LENGTH_IN_SECONDS" << std::endl;
                std::cout << " - GET_CHANNEL_COUNT" << std::endl;
                std::cout << " - GET_BIT_DEPTH" << std::endl;
                std::cout << " - GET_AUDIO_DATA" << std::endl;
                std::cout << " - SET_BUFFER_SIZE:<size>" << std::endl;
            }
        }
    }

    std::string m_uri;
    client m_client;
    client::connection_ptr m_connection;
};

int main() {
    std::string uri = "ws://localhost:9002";

    try {
        std::cout << "Initializing client..." << std::endl;
        utility_client c(uri);
        c.run();
    } catch (websocketpp::exception const& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}

