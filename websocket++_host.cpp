#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "AudioReader.h"
#include <iostream>
#include <functional>
#include <vector>
#include <memory>
#include <algorithm>
#include <cstring>
#include <stdexcept>

// Pull in the WebSocket++ core namespace
using websocketpp::connection_hdl;

typedef websocketpp::server<websocketpp::config::asio> server;

// Define a utility server class
class utility_server {
public:
    utility_server() {
        // Set logging settings
        m_endpoint.set_error_channels(websocketpp::log::elevel::all);
        m_endpoint.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        m_endpoint.init_asio();

        // Set the open handler
        m_endpoint.set_open_handler(std::bind(&utility_server::on_open, this, std::placeholders::_1));

        // Set the message handler
        m_endpoint.set_message_handler(std::bind(&utility_server::on_message, this, &m_endpoint, std::placeholders::_1, std::placeholders::_2));
    }

    void createAudioCatalogue(std::vector<const char*> filenames) {
        for (const char* filename : filenames) {
            std::string songname;
            std::cout << "Provide name for filename: " << filename << std::endl;
            std::cin >> songname;
            try {
                std::shared_ptr<Song> song = getSong(filename, songname);
                songVec.push_back(song);
            } catch (std::exception& e) {
                std::cout << e.what() << std::endl;
            }
        }
    }

    void run() {
        std::cout << "Listening on Port 9002..." << std::endl;

        // Listen on port 9002
        m_endpoint.listen(9002);

        // Queues a connection accept operation
        m_endpoint.start_accept();

        // Start the Asio io_service run loop
        m_endpoint.run();
    }

private:
    void on_open(connection_hdl hdl) {
        std::cout << "Connection established with: " << hdl.lock().get() << std::endl;

        std::string openStatement = "Hello! Welcome to SongShare! Provide the name of a Song you'd like to hear or learn more about.";
        m_endpoint.send(hdl, openStatement, websocketpp::frame::opcode::text);
        
	std::stringstream ss; 
		
	for (std::shared_ptr<Song>& song : songVec)
	{
             ss << song->name;
	     ss << " \n";  
        }
	std::string songNames = ss.str(); 
            m_endpoint.send(hdl, songNames, websocketpp::frame::opcode::text); 
	       
    }

    void sendSerializedData(connection_hdl hdl, const std::vector<std::vector<char>>& data, size_t bufferSize) {
        for (const auto& channelData : data) { // for every channel of audio
            for (size_t i = 0; i < channelData.size(); i += bufferSize) { // for every buffer in that channel
                size_t sendSize = std::min(bufferSize, channelData.size() - i); // if we can get the full buffer send, if not provide what we can.
                m_endpoint.send(hdl, channelData.data() + i, sendSize, websocketpp::frame::opcode::binary);
            }
        }
    }

    void on_message(server* s, connection_hdl hdl, server::message_ptr msg) {
        std::string in = msg->get_payload();
        // We Check if the message starts with this prefix, supplying 0 as the start index.
        if (in.rfind("SELECT_SONG:", 0) == 0) { // returns 0 if found, other std::string::npos
            selectedSong = in.substr(strlen("SELECT_SONG:")); // create a new string minus Select_Song
            auto it = std::find_if(songVec.begin(), songVec.end(), [&](const std::shared_ptr<Song>& song) {
                return song->name == selectedSong;
            });

            if (it != songVec.end()) {
                selectedSongData = *it;
                std::string response = "Selected song: " + selectedSong;
                m_endpoint.send(hdl, response, websocketpp::frame::opcode::text);
            } else {
                std::string error_request = "Song not found";
                m_endpoint.send(hdl, error_request, websocketpp::frame::opcode::text);
            }
            return;
        }

        if (in.rfind("SET_BUFFER_SIZE:", 0) == 0) {
            bufferSize = std::stoi(in.substr(strlen("SET_BUFFER_SIZE:")));
            return;
        }

        if (selectedSongData == nullptr) {
            std::string error_request = "You have not provided a song you'd like to listen to, or learn more about.";
            m_endpoint.send(hdl, error_request, websocketpp::frame::opcode::text);
            return;
        }

        if (in == "GET_SAMPLE_RATE") {
            std::string response = "Sample Rate: " + std::to_string(selectedSongData->sampleRate);
            m_endpoint.send(hdl, response, websocketpp::frame::opcode::text);
        } else if (in == "GET_LENGTH_IN_SECONDS") {
            std::string response = "Length in Seconds: " + std::to_string(selectedSongData->songLength);
            m_endpoint.send(hdl, response, websocketpp::frame::opcode::text);
        } else if (in == "GET_CHANNEL_COUNT") {
            std::string response =  std::to_string(selectedSongData->Mono ? 1 : 2);
            m_endpoint.send(hdl, response, websocketpp::frame::opcode::text);
        } else if (in == "GET_BIT_DEPTH") {
            std::string response = "Bit Depth: " + std::to_string(selectedSongData->bitDepth);
            m_endpoint.send(hdl, response, websocketpp::frame::opcode::text);
        } else if (in == "GET_AUDIO_DATA") {
            sendSerializedData(hdl, selectedSongData->audioData, bufferSize);
        } else if (in == "GET_NUM_SAMPLES"){
	    std::string response = std::to_string(selectedSongData->numSamples); 
            m_endpoint.send(hdl, response, websocketpp::frame::opcode::text);
	} else {
            std::string response = "Unknown command: " + in;
            m_endpoint.send(hdl, response, websocketpp::frame::opcode::text);
        }
    }

    std::vector<std::shared_ptr<Song>> songVec;
    std::string selectedSong;
    std::shared_ptr<Song> selectedSongData;
    server m_endpoint;
    size_t bufferSize = 512;  // Default buffer size
};

int main() {
    std::vector<const char*> filenames = { "/root/audio/NewBanger.wav", "/root/audio/take2.wav" };
    std::cout << "Initializing server..." << std::endl;
    utility_server s;
    s.createAudioCatalogue(filenames);
    s.run();
    return 0;
}

