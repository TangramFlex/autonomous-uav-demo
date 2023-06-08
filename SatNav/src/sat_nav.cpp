#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <thread>
#include <chrono>
#if defined(WRITE_TO_FILE) || defined(READ_FROM_FILE)
#include <fstream>
#include <sys/stat.h>

#endif
#include <chrono>
#include <cstdlib>
// Message of interest
#include "afrl/cmasi/AirVehicleState.hpp"

// Generic transport
#include "TangramTransport.hpp"
static tangram::transport::TangramTransport *transport = nullptr;

#include "afrl_cmasi_DerivedEntityFactory.hpp"


#include "LMCPSerializer.hpp"


//******************************************************************************
/**
 * @brief This function initializes whatever the generic transport is and
 *        configures it statically (without the config file).  Note that a lot
 *        of what's in this function can go away if file-based configuration is
 *        used instead.
 *
 * @return int 0 on success or -1 on error.
 */
static int initTransport(uint64_t flags) {
    // create the transport object
    transport = tangram::transport::TangramTransport::createTransport();
    if (nullptr == transport) {
        fprintf(stderr, "Could not create transport!\n");
        return -1;
    }
    std::string hostname = "127.0.0.1";

   char* hn = std::getenv("TANGRAM_TRANSPORT_zeromq_transport_HOSTNAME");
    printf("Hostname: %s\n", hn);
    if(hn)
    {

        hostname.assign(hn);
    }
    transport->resetTransportOptions();
    transport->setOption("PublishIP", hostname);
    transport->setOption("PublishPort", "6667");
    transport->setOption("SubscribeIP", hostname);
    transport->setOption("SubscribePort", "6668");
    transport->setOption("PublishID", "0");
    // open it
    if (transport->open(flags) == -1) {
        fprintf(stderr, "Could not open transport!\n");
        return -1;
    }

    return 0;
}

//******************************************************************************
int main(int argc, char *argv[]) {

    // setup the transport
    if (initTransport(TTF_WRITE) != 0) {
        fprintf(stderr, "Transport initialization failed!\n");
        exit(1);
    }

    // some transports require a brief delay between initialization and sending
    // of data
    sleep(1);

    // the message object
    afrl::cmasi::AirVehicleState msg;

    // initialize the serializer
    tangram::serializers::Serializer *serializer;
    afrl::cmasi::DerivedEntityFactory derivedEntityFactory;

     tangram::serializers::LMCPSerializer lmcpSerializer(&derivedEntityFactory);
     serializer = &lmcpSerializer;



    // dump the message to the console
    unsigned int count = 0; //count the number of samples
    bool _jammed = true; //are we being jammed?
    msg.dump();
    //Mock Nav will be jammed every 20 seconds for 10 seconds
    float currentlat = 39.628580095139476;
    float currentlon = -84.17131129796783;
    float currentalt = 1337.0;
    //Initialize Message
    msg.setID(1234);
    msg.setU(0.0);
    msg.setV(0.0);
    msg.setW(0.0);
    msg.setUdot(0.0);
    msg.setVdot(0.0);
    msg.setWdot(0.0);
    msg.setHeading(90.0);
    msg.setPitch(1.5);
    msg.setRoll(0.0);
    msg.setP(0.0);
    msg.setQ(0.0);
    msg.setR(0.0);
    msg.setCourse(0.0);
    msg.setGroundspeed(0.0);
    msg.setEnergyAvailable(100.0);
    msg.setActualEnergyRate(.024);
    msg.setCurrentWaypoint(0);
    msg.setCurrentCommand(0);
    msg.setMode(afrl::cmasi::NavigationMode::Waypoint);
    msg.setTime(std::chrono::system_clock::now().time_since_epoch().count());

    while(true) {
    // serialize to bytes
        currentlat = currentlat + 0.0001;
        currentlon = currentlon + 0.0003;
        if(!_jammed)
        {
            msg.getLocation()->setLatitude(currentlat);
            msg.getLocation()->setLongitude(currentlon);
            msg.getLocation()->setAltitude(currentalt);
        }


        std::vector<uint8_t> bytes;
        if (!serializer->serialize(msg, bytes)) {
            transport->close();
            delete transport;
            fprintf(stderr, "Failed to serialize message.\n");
            _exit(1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));


        if (transport->publish(&bytes[0], bytes.size(), "satnav_AirVehicleState", 0) < 0) {
            fprintf(stderr, "Failed to publish message.\n");
            transport->close();
            delete transport;
            _exit(1);
        }
        if(_jammed && count % 10 == 0)
        {
            _jammed = false;
        }
        else if (count % 20 == 0)
        {
            _jammed = true;
        }
        count++;
    }
    sleep(2);

    printf("Closing transport.\n");
    transport->close();
    delete transport;

    printf("Done\n");
    return 0;
}
