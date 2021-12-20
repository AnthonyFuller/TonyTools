#include "main.h"

// NOTE: "Rotated" is actually a cyclic shift
// Output the usage info with a message before it
void printUsage(std::string msg) {
    LOG_AND_EXIT(msg + "\nUsage: HMAScrambler <path to .ini or .scrambled file>");
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printUsage("Invalid arguments.");
    }

    std::string pathStr(argv[1]);

    // Check file exists
    if (!std::filesystem::exists(argv[1])) {
        printUsage("Specified path does not exist, please make sure it is correct.");
    }

    // Load file data into vector
    std::ifstream inputFile(argv[1], std::ifstream::binary);
    std::vector<char> inputData((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();

    std::string outputFileName;
    std::vector<char> outputData;

    uint8_t inputbyte;

    if (pathStr.ends_with(".scrambled")) {
        LOG("Unscrambling...");
        uint32_t key = 0xDE60ED55;
        char outByte;

        // Loop through the input data
        for (int i = 0; i < (inputData.size() - 4); i++) {
            inputbyte = inputData[i];            // Get the input byte
            outByte = inputbyte ^ key;                  // XOR input byte with the key
            key = inputbyte ^ __ROL4__(key, 5);         // Set the key to the input byte XOR'd with the key rotated left 5 times
            outputData.push_back(__ROR1__(outByte, 3)); // Add the output byte rotated right 3 times to the end of the output vector
        }

        uint32_t checksum = 0xE10F732F;
        for (int i = 0; i < outputData.size(); i++) {
            // XOR the checksum rotated left 13 times with the input byte
            checksum = __ROL4__(checksum, 13) ^ (outputData[i]);
        }

        uint32_t fileChecksum{};
        std::memcpy(&fileChecksum, &inputData.at(inputData.size() - 4), sizeof(uint32_t));
        if (checksum != fileChecksum)
        {
            LOG_AND_EXIT("Checksum does not match! Exiting without outputting final file...");
        }

        outputFileName = pathStr.substr(0, pathStr.length() - 10);
    }
    else if (pathStr.ends_with(".ini")) {
        LOG("Scrambling...");
        uint32_t key = 0xDE60ED55;

        uint32_t checksum = 0xE10F732F;
        for (int i = 0; i < inputData.size(); i++) {
            // XOR the checksum rotated left 13 times with the input byte
            checksum = __ROL4__(checksum, 13) ^ (inputData[i]);
        }

        // Loop through the input data
        for (int i = 0; i < inputData.size(); i++) {
            // Reverse of the unscrambling algorithm
            inputbyte = (__ROL1__(inputData[i], 3)) ^ key;   // The input byte is rotated left 3 times and XOR'd with the key
            key = __ROL4__(key, 5) ^ inputbyte;                     // The key is then rotated left 5 times and XOR'd with the input byte
            outputData.push_back((char)inputbyte);                  // Append the output byte to the end of the output vector
        }

        // Append the checksum to the end of the output vector
        for (int i = 0; i < 4; i++) {
            outputData.push_back((char)(checksum >> (i * 8)));
        }

        outputFileName = pathStr + ".scrambled";
    }
    else {
        printUsage("Invalid file specified must end with .scrambled or .ini!");
    }

    // Write data to the file
    std::ofstream outputFile(outputFileName, std::ios::out | std::ofstream::binary);
    std::copy(outputData.begin(), outputData.end(), std::ostreambuf_iterator<char>(outputFile));
    outputFile.close();

    LOG("Done! Outputted to: " + outputFileName);
}