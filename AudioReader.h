#include "AudioFile.h"
#include <iostream>
#include <vector>
#include <memory>
#include <cstring>
#include <stdexcept>

struct Song {
    std::string name; 
    int sampleRate = 0;
    int bitDepth = 0;
    bool Mono = false;
    int numSamples = 0;
    int songLength = 0;
    std::vector<std::vector<char>> audioData{2};  // Correct initialization
};

std::vector<char> serializeDoubleVector(const std::vector<double>& data) {
    std::vector<char> bytes(data.size() * sizeof(double));
    std::memcpy(bytes.data(), data.data(), bytes.size());
    return bytes;
}

std::shared_ptr<Song> getSong(const char* filename, std::string& songname) {
    AudioFile<double> audioFile;
    if (!audioFile.load(filename)) {  // Corrected condition
        throw std::runtime_error("Failed to load file");
    }

    audioFile.printSummary();

    std::shared_ptr<Song> songData = std::make_shared<Song>();
    songData->name = songname; 
    songData->sampleRate = audioFile.getSampleRate();
    songData->bitDepth = audioFile.getBitDepth();
    songData->songLength = audioFile.getLengthInSeconds();
    songData->Mono = audioFile.isMono();
    songData->numSamples =  audioFile.getNumSamplesPerChannel(); 

    const auto& buffer = audioFile.samples;

    if (songData->Mono) {
        if (buffer.size() < 1) {
            throw std::runtime_error("Mono audio file does not have enough channels");
        }
        songData->audioData[0] = serializeDoubleVector(buffer[0]);
    } else {
        if (buffer.size() < 2) {
            throw std::runtime_error("Stereo audio file does not have enough channels");
        }
        songData->audioData[0] = serializeDoubleVector(buffer[0]);
        songData->audioData[1] = serializeDoubleVector(buffer[1]);
    }

    return songData;
}

