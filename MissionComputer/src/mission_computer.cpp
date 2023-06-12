#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <thread>
#include <chrono>
#if defined(WRITE_TO_FILE) || defined(READ_FROM_FILE)
#include <fstream>
#include <sys/stat.h>
#endif

// Message of interest
#include "afrl/cmasi/AirVehicleState.hpp"
#include "afrl/cmasi/GimbalAngleAction.hpp"
#include "afrl/cmasi/GimbalScanAction.hpp"

// Generic transport
#include "TangramTransport.hpp"
static tangram::transport::TangramTransport *transport = nullptr;
// static tangram::transport::TangramTransport *alt_transport = nullptr;

#include "afrl_cmasi_DerivedEntityFactory.hpp"

#include "LMCPSerializer.hpp"
#define BUF_SIZE 65536

// message prototype
void StartGimbalHandler();

//******************************************************************************
/**
 * @brief This function initializes whatever the generic transport is and
 *        configures it statically (without the config file).  Note that a lot
 *        of what's in this function can go away if file-based configuration is
 *        used instead.
 *
 * @return int 0 on success or -1 on error.
 */
static int initTransport(uint64_t flags)
{
    // create the transport object
    transport = tangram::transport::TangramTransport::createTransport();
    if (nullptr == transport)
    {
        fprintf(stderr, "Could not create transport!\n");
        return -1;
    }
    // open it

    std::string hostname = "127.0.0.1";

    char *hn = std::getenv("TANGRAM_TRANSPORT_zeromq_transport_HOSTNAME");
    printf("Hostname: %s\n", hn);
    if (hn)
    {

        hostname.assign(hn);
        printf("hostname: %s\n", hostname.c_str());
    }
    transport->resetTransportOptions();
    transport->setOption("PublishIP", hostname);
    transport->setOption("PublishPort", "6667");
    transport->setOption("SubscribeIP", hostname);
    transport->setOption("SubscribePort", "6668");
    transport->setOption("PublishID", "0");
    if (transport->open(flags) == -1)
    {
        fprintf(stderr, "Could not open transport!\n");
        return -1;
    }

    transport->subscribe("satnav_AirVehicleState");
    transport->subscribe("AirVehicleState");
    transport->subscribe("EntityState");

    return 0;
}

struct Entitydata
{
    double lat;
    double lon;
    float alt;
};

