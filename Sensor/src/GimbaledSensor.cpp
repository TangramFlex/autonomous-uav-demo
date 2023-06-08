#include "GimbaledSensor.hpp"
#include <string>
#include <random>


GimbaledSensor::GimbaledSensor() 
{
    std::cout << "Starting Sensor..." << std::endl;
    azimuth = 0.0;
    angle = 0.0;
    rotation = 0.0;
    tracks.clear();
    ga_transport = tangram::transport::TangramTransport::createTransport();
    gs_transport = tangram::transport::TangramTransport::createTransport();
    avs_transport = tangram::transport::TangramTransport::createTransport();
    en_transport = tangram::transport::TangramTransport::createTransport();
    initTransport(*ga_transport, TTF_READ, "GimbalAngleAction");
    initTransport(*gs_transport, TTF_READ, "MissionComputer_GimbalScanAction");
    initTransport(*avs_transport, TTF_READ, "MissionComputer_AirVehicleState");
    initTransport(*en_transport, TTF_WRITE);
    lat = 0.0;
    lon = 0.0;
    alt = 1337;
    nid = 0;
    entity_state = new afrl::cmasi::EntityState();
    entity_state->setID(1234);
    entity_state->setU(0.0);
    entity_state->setV(0.0);
    entity_state->setW(0.0);
    entity_state->setUdot(0.0);
    entity_state->setVdot(0.0);
    entity_state->setWdot(0.0);
    entity_state->setHeading(90.0);
    entity_state->setPitch(1.5);
    entity_state->setRoll(0.0);
    entity_state->setP(0.0);
    entity_state->setQ(0.0);
    entity_state->setR(0.0);
    entity_state->setCourse(0.0);
    entity_state->setGroundspeed(0.0);
    entity_state->setEnergyAvailable(100.0);
    entity_state->setActualEnergyRate(.024);
    entity_state->setCurrentWaypoint(0);
    entity_state->setCurrentCommand(0);
    entity_state->setMode(afrl::cmasi::NavigationMode::Waypoint);
    derivedEntityFactory = new afrl::cmasi::DerivedEntityFactory();
    serializer = new tangram::serializers::LMCPSerializer(derivedEntityFactory);
    buffer = new uint8_t[BUF_SIZE];
    std::cout << "Sensor Initialized." << std::endl;
    
}

int GimbaledSensor::initTransport(tangram::transport::TangramTransport& transport, uint64_t flags, std::string topic) {

    // open it
    
    std::string hostname = "127.0.0.1";
    
    char* hn = std::getenv("TANGRAM_TRANSPORT_zeromq_transport_HOSTNAME");
    printf("Hostname: %s\n", hn);
    if(hn)
    {
        
        hostname.assign(hn);
        printf("hostname: %s\n", hostname.c_str());
    } 
    transport.resetTransportOptions();
    transport.setOption("PublishIP", hostname);
    transport.setOption("PublishPort", "6667");
    transport.setOption("SubscribeIP", hostname);

    transport.setOption("SubscribePort", "6668");
    
    transport.setOption("PublishID", "0");
    if (transport.open(flags) == -1) {
        fprintf(stderr, "Could not open transport!\n");
        return -1;
    }

    if(!topic.empty())
    {
        transport.subscribe(topic);
    }

    return 0;
}
void GimbaledSensor::processAirVehicleState(afrl::cmasi::AirVehicleState& msg)
{
    lat = msg.getLocation()->getLatitude();
    lon = msg.getLocation()->getLongitude();
    alt = msg.getLocation()->getAltitude();
}
void GimbaledSensor::processGimbalAngleAction(afrl::cmasi::GimbalAngleAction& msg)
{
    azimuth = msg.getAzimuth();
    angle = msg.getElevation();
    rotation = msg.getRotation();
    std::cout << "Moving Gimbal to Az: " << azimuth << " El: " << angle << " Rotation: " << rotation << std::endl;
}

void GimbaledSensor::processGimbalScanAction(afrl::cmasi::GimbalScanAction& msg)
{
    if(foundSomething())
    {
        static std::vector<uint8_t> bytes;
        Entity en;
        en.lat = lat + .00134;
        en.lon = lon + .000421;
        en.id = nid;
        nid++;
        tracks.push_back(en);
        std::cout << "Found Track at " << en.lat << "  " << en.lon << std::endl;
        entity_state->setTime(std::chrono::system_clock::now().time_since_epoch().count());
        entity_state->getLocation()->setLatitude(en.lat);
        entity_state->getLocation()->setLongitude(en.lon);
        entity_state->getLocation()->setAltitude(alt);
        entity_state->setID(en.id);
        if(!serializer->serialize(*entity_state, bytes))
        {
            std::cout << "Failed to serialize" << std::endl;
        }
        en_transport->publish(&bytes[0], bytes.size(), "EntityState");
        bytes.clear();
    }


}

bool GimbaledSensor::foundSomething()
{
    return std::rand() % (7 + 1) == 2;
}

void GimbaledSensor::operator()(int x) 
{
    //uint8_t buffer[BUF_SIZE];
    int32_t bytesRead = 0;
    while(true)
    {
       
        
        
        //check each transport for messages
    
        bytesRead = avs_transport->recv(buffer, BUF_SIZE, TTF_NONBLOCK);
       
        if(bytesRead > 0)
        {
           
            auto msg = serializer->deserialize(buffer, bytesRead, "AirVehicleState");
            afrl::cmasi::AirVehicleState* res = dynamic_cast<afrl::cmasi::AirVehicleState*>(msg);
            processAirVehicleState(*res);
            delete msg;
        }
        bytesRead = ga_transport->recv(buffer, BUF_SIZE, TTF_NONBLOCK);
        if(bytesRead > 0)
        {
        
            auto msg = serializer->deserialize(buffer, bytesRead, "GimbalAngleAction");
            afrl::cmasi::GimbalAngleAction* res = dynamic_cast<afrl::cmasi::GimbalAngleAction*>(msg);
            processGimbalAngleAction(*res);
            delete msg;
            
        }
        bytesRead = gs_transport->recv(buffer, BUF_SIZE, TTF_NONBLOCK);
        if(bytesRead > 0)
        {
            auto msg = serializer->deserialize(buffer, bytesRead, "GimbalScanAction");
            afrl::cmasi::GimbalScanAction* res = dynamic_cast<afrl::cmasi::GimbalScanAction*>(msg);
            processGimbalScanAction(*res);
            delete msg;
        }
        
    }
}