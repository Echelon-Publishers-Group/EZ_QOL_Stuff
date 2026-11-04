// Vanilla (non-CF) RPC ids for the features moved from EZLoderScreen + the sleeping bag.
// Range 47311xxx is reserved for EZ_QOL; EZLoderScreen keeps 82341xxx (EZ_RPC_CLIENT_PREFS etc.).
const int EZ_QOL_RPC_FLIP_REQUEST = 47311001; // client -> server Param1<Object> (keybind flip)
const int EZ_QOL_RPC_BAG_NOTIFY   = 47311010; // server -> client Param1<string> (sleeping bag message)
