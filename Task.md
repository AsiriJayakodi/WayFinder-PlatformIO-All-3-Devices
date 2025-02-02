
type
sourceID
destinationID
transmitionID
date
time
dataLength
data
XOR


type:
    0: Predefined msg
    1: GPS Coordinates
    255: ACK


CreateGPSData(long,lat)
CreatePredefinedMsgData(msg_id)
CreatePayload(type,sourceID,destinationID,data)

CheakType(resivedPayload)
CheakType(resivedPayload)
