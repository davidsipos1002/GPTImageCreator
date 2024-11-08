#include <iostream>
#include <optional>

#include <cal_types.h>
#include <gpt.hpp>
#include <fat.hpp>
#include <json.hpp>
#include <utf8.h>

using json = nlohmann::json;

int main(int argc, char *argv[])
{
    if (argc < 2)
        return 1;

    std::ifstream in(argv[1]);
    json jsonConfig = json::parse(in);
    in.close();   

    std::string outputImagePath = jsonConfig["output"];

    // Create the partition config
    std::vector<ConfigurationParitition> partitionConfig;
    partitionConfig.reserve(8);
    for (auto const &jsonPartition : jsonConfig["partitions"])
    {
        ConfigurationParitition partition;

        if (jsonPartition["type"] == "EFI")
            partition.Type = EFI_PART_TYPE_EFI_SYSTEM_PART_GUID;
        else if (jsonPartition["type"] == "BDP")
            partition.Type = EFI_PART_TYPE_MICROSOFT_BASIC_DATA_GUID;
        else if (jsonPartition["type"] == "LINUX_SWAP")
            partition.Type = EFI_PART_TYPE_LINUX_SWAP_GUID;
        else
            return 2;
        
        partition.LBACount = jsonPartition["size"].get<QWORD>() * 2;
        partition.PartitionName = utf8::utf8to16(jsonPartition["name"].get<std::string>());
        partitionConfig.push_back(partition);
    }

    std::ofstream f(outputImagePath);
    f.close();

    GptDisk gptDisk(outputImagePath);
    gptDisk.configureDisk(partitionConfig);
    gptDisk.createDisk(); 

    for (auto const &jsonFilesystem : jsonConfig["filesystems"])
    {
        std::u16string partitionName = utf8::utf8to16(jsonFilesystem["partition"].get<std::string>());
        std::optional<GptPartition> diskPartition = gptDisk.getPartition(partitionName);
        if (!diskPartition.has_value())
            return 3; 

        Fat fat(outputImagePath, diskPartition.value());
        fat.createFilesystem();
        fat.openFilesystem();

        for (auto const &jsonDirectory : jsonFilesystem["directories"])
        {
            if (!fat.createDirectory(jsonDirectory.get<std::string>()))
                return 4;
        }

        for (auto const &jsonFile : jsonFilesystem["files"])
        {
            if (!fat.createFile(jsonFile["destination"].get<std::string>(), jsonFile["source"].get<std::string>()))
                return 5;
        }

        fat.closeFilesystem();
    }

    return 0;
}
