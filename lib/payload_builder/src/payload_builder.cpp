#include "payload_builder.h"
#include <vector>
#include <cstring>
#include <cstdlib>

void PayloadBuilder::getCurrentDateTime(uint8_t *buffer) {
    time_t now = time(nullptr);
    struct tm *tm_struct = localtime(&now);
    buffer[0] = tm_struct->tm_year - 100;
    buffer[1] = tm_struct->tm_mon + 1;
    buffer[2] = tm_struct->tm_mday;
    buffer[3] = tm_struct->tm_hour;
    buffer[4] = tm_struct->tm_min;
    buffer[5] = tm_struct->tm_sec;
}

uint8_t PayloadBuilder::calculateXORChecksum(const std::vector<uint8_t>& data) {
    uint8_t checksum = 0;
    for (uint8_t byte : data) {
        checksum ^= byte;
    }
    return checksum;
}

void PayloadBuilder::configure_device(uint8_t srcID, uint8_t destID) {
    sourceID = srcID;
    destinationID = destID;
}

std::vector<uint8_t> PayloadBuilder::create_gps_payload(uint16_t transmissionID, float longitude, float latitude) {
    std::vector<uint8_t> payload;
    uint8_t dateTime[6];
    getCurrentDateTime(dateTime);

    payload.push_back(0x01);
    payload.push_back(sourceID);
    payload.push_back(destinationID);
    payload.push_back(transmissionID >> 8);
    payload.push_back(transmissionID & 0xFF);
    payload.insert(payload.end(), dateTime, dateTime + 6);
    payload.push_back(8);
    payload.insert(payload.end(), reinterpret_cast<uint8_t*>(&longitude), reinterpret_cast<uint8_t*>(&longitude) + 4);
    payload.insert(payload.end(), reinterpret_cast<uint8_t*>(&latitude), reinterpret_cast<uint8_t*>(&latitude) + 4);
    payload.push_back(calculateXORChecksum(payload));

    lastPayloadSize = payload.size();
    return payload;
}

std::vector<uint8_t> PayloadBuilder::create_p_msg_payload(uint16_t transmissionID, uint8_t msgID) {
    std::vector<uint8_t> payload;
    uint8_t dateTime[6];
    getCurrentDateTime(dateTime);

    payload.push_back(0x02);
    payload.push_back(sourceID);
    payload.push_back(destinationID);
    payload.push_back(transmissionID >> 8);
    payload.push_back(transmissionID & 0xFF);
    payload.insert(payload.end(), dateTime, dateTime + 6);
    payload.push_back(1);
    payload.push_back(msgID);
    payload.push_back(calculateXORChecksum(payload));

    lastPayloadSize = payload.size();
    return payload;
}

std::vector<uint8_t> PayloadBuilder::create_c_msg_payload(uint16_t transmissionID, const std::string& msg) {
    if (msg.size() > MAX_PAYLOAD_SIZE - 14) return {};
    std::vector<uint8_t> payload;
    uint8_t dateTime[6];
    getCurrentDateTime(dateTime);

    payload.push_back(0x03);
    payload.push_back(sourceID);
    payload.push_back(destinationID);
    payload.push_back(transmissionID >> 8);
    payload.push_back(transmissionID & 0xFF);
    payload.insert(payload.end(), dateTime, dateTime + 6);
    payload.push_back(msg.size());
    payload.insert(payload.end(), msg.begin(), msg.end());
    payload.push_back(calculateXORChecksum(payload));

    lastPayloadSize = payload.size();
    return payload;
}

size_t PayloadBuilder::get_last_payload_size() const {
    return lastPayloadSize;
}

//Decoders

uint8_t PayloadBuilder::identify_type_and_check_checksum(const std::vector<uint8_t>& payload) {
    if (payload.empty()) return 101;
    uint8_t checksum = calculateXORChecksum(std::vector<uint8_t>(payload.begin(), payload.end() - 1));
    return (checksum == payload.back()) ? payload[0] : 101;
}

PayloadBuilder::GPSData PayloadBuilder::decode_gps_payload(const std::vector<uint8_t>& payload) {
    GPSData data;
    std::memcpy(&data.longitude, &payload[12], 4);
    std::memcpy(&data.latitude, &payload[16], 4);
    return data;
}

PayloadBuilder::PMsgData PayloadBuilder::decode_p_msg_payload(const std::vector<uint8_t>& payload) {
    PMsgData data;
    data.msgID = payload[12];
    return data;
}

PayloadBuilder::CMsgData PayloadBuilder::decode_c_msg_payload(const std::vector<uint8_t>& payload) {
    CMsgData data;
    uint8_t length = payload[11];
    data.message.assign(payload.begin() + 12, payload.begin() + 12 + length);
    return data;
}

PayloadBuilder::PayloadDetails PayloadBuilder::get_payload_details(const std::vector<uint8_t>& payload) {
    PayloadDetails details;
    details.type = payload[0];
    details.sourceID = payload[1];
    details.destinationID = payload[2];
    details.transmissionID = (payload[3] << 8) | payload[4];
    std::memcpy(details.dateTime, &payload[5], 6);
    details.dataLength = payload[11];
    return details;
}
