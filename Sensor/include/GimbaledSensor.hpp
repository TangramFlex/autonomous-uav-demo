/**
 * @brief A sample senser that is on a gimbal. Can be controlled 
 * with LMCP messages and will return EntityStates when it 
 * "detects" a target.
 * GimbaledAngleAction - adjust the pointing angle
 * GimbalScanAction - scan the sensor, add tracks when present
 * 
 */
#include "afrl/cmasi/GimbalAngleAction.hpp"
#include "afrl/cmasi/GimbalScanAction.hpp"
#include "afrl/cmasi/AirVehicleState.hpp"
#include "afrl/cmasi/EntityState.hpp"
#include "TangramTransport.hpp"
#include "LMCPSerializer.hpp"
#include "afrl_cmasi_DerivedEntityFactory.hpp"

#define BUF_SIZE 65535
class GimbaledSensor {
    public:
    GimbaledSensor();
    void operator()(int x);
    void processGimbalAngleAction(afrl::cmasi::GimbalAngleAction& msg);
    void processGimbalScanAction(afrl::cmasi::GimbalScanAction& msg);
    void processAirVehicleState(afrl::cmasi::AirVehicleState& msg);
    int initTransport(tangram::transport::TangramTransport& transport, uint64_t flags, std::string topic = "");
    private:
    typedef struct Entity{
        float lat;
        float lon;
        unsigned long id;
    };
    bool foundSomething();
    float angle;
    float azimuth;
    float rotation;
    std::vector<Entity> tracks;
    tangram::transport::TangramTransport* ga_transport;
    tangram::transport::TangramTransport* gs_transport;
    tangram::transport::TangramTransport* avs_transport;
    tangram::transport::TangramTransport* en_transport;
    float lat;
    float lon;
    float alt;
    unsigned long nid;
    afrl::cmasi::EntityState* entity_state;
    tangram::serializers::Serializer *serializer;
    afrl::cmasi::DerivedEntityFactory *derivedEntityFactory;
    uint8_t* buffer;
};
