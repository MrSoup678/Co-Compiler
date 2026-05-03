#include <type_traits>
#include "confighandler.h"
#include "KeyValue.h"
#include "enums.h"
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>


#pragma pack(1)
struct CSCommand
{
    bool csIsEnabled;
    char padding[3]; // FIXME: This doesn't exist outside of CS:GO
    int csSpecial;
    char csExecutable[260];
    char csArguments[260];
    bool csIsLongFilename;
    char padding2[3];
    bool csEnsureCheck;
    char padding3[3];
    char csEnsureFile[260];
    int csUseProcessWindow;
    bool csNoWait;
    char padding4[3];
};

struct CSCommandNONCSGO
{
    bool csIsEnabled;
    int csSpecial;
    char csExecutable[260];
    char csArguments[260];
    bool csIsLongFilename;
    char padding2[3];
    bool csEnsureCheck;
    char padding3[3];
    char csEnsureFile[260];
    int csUseProcessWindow;
    bool csNoWait;
    char padding4[3];
};




struct CSSequence
{
    char csName[128];
    unsigned int csCommandCount;
};

struct CSHeader
{
    char csSignature[31];
    float csVersion;
    unsigned int csSequenceCount;
};
#pragma pack()




bool CConfigHandler::IsBinaryWCConfig(const char* data) {
    auto header = reinterpret_cast<const CSHeader*>(data);

    return std::string(header->csSignature, 31).contains("Worldcraft Command Sequences\r\n\x1a");
}

int SourceSpecialToCocompilerSpecial(int special)
{
    switch (special) {
        case SRC_SPECIAL_CHANGE_DIRECTORY:
            return CC_SPECIAL_CHANGE_DIRECTORY;
        case SRC_SPECIAL_COPY_FILE:
            return CC_SPECIAL_COPY_FILE;
        case SRC_SPECIAL_DELETE_FILE:
            return CC_SPECIAL_DELETE_FILE;
        case SRC_SPECIAL_RENAME_FILE:
            return CC_SPECIAL_RENAME_FILE;
        case SRC_SPECIAL_GENERATE_GRID_NAV:
            return CC_SPECIAL_GENERATE_GRID_NAV;
        case SRC_SPECIAL_COPY_FILE_IF_EXISTS: //strata specific.
            return CC_SPECIAL_COPY_FILE_IF_EXISTS;
        default:
            return SPECIAL_EXEC;
    }
}


int CocompilerSpecialToSourceSpecial(int special)
{
    switch (special) {
        case CC_SPECIAL_CHANGE_DIRECTORY:
            return SRC_SPECIAL_CHANGE_DIRECTORY;
        case CC_SPECIAL_COPY_FILE:
            return SRC_SPECIAL_COPY_FILE;
        case CC_SPECIAL_DELETE_FILE:
            return SRC_SPECIAL_DELETE_FILE;
        case CC_SPECIAL_RENAME_FILE:
            return SRC_SPECIAL_RENAME_FILE;
        case CC_SPECIAL_GENERATE_GRID_NAV:
            return SRC_SPECIAL_GENERATE_GRID_NAV;
        case CC_SPECIAL_COPY_FILE_IF_EXISTS:   //Strata specific.
            return SRC_SPECIAL_COPY_FILE_IF_EXISTS;
        default:
            return SPECIAL_EXEC;
    }
}


QJsonDocument CConfigHandler::ConvertBinaryToJSON(const char* data,bool isCSGO) {

    QJsonDocument document;
    QJsonObject configuration;
    QJsonObject sequenceList;

    auto header = reinterpret_cast<const CSHeader*>(data);
    data += sizeof(CSHeader);

    for(int i = 0; i < header->csSequenceCount; i++)
    {
        auto sequence = reinterpret_cast<const CSSequence *>(data);

        data += sizeof(CSSequence);

        QJsonArray commands;
        for(int j = 0; j < sequence->csCommandCount; j++)
        {
            QJsonObject jsonCommand;
            if(isCSGO)
            {
                auto command = reinterpret_cast<const CSCommand*>(data);
                jsonCommand.insert("enabled", command->csIsEnabled);
                jsonCommand.insert("command", command->csExecutable);
                jsonCommand.insert("parameters", command->csArguments);
                jsonCommand.insert("ensure",command->csEnsureCheck);
                jsonCommand.insert("special", SourceSpecialToCocompilerSpecial(command->csSpecial));
                jsonCommand.insert("ensured", command->csEnsureCheck);
                jsonCommand.insert("ensure_file", command->csEnsureFile);
                jsonCommand.insert("no_wait", command->csNoWait);
            }
            else
            {
                auto command = reinterpret_cast<const CSCommandNONCSGO*>(data);
                jsonCommand.insert("enabled", command->csIsEnabled);
                jsonCommand.insert("command", command->csExecutable);
                jsonCommand.insert("parameters", command->csArguments);
                jsonCommand.insert("ensure",command->csEnsureCheck);
                jsonCommand.insert("special", SourceSpecialToCocompilerSpecial(command->csSpecial));
                jsonCommand.insert("ensured", command->csEnsureCheck);
                jsonCommand.insert("ensure_file", command->csEnsureFile);
                jsonCommand.insert("no_wait", command->csNoWait);
            }
            commands.push_back(jsonCommand);
            data += sizeof (CSCommand);
        }

        sequenceList.insert(sequence->csName, commands);

    }

    configuration.insert("sequences",sequenceList);
    document.setObject(configuration);

    return document;
}

