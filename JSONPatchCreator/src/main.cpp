#include "json.hpp"
#include "iostream"
#include "fstream"

using json = nlohmann::json;

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cout << "Invalid usage: JSONPatchCreator.exe <original json> <modified json> <patch file output>" << std::endl;
        return 0;
    }

    try {
        std::ifstream og(argv[1]);
        json original = json::parse(og);

        std::ifstream mo(argv[2]);
        json modified = json::parse(mo);

        json patch = json::diff(original, modified);

        std::ofstream out(argv[3]);
        out << patch;

        std::cout << "Successfully created patch json." << std::endl;
    } catch (std::exception e) {
        std::cout << "Failed to read/write json or generate the patch!" << std::endl;
        return 0;
    }

    return 0;
}
