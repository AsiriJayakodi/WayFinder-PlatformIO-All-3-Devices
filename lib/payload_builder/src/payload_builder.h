#ifndef PAYLOAD_BUILDER_H
#define PAYLOAD_BUILDER_H

#include <vector>
#include <string>
#include <cstdint>
#include <ctime>

#define MAX_PAYLOAD_SIZE 100

class PayloadBuilder {
public:
    struct GPSData {
        float longitude;
        float latitude;
    };

    struct PMsgData {
        uint8_t msgID;
    };

    struct CMsgData {
        std::string message;
    };

    struct PayloadDetails {
        uint8_t type;
        uint8_t sourceID;
        uint8_t destinationID;
        uint16_t transmissionID;
        uint8_t dateTime[6];
        uint8_t dataLength;
    };

    void configure_device(uint8_t srcID, uint8_t destID);
    std::vector<uint8_t> create_gps_payload(uint16_t transmissionID, float longitude, float latitude);
    std::vector<uint8_t> create_p_msg_payload(uint16_t transmissionID, uint8_t msgID);
    std::vector<uint8_t> create_c_msg_payload(uint16_t transmissionID, const std::string& msg);
    size_t get_last_payload_size() const;
    uint8_t identify_type_and_check_checksum(const std::vector<uint8_t>& payload);
    GPSData decode_gps_payload(const std::vector<uint8_t>& payload);
    PMsgData decode_p_msg_payload(const std::vector<uint8_t>& payload);
    CMsgData decode_c_msg_payload(const std::vector<uint8_t>& payload);
    PayloadDetails get_payload_details(const std::vector<uint8_t>& payload);

private:
    uint8_t sourceID;
    uint8_t destinationID;
    size_t lastPayloadSize = 0;
    void getCurrentDateTime(uint8_t *buffer);
    uint8_t calculateXORChecksum(const std::vector<uint8_t>& data);
};

#endif // PAYLOAD_BUILDER_H