QJsonDocument CConfigHandler::ConvertTextToJSON(const char* data)
{
    auto config = KeyValueRoot();
    auto err = config.Parse(reinterpret_cast<const char *>(data));

    if(err != KeyValueErrorCode::NONE)
        return {};

    QJsonDocument jsonConfiguration{};
    QJsonObject jsonConfigObject{};
    QJsonObject jsonSequenceList{};

    if(auto &sequences = config.Get("Command Sequences"); sequences.IsValid())
    {
        for(int i = 0; i < sequences.ChildCount(); i++)
        {
            auto &sequence = sequences.At(i);

            QJsonArray jsonSequenceArray{};
            for(int j = 0; j < sequence.ChildCount(); j++)
            {
                QJsonObject jsonSequenceObject{};

                auto &configParams = sequence.At(j);

                //TODO: change backslashes to forward slashes.
                jsonSequenceObject.insert("enabled", configParams.Get("enabled").Value().string[0] == '1');
                jsonSequenceObject.insert("command", configParams.Get("run").Value().string);
                jsonSequenceObject.insert("parameters",configParams.Get("params").Value().string);
                jsonSequenceObject.insert("ensured", configParams.Get("ensure_check").Value().string[0] == '1');
                //STRATA: stoi will return 0 on none, its hammer will convert 0 to none back again. This may change without warning.
                jsonSequenceObject.insert("special", SourceSpecialToCocompilerSpecial(std::stoi(configParams.Get("special_cmd").Value().string)));
                jsonSequenceObject.insert("ensure_file", configParams.Get("ensure_fn").Value().string);
                jsonSequenceObject.insert("no_wait", false); //Strata does away with it, I do not, so false by default it is.
                jsonSequenceArray.push_back(jsonSequenceObject);
            }

            jsonSequenceList.insert(sequence.Key().string, jsonSequenceArray);
        }
        jsonConfigObject.insert("sequences",jsonSequenceList);
        jsonConfiguration.setObject(jsonConfigObject);
    }

    return jsonConfiguration;
}

std::unique_ptr<QByteArray> CConfigHandler::ExportToWCConfig(const QJsonDocument & object, bool kvConfig, bool isCSGO)
{
    std::unique_ptr<QByteArray> wcConfig = std::make_unique<QByteArray>();
    auto jsonSeqs = object.object()["sequences"].toObject();
    if (kvConfig) {
        auto kvConfig = KeyValueRoot();
        kvConfig.Parse("\"Command Sequences\" {}");
        //char* debugBuffer = kvConfig.ToString();

    } else {
        //binary.
        wcConfig->reserve(1024*1024);
        //header.
        QBuffer memFile(wcConfig.get());
        memFile.open(QIODevice::WriteOnly);
        CSHeader header = CSHeader();
        std::strncpy(header.csSignature,"Worldcraft Command Sequences\r\n\x1a",31);
        header.csVersion = 0.2;
        header.csSequenceCount = jsonSeqs.keys().count();
        memFile.write((char*)&header,sizeof(CSHeader));
        //memFile << header;
        for (const auto keySequence : jsonSeqs.keys()) {
            QJsonArray jsonCommands = jsonSeqs.value(keySequence).toArray();
            CSSequence sequence = CSSequence();
            std::memset(&sequence,0,sizeof(sequence));
            std::strncpy(sequence.csName,keySequence.toStdString().data(),sizeof(sequence.csName));
            sequence.csCommandCount = jsonCommands.count();
            memFile.write((char*)&sequence,sizeof(CSSequence));
            //memFile << sequence;
            for (const auto jsonCommand : jsonCommands) {
                auto cmdObj = jsonCommand.toObject();
                CSCommand command = CSCommand();
                std::memset(&command,0,sizeof(command));
                command.csIsEnabled = cmdObj["enabled"].toBool();
                    std::string strBuf;
                command.csSpecial = CocompilerSpecialToSourceSpecial(cmdObj["special"].toInt());
                strBuf = cmdObj["command"].toString().toStdString();
                std::strncpy(command.csExecutable,strBuf.data(),sizeof(command.csExecutable));
                strBuf = cmdObj["parameters"].toString().toStdString();
                std::strncpy(command.csArguments,strBuf.data(),sizeof(command.csArguments));
                command.csIsLongFilename = true;
                command.csEnsureCheck = cmdObj["ensured"].toBool();
                strBuf = cmdObj["ensure_file"].toString().toStdString();
                std::strncpy(command.csEnsureFile,strBuf.data(),sizeof(command.csEnsureFile));
                command.csUseProcessWindow = false;
                command.csNoWait = cmdObj["no_wait"].toBool();

                memFile.write((char*)&command.csIsEnabled,sizeof(command.csIsEnabled));
                if(isCSGO)
                {
                    memFile.write((char*)&command.padding,sizeof(command.padding));
                }
                memFile.write((char*)&command.csSpecial,sizeof(command.csSpecial));
                memFile.write((char*)&command.csExecutable,sizeof(command.csExecutable));
                memFile.write((char*)&command.csArguments,sizeof(command.csArguments));
                memFile.write((char*)&command.csIsLongFilename,sizeof(command.csIsLongFilename));
                memFile.write((char*)&command.padding2,sizeof(command.padding2));
                memFile.write((char*)&command.csEnsureCheck,sizeof(command.csEnsureCheck));
                memFile.write((char*)&command.padding3,sizeof(command.padding3));
                memFile.write((char*)&command.csEnsureFile,sizeof(command.csEnsureFile));
                memFile.write((char*)&command.csUseProcessWindow,sizeof(command.csUseProcessWindow));
                memFile.write((char*)&command.csNoWait,sizeof(command.csNoWait));
                memFile.write((char*)&command.padding4,sizeof(command.padding4));
            }
        };
        memFile.close();

    }
    return wcConfig;
}

