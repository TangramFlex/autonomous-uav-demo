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
#include "AltNav/AltNavPosition.hpp"

// Generic transport
#include "TangramTransport.hpp"
static tangram::transport::TangramTransport *transport = nullptr;

#include "AltNav_DerivedEntityFactory.hpp"


#include "DirectSerializer.hpp"


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
    AltNav::AltNavPosition msg;

    // initialize the serializer
    tangram::serializers::Serializer *serializer;
    AltNav::DerivedEntityFactory derivedEntityFactory;

     tangram::serializers::DirectSerializer directSerializer(&derivedEntityFactory);
     serializer = &directSerializer;



    // dump the message to the console
    unsigned int count = 0; //count the number of samples
    //Mock Nav will be jammed every 20 seconds for 10 seconds
    double currentlat = 39.628580095139476;
    double currentlon = -84.17131129796783;
    float currentalt = 1337.0;
    //Initialize Message

    msg.dump();

    while(true) {
    // serialize to bytes
        currentlat = currentlat + 0.0001;
        currentlon = currentlon + 0.0003;

        msg.setAlt(currentalt);
        msg.setLat(currentlat);
        msg.setLon(currentlon);

        std::vector<uint8_t> bytes;
        if (!serializer->serialize(msg, bytes)) {
            transport->close();
            delete transport;
            fprintf(stderr, "Failed to serialize message.\n");
            _exit(1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));


        if (transport->publish(&bytes[0], bytes.size(), "AltNavPosition", 0) < 0) {
            fprintf(stderr, "Failed to publish message.\n");
            transport->close();
            delete transport;
            _exit(1);
        }
        sleep(1);
        count++;
    }
    sleep(2);

    printf("Closing transport.\n");
    transport->close();
    delete transport;

    printf("Done\n");
    return 0;
}
