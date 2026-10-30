// Shared CF RPC namespace "EZ_QOL". Drone names live in EZ_QOL_DroneRPC.c (ClearcREEk).
const string EZ_QOL_RPC_NS = "EZ_QOL";

// Garage, client -> server (target object = EZ_QOL_Garage)
const string EZ_QOL_RPC_GARAGE_REQUESTSTATE = "EZ_QOL_Garage_RequestState"; // Param1<int> dummy
const string EZ_QOL_RPC_GARAGE_PARK         = "EZ_QOL_Garage_Park";         // Param1<int> dummy
const string EZ_QOL_RPC_GARAGE_RETRIEVE     = "EZ_QOL_Garage_Retrieve";     // Param1<int> dummy
// Garage, server -> client
const string EZ_QOL_RPC_GARAGE_STATE        = "EZ_QOL_Garage_State";        // Param3<bool hasVehicle, string vehType, bool locked>
const string EZ_QOL_RPC_GARAGE_RESULT       = "EZ_QOL_Garage_Result";       // Param2<bool ok, string reasonKey>
const string EZ_QOL_RPC_GARAGE_CONFIG       = "EZ_QOL_Garage_Config";       // Param1<string json of EZ_QOL_GarageClientConfig>