//******************************************************************************
int main(int argc, char *argv[])
{

    uint8_t buffer[BUF_SIZE];
    // setup the transport
    if (initTransport(12) != 0)
    {
        fprintf(stderr, "Transport initialization failed!\n");
        exit(1);
    }
    double currentLat = 0.0;
    double currentLon = 0.0;
    double currentAlt = 0.0;
    // some transports require a brief delay between initialization and sending
    // of data
    sleep(1);

    // initialize the serializer
    tangram::serializers::Serializer *serializer;
    afrl::cmasi::DerivedEntityFactory derivedEntityFactory;

    tangram::serializers::LMCPSerializer lmcpSerializer(&derivedEntityFactory);
    serializer = &lmcpSerializer;

    // Mock Nav will be jammed every 20 seconds for 10 seconds
    int bytesRead = 0;
    bool jammed = false;
    afrl::cmasi::AirVehicleState *alt_res;
    afrl::cmasi::AirVehicleState *satNav_res;
    std::vector<Entitydata *> enemies;
    bool UseAlt = false;
    std::cout << "started" << std::endl;
    while (true)
    {
        std::string topic;
        bytesRead = transport->recv(buffer, BUF_SIZE, topic);
        if (bytesRead > 0)
        {
            if (topic == "EntityState")
            {
                auto ent_msg = dynamic_cast<afrl::cmasi::EntityState *>(serializer->deserialize(buffer, bytesRead, "EntityState"));
                if (!ent_msg)
                {
                    fprintf(stderr, "Could not determine or serialize the message\n");
                    continue;
                }
                Entitydata *data = new Entitydata;
                data->lat = ent_msg->getLocation()->getLatitude();
                data->lon = ent_msg->getLocation()->getLongitude();
                data->alt = ent_msg->getLocation()->getAltitude();

                enemies.push_back(data);
            }
            else if (topic == "AirVehicleState")
            {
                auto alt_msg = dynamic_cast<afrl::cmasi::AirVehicleState *>(serializer->deserialize(buffer, bytesRead, "AirVehicleState"));
                if (!alt_msg)
                {
                    fprintf(stderr, "Could not determine or serialize the message\n");
                    continue;
                }
                delete alt_res;
                alt_res = alt_msg;
                if (!jammed && satNav_res != nullptr)
                {
                    continue;
                }
            }
            else if (topic == "satnav_AirVehicleState")
            {
                auto satNav_msg = dynamic_cast<afrl::cmasi::AirVehicleState *>(serializer->deserialize(buffer, bytesRead, "AirVehicleState"));
                if (!satNav_msg)
                {
                    fprintf(stderr, "Could not determine or serialize the message\n");
                    continue;
                }

                if (satNav_res == nullptr)
                {
                    delete satNav_res;
                    satNav_res = satNav_msg;
                }
                else
                {
                    if ((int)(satNav_res->getLocation()->getLatitude() * 100000) == (int)(satNav_msg->getLocation()->getLatitude() * 100000))
                    {
                        jammed = true;
                        if (alt_res != nullptr)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        jammed = false;
                        delete satNav_res;
                        satNav_res = satNav_msg;
                    }
                }
            }
            else
            {
                continue;
            }
            // Check for jamming

            if (!jammed)
            {
                if (satNav_res != nullptr)
                {
                    UseAlt = false;
                    currentLat = satNav_res->getLocation()->getLatitude();
                    currentAlt = satNav_res->getLocation()->getAltitude();
                    currentLon = satNav_res->getLocation()->getLongitude();
                }
                else
                {
                    UseAlt = true;
                    currentLat = alt_res->getLocation()->getLatitude();
                    currentAlt = alt_res->getLocation()->getAltitude();
                    currentLon = alt_res->getLocation()->getLongitude();
                }
            }
            else
            {
                if (alt_res != nullptr)
                {
                    UseAlt = true;
                    currentLat = alt_res->getLocation()->getLatitude();
                    currentAlt = alt_res->getLocation()->getAltitude();
                    currentLon = alt_res->getLocation()->getLongitude();
                }
                else
                {
                    UseAlt = false;
                    currentLat = satNav_res->getLocation()->getLatitude();
                    currentAlt = satNav_res->getLocation()->getAltitude();
                    currentLon = satNav_res->getLocation()->getLongitude();
                }
            }
        }
        else
        {
            continue;
        }
        topic = "";
        std::string tracks = "";
        for (auto enemy : enemies)
        {
            tracks += "[";
            tracks += std::to_string(enemy->lat) + ", ";
            tracks += std::to_string(enemy->lon) + ", ";
            tracks += std::to_string(enemy->alt) + ", ";
            tracks += "], ";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "****** Platform Status ******" << std::endl;
        if (alt_res == nullptr)
        {
            std::cout << "Location: " << currentLat << " : " << currentLon << " Altitude: " << currentAlt << " Jammed: " << jammed  << std::endl;
        }
        else
        {
            std::cout << "Location: " << currentLat << " : " << currentLon << " ALT: " << currentAlt << " Jammed: " << jammed << " Alt Data: " << UseAlt << std::endl;
        }
        std::cout << "Action: Waiting" << std::endl;
        std::cout << "Tracks:" << tracks << std::endl;

        afrl::cmasi::AirVehicleState outgoingMsg;

        outgoingMsg.getLocation()->setLatitude(currentLat);
        outgoingMsg.getLocation()->setAltitude(currentAlt);
        outgoingMsg.getLocation()->setLongitude(currentLon);

        std::vector<uint8_t> bytes;

        if (!lmcpSerializer.serialize(outgoingMsg, bytes))
        {
            std::cout << "failed to serialize airvehiclestate" << std::endl;
            continue;
        }

        if (transport->publish(bytes.data(), bytes.size(), "MissionComputer_AirVehicleState", 0) < 0)
        {
            fprintf(stderr, "Failed to publish MissionComputer_AirVehicleState.\n");
            transport->close();
            delete transport;
            _exit(1);
        }

        afrl::cmasi::GimbalScanAction GSA;

        bytes.clear();

        if (!lmcpSerializer.serialize(GSA, bytes))
        {
            std::cout << "failed to serialize Gimbal scan Action" << std::endl;
            continue;
        }
        if (transport->publish(bytes.data(), bytes.size(), "MissionComputer_GimbalScanAction", 0) < 0)
        {
            fprintf(stderr, "Failed to publish GimbalScanAction.\n");
            transport->close();
            delete transport;
            _exit(1);
        }

        bytesRead = 0;
    }
    sleep(2);

    printf("Closing transport.\n");
    transport->close();
    delete transport;

    printf("Done\n");
    return 0;
}
